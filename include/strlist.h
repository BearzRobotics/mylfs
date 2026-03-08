#ifndef AEBCC183_284B_43E4_B954_B5011A4AE8C8
#define AEBCC183_284B_43E4_B954_B5011A4AE8C8
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h> // Required for malloc, realloc, free



typedef struct {
    char   **data;
    size_t   size;
    size_t   cap;
} StrList;

void strlistInit(StrList *v);
void strlistPush(StrList *v, char *value);
void strlistFree(StrList *v);



#endif /* AEBCC183_284B_43E4_B954_B5011A4AE8C8 */
