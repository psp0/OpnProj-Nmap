#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include "port_scan.h"

#define PORT_COUNT 10

// 포트와 용도를 저장하는 구조체
typedef struct {
    int port;
    char description[50];
} PortInfo;

// 기본 포트 설정
PortInfo portList[PORT_COUNT] = {
    {80, "HTTP"}, {22, "SSH"}, {21, "FTP"}, {25, "SMTP"},
    {443, "HTTPS"}, {53, "DNS"}, {110, "POP3"}, {143, "IMAP"},
    {3306, "MySQL"}, {8080, "HTTP-ALT"}
};

// 포트 번호 반환 함수
int getPortByDescription(const char *description) {
    for (int i = 0; i < PORT_COUNT; i++) {
        if (strcmp(portList[i].description, description) == 0) {
            return portList[i].port;
        }
    }
    return -1;
}

// 포트 스캔 기능
void portScan(const char *ip, int targetPort) {
    int sock;
    struct sockaddr_in addr;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("Socket creation failed");
        return;
    }

    addr.sin_family = AF_INET;
    addr.sin_port = htons(targetPort);
    inet_pton(AF_INET, ip, &addr.sin_addr);

    printf("Scanning port %d...\n", targetPort);
    if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) == 0) {
        printf("[+] Port %d is open\n", targetPort);
    } else {
        printf("[-] Port %d is closed or filtered\n", targetPort);
    }

    close(sock);
}

void externalPortScan(const char *ip, const char *description) {
    int port = getPortByDescription(description);
    if (port == -1) {
        printf("Unsupported description: %s\n", description);
        return;
    }
    printf("Scanning %s (port %d) on IP: %s\n", description, port, ip);
    portScan(ip, port);
}
