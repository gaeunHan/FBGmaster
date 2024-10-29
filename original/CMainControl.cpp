#include "CMainControl.h"
#include "Macros.h"

#include <SPI.h>
void CMainControl::init()
{
	FTSensor.reset();

	rtStatus.reset();
	

}
void CMainControl::stop()
{
	bRunSPI = false;
	FTSensor.stop();

	
	bRun = false;

	rtStatus.isRunning = false;
	dprintf("Stopped Main Control");
	
}

CMainControl::CMainControl()
{
	//FTSensor.serialObj = _SerialPacket;
	   
}
CMainControl::CMainControl(CSerialPacket&serialPackiet)
{
	this->_SerialPacket = serialPackiet;

	FTSensor.serialObj = _SerialPacket;


}
void CMainControl::setSerialObj(CSerialPacket &serialPackiet)
{
	_SerialPacket = serialPackiet;
}
CMainControl::~CMainControl()
{

}
void CMainControl::dprintf(const char * format, ...)
{
	va_list args;
	va_start(args, format);

	char temp[100];
	vsprintf(temp, format, args);

	_SerialPacket.dprintf(temp);


}
void CMainControl::run()
{
	if (bTimerTrigger)
	{
		Threads::Mutex mylock;
		mylock.lock();

		setMainStartTimeTick();	//at the beginning of runTimer

//////////////////////////////////////////////////////////////
		loopTick(); //for updating timestamp

		//////////////////////////////////////////////////////////////
		// Define Run Main Function


		mainJobControl();


		

		//dprintf("Val: %f", mDataLog.EMSensor[0]);
	//////////////////////////////////////////////////////////////



		queueDataLog();

		loopUpdate(); //for updating loop info, which should run at the end of control loop



		bTimerTrigger = false;
		mylock.unlock();

	}
}
void CMainControl::timerTrigger() //timer tick
{
	if (!bTimerTrigger)
	{
		
		bTimerTrigger = true;

	}
	else
	{
		//dprintf("RT Task is incomplete!!!");
	}



}
void CMainControl::loopTick()
{
	rtStatus.tick();
}
void CMainControl::loopUpdate()
{
	rtStatus.update();


	if (rtStatus.loopPeriod > rtStatus.controlPeriod)
	{
		dprintf("Exceed RT Loop!!!, loopPeriod: %d, controlPeriod: %d",
			rtStatus.loopPeriod, rtStatus.controlPeriod);

		rtStatus.rtInvalid = true;
	
	}
	else
		rtStatus.rtInvalid = false;
}
void CMainControl::updateElpasedTime()
{

}

void CMainControl::mainJobControl()
{

	// get wavelength value
	double timeStamp = 0.0;
	short numPeaks[4] = { 0, };
	double peakVals[PEAK_CH*NUM_GRATING] = { 0.0, };
	double avgPeakVals[PEAK_CH] = { 0.0, };

	Interrogator.getPeaks(timeStamp, numPeaks, peakVals, avgPeakVals);

	// get FT sensor value
	double rawFTVals[6] = { -1.0, };
	FTSensor.readSensor();
	FTSensor.getValue(rawFTVals);

	double waveforce[3] = { 0.0, };
	Calibration.calCalib(avgPeakVals);
	Calibration.getValue(waveforce);


	float LPFforce[3] = { 0.0, };
	LPFforce[0] = LPFx(FxReg, (float)waveforce[0], LPFCoefA, LPFCoefB);
	LPFforce[1] = LPFy(FyReg, (float)waveforce[1], LPFCoefA, LPFCoefB);
	LPFforce[2] = LPFz(FzReg, (float)waveforce[2], LPFCoefA, LPFCoefB);

	//
	memcpy(&mDataLog.itimeStamp, &timeStamp, sizeof(mDataLog.itimeStamp));
	memcpy(&mDataLog.avgWavelength, &avgPeakVals, sizeof(mDataLog.avgWavelength));
	memcpy(mDataLog.FTSensorRaw, rawFTVals, sizeof(mDataLog.FTSensorRaw));
	memcpy(mDataLog.FBGForce, waveforce, sizeof(mDataLog.FBGForce));
	memcpy(mDataLog.LPFForce, LPFforce, sizeof(LPFforce));

	
}

void CMainControl::setMainStartTimeTick()
{
	if (rtStatus.loopCount == 0)
	{
		dprintf("Start Main Control");
		rtStatus.startTime = millis();

		rtStatus.isRunning = true;
		bRun = true;

		bRunSPI = true;

		
	}
}



void CMainControl::setStatusLog()
{

	
	uint32_t rtInvalid = rtStatus.rtInvalid & 0b0001;

	uint32_t testFlag = (bFlag << 1) & 0b0010;

	mDataLog.statusFlag = testFlag | rtInvalid;

}

void CMainControl::setDataLog()
{
	mDataLog.timeStamp = rtStatus.elapsedTime;
	mDataLog.loopPeriod = rtStatus.loopPeriod;


	//memcpy(&mDataLog.data, &mParamTask.raw_vacPressure, sizeof(mParamTask.raw_vacPressure));
	//If data is array form....
	//memcpy(&log.data, &mParamTask.raw_vacPressure, sizeof(mParamTask.raw_vacPressure));


}

void CMainControl::FTSPIThread()
{
	if (bRun)
	{
		if(bRunSPI)
			FTSensor.readSensor();
	}

}
void CMainControl::threadUDP()
{
	if (bUDPConnected)
		Interrogator.readPacket();

}

void CMainControl::streamDataQueue()
{

	if (bRun)
	{

		//if (mDataLogQueue.size() > 0)
		if (queSize > 0)
		{
			DataLog curDataLog;

			Threads::Mutex mylock;
			mylock.lock();
			curDataLog = mDataLogQueue.front();
			mDataLogQueue.pop_front();
			--queSize;
			mylock.unlock();

			sendDataLog(curDataLog);


		}


	}

	

}


void CMainControl::queueDataLog() {

	setStatusLog();

	setDataLog();

	/*Threads::Mutex mylock;
	mylock.lock();*/
	mDataLogQueue.push_back(mDataLog);
	queSize++;
	//mylock.unlock();
	//dprintf("Queue In: %d/%d", queSize, mDataLogQueue.size());
}



void CMainControl::sendDataLog()
{
	sendDataLog(mDataLog);
}


void CMainControl::sendDataLog(DataLog &log)
{
	_SerialPacket.setCode(COMMAND_CODE_DATA_STREAM);
	_SerialPacket.encode(log);
	_SerialPacket.send_packet();
}
float CMainControl::LPFx(float *Reg, float Fz, double *DenCoeff, double *NumCoeff)
{
	float y;
	Reg[0] = DenCoeff[0] * Fz - DenCoeff[1] * Reg[1] - DenCoeff[2] * Reg[2];
	y = NumCoeff[0] * Reg[0] + NumCoeff[1] * Reg[1] + NumCoeff[2] * Reg[2];

	Reg[1] = Reg[0];
	Reg[2] = Reg[1];
	return y;
}
float CMainControl::LPFy(float *Reg, float Fz, double *DenCoeff, double *NumCoeff)
{
	float y;
	Reg[0] = DenCoeff[0] * Fz - DenCoeff[1] * Reg[1] - DenCoeff[2] * Reg[2];
	y = NumCoeff[0] * Reg[0] + NumCoeff[1] * Reg[1] + NumCoeff[2] * Reg[2];

	Reg[1] = Reg[0];
	Reg[2] = Reg[1];
	return y;
}
float CMainControl::LPFz(float *Reg, float Fz, double *DenCoeff, double *NumCoeff)
{
	float y;
	Reg[0] = DenCoeff[0] * Fz - DenCoeff[1] * Reg[1] - DenCoeff[2] * Reg[2];
	y = NumCoeff[0] * Reg[0] + NumCoeff[1] * Reg[1] + NumCoeff[2] * Reg[2];

	Reg[1] = Reg[0];
	Reg[2] = Reg[1];
	return y;
}