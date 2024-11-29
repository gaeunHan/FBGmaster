#include <stdio.h>
#include <iostream>
#include <unistd.h>
#include <signal.h>
#include <sys/mman.h>
#include "posix_rt.h"
#include "linux_CInterrogator.h"

// FSM
enum InterrogatorState {
    INIT_STATE,
    CONNECT_UDP,
    ENABLE_PEAKS,
    READ_PEAKS,
    ERROR_STATE,
    DISCONNECT_STATE
};

// Interrogator communication class
class InterrogatorManager {
private:
    CInterrogator interrogator;
    InterrogatorState currentState;
    POSIX_TASK rtTask;

    // state translation logic
    InterrogatorState processState() {
        switch(currentState) {
            case INIT_STATE: {
                interrogator.init();
                return CONNECT_UDP;
            }
            case CONNECT_UDP: {
                if (interrogator.connectUDP() < 0) {
                    DBG_ERROR("UDP connection fail.");
                    return ERROR_STATE;
                }
                return ENABLE_PEAKS;
            }
            case ENABLE_PEAKS: {
                interrogator.enablePeakDatagrams();
                return READ_PEAKS;
            }
            case READ_PEAKS: {
                interrogator.readPacket();
                
                double timeStamp;
                short numPeaks[4];
                double peakVals[PEAK_CH * NUM_GRATING];
                double avgPeakVals[PEAK_CH];

                interrogator.getPeaks(timeStamp, numPeaks, peakVals, avgPeakVals);
                
                // print the data
                DBG_INFO("Timestamp: %f", timeStamp);
                for (int i = 0; i < 4; ++i) {
                    DBG_INFO("Num Peaks [%d]: %d", i, numPeaks[i]);
                    DBG_INFO("Avg Peak [%d]: %f", i, avgPeakVals[i]);
                }

                return READ_PEAKS;
            }
            case ERROR_STATE: {
                // error recovery
                interrogator.disconnectUDP();
                sleep(1);
                return INIT_STATE;
            }
            case DISCONNECT_STATE: {
                interrogator.disconnectUDP();
                return INIT_STATE;
            }
            default:
                return ERROR_STATE;
        }
    }

    // RT task callback function
    static void rtTaskCallback(void* arg) {
        InterrogatorManager* manager = static_cast<InterrogatorManager*>(arg);
        
        while(1) {
            manager->currentState = manager->processState();
            wait_next_period(NULL);
        }
    }

public:
    InterrogatorManager() : currentState(INIT_STATE) {}

    int initRTTask() {
        // create RT task
        if (create_rt_task(&rtTask, (const PCHAR)"INTERROGATOR_TASK", 0, 97) != RET_SUCC) {
            DBG_ERROR("fail to create RT task");
            return RET_FAIL;
        }

        // set RT period
        if (set_task_period(&rtTask, SET_TM_NOW, 1000000) != RET_SUCC) {
            DBG_ERROR("fail to set RT period");
            return RET_FAIL;
        }

        // start RT task
        if (start_task(&rtTask, rtTaskCallback, this) != RET_SUCC) {
            DBG_ERROR("fail to start RT task");
            return RET_FAIL;
        }

        return RET_SUCC;
    }

    void signalHandler(int signal) {
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
    // signal handler
    signal(SIGTERM, globalSignalHandler);
    signal(SIGINT, globalSignalHandler);

    /* Lock all current and future pages from preventing of being paged to swap */
    mlockall(MCL_CURRENT | MCL_FUTURE);

    // init logger
    init_lowlevel_logger(TRUE);

    // create an interrogator manager
    InterrogatorManager manager;

    // init RT task
    if (manager.initRTTask() != RET_SUCC) {
        DBG_ERROR("fail to init RT task");
        return RET_FAIL;
    }

    // infinite wait
    pause();

    return RET_SUCC;
}