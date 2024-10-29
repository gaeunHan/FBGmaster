
#include <SPI.h>
#include "CInterrogator.h"

uint16_t teensyPort = 51970;
IPAddress micronsIP(10, 0, 0, 55);
uint16_t micronsPort = 51971;

void CInterrogator::init()
{


}



int CInterrogator::disconnectUDP() {
	writeCommand("#DisableUdpPeakDatagrams", "", 0);
	fconnectUDP = 0;

	TcpComm.stop();


	UdpComm.flush();
	UdpComm.stop();



	return 0;
}



int CInterrogator::connectUDP() {

	int res = -1;

	if (Ethernet.hardwareStatus() == EthernetNoHardware) {
		//Serial.println("Ethernet shield was not found.  Sorry, can't run without hardware. :(");
		fhardwareStatus = 0;
		udpState = 1;
	}
	else fhardwareStatus = 1;

	if (Ethernet.linkStatus() == LinkOFF) {
		flinkStatus = 0;
		udpState = 2;
	}
	else flinkStatus = 1;

	if ((fhardwareStatus | flinkStatus) == 1) {
		if (TcpComm.connect(micronsIP, micronsPort)) {
			UdpComm.begin(teensyPort);
			udpState = 0;
		}
		else {
			udpState = 3;
		}
	}


	//TcpComm.connect(micronsIP, micronsPort);
	//UdpComm.begin(teensyPort);
	res = 1;
	return res;

}



void CInterrogator::enablePeakDatagrams() {
	if (TcpComm.connected()) {
		udpState = 4;
		std::string addressPort = "10.0.0.2 51970";
		writeCommand("#EnableUdpPeakDatagrams", addressPort, 0);
	}
	else {
		udpState = 5;
	}
}


int CInterrogator::writeCommand(std::string command, std::string argument, uint8_t requestOptions)
{
	hWriteHeader writeHeader;
	writeHeader.requestOption = requestOptions;
	writeHeader.commandSize = command.size();
	writeHeader.argSize = uint32_t(argument.size());

	//write using tcp/ip
	TcpComm.write((char *)&writeHeader, sizeof(writeHeader));
	TcpComm.write(command.c_str(), (int)writeHeader.commandSize);
	TcpComm.write(argument.c_str(), (int)writeHeader.argSize);

	return 0;
}



void CInterrogator::readPacket()
{

	uint8_t recvBuff[TOTAL_RECV_BUFF_BYTES] = { 0, };
	int packetSize = UdpComm.parsePacket();

	if (UdpComm.available()) {
		UdpComm.read(recvBuff, packetSize);
		procPacket(recvBuff, packetSize);
	}

}

void CInterrogator::procPacket(uint8_t *packetChunk, int packetLength)
{
	double tempTime = 0.0;
	//peakContents peakData;
	memcpy(&ulTimeStampH, &packetChunk[16], sizeof(ulTimeStampH));
	memcpy(&ulTimeStampL, &packetChunk[20], sizeof(ulTimeStampL));


	timeStamp = double(ulTimeStampH) + 1e-9*double(ulTimeStampL);

	static double timeStampEX = timeStamp;
	timeStamp -= timeStampEX;



	memcpy(numPeaks, &packetChunk[24], sizeof(numPeaks));
	memcpy(waveLength, &packetChunk[56], sizeof(waveLength));
	int cntPeak = 0;
	//calculate average
	for (int i = 0; i < 4; i++) {
		short peaksCh = numPeaks[i];
		double sumPeaks = 0.0;

		for (int j = 0; j < peaksCh; j++) {

			sumPeaks += waveLength[cntPeak];
			cntPeak++;
		}
		if (sumPeaks != 0) {
			avgPeak[i] = sumPeaks / (double)peaksCh;
		}
	}
}


void CInterrogator::getPeaks(double &tStamp, short *numpeaks, double *peaks, double *avgPeaks) {

	tStamp = timeStamp;
	memcpy(numpeaks, numPeaks, sizeof(numPeaks));
	memcpy(peaks, waveLength, sizeof(waveLength));
	memcpy(avgPeaks, avgPeak, sizeof(avgPeak));
}

void CInterrogator::teensyMAC(uint8_t *mac) {

	//	static char teensyMac[23];

#if defined(HW_OCOTP_MAC1) && defined(HW_OCOTP_MAC0)

	for (uint8_t by = 0; by < 2; by++) mac[by] = (HW_OCOTP_MAC1 >> ((1 - by) * 8)) & 0xFF;
	for (uint8_t by = 0; by < 4; by++) mac[by + 2] = (HW_OCOTP_MAC0 >> ((3 - by) * 8)) & 0xFF;

#define MAC_OK

#else

	mac[0] = 0x04;
	mac[1] = 0xE9;
	mac[2] = 0xE5;

	uint32_t SN = 0;
	__disable_irq();

#if defined(HAS_KINETIS_FLASH_FTFA) || defined(HAS_KINETIS_FLASH_FTFL)

	FTFL_FSTAT = FTFL_FSTAT_RDCOLERR | FTFL_FSTAT_ACCERR | FTFL_FSTAT_FPVIOL;
	FTFL_FCCOB0 = 0x41;
	FTFL_FCCOB1 = 15;
	FTFL_FSTAT = FTFL_FSTAT_CCIF;
	while (!(FTFL_FSTAT & FTFL_FSTAT_CCIF)); // wait
	SN = *(uint32_t *)&FTFL_FCCOB7;

#define MAC_OK

#elif defined(HAS_KINETIS_FLASH_FTFE)

	kinetis_hsrun_disable();
	FTFL_FSTAT = FTFL_FSTAT_RDCOLERR | FTFL_FSTAT_ACCERR | FTFL_FSTAT_FPVIOL;
	*(uint32_t *)&FTFL_FCCOB3 = 0x41070000;
	FTFL_FSTAT = FTFL_FSTAT_CCIF;
	while (!(FTFL_FSTAT & FTFL_FSTAT_CCIF)); // wait
	SN = *(uint32_t *)&FTFL_FCCOBB;
	kinetis_hsrun_enable();

#define MAC_OK

#endif

	__enable_irq();

	for (uint8_t by = 0; by < 3; by++) mac[by + 3] = (SN >> ((2 - by) * 8)) & 0xFF;

#endif

#ifdef MAC_OK
	//sprintf(teensyMac, "MAC: %02x:%02x:%02x:%02x:%02x:%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
	//Serial.println(teensyMac);
#else
	Serial.println("ERROR: could not get MAC");
#endif
}

