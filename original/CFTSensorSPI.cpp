#include "CFTSensorSPI.h"

#include <Arduino.h>
#include <SPI.h>
#include "Macros.h"

#include <TeensyThreads.h>

//#include "TsyDMASPI.h"
#include <TsyDMASPI.h>

#define DMASIZE 56
uint8_t src[DMASIZE];
volatile uint8_t dest[DMASIZE];

void setSrc()
{
	for (size_t i = 0; i < DMASIZE; i++)
	{
		src[i] = i;
	}
}
void clrDest(uint8_t *dest_)
{
	memset((void *)dest_, 0x00, DMASIZE);
}
//void dumpBuffer(const volatile uint8_t *buf, const char *prefix)
//{
//	Serial.print(prefix);
//	for (size_t i = 0; i < DMASIZE; i++)
//	{
//		Serial.printf("0x%02x ", buf[i]);
//	}
//	Serial.print('\n');
//}
//void compareBuffers(const uint8_t *src_, const uint8_t *dest_)
//{
//	int n = memcmp((const void *)src_, (const void *)dest_, DMASIZE);
//	if (n == 0)
//	{
//		Serial.println("src and dest match");
//	}
//	else
//	{
//		Serial.println("src and dest don't match");
//		dumpBuffer(src_, " src: ");
//		dumpBuffer(dest_, "dest: ");
//	}
//}

CFTSensorSPI::CFTSensorSPI()
{
	init();
}


void CFTSensorSPI::init()
{
	//==== SPI with FT board ====// pcw0221

	//TsyDMASPI0.begin(CS_SPI_SLAVE, SPISettings(8000000, MSBFIRST, SPI_MODE0));
	
	//SPIM.begin(); // initialize it... 
	//pinMode(CS_SPI_SLAVE, OUTPUT);
	//digitalWrite(CS_SPI_SLAVE, HIGH);


	TsyDMASPI0.begin(CS_SPI_SLAVE, SPISettings(1000000, MSBFIRST, SPI_MODE0));


	//TsyDMASPI0.clearTransaction();
	//serialObj.dprintf("SPI Initialized");
}


volatile uint16_t read_buffer[SZ_SPI_PACKET_MAX] = { 0, };
volatile uint16_t addr_buffer[SZ_SPI_PACKET_MAX] = { 0, };


void CFTSensorSPI::reset()
{
	TsyDMASPI0.clearTransaction();


	//init();
	mEncodePos = 0;
	memset(rx_buffer, 0, sizeof(rx_buffer));
	//rx_buffer[SZ_SPI_RX_BUFFER] = { 0, };

	//find extra packetSize

	uint16_t addr[SZ_SPI_PACKET_MAX] = { 0, };
	uint16_t recv[SZ_SPI_PACKET_MAX] = { 0, };

	int extraPos = -1;

	for (uint32_t i = 0; i < SZ_SPI_PACKET_MAX; i++)
	{
		addr[i] = 0XFF00 + i;
		addr_buffer[i] = i;

	}


	TsyDMASPI0.transfer((uint8_t*)addr, (uint8_t*)recv, sizeof(uint16_t)*SZ_SPI_NUM_READ);

	
	for (uint32_t i = 0; i < SZ_SPI_NUM_READ; i++)
	{
		extraPos = (int)addr[i] - (int)recv[i];
		//serialObj.dprintf("%d: 0x%04x at  0x%04x", extraPos, read_buffer[i], addr[i]);
	}

	if (extraPos >= 0 && extraPos < 10)
	{
		isValid = true;
		offsetPos = extraPos;
		serialObj.dprintf("SPI is initializaed! extraPos: %d:", extraPos);
	}
	else
		serialObj.dprintf("SPI is not initializaed! extraPos: %d:", extraPos);


	//TsyDMASPI0.transfer((uint8_t*)addr, (uint8_t*)recv, sizeof(uint16_t)*SZ_SPI_NUM_READ);

	//for (uint32_t i = 0; i < SZ_SPI_NUM_READ; i++)
	//{
	//	extraPos = (int)addr[i] - (int)recv[i];
	//	serialObj.dprintf("%d: 0x%04x at  0x%04x", extraPos, recv[i], addr[i]);
	//}

	//if (extraPos >= 0 && extraPos < 10)
	//{
	//	isValid = true;
	//	offsetPos = extraPos;

	//}
	//else
	//	serialObj.dprintf("SPI is not initializaed! extraPos: %d:", extraPos);


}


void CFTSensorSPI::stop()
{

	//SPIM.end();
	//digitalWrite(CS_SPI_SLAVE, HIGH);


	
	/*TsyDMASPI0.clearTransaction();

	SPIM.end();
	digitalWrite(CS_SPI_SLAVE, HIGH);*/

	//TsyDMASPI0.end();
	isValid = false;
	serialObj.dprintf("SPI Stopped!");
	convCount = 0;
}

void CFTSensorSPI::readSensor()
{
	//data: 8byes(double) x 6 = 48 bytes
	//header: 8bytes each 2bytes: 0XAA, 0XBB, at the tail 0XCC, 0XDD
	//total number of bytes = header(8bytes) + data (48bytes) = 56 bytes

	if (!isValid && ((SZ_SPI_NUM_READ + offsetPos) < SZ_SPI_PACKET_MAX))
		return;


	uint16_t readout[SZ_SPI_NUM_READ] = { 0, };
	

	TsyDMASPI0.transfer((uint8_t*)addr_buffer, (uint8_t*)read_buffer, sizeof(uint16_t)*(SZ_SPI_NUM_READ + offsetPos));
	//TsyDMASPI0.yield();

	//memcpy(readout, &read_buffer[offsetPos], SZ_SPI_NUM_READ);
	for (uint32_t i = 0; i < SZ_SPI_NUM_READ; i++)
	{
		readout[i] = read_buffer[i + offsetPos];
	}

	////for debugging...
	//if (convCount == 0)
	//{

	//	for (int i = 0; i < SZ_SPI_NUM_READ; i++)
	//	{
	//		int extraPos = (int)addr_buffer[i] - (int)read_buffer[i];
	//		
	//		serialObj.dprintf("[%d] 0x%04x ", i, readout[i]);
	//	}

	//}

	memcpy(mFTData, &readout[2], sizeof(double) * 6);


	convCount++;
	////////////////////////////////////////////////////////


}

void CFTSensorSPI::getValue(double *dst)
{
	/*for (int i = 0; i < ARRAY_SIZE(mFTData); i++)
		dst[i] = mFTData[i];*/
	
	memcpy(dst, mFTData, sizeof(mFTData));


}



