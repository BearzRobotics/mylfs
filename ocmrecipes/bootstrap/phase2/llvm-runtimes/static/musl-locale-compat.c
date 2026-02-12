#define _GNU_SOURCE
#include <locale.h>
#include <stdlib.h>

/*
 * musl compatibility wrappers for glibc-style *_l functions.
 * These ignore locale_t and fall back to the default locale.
 * Good enough for libc++ localization support on musl.
 */

long long strtoll_l(const char *nptr, char **endptr, int base, locale_t loc) {
    (void)loc;
    return strtoll(nptr, endptr, base);
}

unsigned long long strtoull_l(const char *nptr, char **endptr, int base, locale_t loc) {
    (void)loc;
    return strtoull(nptr, endptr, base);
}

long strtol_l(const char *nptr, char **endptr, int base, locale_t loc) {
    (void)loc;
    return strtol(nptr, endptr, base);
}

unsigned long strtoul_l(const char *nptr, char **endptr, int base, locale_t loc) {
    (void)loc;
    return strtoul(nptr, endptr, base);
}

double strtod_l(const char *nptr, char **endptr, locale_t loc) {
    (void)loc;
    return strtod(nptr, endptr);
}

float strtof_l(const char *nptr, char **endptr, locale_t loc) {
    (void)loc;
    return strtof(nptr, endptr);
}

long double strtold_l(const char *nptr, char **endptr, locale_t loc) {
    (void)loc;
    return strtold(nptr, endptr);
}
