#include "port_monitor.h"
#include "NmapOps.h"
#include "output.h"
#include "nmap_error.h"

extern NmapOps o;

void PortMonitor::addTarget(Target* target) {
    MonitoredPort port;
    port.target = target;
    port.is_open = false;
    port.last_check = 0;
    monitored_ports.push_back(port);
    
    log_write(LOG_STDOUT, "Added monitoring for %s\n", 
              target->targetipstr());
}

void PortMonitor::startMonitoring() {
    if (monitored_ports.empty()) {
        fatal("No ports to monitor");
    }
    
    running = true;
    if (pthread_create(&monitor_thread, NULL, monitorLoop, this) != 0) {
        fatal("Failed to create monitoring thread");
    }
}

void PortMonitor::stopMonitoring() {
    running = false;
    pthread_join(monitor_thread, NULL);
}

bool PortMonitor::checkPort(Target* target) {
    Port port;
    Port *current = NULL;
    
    // 첫 번째 포트 가져오기
    current = target->ports.nextPort(NULL, &port, IPPROTO_TCP, PORT_UNKNOWN);
    if (!current) return false;
    
    int portno = current->portno;
    
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) return false;

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(portno);
    
    const struct sockaddr_storage* ss = target->TargetSockAddr();
    memcpy(&addr.sin_addr, &((struct sockaddr_in*)ss)->sin_addr, sizeof(struct in_addr));

    struct timeval timeout;
    timeout.tv_sec = 3;
    timeout.tv_usec = 0;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));

    bool is_open = (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) == 0);
    close(sock);
    return is_open;
}

void* PortMonitor::monitorLoop(void* arg) {
    PortMonitor* monitor = (PortMonitor*)arg;
    
    while (monitor->running) {
        for (auto& port : monitor->monitored_ports) {
            bool current_state = monitor->checkPort(port.target);
            
            if (current_state != port.is_open) {
                monitor->notifyStateChange(port, current_state);
                port.is_open = current_state;
            }
            
            port.last_check = time(NULL);
        }
        
        usleep(monitor->check_interval * 1000000);
    }
    
    return NULL;
}

void PortMonitor::notifyStateChange(const MonitoredPort& port, bool new_state) {
    char timestr[128];
    time_t now = time(NULL);
    strftime(timestr, sizeof(timestr), "%Y-%m-%d %H:%M:%S", localtime(&now));
    
    log_write(LOG_NORMAL, "[%s] %s - State changed: %s -> %s\n",
              timestr,
              port.target->targetipstr(),
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