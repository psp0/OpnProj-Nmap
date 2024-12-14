#include <iostream> // 입출력
#include <vector> // 데이터 구조
#include <string> // 문자열 처리
#include <sys/socket.h> // 소켓 프로그래밍
#include <arpa/inet.h> // 네트워크 주소 변환
#include <pthread.h> // 멀티스레딩
#include <unistd.h>
#include <cstring>
#include <netinet/in.h>
#include <ctime>

class PortMonitor {
private:
    // 포트 상태 정보 구조체
    struct PortStatus {
        std::string host; // 호스트 주소
        int port; // 포트 번호
        bool is_open; // 포트 상태 (열려있는지 여부)
        time_t last_check; // 마지막 체크 시간
        std::string service_name; // 서비스 이름 (옵션)
    };

    std::vector<PortStatus> monitored_ports; // 모니터링할 포트 목록 (동적 배열)
    pthread_t monitor_thread; // 모니터링 스레드
    bool running; // 모니터링 실행 상태
    int check_interval; // 체크 주기

public:
    // 생성자
    PortMonitor(int interval = 60) : check_interval(interval), running(false) {}

    // 포트 추가
    void addPort(const std::string& host, int port, const std::string& service = "") {
        PortStatus status;
        status.host = host;
        status.port = port;
        status.is_open = false; // 초기 상태
        status.last_check = 0;
        status.service_name = service;
        monitored_ports.push_back(status); // 모니터링할 포트 목록에 추가
        
        std::cout << "모니터링 포트 추가 " << host << ":" << port;
        if (!service.empty()) {
            std::cout << " (" << service << ")";
        }
        std::cout << std::endl;
    }

    void start() {
        // 모니터링할 포트가 없으면 예외 발생
        if (monitored_ports.empty()) {
            throw std::runtime_error("모니터링할 포트가 없습니다.");
        }
        running = true;
        // 모니터링 스레드(pthread) 생성, 실패하면 예외 발생
        if (pthread_create(&monitor_thread, NULL, monitorLoop, this) != 0) {
            throw std::runtime_error("thread 생성 실패");
        }
    }

    void stop() {
        running = false;
        // 모니터링 스레드 종료 대기
        pthread_join(monitor_thread, NULL);
    }

private:
    // 포트 상태 체크 함수
    bool checkPort(const std::string& host, int port) {
        // TCP 소켓 생성 
        int sock = socket(AF_INET, SOCK_STREAM, 0); 
        if (sock < 0) return false;

        // 주소 구조체 설정
        struct sockaddr_in addr;
        memset(&addr, 0, sizeof(addr)); // 구조체 초기화
        addr.sin_family = AF_INET; // IPv4 주소 체계
        addr.sin_port = htons(port); // 포트 번호 설정

        // 타임아웃 설정
        struct timeval timeout;
        timeout.tv_sec = 3;  // 3초 타임아웃
        timeout.tv_usec = 0;

        // 수신 타임아웃 설정
        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
        // 송신 타임아웃 설정
        setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));

        // IP 주소 변환 (문자열 -> 네트워크 주소)
        if (inet_pton(AF_INET, host.c_str(), &addr.sin_addr) <= 0) {
            close(sock);
            return false;
        }

        // 포트 연결 시도
        bool is_open = (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) == 0);
        close(sock);
        return is_open;
    }

    // 포트 상태 변경 알림 함수
    void notifyStateChange(const PortStatus& status, bool new_state) {
        time_t now = time(NULL);
        char time_str[64];
        strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", localtime(&now));

        printf("[%s] %s:%d (%s) - 상태 변경: %s -> %s\n",
            time_str, // 현재 시간
            status.host.c_str(), // 호스트
            status.port, // 포트
            status.service_name.empty() ? "unknown" : status.service_name.c_str(),
            status.is_open ? "OPEN" : "CLOSED",
            new_state ? "OPEN" : "CLOSED"
        );
    }

    // 모니터링 스레드 함수
    static void* monitorLoop(void* arg) {
        PortMonitor* monitor = (PortMonitor*)arg;
        
        while (monitor->running) {
            for (auto& status : monitor->monitored_ports) {
                bool current_state = monitor->checkPort(status.host, status.port);
                
                if (current_state != status.is_open) {
                    monitor->notifyStateChange(status, current_state);
                    status.is_open = current_state;
                }
                
                status.last_check = time(NULL);
            }
            
            sleep(monitor->check_interval);
        }
        
        return NULL;
    }
};

int main(int argc, char* argv[]) {
    try {
        
        PortMonitor monitor(10);

        // 모니터링할 포트 추가
        monitor.addPort("127.0.0.1", 80, "HTTP");
        monitor.addPort("127.0.0.1", 443, "HTTPS");
        monitor.addPort("127.0.0.1", 22, "SSH");

        monitor.addPort("127.0.0.1", 1500, "TEST");
        monitor.addPort("192.168.10.251", 8006, "PROXMOX");

        std::cout << "포트 모니터링 시작" << std::endl;

        // 모니터링 시작
        monitor.start();

        // 엔터키 입력 대기
        getchar();

        // 모니터링 중지
        monitor.stop();
        std::cout << "Monitoring stopped." << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}