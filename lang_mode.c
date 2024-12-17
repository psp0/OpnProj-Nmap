#include <stdio.h>
#include "port_scan.h" // externalPortScan 함수 포함
#include "lang_mode.h"

void lang_mode(const char **targets, int targetCount, const char *description) {
    for (int i = 0; i < targetCount; i++) {
        printf("Executing lang_mode: Scanning target: %s with description: %s\n", targets[i], description);
        externalPortScan(targets[i], description); // port_scan.c 함수 호출
    }
}
