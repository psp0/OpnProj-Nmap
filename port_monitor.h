#ifndef PORT_MONITOR_H
#define PORT_MONITOR_H

#include "nmap.h"
#include "Target.h"
#include <vector>
#include <pthread.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string>
#include <ctime>
#include <fcntl.h>          
#include <sys/select.h>     

class PortMonitor {
public:
    struct MonitoredPort {
        Target* target;
        int portno;
        bool is_open;
        time_t last_check;
    };

    static PortMonitor& getInstance() {
        static PortMonitor instance;
        return instance;
    }

    void addTarget(Target* target);
    void startMonitoring();
    void stopMonitoring();
    bool checkPort(const MonitoredPort& port);

private:
    PortMonitor() : running(false), check_interval(1) {}
    ~PortMonitor() {}
    PortMonitor(const PortMonitor&) = delete;
    PortMonitor& operator=(const PortMonitor&) = delete;

    std::vector<MonitoredPort> monitored_ports;
    pthread_t monitor_thread;
    bool running;
    int check_interval;  // seconds

    static void* monitorLoop(void* arg);
    void notifyStateChange(const MonitoredPort& port, bool new_state);
};

void port_monitor_init();
void port_monitor_cleanup();

#endif