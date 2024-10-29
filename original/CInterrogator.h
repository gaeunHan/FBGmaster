// CInterrogator.h

#ifndef _CINTERROGATOR_h
#define _CINTERROGATOR_h

#if defined(ARDUINO) && ARDUINO >= 100
#include "arduino.h"
#else
#include "WProgram.h"
#endif

#include <iostream>
#include <string>
#include <stdio.h>
#include "Macros.h"
#include "CSerialPacket.h"
#include <NativeEthernet.h>
#include <NativeEthernetUdp.h>
#include <NativeEthernetClient.h>



struct hWriteHeader
{
	uint8_t requestOption;
	uint8_t reserved;
	uint16_t commandSize;
	uint32_t argSize;
};

class CInterrogator
{
protected:


public:


	CSerialPacket serialObj;

	EthernetUDP UdpComm;
	EthernetClient TcpComm;

	uint8_t fconnectUDP = 0;
	uint8_t fhardwareStatus = -1; // If this flag = 0, no ethernet shield
	uint8_t flinkStatus = -1; // If this flag = 0, not connected with ethernet cable
	uint8_t fcnnectClient = 0;
	uint8_t udpState = 255;
	uint32_t ulTimeStampH;
	uint32_t ulTimeStampL;

	bool bUDPConnected = false;


	int ffailRead = 2;

	double timeStamp;
	double waveLength[PEAK_CH*NUM_GRATING];
	short numPeaks[4];

	unsigned int mLength;
	unsigned int mDecodePos = 0;
	unsigned char *mPacket;
	double avgPeak[4] = { 0.0, };

	int tcnt = 1;

public:

	bool mConvertByteOrder = true;
	uint16_t convertByteOrder(const short a);
	uint32_t convertByteOrder(const int a);
	uint64_t convertByteOrder(const uint64_t a);

public:

	void init();
	int connectUDP();
	int disconnectUDP();
	void enablePeakDatagrams();
	void teensyMAC(uint8_t *mac);
	int writeCommand(std::string command, std::string argument, uint8_t requestOptions);
	void readPacket();
	void procPacket(uint8_t *packetChunk, int packetLength);
	void getPeaks(double &tStamp, short *numpeaks, double *peaks, double *avgPeaks);

};


#endif

