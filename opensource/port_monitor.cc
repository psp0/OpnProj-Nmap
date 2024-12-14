#include "port_monitor.h"
#include "NmapOps.h"
#include "output.h"
#include "timing.h"

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
    // Nmap의 기존 스캔 기능 활용
    PortList* plist = &target->ports;
    return plist->isPortOpen(target->ports.nextPort(0, NULL, IPPROTO_TCP));
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
        
        sleep(monitor->check_interval);
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