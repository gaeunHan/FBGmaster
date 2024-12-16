#include <stdio.h>
#include <iostream>
#include <unistd.h>
#include <signal.h>
#include <sys/mman.h>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <fstream>
#include <iomanip>
#include "posix_rt.h"
#include "linux_CInterrogator.h"
#include "EcatMasterBase.h"
#include "SlaveKistFT.h"

/*****************************************************************************/

using namespace std;

/*****************************************************************************/

#define ETHERCAT_TASK_PERIOD (2000000) // 2 ms
#define NO_OF_KIST_FT_SENSOR (1)
#define MAX_SAMPLES (5000)

/*****************************************************************************/
struct termios orig_termios;

// 터미널 설정을 원래대로 복원
void disable_raw_mode() {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}

// raw 모드 활성화: 입력 즉시 처리, 화면에 표시 x.
void enable_raw_mode() {
    tcgetattr(STDIN_FILENO, &orig_termios);
    atexit(disable_raw_mode); // 프로그램 종료 시 터미널 설정 복원

    struct termios raw = orig_termios;
    raw.c_lflag &= ~(ECHO | ICANON);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}
/*****************************************************************************/

// FSM states
enum InterrogatorState {
    INIT_STATE,
    CONNECT_UDP,
    ENABLE_PEAKS,
    READ_PEAKS,
    DISCONNECT_STATE
};

// Data structure for logging
struct LogData {
    RTTIME timestamp;
    double interrogatorData[PEAK_CH];
    int ftData[6];
};

// Global variables
POSIX_TASK rtTask;
POSIX_TASK g_taskKeyIn;

InterrogatorState currentState = INIT_STATE;
CInterrogator interrogator;

CEcatMasterBase g_clEMaster;
CSlaveKistFT g_clKistFTSensor[NO_OF_KIST_FT_SENSOR];

CHAR g_cKeyPress = ' ';
INT g_nFtMode[NO_OF_KIST_FT_SENSOR] = {0, };

vector<LogData> logBuffer;
mutex logMutex;
bool isReadyToLog = false;

/*****************************************************************************/

void nrtTaskInterrogator(void* arg) {
    while (true) {
        switch(currentState) {
            case INIT_STATE:
                if(interrogator.init()) {
                    currentState = DISCONNECT_STATE;
                } else {
                    currentState = CONNECT_UDP;
                }
                break;
            case CONNECT_UDP:
                if (interrogator.connectUDP() < 0) {
                    DBG_ERROR("UDP connection failed.");
                    currentState = INIT_STATE;
                } else {
                    currentState = ENABLE_PEAKS;
                }
                break;
            case ENABLE_PEAKS:
                interrogator.enablePeakDatagrams();
                currentState = READ_PEAKS;
                isReadyToLog = true;
                break;
            case READ_PEAKS:
                if(interrogator.readPacket()) {
                    currentState = CONNECT_UDP;
                    isReadyToLog = false;
                }
                break;
            case DISCONNECT_STATE:
                interrogator.disconnectUDP();
                currentState = INIT_STATE;
                isReadyToLog = false;
                break;
        }
        usleep(1000); // Sleep for 1ms to prevent busy waiting
    }
}

void DoTask()
{
    switch (g_cKeyPress)
    {
    // default operation
    case 'a':
    case 'A':
        for (int nFTCount = 0; nFTCount < (int)NO_OF_KIST_FT_SENSOR; nFTCount++)
        {
            g_clKistFTSensor[nFTCount].SetOperationMode((INT)DEFAULT_OP);
        }
        break;
    // adc raw value
    case 's':
    case 'S':
        for (int nFTCount = 0; nFTCount < (int)NO_OF_KIST_FT_SENSOR; nFTCount++)
        {
            g_clKistFTSensor[nFTCount].SetOperationMode((INT)ADC_VALUE);
        }
        break;
    // Bias Save To Flash
    case 'd':
    case 'D':
        for (int nFTCount = 0; nFTCount < (int)NO_OF_KIST_FT_SENSOR; nFTCount++)
        {
            g_clKistFTSensor[nFTCount].SetOperationMode((INT)BIAS_FLASH_SAVE);
        }
        break;
    // Bias Do Not Save To Flash
    case 'f':
    case 'F':
        for (int nFTCount = 0; nFTCount < (int)NO_OF_KIST_FT_SENSOR; nFTCount++)
        {
            g_clKistFTSensor[nFTCount].SetOperationMode((INT)BIAS_NO_SAVE);
        }
        break;
    default:
        for (int nFTCount = 0; nFTCount < (int)NO_OF_KIST_FT_SENSOR; nFTCount++)
        	g_nFtMode[nFTCount] = 0;
        break;
    }
    g_cKeyPress = ' ';
}

void proc_keyboard_task(void* arg)
{
    while (1)
    {
        g_cKeyPress = (char)getchar();
    }
}

void saveLogToFile() {
    ofstream logFile("/home/robogram/Test/CIIRobotRT/Examples/KISTFTSensor/logging/ft_fbg_02.txt");
    if (!logFile.is_open()) {
        DBG_ERROR("Failed to open log file");
        return;
    }

    logFile << fixed << setprecision(6); // 6th decimal place

    // logging in CSV style
    for (const auto& data : logBuffer) {
        // time logging in ms
        logFile << data.timestamp / 1000000 << ",";

        // logging FBG sensro data
        for (int i = 0; i < PEAK_CH; i++) {
            logFile << data.interrogatorData[i] << ",";
        }

        // logging FT sensor data
        for (int i = 0; i < 6; i++) {
            logFile << data.ftData[i] << (i < 5 ? "," : "\n");
        }
    }

    logFile.close();
    DBG_INFO("Log saved to file");
}

void signalHandler(int signal) {
    disable_raw_mode();
    init_lowlevel_logger(TRUE);
    DBG_INFO("Program terminating. Signal: %d", signal);
    saveLogToFile();
    system("stty sane");
    exit(0);
}

void rtTaskEcatAndLogging(void* arg) {
    RTTIME tmCurrent = 0, tmPrev = 0, tmEcat = 0;
    RTTIME tmEcatSend = 0;
    RTTIME tmPeriod = 0, tmResp = 0;
    RTTIME tmMaxPeriod = 0, tmMaxResp = 0;
    int nCnt = 0;

    double fbg_timestamp;
    short numPeaks[4];
    double peakVals[PEAK_CH * NUM_GRATING];

    LogData data;

    while (true) {
        wait_next_period(NULL);

        tmCurrent = read_timer();

        g_clEMaster.ReadSlaves();

        if (isReadyToLog && g_clEMaster.IsAllSlavesOp() && g_clEMaster.IsMasterOp()) {
            
            data.timestamp = read_timer();

            DoTask();

            interrogator.getPeaks(fbg_timestamp, numPeaks, peakVals, data.interrogatorData);

            // Get FT sensor data
            data.ftData[0] = g_clKistFTSensor[0].GetFx();
            data.ftData[1] = g_clKistFTSensor[0].GetFy();
            data.ftData[2] = g_clKistFTSensor[0].GetFz();
            data.ftData[3] = g_clKistFTSensor[0].GetTx();
            data.ftData[4] = g_clKistFTSensor[0].GetTy();
            data.ftData[5] = g_clKistFTSensor[0].GetTz();

            // Log data
            {
                lock_guard<mutex> lock(logMutex);
                logBuffer.push_back(data);
                if (logBuffer.size() >= MAX_SAMPLES) {
                    saveLogToFile();
                    disable_raw_mode();
                    signalHandler(SIGTERM);
                }
            }
        }

        tmEcatSend = read_timer();
        g_clEMaster.WriteSlaves(tmEcatSend);
        tmEcat = read_timer();

        if (1001 < nCnt)
        {
            tmPeriod = tmCurrent - tmPrev;
            tmResp = tmEcat - tmCurrent;

            if (tmPeriod > tmMaxPeriod) tmMaxPeriod = tmPeriod;
            if (tmResp > tmMaxResp) tmMaxResp = tmResp;
        }
        tmPrev = tmCurrent;

        //debugging
        if (!(nCnt % 500)) // print every second
        {
            if(isReadyToLog){
                printf("Task Period:%lu.%06lu ms Task Ecat Resp:%lu.%03lu us\n", tmPeriod / 1000000, tmPeriod % 1000000, tmResp / 1000, tmResp % 1000); 
                printf("Max Period:%lu.%06lu ms Max Ecat Resp:%lu.%03lu us\n", tmMaxPeriod / 1000000, tmMaxPeriod % 1000000, tmMaxResp / 1000, tmMaxResp % 1000);
                printf("Master State: %02x, Slave State: %02x, DomainState: %02x, No. Of. Slaves: %d\n", g_clEMaster.GetMasterState(), g_clEMaster.GetSlaveState(), g_clEMaster.GetDomainState(), g_clEMaster.GetRespSlaveNo());
                
                for (int nFTCount = 0; nFTCount < (int)NO_OF_KIST_FT_SENSOR; nFTCount++)
                {
                    printf("[SLAVE %d]\n", nFTCount);
                    printf("Fx: %d, Fy: %d, Fz: %d\n", g_clKistFTSensor[nFTCount].GetFx(), g_clKistFTSensor[nFTCount].GetFy(), g_clKistFTSensor[nFTCount].GetFz());
                    printf("Tx: %d, Ty: %d, Tz: %d\n", g_clKistFTSensor[nFTCount].GetTx(), g_clKistFTSensor[nFTCount].GetTy(), g_clKistFTSensor[nFTCount].GetTz());
                    printf("Count: %dn", g_clKistFTSensor[nFTCount].GetCount());
                }
                printf("\n\n");

                for (int i = 0; i < 4; ++i) {
                        printf("Num Peaks [%d]: %d\n", i, numPeaks[i]);
                        printf("Avg Peak [%d]: %f\n", i, data.interrogatorData[i]);
                    }
                    printf("\n\n");
            }
            else{
                printf("No response from Interrogator\n");
            }
            
        }   

        nCnt++;
    }
}

/*****************************************************************************/

int main() {
    // terminal setup
    enable_raw_mode();
    atexit(disable_raw_mode);

    // Signal handler setup
    signal(SIGTERM, signalHandler);
    signal(SIGINT, signalHandler);
    
    // Initialize logger
    init_lowlevel_logger(TRUE);


    // Initialize EtherCAT master and slaves
    for (int nFTCount = 0; nFTCount < NO_OF_KIST_FT_SENSOR; nFTCount++) {
        g_clEMaster.AddSlave(&g_clKistFTSensor[nFTCount]);
    }
    
    if (FALSE == g_clEMaster.Init((UINT32)ETHERCAT_TASK_PERIOD, TRUE)) {
        DBG_ERROR("Cannot Init EtherCAT Master");
        return RET_FAIL;
    }
    DBG_TRACE("EtherCAT Master: SUCCESS!");

    // Lock memory to prevent paging
    mlockall(MCL_CURRENT | MCL_FUTURE);

    init_lowlevel_logger(FALSE);

    // Create and start NRT task for Interrogator
    pthread_t nrtThreadId;
    int result = pthread_create(&nrtThreadId, NULL, (void *(*)(void *))nrtTaskInterrogator, NULL);
    if (result != 0) {
        DBG_ERROR("Failed to create NRT thread");
        return RET_FAIL;
    }

    // Create and start RT task for EtherCAT and logging
    if (create_rt_task(&rtTask, (const PCHAR)"ECAT_LOGGING_RT_TASK", 0, 99) != RET_SUCC) {
        DBG_ERROR("Failed to create RT task");
        return RET_FAIL;
    }
    if (set_task_period(&rtTask, SET_TM_NOW, ETHERCAT_TASK_PERIOD) != RET_SUCC) {
        DBG_ERROR("Failed to set RT task period");
        return RET_FAIL;
    }
    if (create_rt_task(&g_taskKeyIn, (const PCHAR)"Keyboard Input Task", 0, 50) != RET_SUCC) {
        DBG_ERROR("Failed to create Keyboard Input Task");
        return RET_FAIL;
    }
    if (start_task(&rtTask, rtTaskEcatAndLogging, NULL) != RET_SUCC) {
        DBG_ERROR("Failed to start RT task");
        return RET_FAIL;
    }
    if (start_task(&g_taskKeyIn, &proc_keyboard_task, NULL) != RET_SUCC) {
        DBG_ERROR("Failed to create Keyboard Input Task");
        return RET_FAIL;
    }

    pause();

    disable_raw_mode();

    return RET_SUCC;
}