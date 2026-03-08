#include "dprint.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

const char* green = "\x1b[32m";
const char* red = "\x1b[31m";
const char* gray = "\x1b[90m";
const char* yellow = "\x1b[33m";
const char* bold = "\x1b[1m";
const char* reset = "\x1b[0m";


void passed(const char *fmt, ...) {
    const char *nocolor = getenv("NO_COLOR");
    
    char mesg[4096];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(mesg, sizeof mesg, fmt, ap);
    va_end(ap);

    if (nocolor != NULL) {
        printf("    [Passed]    %s", mesg);
    } else {
        printf("%s  [Passed]%s    %s", green, reset, mesg);
    }
    
}

void built(int count, const char *fmt, ...) {
    const char *nocolor = getenv("NO_COLOR");
    
    char mesg[4096];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(mesg, sizeof mesg, fmt, ap);
    va_end(ap);

    if (nocolor != NULL) {
        printf("    [%d]    %s", count, mesg);
    } else {
        printf("%s    [%d]%s    %s", gray, count, reset, mesg);
    }
}

void failed(const char *fmt, ...) {
    const char *nocolor = getenv("NO_COLOR");
    char mesg[4096];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(mesg, sizeof mesg, fmt, ap);
    va_end(ap);
    
    if (nocolor != NULL) {
        printf("    [Failed]    %s", mesg);
    } else {
        printf("%s  [Failed]%s    %s", red, reset, mesg);
    }
}

void skip(const char *fmt, ...) {
    const char *nocolor = getenv("NO_COLOR");
    char mesg[4096];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(mesg, sizeof mesg, fmt, ap);
    va_end(ap);

    if (nocolor != NULL) {
        printf("    [skip]    %s", mesg);
    } else {
        printf("%s    [skip]%s    %s", yellow, reset, mesg);
    }
}



void warn(const char *fmt, ...) {
    const char *nocolor = getenv("NO_COLOR");
 
    char mesg[4096];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(mesg, sizeof mesg, fmt, ap);
    va_end(ap);

    if (nocolor != NULL) {
        printf("    [warn]    %s", mesg);
    } else {
        printf("%s    [warn]%s    %s", yellow, reset, mesg);
    }
}

void header(const char *fmt, ...) {
    const char *nocolor = getenv("NO_COLOR");

    char mesg[4096];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(mesg, sizeof mesg, fmt, ap);
    va_end(ap);

    if (nocolor != NULL) {
        printf("%s\n", mesg);
        printf("========================================\n");
    } else {
        printf("%s%s%s\n", bold, mesg, reset);
        printf("========================================\n");
    }
}