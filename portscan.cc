#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <algorithm>
#include "portlist.h"
#include "NmapOps.h"
#include "output.h"
#include "nmap_error.h"
#include "Target.h"
#include "scan_engine.h"

#define FILE_NAME "ipports.txt"

extern NmapOps o;

// 파일 초기화
void initialize_file() {
    FILE *file = fopen(FILE_NAME, "r");
    if (file == NULL) {
        file = fopen(FILE_NAME, "w");
        if (file == NULL) {
            fatal("Failed to create file: %s", FILE_NAME);
        }
        fclose(file);
    }
}

// 파일에서 IP와 포트 정보 읽기
void read_ipports_file(std::vector<std::pair<std::string, int>> &ipport_data) {
    FILE *file = fopen(FILE_NAME, "r");
    if (file == NULL) {
        fatal("Failed to open file: %s", FILE_NAME);
    }

    char line[128];
    while (fgets(line, sizeof(line), file)) {
        char ip[64];
        int port;
        if (sscanf(line, "%63[^:]:%d", ip, &port) == 2) {
            ipport_data.emplace_back(std::string(ip), port);
        }
    }

    fclose(file);
}

// Nmap를 통한 포트 스캔 수행
void perform_port_scan() {
    PortList portlist;
    initialize_file();

    // IP와 포트를 사용하여 스캔 수행
    std::vector<std::pair<std::string, int>> ipport_data;
    read_ipports_file(ipport_data);

    for (const auto &entry : ipport_data) {
        const std::string &ip = entry.first;
        int port = entry.second;

        // Target 설정
        Target target;
        if (target.TargetSockAddr(ip.c_str()) != 0) {
            fprintf(stderr, "Failed to set target for IP: %s\n", ip.c_str());
            continue;
        }

        // 스캔할 타겟 목록 및 포트 설정
        std::vector<Target *> targets = {&target};
        struct scan_lists ports;
        ports.tcp_ports.push_back(port);

        // ultra_scan 호출
        printf("Scanning IP: %s, Port: %d\n", ip.c_str(), port);
        ultra_scan(targets, &ports, SYN_SCAN);
    }
}