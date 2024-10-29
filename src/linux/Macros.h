#pragma once
#ifndef MACRO_HEADER
#define MACRO_HEADER



//Timers
#define TIMER_PERIOD_MAIN_CONTROL		1000 // Control loop period 1ms a.k.a 1 kHz
//#define TIMER_PERIOD_MAIN_CONTROL		2000 // Control loop period 1ms a.k.a 500 kHz
#define DATA_STREAM_RATE				TIMER_PERIOD_MAIN_CONTROL/1000.f

///////////////////////////////////////////////////////////////////////////////////////////
// Command Code
#define SZ_PACKET_COMMAND_CODE			2

#define COMMAND_CODE_TEST				0XFFFF // Reserved for testing codes
#define COMMAND_CODE_DEBUG_PRINT		0XFFFD // Printing debug line, eg) dprintf(xxxx)

//Control Main Functions..start with "F" codes
#define COMMAND_CODE_CONNECT			0XF000 // Connect PC Btw Teensy
#define COMMAND_CODE_DISCONNECT			0XF001 // Disconnect PC Btw Teensy

#define COMMAND_CODE_ECHO				0XF002 // Echo Command
#define COMMAND_CODE_RECV_DATA			0XF003 // Recieve Data at Teensy from PC
#define COMMAND_CODE_TRNS_DATA			0XF004 // Transmit Data from Teensy to PC


//Main Loop Control Functions..start with "A" codes
#define COMMAND_CODE_MAIN_CONTROL_RUN	0XA001  // Run main RT timer	
#define COMMAND_CODE_MAIN_CONTROL_STOP	0XA002	// Stop main RT timer	

#define COMMAND_CODE_DATA_STREAM		0XA00D	// Stream packet data


// Connect Interrogator .. start with "B" codes
#define COMMAND_CODE_UDP_CONNECT		0XB001
#define COMMAND_CODE_UDP_DISCONNECT		0XB002
#define COMMAND_CODE_INTERRO_STREAM_ON	0XB003

///////////////////////////////////////////////////////////////////////////////////////////




// Variable Type Code
#define SZ_VAR_TYPE_CODE				2
#define VAR_TYPE_CHAR					0X0000
#define VAR_TYPE_UCHAR					0X0001

#define VAR_TYPE_INT16					0X0010
#define VAR_TYPE_UINT16					0X0020
#define VAR_TYPE_INT32					0X0030
#define VAR_TYPE_UINT32					0X0040

#define VAR_TYPE_FLOAT					0x0100
#define VAR_TYPE_DOUBLE					0x0200


// Interrogator Info.
#define PEAK_CH 4
#define NUM_GRATING 5
#define TOTAL_RECV_BUFF_BYTES 200
#define PEAK_PACKET_BYTES	56 + PEAK_CH*30
#define PEAK_HEADER_MAX_SIZE 56


/////////////////////////////////////////////////////////////////////////////////
//Definition of Packet Stream

#define SZ_PACKET_STREAM				sizeof(DataLog)

#define ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]))

struct DataLog {
	unsigned int timeStamp = 0; //timestamp in millisecond
	unsigned int loopPeriod = 0; //loop elapsed time in microsecond

	double itimeStamp = 0.0;
	double avgWavelength[PEAK_CH] = { 0.0, };
	double FBGForce[3] = { 0.0, };
	float LPFForce[3] = { 0.0, };
	int taskType = -1; //-1 or 0: idel, 1: Target Pos,....
	unsigned int statusFlag = 0; // 32bit flags 
};

/////////////////////////////////////////////////////////////////////////////////



////////////////////////////////////////////////////////////////////////////
// Settings for Teensy

//used for setting parameters, ex) amplitude, velocity, etc
typedef struct {
	unsigned int param;


} TaskParam;

//used for defining type of operation
// Task Type
typedef enum {
	RUN_TASK_NONE,
	RUN_TASK_MONITOR
}RUN_TASK_TYPE;



#endif