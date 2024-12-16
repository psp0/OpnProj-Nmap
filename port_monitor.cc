#include "port_monitor.h"
#include "NmapOps.h"
#include "output.h"
#include "nmap_error.h"
#include "portlist.h"
#include <cerrno>
#include <cstring>

extern NmapOps o;

void PortMonitor::addTarget(Target* target) {
    if (!o.portlist) {
        fatal("No ports specified for monitoring");
        return;
    }

    // TCP 포트 범위 문자열 파싱
    unsigned short *tcp_ports = NULL;
    int tcp_count = 0;
    getpts_simple(o.portlist, SCAN_TCP_PORT, &tcp_ports, &tcp_count);
    
    // 각 포트에 대한 모니터링 설정
    for (int i = 0; i < tcp_count; i++) {
        MonitoredPort port;
        port.target = target;
        port.portno = tcp_ports[i];
        port.is_open = false;
        port.last_check = 0;
        monitored_ports.push_back(port);
    }
    
    // 메모리 해제
    free(tcp_ports);
    
    log_write(LOG_STDOUT, "Added monitoring for %s\n", 
              target->targetipstr());
}


void PortMonitor::startMonitoring() {
    if (monitored_ports.empty()) {
        fatal("No ports to monitor");
    }
    
    running = true;
    int ret = pthread_create(&monitor_thread, NULL, monitorLoop, this);
    if (ret != 0) {
        fatal("Failed to create monitoring thread: %s", strerror(ret));
    }
}

void PortMonitor::stopMonitoring() {
    running = false;
    int ret = pthread_join(monitor_thread, NULL);
    if (ret != 0) {
        error("Failed to join monitoring thread: %s", strerror(ret));
    }
}

bool PortMonitor::checkPort(const MonitoredPort& port) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        if (o.debugging) {
            printf("Debug: Socket creation failed: %s\n", strerror(errno));
            fflush(stdout);
        }
        return false;
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port.portno);
    
    const struct sockaddr_storage* ss = port.target->TargetSockAddr();
    memcpy(&addr.sin_addr, &((struct sockaddr_in*)ss)->sin_addr, sizeof(struct in_addr));

    struct timeval timeout;
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;
    
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));

    int result = connect(sock, (struct sockaddr*)&addr, sizeof(addr));
    if (o.debugging && result != 0) {
        printf("Debug: Connect failed for port %d: %s\n", 
               port.portno, strerror(errno));
        fflush(stdout);
    }

    close(sock);
    return (result == 0);
}

void* PortMonitor::monitorLoop(void* arg) {
    PortMonitor* monitor = (PortMonitor*)arg;
    
    printf("\nInitial Port States:\n");
    // 초기 상태 확인
    for (auto& port : monitor->monitored_ports) {
        bool state = monitor->checkPort(port);
        port.is_open = state;
        
        char timestr[128];
        time_t now = time(NULL);
        strftime(timestr, sizeof(timestr), "%Y-%m-%d %H:%M:%S", localtime(&now));
        
        printf("[%s] %s:%d - Initial State: %s\n",
            timestr,
            port.target->targetipstr(),
            port.portno,
            state ? "OPEN" : "CLOSED");

        // stdout을 즉시 flush
        fflush(stdout);
        
        port.last_check = now;
    }
    
    printf("\nMonitoring for changes... (Press Ctrl+C to stop)\n");
    fflush(stdout);

    // 지속적인 모니터링
    while (monitor->running) {
        for (auto& port : monitor->monitored_ports) {
            bool current_state = monitor->checkPort(port);
            
            // 디버깅을 위한 출력
            if (o.debugging) {
                printf("Debug: Checking %s:%d - Current: %s, Previous: %s\n",
                    port.target->targetipstr(),
                    port.portno,
                    current_state ? "OPEN" : "CLOSED",
                    port.is_open ? "OPEN" : "CLOSED");
                fflush(stdout);
            }
            
            // 상태가 변경되었을 때만 알림
            if (current_state != port.is_open) {
                char timestr[128];
                time_t now = time(NULL);
                strftime(timestr, sizeof(timestr), "%Y-%m-%d %H:%M:%S", localtime(&now));
                
                printf("[%s] %s:%d - State changed: %s -> %s\n",
                    timestr,
                    port.target->targetipstr(),
                    port.portno,
                    port.is_open ? "OPEN" : "CLOSED",
                    current_state ? "OPEN" : "CLOSED");
                fflush(stdout);
                
                port.is_open = current_state;
                port.last_check = now;
            }
        }
        
        sleep(1);  // 1초 간격으로 체크
    }
    
    return NULL;
}

void PortMonitor::notifyStateChange(const MonitoredPort& port, bool new_state) {
    char timestr[128];
    time_t now = time(NULL);
    strftime(timestr, sizeof(timestr), "%Y-%m-%d %H:%M:%S", localtime(&now));
    
    log_write(LOG_NORMAL, "[%s] %s:%d - State changed: %s -> %s\n",
              timestr,
              port.target->targetipstr(),
              port.portno,
              port.is_open ? "OPEN" : "CLOSED",
              new_state ? "OPEN" : "CLOSED");
}

void port_monitor_init() {
    if (o.portmonitor) {
        if (o.verbose)
            log_write(LOG_STDOUT, "Starting port monitoring mode...\n");
        PortMonitor& monitor = PortMonitor::getInstance();
        monitor.startMonitoring();
    }
}

void port_monitor_cleanup() {
    if (o.portmonitor) {
        PortMonitor::getInstance().stopMonitoring();
    }
}