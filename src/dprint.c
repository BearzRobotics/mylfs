#include "dprint.h"
#include <stdio.h>
#include <stdlib.h>

const char* green = "\x1b[32m";
const char* red = "\x1b[31m";
const char* gray = "\x1b[90m";
const char* yellow = "\x1b[33m";
const char* bold = "\x1b[1m";
const char* reset = "\x1b[0m";


void passed(const char *mesg) {
    const char *nocolor = getenv("NO_COLOR");
    
    if (nocolor != NULL) {
        printf("Passed]    %s\n", mesg);
    } else {
        printf("%s[Passed]%s    %s\n", green, reset, mesg);
    }
    
}

void built(int count, const char *mesg) {
    const char *nocolor = getenv("NO_COLOR");
    if (nocolor != NULL) {
        printf("    [%d]    %s\n", count, mesg);
    } else {
        printf("%s    [%d]%s    %s\n", gray, count, reset, mesg);
    }
}

void failed(const char *mesg) {
    const char *nocolor = getenv("NO_COLOR");
    if (nocolor != NULL) {
        printf("[Failed]    %s\n", mesg);
    } else {
        printf("%s[Failed]%s    %s\n", red, reset, mesg);
    }
}

void skip(const char *mesg) {
    const char *nocolor = getenv("NO_COLOR");
    if (nocolor != NULL) {
        printf("    [skip]    %s\n", mesg);
    } else {
        printf("%s    [skip]%s    %s\n", yellow, reset, mesg);
    }
}

void header(const char *mesg) {
    const char *nocolor = getenv("NO_COLOR");
    if (nocolor != NULL) {
        printf("%s\n", mesg);
        printf("========================================\n");
    } else {
        printf("%s%s%s\n", bold, mesg, reset);
        printf("========================================\n");
    }
}