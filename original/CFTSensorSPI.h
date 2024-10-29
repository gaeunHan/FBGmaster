#pragma once
#ifndef _FT_SENSOR_SPI_H_
#define _FT_SENSOR_SPI_H_

#include "Arduino.h"
//SPI with FT board ============//pcw0221
#define SPINUM 0

#if SPINUM == 0
#define SPIM SPI
#define CS_SPI_SLAVE 10

#elif SPINUM == 1
#define SPIM SPI1
#define CS_SPI_SLAVE 0

#elif SPINUM == 2
#define SPIM SPI2
#define CS_SPI_SLAVE 33


#endif


#define SZ_SPI_HEADER		sizeof(uint16_t)*4 //8
#define SZ_SPI_DATA			sizeof(double)*6 //8*6=48
#define SZ_SPI_TOTAL_PACKET	SZ_SPI_HEADER + SZ_SPI_DATA //8+48 = 56
#define SZ_SPI_NUM_READ		(SZ_SPI_HEADER + SZ_SPI_DATA)/2 //28
#define SZ_SPI_PACKET_MAX 	SZ_SPI_TOTAL_PACKET+20

#define SZ_SPI_RX_BUFFER	SZ_SPI_NUM_READ*100
 // pcw0221
//double rxData[6] = { 0.0, };


#include "CSerialPacket.h" //for debuggig
class CFTSensorSPI
{
public:
	CFTSensorSPI();


public:
	
	int convCount = 0;
	void stop();
	void init();
	uint16_t rx_buffer[SZ_SPI_RX_BUFFER] = { 0, };

	uint16_t mEncodePos = 0;

	//uint16_t recv[SZ_SPI_PACKET_MAX] = { 0, };
	//uint16_t addr[SZ_SPI_PACKET_MAX] = { 0, };


	double mFTData[6] = { 0.0, };

	void reset();

	
	void readSensor();
	bool isValid = false;
	uint16_t offsetPos = 0;

	void getValue(double *val);
	//void getValue(double &[]dst);

	bool bSuccess = false;

	CSerialPacket serialObj;

};

#endif