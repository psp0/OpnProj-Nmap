#include "portmonitor.h"
#include "nmap.h"
#include "NmapOps.h"
#include "output.h"

#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <netinet/in.h>
#include <errno.h>

namespace nmap {

PortMonitor::PortMonitor(int interval) 
    : check_interval(interval), running(false) {}

PortMonitor::~PortMonitor() {
    if (running) {
        stop();
    }
}

void PortMonitor::addPort(const std::string& host, int port, const std::string& service) {
    monitored_ports.emplace_back(host, port, service);
    
    if (o.verbose) {  // nmap의 verbose 옵션 활용
        log_write(LOG_STDOUT, "Added monitoring for %s:%d", 
                 host.c_str(), port);
        if (!service.empty()) {
            log_write(LOG_STDOUT, " (%s)", service.c_str());
        }
        log_write(LOG_STDOUT, "\n");
    }
}

bool PortMonitor::checkPort(const std::string& host, int port) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        if (o.debugging > 1) {  // nmap의 디버깅 레벨 활용
            log_write(LOG_STDOUT, "Failed to create socket: %s\n", 
                     strerror(errno));
        }
        return false;
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);

    struct timeval timeout;
    timeout.tv_sec = 3;
    timeout.tv_usec = 0;

    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));

    if (inet_pton(AF_INET, host.c_str(), &addr.sin_addr) <= 0) {
        close(sock);
        return false;
    }

    bool is_open = (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) == 0);
    close(sock);
    return is_open;
}

void PortMonitor::notifyStateChange(const PortStatus& status, bool new_state) {
    time_t now = time(NULL);
    char time_str[64];
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", 
             localtime(&now));

    log_write(LOG_NORMAL,
        "[%s] %s:%d (%s) - Status Change: %s -> %s\n",
        time_str,
        status.host.c_str(),
        status.port,
        status.service_name.empty() ? "unknown" : status.service_name.c_str(),
        status.is_open ? "OPEN" : "CLOSED",
        new_state ? "OPEN" : "CLOSED"
    );
}

void* PortMonitor::monitorLoop(void* arg) {
    PortMonitor* monitor = static_cast<PortMonitor*>(arg);
    
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

bool PortMonitor::start() {
    if (monitored_ports.empty()) {
        error("No ports to monitor");
        return false;
    }

    running = true;
    if (pthread_create(&monitor_thread, NULL, monitorLoop, this) != 0) {
        error("Failed to create monitoring thread: %s", strerror(errno));
        running = false;
        return false;
    }

    return true;
}

void PortMonitor::stop() {
    running = false;
    pthread_join(monitor_thread, NULL);
}

} // namespace nmap