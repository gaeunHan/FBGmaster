// CInterrogator.h

#ifndef _CINTERROGATOR_h
#define _CINTERROGATOR_h

#include <iostream>
#include <string>
#include <stdio.h>
#include <cstring> // for memset()
#include "Macros.h"
#include <sys/socket.h> // For socket functions
#include <arpa/inet.h> // For IP address conversion
#include <unistd.h>    // For close function

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

	int tcpSockfd; // TCP 소켓 파일 디스크립터
    int udpSockfd; // UDP 소켓 파일 디스크립터
    struct sockaddr_in server_addr; // 서버 주소 구조체

	uint8_t fconnectUDP = 0;
	uint8_t udpState = 255;
	uint32_t ulTimeStampH;
	uint32_t ulTimeStampL;

    /* Sensing data var. */
	double timeStamp;
	double waveLength[PEAK_CH*NUM_GRATING];
	short numPeaks[4];
	unsigned int mLength;
	unsigned int mDecodePos = 0;
	unsigned char *mPacket;
	double avgPeak[4] = { 0.0, };

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
	int writeCommand(std::string command, std::string argument, uint8_t requestOptions);
	void readPacket();
	void procPacket(uint8_t *packetChunk, int packetLength);
	void getPeaks(double &tStamp, short *numpeaks, double *peaks, double *avgPeaks);

};


#endif

