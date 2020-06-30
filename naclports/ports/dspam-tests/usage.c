#include <sys/types.h>
#include <sys/sysinfo.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dspam/libdspam.h>
#include "core.h"
#include "util.h"

#define USERNAME "usagetest"
#define HOME "./etc"

int parseLine(char* line){
    int i = strlen(line);
    while (*line < '0' || *line > '9') line++;
    line[i-3] = '\0';
    i = atoi(line);
    return i;
}

int getPeakKB(){
    FILE* file = fopen("/proc/self/status", "r");
    int result = -1;
    char line[128];

    while (fgets(line, 128, file) != NULL){
        if (strncmp(line, "VmPeak:", 7) == 0){
            result = parseLine(line);
            break;
        }
    }
    fclose(file);
    return result;
}

int getMemKB(){
    FILE* file = fopen("/proc/self/status", "r");
    int result = -1;
    char line[128];

    while (fgets(line, 128, file) != NULL){
        if (strncmp(line, "VmSize:", 7) == 0){
            result = parseLine(line);
            break;
        }
    }
    fclose(file);
    return result;
}

void printPeakKB(void){
    int mem = getPeakKB();
    printf("Overall peak usage: %d kB (%d MB)\n", mem, mem/1000);
}
void printMemKB(void){
    int mem = getMemKB();
    printf("Mem usage: %d kB (%d MB)\n", mem, mem/1000);
}

char* readFromFile(char* fileName){
    stdin = freopen(fileName, "r", stdin);
    return read_message();
}

int main (int argc, char **argv) {
    char* message = 0;
    char fileName[256];
    int res = 0;
    int i = 0;
    DSPAM_CTX* CTX;
    // check some of the test emails
    for(i = 0; i <= 9; ++i){
        printf("-----------------------\n");
        sprintf(fileName, "input/test%d.dat", i);
        CTX = create_context(USERNAME, HOME, LEARN_SPAM);
        check_create(CTX);
        printf("Post-create before reading %s\n", fileName);
        printMemKB();
        message = readFromFile(fileName);
        printf("Pre-run after reading %s\n", fileName);
        printMemKB();
        CTX = run(CTX, LEARN_SPAM, message);
        printf("Pre-destory after adding %s\n", fileName);
        printMemKB();
        if( CTX == NULL){
            return 1;
        }
        dspam_destroy(CTX);
        printf("Post-destory after adding %s\n", fileName);
        printMemKB();
        printPeakKB();
    }
    // run with the ipsum text
    for(i = 0; i <= 7; ++i){
        printf("-----------------------\n");
        sprintf(fileName, "input/usage%d.dat", i);
        CTX = create_context(USERNAME, HOME, LEARN_SPAM);
        check_create(CTX);
        printf("Post-create before reading %s\n", fileName);
        printMemKB();
        message = readFromFile(fileName);
        printf("Pre-run after reading %s\n", fileName);
        printMemKB();
        CTX = run(CTX, LEARN_SPAM, message);
        printf("Pre-destory after adding %s\n", fileName);
        printMemKB();
        if( CTX == NULL){
            return 1;
        }
        dspam_destroy(CTX);
        printf("Post-destory after adding %s\n", fileName);
        printMemKB();
        printPeakKB();
    }
    
    return 0;
}

