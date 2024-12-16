#ifndef PORT_SCAN_H
#define PORT_SCAN_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <string>
#include <algorithm>
#include "portlist.h"
#include "NmapOps.h"
#include "output.h"
#include "nmap_error.h"
#include "Target.h"
#include "scan_engine.h" // ultra_scan 함수를 사용하기 위해 추가

#define FILE_NAME "ipports.txt"

// 외부 변수 선언
extern NmapOps o;

// 함수 선언
// 파일 초기화 함수
void initialize_file();

// 파일에서 IP와 포트 정보 읽기 함수
void read_ipports_file(std::vector<std::pair<std::string, int>> &ipport_data);

// 포트 순서를 기반으로 포트 맵 초기화 함수
void initialize_port_map_from_file(PortList &portlist, int protocol);

// Nmap를 통한 포트 스캔 수행 함수
void perform_port_scan();

#endif // PORT_SCAN_H
