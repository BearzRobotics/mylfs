#include "strlist.h"// We don't return the type because we're
// operating directly on the struct passed by pointer
void strlistInit(StrList *v) {
    v->size = 0;
    v->cap = 8;
    v->data = malloc(v->cap * sizeof(char*));
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


void strlistFree(StrList *v) {
    free(v->data);
}