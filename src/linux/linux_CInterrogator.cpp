// linux_CInterrogator.cpp

#include "linux_CInterrogator.h"

const std::string micronsIP = "10.0.0.55";
const uint16_t micronsPort = 51971;

void CInterrogator::init()
{


}



int CInterrogator::disconnectUDP() {
    // Interrogator에 피크 데이터그램 비활성화 명령 전송
	writeCommand("#DisableUdpPeakDatagrams", "", 0);
	
    // UDP 연결 상태 플래그 업데이트
    fconnectUDP = 0;

	// TCP 소켓 종료
    if (tcpSockfd >= 0) {
        close(tcpSockfd); // TCP 소켓을 닫음
        tcpSockfd = -1;   // 소켓 디스크립터 초기화
    }

	// UDP 소켓 종료
    if (udpSockfd >= 0) {
        close(udpSockfd); // 소켓을 닫음
        udpSockfd = -1;   // 소켓 디스크립터 초기화
    }



	return 0;
}



int CInterrogator::connectUDP() {
    // UDP 소켓 생성
    udpSockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (udpSockfd < 0) {
        std::cerr << "Error creating UDP socket." << std::endl;
        udpState = 1; // UDP 소켓 생성 실패
        return -1; 
    }

    // 서버 주소 설정 (TCP 연결용)
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(micronsPort);
    inet_pton(AF_INET, micronsIP.c_str(), &server_addr.sin_addr);

    // TCP 소켓 생성
    tcpSockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (tcpSockfd < 0) {
        std::cerr << "Error creating TCP socket." << std::endl;
        close(udpSockfd); 
        udpState = 2; // TCP 소켓 생성 실패
        return -1; // 소켓 생성 실패
    }

    // TCP 소켓에 대한 연결 시도
    if (connect(tcpSockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "TCP connection failed." << std::endl;
        close(udpSockfd); 
        close(tcpSockfd); 
        udpState = 3; // TCP 소켓 연결 실패에 따른 UDP 연결 실패
        return -1; // 연결 실패
    }

    std::cout << "TCP connection succeed" << std::endl;


    // 로컬 주소 설정 (UDP 연결용)
    struct sockaddr_in local_addr;
    memset(&local_addr, 0, sizeof(local_addr));
    local_addr.sin_family = AF_INET;
    local_addr.sin_port = htons(51970); // 로컬 UDP 포트 설정
    local_addr.sin_addr.s_addr = INADDR_ANY; // 모든 인터페이스에서 수신

    // TCP 연결 성공 시에만 UDP 소켓 바인딩
    if (bind(udpSockfd, (struct sockaddr*)&local_addr, sizeof(local_addr)) < 0) {
        std::cerr << "Error binding UDP socket to local address." << std::endl;
        close(udpSockfd);
        close(tcpSockfd); 
        udpState = 3; // UDP 소켓 바인딩 실패
        return -1; // 바인딩 실패
    }

    // UDP 상태 업데이트
    std::cout << "UDP connection succeed" << std::endl;
    udpState = 0; // UDP 연결 성공

    // recvfrom() 타임아웃 설정
    struct timeval timeout;
    timeout.tv_sec = 5;    // 타임아웃 시간 (초)
    timeout.tv_usec = 0;   // 타임아웃 시간 (마이크로초)
    if (setsockopt(udpSockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
        std::cerr << "Error setting socket timeout." << std::endl;
        return -1;
    }


    return 1; // 성공적으로 연결됨
}



void CInterrogator::enablePeakDatagrams() {
	if (tcpSockfd >= 0) {
		udpState = 4;
		std::string addressPort = "10.0.0.2 51970";
		writeCommand("#EnableUdpPeakDatagrams", addressPort, 0);
	}
	else {
		udpState = 5; // No TCP connection
	}
}


int CInterrogator::writeCommand(std::string command, std::string argument, uint8_t requestOptions)
{
	hWriteHeader writeHeader;
	writeHeader.requestOption = requestOptions;
	writeHeader.commandSize = command.size();
	writeHeader.argSize = uint32_t(argument.size());

	//write using tcp/ip
    ssize_t bytesSent = send(tcpSockfd, (char *)&writeHeader, sizeof(writeHeader), 0);
    if (bytesSent < 0) {
        std::cerr << "Error sending header." << std::endl;
        return -1; 
    }

    bytesSent = send(tcpSockfd, command.c_str(), writeHeader.commandSize, 0);
    if (bytesSent < 0) {
        std::cerr << "Error sending command." << std::endl;
        return -1; 
    }

    bytesSent = send(tcpSockfd, argument.c_str(), writeHeader.argSize, 0);
    if (bytesSent < 0) {
        std::cerr << "Error sending argument." << std::endl;
        return -1;
    }

	return 0;
}



void CInterrogator::readPacket()
{
    uint8_t recvBuff[TOTAL_RECV_BUFF_BYTES] = {0};
    socklen_t addrLen = sizeof(server_addr);

    std::cout << "Receiving packet ..." << std::endl;
    ssize_t packetSize = recvfrom(udpSockfd, recvBuff, sizeof(recvBuff), 0, (struct sockaddr*)&server_addr, &addrLen);

    if (packetSize < 0) {
        if (errno == EWOULDBLOCK || errno == EAGAIN) {
            std::cerr << "Timeout: No packet received within the specified time." << std::endl;
            memset(recvBuff, 0, sizeof(recvBuff)); // 타임아웃 시 버퍼 초기화
            return;
        } else {
            std::cerr << "Error receiving packet." << std::endl;
            return;
        }
    }

    std::cout << "Packet received successfully." << std::endl;
    procPacket(recvBuff, packetSize); // 정상 패킷이 수신된 경우에만 처리
}




void CInterrogator::procPacket(uint8_t *packetChunk, int packetLength)
{
    std::cout << "process packet" << std::endl;

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