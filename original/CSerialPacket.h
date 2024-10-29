#pragma once
#ifndef _CSERIALPACKET_H
#define _CSERIALPACKET_H

#include "Arduino.h"
#include "Macros.h"
#include <string>
using namespace std;




class CSerialPacket
{
public:
	CSerialPacket();
	CSerialPacket(usb_serial_class *pObj);

	~CSerialPacket();
private:
	bool mFlag_Recv_Wait = false;
	bool mReadDataReady = false;
	bool mReadCommandReady = true;

	usb_serial_class *pSerial;
public:
	int mNumByteIn = 0;
	uint16_t mCommand_Code = 0;
	int mDecodePos = 0;
	char mRxBuffer[SERIAL_SZ_RX_BUFFER];
	char mTxBuffer[SERIAL_SZ_TX_BUFFER];
	char mCharBuffer[SERIAL_SZ_CHAR_BUFFER];

	int mEncodePos = 0;
	int mSzPacket = -1;


private:
	void init();
	void setSerialObj(usb_serial_class *pObj);


public:
	////////////////////////////////////////////////////////////////////////////////////////////////////

	unsigned int decodeStr(char *serial, size_t sz)
	{
		memcpy(serial, &mRxBuffer[mDecodePos], sz);
		mDecodePos += sz;
		return  mDecodePos;
	}
	unsigned int encodeStr(char *serial, size_t sz)
	{
		memcpy(&mTxBuffer[mEncodePos], serial, sz);

		mEncodePos += sz;
		return mEncodePos;
	}


	template<class T>
	unsigned int decode(T & val)
	{
		unsigned int sz = sizeof(val);
		memcpy(&val, &mRxBuffer[mDecodePos], sz);
		mDecodePos += sz;
		return  mDecodePos;
	}


	template<class T>
	unsigned int encode(T &val)
	{
		unsigned int bytes = sizeof(val);


		if (mEncodePos + bytes <= sizeof(mTxBuffer))
		{
			memcpy(&mTxBuffer[mEncodePos], &val, bytes);

			mEncodePos += bytes;
		}
		return mEncodePos;

	}
	



	template<typename  T>
	void message_out(uint16_t code, T & val, uint16_t varType)
	{	

		setCode(code);

		encode(varType);

		encode(val);

		send_packet();
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////
	uint32_t dprintf(string &str);
	uint32_t dprintf(const char * format, ...);

	

	//Send value only: code with COMMAND_CODE_TRNS_DATA
	void message_out(int16_t & val);
	void message_out(int & val);
	void message_out(uint32_t & val);
	void message_out(double & val);
	void message_out(float & val);

	//Send specific command code and value...
	void message_out(uint16_t code, int16_t & val);
	void message_out(uint16_t code, int & val);
	void message_out(uint16_t code, uint32_t & val);
	void message_out(uint16_t code, double & val);
	void message_out(uint16_t code, float & val);



	//Send recieved command-code back to PC
	void echo_command(uint16_t command_code);
	void echo_command()
	{
		echo_command(mCommand_Code);
	}
	//Send recieved value back to PC
	void echo_message();



	void send_packet();
	bool procPacket();
	int readPacketSize();
	void setCode(uint16_t code);


	void setFlagRecvWait(bool flag);

	bool getFlagRecvWait();

	bool isCommandReadReady();

	bool isDataReadReady();


	void setCommandReadReady(bool flag);

	void setDataReadReady(bool flag);


	uint16_t getCommandCode();

	

	
};
#endif
