#define _GNU_SOURCE
#include <stdarg.h>
#include <string.h>

#include "strlist.h"

// We don't return the type because we're
// operating directly on the struct passed by pointer
void strlistInit(StrList *v) {
    v->size = 0;
    v->cap = 8;
    v->data = malloc(v->cap * sizeof(char*));
    if (!v->data) {
        perror("malloc");
        exit(1);
    }
}

void strlistPush(StrList *v, char *value) {
    if (v->size == v->cap) {
        v->cap *= 2;
        v->data = realloc(v->data, v->cap * sizeof(char *));
        if (!v->data) {
            perror("realloc");
            exit(1);
        }
    }
    v->data[v->size++] = value;
}

char *strlistPushf(StrList *v, const char *fmt, ...) {
    char *s = NULL;
    va_list ap;
    va_start(ap, fmt);
    int n = vasprintf(&s, fmt, ap);
    va_end(ap);
    if (n < 0) {
        perror("vasprintf");
        exit(1);
    }
    strlistPush(v, s);
    return s;
}

bool strlistContains(const StrList *v, const char *value) {
    for (size_t i = 0; i < v->size; i++) {
        if (v->data[i] && strcmp(v->data[i], value) == 0) {
            return true;
        }
    }
    return false;
}

void strlistFree(StrList *v) {
    for (size_t i = 0; i < v->size; i++) {
        free(v->data[i]);
    }
    free(v->data);
    v->data = NULL;
    v->size = 0;
    v->cap = 0;
}
