#include "CSerialPacket.h"

#include <TeensyThreads.h>


CSerialPacket::CSerialPacket()
{
	init();
}

CSerialPacket::CSerialPacket(usb_serial_class *pObj)
{
	setSerialObj(&Serial);
	init();
}

CSerialPacket::~CSerialPacket()
{
}

void CSerialPacket::init()
{
	mFlag_Recv_Wait = false;
}
void CSerialPacket::setSerialObj(usb_serial_class *pObj)
{
	pSerial = pObj;
}

void CSerialPacket::message_out(uint16_t code, int & val)
{
	message_out(code, val, VAR_TYPE_INT32);
}
void CSerialPacket::message_out(uint16_t code, uint32_t & val)
{
	message_out(code, val, VAR_TYPE_UINT32);
}
void CSerialPacket::message_out(uint16_t code, double & val)
{
	message_out(code, val, VAR_TYPE_DOUBLE);
}
void CSerialPacket::message_out(uint16_t code, float & val)
{
	message_out(code, val, VAR_TYPE_FLOAT);
}
void CSerialPacket::message_out(uint16_t code, int16_t & val)
{
	message_out(code, val, VAR_TYPE_INT16);
}

void CSerialPacket::message_out(int16_t& val)
{	//CSerialPacket 이름공간 
	message_out(COMMAND_CODE_TRNS_DATA, val);
}
void CSerialPacket::message_out(int& val)
{	//CSerialPacket 이름공간 
	message_out(COMMAND_CODE_TRNS_DATA, val);
}
void CSerialPacket::message_out(uint32_t& val)
{	//CSerialPacket 이름공간 
	message_out(COMMAND_CODE_TRNS_DATA, val);
}
void CSerialPacket::message_out(double& val)
{	//CSerialPacket 이름공간 
	message_out(COMMAND_CODE_TRNS_DATA, val);
}
void CSerialPacket::message_out(float& val)
{	//CSerialPacket 이름공간 
	CSerialPacket::message_out(COMMAND_CODE_TRNS_DATA, val);
}


void CSerialPacket::echo_message()
{
	uint16_t varType = 0;
	decode(varType);

	switch (varType)
	{
	case VAR_TYPE_INT16:
	{
		int16_t message = 0;
		decode(message);
		message_out(COMMAND_CODE_TRNS_DATA, message, varType);
		break;
	}
	case VAR_TYPE_UINT16:
	{
		uint16_t message = 0;
		decode(message);
		message_out(COMMAND_CODE_TRNS_DATA, message, varType);
		break;
	}
	case VAR_TYPE_INT32:
	{
		int32_t message = 0;
		decode(message);
		message_out(COMMAND_CODE_TRNS_DATA, message, varType);
		break;
	}
	case VAR_TYPE_UINT32:
	{
		uint32_t message = 0;
		decode(message);
		message_out(COMMAND_CODE_TRNS_DATA, message, varType);
		break;
	}
	case VAR_TYPE_FLOAT:
	{
		float message = 0;
		decode(message);
		message_out(COMMAND_CODE_TRNS_DATA, message, varType);
		break;
	}
	case VAR_TYPE_DOUBLE:
	{
		double message = 0;
		decode(message);
		message_out(COMMAND_CODE_TRNS_DATA, message, varType);
		break;
	}

	}
}

void CSerialPacket::echo_command(uint16_t command) //Achnowledgement
{
	setCode(command);

	encode(command);

	send_packet();

}

bool CSerialPacket::procPacket()
{

	bool res = false;

	if (readPacketSize() < 0)
	{
		return res;
	}


	if (mSzPacket > 0)
	{
		int numDataIn = pSerial->available();
		pSerial->readBytes(&mRxBuffer[mNumByteIn], numDataIn);

		mNumByteIn += numDataIn;

	}
	else
		mNumByteIn = 0;


	if (mNumByteIn == mSzPacket)
	{

		mDecodePos = 0;
		mSzPacket = -1;
		mNumByteIn = 0;


		setDataReadReady(false);
		setCommandReadReady(true);

		res = true;
	}



	return res;
}

int CSerialPacket::readPacketSize()
{
	if (mSzPacket < 0)
	{
		if (pSerial->available() > 0)
		{
			//char szData;
			//pSerial->readBytes(&szData, 1);
			////pSerial->readBytes(temp, sizeof(mSzPacket));
			//memset(&mSzPacket, 0, sizeof(mSzPacket));
			//memcpy(&mSzPacket, &szData, sizeof(szData));


			char szData[2] = { 0, };

			pSerial->readBytes(szData, sizeof(szData));

			memset(&mSzPacket, 0, sizeof(mSzPacket));
			memcpy(&mSzPacket, &szData, sizeof(szData));


		}

	}
	//mSzPacket = -1;

	return mSzPacket;

}





void CSerialPacket::setCode(uint16_t code)
{
	memset(mTxBuffer, 0, sizeof(mTxBuffer));

	mEncodePos = 0;			//encode의 위치를 나타내기위해 사용할 것임
	memcpy(&mTxBuffer[mEncodePos], &code, sizeof(code));	//코드값(int=2바이트)만큼 초기화
	mEncodePos += sizeof(code);		//encode의 위치를 2바이트만큼 건너뛰어줌
}

void CSerialPacket::send_packet()
{
	unsigned int numPacket = mEncodePos + sizeof(size_t);
	memcpy(&mTxBuffer[mEncodePos], &numPacket, sizeof(numPacket));
	mEncodePos += sizeof(numPacket);

	mTxBuffer[mEncodePos] = 13;			//mydata = ____ ____ 13
	mEncodePos++;
	mTxBuffer[mEncodePos] = 10;			//mydata = ____ ____ 1310
	mEncodePos++;

	Serial.write(mTxBuffer, mEncodePos); //mydata를 encodePos길이만큼 시리얼통신으로 보낼것.
}

uint16_t CSerialPacket::getCommandCode()
{
	uint16_t command_code = 0;
	if (Serial.available() >= SZ_PACKET_COMMAND_CODE)
	{
		char buffer[SZ_PACKET_COMMAND_CODE];

		Serial.readBytes(buffer, SZ_PACKET_COMMAND_CODE);


		memcpy(&command_code, buffer, sizeof(buffer));


		mSzPacket = -1; //just in case
		setDataReadReady(true);
		setCommandReadReady(false);
		mCommand_Code = command_code;
	}

	return command_code;
}

void CSerialPacket::setFlagRecvWait(bool flag)
{
	mFlag_Recv_Wait = flag;
}
bool CSerialPacket::getFlagRecvWait()
{
	return mFlag_Recv_Wait;
}

bool CSerialPacket::isCommandReadReady()
{
	return mReadCommandReady;
}
bool CSerialPacket::isDataReadReady()
{
	return mReadDataReady;
}

void CSerialPacket::setCommandReadReady(bool flag)
{
	mReadCommandReady = flag;
}
void CSerialPacket::setDataReadReady(bool flag)
{
	mReadDataReady = flag;
}
uint32_t CSerialPacket::dprintf(string &str)
{

	unsigned int szByte = (unsigned int)str.size();


	memset(mCharBuffer, 0, strlen(mCharBuffer));
	strcpy(mCharBuffer, str.c_str());

	setCode(COMMAND_CODE_DEBUG_PRINT);
	if (mEncodePos + szByte <= sizeof(mTxBuffer))
	{
		memcpy(&mTxBuffer[mEncodePos], &mCharBuffer, szByte);

		mEncodePos += szByte;
	}
	send_packet();

	threads.delay(1000);

	return szByte;

}

uint32_t CSerialPacket::dprintf(const char * format, ...)
{
	uint32_t szByte = sprintf(mCharBuffer, format);

	va_list args;
	va_start(args, format);


	vsprintf(mCharBuffer, format, args);


	//CString cstr(str);
	//_PrintResult(cstr);


	setCode(COMMAND_CODE_DEBUG_PRINT);
	if (mEncodePos + szByte <= sizeof(mTxBuffer))
	{
		memcpy(&mTxBuffer[mEncodePos], &mCharBuffer, szByte);

		mEncodePos += szByte;
	}

	send_packet();

	threads.delay(1);
	return szByte;
}