#include <stdio.h>
#include <iostream>
#include <unistd.h>
#include <signal.h>
#include <sys/mman.h>
#include <queue>
#include <mutex>
#include <condition_variable>
#include "posix_rt.h"
#include "linux_CInterrogator.h"

#define ETHERCAT_TASK_PERIOD (1000000) // 1 ms

// FSM states
enum InterrogatorState {
    INIT_STATE,
    CONNECT_UDP_STATE,
    ENABLE_PEAKS_STATE,
    READ_PEAKS_STATE,
    ERROR_STATE,
    DISCONNECT_STATE
};

// Peak data structure for inter-task communication
struct PeakData {
    double timeStamp;
    short numPeaks[4];
    double peakVals[PEAK_CH * NUM_GRATING];
    double avgPeakVals[PEAK_CH];
};

// Thread-safe peak data queue
class PeakDataQueue {
private:
    std::queue<PeakData> dataQueue;
    std::mutex mtx;
    std::condition_variable cv;
    const size_t MAX_QUEUE_SIZE = 10;

public:
    void push(const PeakData& data) {
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [this]{ return dataQueue.size() < MAX_QUEUE_SIZE; });
        
        dataQueue.push(data);
        lock.unlock();
        cv.notify_one();
    }

    PeakData pop() {
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [this]{ return !dataQueue.empty(); });
        
        PeakData data = dataQueue.front();
        dataQueue.pop();
        lock.unlock();
        cv.notify_one();
        
        return data;
    }
};

// Interrogator communication class
class InterrogatorManager {
private:
    CInterrogator interrogator;
    InterrogatorState currentState;
    POSIX_TASK rtTask;
    pthread_t nrtThreadId;
    PeakDataQueue peakDataQueue;
    RTTIME maxPeriod;

    // State transition logic for RT task
    InterrogatorState processRTState() {
        switch(currentState) {
            case INIT_STATE: {
                if(interrogator.init()){
                    DBG_ERROR("INIT failed.");
                    return DISCONNECT_STATE;
                } 
                return CONNECT_UDP_STATE;
            }
            case CONNECT_UDP_STATE: {
                if(interrogator.connectUDP()) {
                    DBG_ERROR("UDP connection failed.");
                    return INIT_STATE;
                }
                return ENABLE_PEAKS_STATE;
            }
            case ENABLE_PEAKS_STATE: {
                if(interrogator.enablePeakDatagrams()){
                    DBG_ERROR("Interrogator cannot be enabled.");
                    return CONNECT_UDP_STATE;
                }
                return READ_PEAKS_STATE;
            }
            case READ_PEAKS_STATE: {
                if(interrogator.readPacket()){
                    DBG_ERROR("Error receiving packet.");
                    return ENABLE_PEAKS_STATE;
                }
                
                PeakData peakData;
                interrogator.getPeaks(
                    peakData.timeStamp, 
                    peakData.numPeaks, 
                    peakData.peakVals, 
                    peakData.avgPeakVals
                );
                
                // Push peak data to queue (non-blocking)
                peakDataQueue.push(peakData);

                return READ_PEAKS_STATE;
            }
            case DISCONNECT_STATE: {
                if(interrogator.disconnectUDP()){
                    DBG_ERROR("Error while disconnecting");
                }
                return INIT_STATE;
            }
        }
    }

    // RT task callback function with performance monitoring
    static void rtTaskCallback(void* arg) {
        InterrogatorManager* manager = static_cast<InterrogatorManager*>(arg);
        int nCnt = 0;
        RTTIME tmCurrent = 0, tmPrev = 0, tmPeriod = 0, tmMaxPeriod = 0;

        while(1) {
            wait_next_period(NULL);
            tmCurrent = read_timer();
            
            manager->currentState = manager->processRTState();

            if(1001<nCnt){
                tmPeriod = tmCurrent - tmPrev;
                if(tmPeriod > tmMaxPeriod) tmMaxPeriod = tmPeriod;
            }
            tmPrev = tmCurrent;

            if(!(nCnt % 1000)){
                printf("Task Period: %lu.%06lums, Max Period: %lu.%06lums\n", tmPeriod/1000000, tmPeriod%1000000, tmMaxPeriod/1000000, tmMaxPeriod%1000000);
            }
            // printf("Task Period: %lu.%06lums, Max Period: %lu.%06lums\n", tmPeriod/1000000, tmPeriod%1000000, tmMaxPeriod/1000000, tmMaxPeriod%1000000);
            nCnt++;
        }
    }

    // NRT task callback function for data processing
    static void* nrtTaskCallback(void* arg) {
        InterrogatorManager* manager = static_cast<InterrogatorManager*>(arg);
        int nCnt = 0;
        
        // 스레드 우선순위 설정 (옵션)
        struct sched_param param;
        param.sched_priority = 50;  // 기본 우선순위
        pthread_setschedparam(pthread_self(), SCHED_OTHER, &param);

        while(1) {
            PeakData peakData = manager->peakDataQueue.pop();
            
            // 로깅 및 데이터 처리
            
            if(!(nCnt % 1000)){
                printf("Timestamp: %f\n", peakData.timeStamp);
                for (int i = 0; i < 4; ++i) {
                    printf("Num Peaks [%d]: %d\n", i, peakData.numPeaks[i]);
                    printf("Avg Peak [%d]: %f\n", i, peakData.avgPeakVals[i]);
                }
                printf("\n\n");
            }
            nCnt++;

        }
        return NULL;
    }

public:
    InterrogatorManager() : currentState(INIT_STATE), maxPeriod(0) {}

    int initRTTask() {
        // Create RT task
        if (create_rt_task(&rtTask, (const PCHAR)"INTERROGATOR_RT_TASK", 0, 99) != RET_SUCC) {
            DBG_ERROR("Failed to create RT task");
            return RET_FAIL;
        }

        // Set RT task period (1ms)
        if (set_task_period(&rtTask, SET_TM_NOW, ETHERCAT_TASK_PERIOD) != RET_SUCC) {
            DBG_ERROR("Failed to set RT task period");
            return RET_FAIL;
        }

        // Start RT task
        if (start_task(&rtTask, rtTaskCallback, this) != RET_SUCC) {
            DBG_ERROR("Failed to start RT task");
            return RET_FAIL;
        }

        // Create NRT task
        int result = pthread_create(&nrtThreadId, NULL, nrtTaskCallback, this);
        if (result != 0) {
            DBG_ERROR("Failed to create NRT thread");
            return RET_FAIL;
        }

        return RET_SUCC;
    }

    void signalHandler(int signal) {
        init_lowlevel_logger(TRUE);
        DBG_INFO("RT-POSIX has been signalled [%d]", signal);
        currentState = DISCONNECT_STATE;
    }
};

void globalSignalHandler(int signal) {
    static InterrogatorManager* managerInstance = nullptr;
    
    if (managerInstance == nullptr) {
        managerInstance = new InterrogatorManager();
    }
    
    managerInstance->signalHandler(signal);
    exit(0);
}

int main() {
    // Signal handler setup
    signal(SIGTERM, globalSignalHandler);
    signal(SIGINT, globalSignalHandler);
    init_lowlevel_logger(TRUE);    

    // Lock memory to prevent paging
    mlockall(MCL_CURRENT | MCL_FUTURE);

    // Initialize logger
    init_lowlevel_logger(FALSE);

    // Create interrogator manager
    InterrogatorManager manager;

    // Initialize RT and NRT tasks
    if (manager.initRTTask() != RET_SUCC) {
        DBG_ERROR("Failed to initialize tasks");
        return RET_FAIL;
    }

    // Infinite wait
    pause();

    return RET_SUCC;
}