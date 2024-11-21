#ifndef PORTMONITOR_H
#define PORTMONITOR_H

#include <string>
#include <vector>
#include <ctime>
#include <pthread.h>

namespace nmap {

class PortMonitor {
public:
    struct PortStatus {
        std::string host;
        int port;
        bool is_open;
        time_t last_check;
        std::string service_name;
        
        PortStatus(const std::string& h, int p, const std::string& s = "")
            : host(h), port(p), is_open(false), last_check(0), service_name(s) {}
    };

    explicit PortMonitor(int interval = 60);
    ~PortMonitor();

    // 복사 생성자와 대입 연산자 삭제 (스레드 때문에)
    PortMonitor(const PortMonitor&) = delete;
    PortMonitor& operator=(const PortMonitor&) = delete;

    void addPort(const std::string& host, int port, const std::string& service = "");
    bool start();
    void stop();

private:
    std::vector<PortStatus> monitored_ports;
    pthread_t monitor_thread;
    bool running;
    int check_interval;

    bool checkPort(const std::string& host, int port);
    void notifyStateChange(const PortStatus& status, bool new_state);
    static void* monitorLoop(void* arg);
};

} // namespace nmap

#endif // PORTMONITOR_H
