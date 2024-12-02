
#include <stdio.h>
#include <iostream>
#include <unistd.h>
#include <signal.h>
#include <sys/mman.h>
#include "posix_rt.h"
#include "linux_CInterrogator.h"

#define ETHERCAT_TASK_PERIOD (1000000) // 1 ms

enum InterrogatorState // FSM
{
    INIT_STATE,
    CONNECT_UDP,
    ENABLE_PEAKS,
    READ_PEAKS,
    ERROR_STATE,
    DISCONNECT_STATE
};

struct InterrogatorData
{
    double timeStamp;
    short numPeaks[4];
    double peakVals[PEAK_CH * NUM_GRATING];
    double avgPeakVals[PEAK_CH];
};

CInterrogator interrogator;
InterrogatorState currentState;
InterrogatorData data;
POSIX_TASK g_taskInterrogator;
//POSIX_TASK g_taskKeyIn;

InterrogatorState DoTask()
{
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
            DBG_TRACE("UDP connection: SUCCESS!");
            return ENABLE_PEAKS;
        }
        case ENABLE_PEAKS: {
            interrogator.enablePeakDatagrams();
            return READ_PEAKS;
        }
        case READ_PEAKS: {
            interrogator.readPacket();            

            interrogator.getPeaks(data.timeStamp, data.numPeaks, data.peakVals, data.avgPeakVals);

            return READ_PEAKS;
        }
        case ERROR_STATE: {
            // error recovery
            interrogator.disconnectUDP();
            DBG_ERROR("Communication Error");
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

void proc_interrogator_task(void* arg)
{
    int nCnt = 0;

    RTTIME tmCurrent = 0, tmPrev = 0;
    RTTIME tmPeriod = 0;
    RTTIME tmMaxPeriod = 0;


    while (1)
    {
        wait_next_period(NULL);

        tmCurrent = read_timer();

        DoTask();

        if (1001 < nCnt)
        {
            tmPeriod = tmCurrent - tmPrev;

            if (tmPeriod > tmMaxPeriod) tmMaxPeriod = tmPeriod;
        }
        tmPrev = tmCurrent;

        if (!(nCnt % 1000)) // print every second
        {
            printf("Task Period:%lu.%06lums, Max Period:%lu.%06lu ms\n", tmPeriod / 1000000, tmPeriod % 1000000, tmMaxPeriod / 1000000, tmMaxPeriod % 1000000); 
            
            printf("Timestamp: %f\n", data.timeStamp);

            for (int i = 0; i < 4; ++i) {
                printf("Num Peaks [%d]: %d\n", i, data.numPeaks[i]);
                printf("Avg Peak [%d]: %f\n", i, data.avgPeakVals[i]);
            }

            printf("\n\n");
        }
        nCnt++;
    }
}

// void proc_keyboard_task(void* arg)
// {
//     while (1)
//     {
//         g_cKeyPress = (char)getch();
//     }
// }

void signal_handler(int nSignal)
{
    init_lowlevel_logger(TRUE);
    DBG_INFO("RT-POSIX has been signalled [%d]", nSignal);
    exit(0);
}

int main()
{
    signal(SIGTERM, signal_handler);
    signal(SIGINT, signal_handler);

    init_lowlevel_logger(TRUE);

    /* Lock all current and future pages from preventing of being paged to swap */
    mlockall(MCL_CURRENT | MCL_FUTURE);

    init_lowlevel_logger(FALSE);

    create_rt_task(&g_taskInterrogator, (const PCHAR)"INTERROGATOR_TASK", 0, 99);
    set_task_period(&g_taskInterrogator, SET_TM_NOW, (int)ETHERCAT_TASK_PERIOD); // 1 millisecond

    //create_rt_task(&g_taskKeyIn, (const PCHAR)"Keyboard Input Task", 0, 50);

    start_task(&g_taskInterrogator, &proc_interrogator_task, NULL);

    //start_task(&g_taskKeyIn, &proc_keyboard_task, NULL);

    pause();

    return RET_SUCC;
}
