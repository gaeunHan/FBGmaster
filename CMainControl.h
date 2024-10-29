#pragma once

#ifndef _CMAINCONTROL_H
#define _CMAINCONTROL_H

#include "Macros.h"
#include "CSerialPacket.h"
#include <deque>
#include <thread>   // POSIX 스레드 사용
#include <mutex>    // POSIX 뮤텍스 사용

using namespace std;

#include "CInterrogator.h"
#include "CCalibration.h"

struct LoopStauts
{
	bool isRunning = false;
	
	bool rtInvalid = false;

	uint64_t loopCount = 0;
	uint32_t loopPeriod = 0; // in microsecond
	uint32_t currentTime = 0; // in microsecond

	uint32_t startTime = 0; // in millisencond
	uint32_t elapsedTime = 0; // in millisencond

	const uint32_t controlPeriod = TIMER_PERIOD_MAIN_CONTROL;

	const uint32_t streamRate = DATA_STREAM_RATE;



	
	void reset()
	{
		isRunning = 0;
		loopCount = 0, startTime = 0, currentTime = 0, loopPeriod = 0;
		elapsedTime = 0;
	}
	void tick()
	{
		elapsedTime = millis() - startTime; // in millisencond
		currentTime = micros(); // in microsecond

	}
	void update()
	{

		++loopCount;

		loopPeriod = micros() - currentTime; // in microsecond

	}
};

//extern std::deque< DataLog> dataQueue;
//volatile int queSize = 0;

class CMainControl
{
protected:

public:
#define BufSize 500
	uint16_t rx_buffer[BufSize] = { 0, }; // pcw0221
	double rxData[6] = { 0.0, };

	void init();
	CMainControl();
	CMainControl(CSerialPacket &_SerialPackiet);
	~CMainControl();
	void dprintf(const char * format, ...);

	void setSerialObj(CSerialPacket &serialPackiet);
	
	CSerialPacket _SerialPacket;
	LoopStauts rtStatus;
	// Settings for Teensy
	DataLog mDataLog;
	deque< DataLog> mDataLogQueue;

	volatile bool bUDPConnected = false;
	//for changing variables in running thread
	volatile bool bTimerTrigger = false;
	volatile bool bRun = false;
	//static bool bRun = false;
	volatile int queSize = 0;

	volatile bool bRunSPI = false;

	bool bFlag = 1;

	CInterrogator Interrogator;
	CCalibration Calibration;

	int CONTROL_COMMAND = 0;


	static void RunTimer(CMainControl *pControl)
	{
		//pControl->init();
		pControl->timerTrigger();
		
		
	}
	static void StopTimer(CMainControl *pControl)
	{
		pControl->stop();

	}
	static void streamThread(void *param)
	//static void streamThread(CMainControl *pControl)
	{
		CMainControl *pControl = reinterpret_cast<CMainControl*>(param);
		
		while (1)
		{
			pControl->streamDataQueue();
		}

	}
	static void mainThread(void *param)
	{
		CMainControl *pControl = reinterpret_cast<CMainControl*>(param);

		while (1) {
			
			pControl->run();
		
		}
	}
	static void interrogatorThread(void *param)
	{
		CMainControl *pControl = reinterpret_cast<CMainControl*>(param);

		while (1) {

			pControl->threadUDP();
			//yield();

		}
	}
	void timerTrigger();
	void loopTick();
	void loopUpdate();
	void stop();

	void mainJobControl();

	void setMainStartTimeTick();


	void setStatusLog();
	void setDataLog();

	void queueDataLog();

	
	void sendDataLog();
	void sendDataLog(DataLog &log);

	void streamDataQueue();

	void updateElpasedTime();

	void run(); //main function
	void threadUDP();
	
	float LPFx(float *Reg, float Fx, double *DenCoeff, double *NumCoeff);
	float LPFy(float *Reg, float Fy, double *DenCoeff, double *NumCoeff);
	float LPFz(float *Reg, float Fz, double *DenCoeff, double *NumCoeff);

	double LPFCoefA[3] = { 1.0000000e+00, -1.8226949e+00,   8.3718165e-01 };
	double LPFCoefB[3] = { 3.6216815e-03, 7.2433630e-03,  3.6216815e-03 };
	float FxReg[3] = { 0.0, 0.0, 0.0 };
	float FyReg[3] = { 0.0, 0.0, 0.0 };
	float FzReg[3] = { 0.0, 0.0, 0.0 };
};

#endif
