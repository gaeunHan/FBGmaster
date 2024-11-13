#include "linux_CInterrogator.h"

int main() {
    CInterrogator interrogator; // Interrogator 객체 생성

    // Interrogator 초기화
    interrogator.init();

    // UDP 및 TCP 연결 시도
    if (interrogator.connectUDP() < 0) {
        std::cerr << "Failed to connect via UDP." << std::endl;
        return -1;
    }
    std::cout << "Interrogator is connected" << std::endl;


    // 피크 데이터그램 활성화
    interrogator.enablePeakDatagrams();
    std::cout << "UDP peak datagrams are enabled" << std::endl;


    // interrogator와 연결 성공 시 피크 데이터 수신 루프 시작
    while (true) {
        std::cout << "read packet ..." << std::endl;
        interrogator.readPacket(); // 패킷 수신 및 처리
        std::cout << "packet is read" << std::endl;

        // 수신된 피크 값 출력
        double timeStamp;
        short numPeaks[4];
        double peakVals[PEAK_CH * NUM_GRATING];
        double avgPeakVals[PEAK_CH];

        // 피크 값 요청
        interrogator.getPeaks(timeStamp, numPeaks, peakVals, avgPeakVals);
        std::cout << "peak datagram is received" << std::endl;

        // 수신된 값 출력
        std::cout << "Timestamp: " << timeStamp << std::endl;
        for (int i = 0; i < 4; ++i) {
            std::cout << "Num Peaks [" << i << "]: " << numPeaks[i] << std::endl;
            std::cout << "Avg Peak [" << i << "]: " << avgPeakVals[i] << std::endl;
        }
        // 데이터를 주기적으로 가져오기 위한 대기 시간
        sleep(0.1);
    }

    // 연결 해제
    interrogator.disconnectUDP();

    return 0;
}
