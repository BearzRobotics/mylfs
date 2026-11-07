#ifndef AEBCC183_284B_43E4_B954_B5011A4AE8C8
#define AEBCC183_284B_43E4_B954_B5011A4AE8C8

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h> // Required for malloc, realloc, free

// A growable list of heap allocated strings. The list owns its elements:
// strlistFree() frees every element, so only push strings from strdup/malloc.
// A NULL may be pushed as a terminator so data can be handed to execve().
typedef struct {
    char   **data;
    size_t   size;
    size_t   cap;
} StrList;

void strlistInit(StrList *v);
void strlistPush(StrList *v, char *value);
// printf style push. Returns the new string (owned by the list).
char *strlistPushf(StrList *v, const char *fmt, ...);
bool strlistContains(const StrList *v, const char *value);
void strlistFree(StrList *v);

#endif /* AEBCC183_284B_43E4_B954_B5011A4AE8C8 */
