#include <stdio.h>
#include <iostream>
#include <unistd.h>
#include <signal.h>
#include <sys/mman.h>
#include "posix_rt.h"
#include "linux_CInterrogator.h"

// FSM 상태 열거형
enum InterrogatorState {
    INIT_STATE,
    CONNECT_UDP,
    ENABLE_PEAKS,
    READ_PEAKS,
    ERROR_STATE,
    DISCONNECT_STATE
};

// Interrogator 통신 클래스
class InterrogatorManager {
private:
    CInterrogator interrogator;
    InterrogatorState currentState;
    POSIX_TASK rtTask;

    // 상태 전이 로직
    InterrogatorState processState() {
        switch(currentState) {
            case INIT_STATE: {
                interrogator.init();
                return CONNECT_UDP;
            }
            case CONNECT_UDP: {
                if (interrogator.connectUDP() < 0) {
                    DBG_ERROR("UDP 연결 실패");
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
                
                // 로깅 또는 추가 처리
                DBG_INFO("Timestamp: %f", timeStamp);
                for (int i = 0; i < 4; ++i) {
                    DBG_INFO("Num Peaks [%d]: %d", i, numPeaks[i]);
                    DBG_INFO("Avg Peak [%d]: %f", i, avgPeakVals[i]);
                }

                return READ_PEAKS;
            }
            case ERROR_STATE: {
                // 오류 복구 로직
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

    // RT 태스크 콜백 함수
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
        // RT 태스크 생성 및 설정
        if (create_rt_task(&rtTask, (const PCHAR)"INTERROGATOR_TASK", 0, 97) != RET_SUCC) {
            DBG_ERROR("RT 태스크 생성 실패");
            return RET_FAIL;
        }

        // 1ms 주기 설정
        if (set_task_period(&rtTask, SET_TM_NOW, 1000000) != RET_SUCC) {
            DBG_ERROR("태스크 주기 설정 실패");
            return RET_FAIL;
        }

        // 태스크 시작
        if (start_task(&rtTask, rtTaskCallback, this) != RET_SUCC) {
            DBG_ERROR("RT 태스크 시작 실패");
            return RET_FAIL;
        }

        return RET_SUCC;
    }

    void signalHandler(int signal) {
        DBG_INFO("시그널 수신: %d", signal);
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
    // 시그널 핸들러 설정
    signal(SIGTERM, globalSignalHandler);
    signal(SIGINT, globalSignalHandler);

    // 메모리 잠금
    mlockall(MCL_CURRENT | MCL_FUTURE);

    // 로거 초기화
    init_lowlevel_logger(TRUE);

    // Interrogator 관리자 생성
    InterrogatorManager manager;

    // RT 태스크 초기화
    if (manager.initRTTask() != RET_SUCC) {
        DBG_ERROR("RT 태스크 초기화 실패");
        return RET_FAIL;
    }

    // 무한 대기
    pause();

    return RET_SUCC;
}