#ifndef PORT_MONITOR_H
#define PORT_MONITOR_H

#include "nmap.h"
#include "NmapOps.h"
#include <vector>
#include <string>
#include <pthread.h>
#include <time.h>

class PortMonitor {
public:
    static PortMonitor& getInstance() {
        static PortMonitor instance;
        return instance;
    }

    void addTarget(Target* target);
    void startMonitoring();
    void stopMonitoring();
    
private:
    PortMonitor() : running(false), check_interval(60) {}
    
    struct MonitoredPort {
        Target* target;
        bool is_open;
        time_t last_check;
    };

    std::vector<MonitoredPort> monitored_ports;
    pthread_t monitor_thread;
    bool running;
    int check_interval;

    static void* monitorLoop(void* arg);
    bool checkPort(Target* target);
    void notifyStateChange(const MonitoredPort& port, bool new_state);
};

// 새로운 명령줄 옵션을 위한 함수
void port_monitor_init();
void port_monitor_cleanup();

#endif