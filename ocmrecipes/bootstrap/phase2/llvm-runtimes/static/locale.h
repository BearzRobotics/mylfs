#ifndef A8C4B50E_ACED_45B4_B568_371108491B34
#define A8C4B50E_ACED_45B4_B568_371108491B34

#pragma once

// Pull in the real musl locale.h first
#include_next <locale.h>

#ifdef __cplusplus
extern "C" {
#endif

// musl may not declare these glibc-style locale variants.
// libc++ expects them for localization support.
long long strtoll_l(const char *nptr, char **endptr, int base, locale_t loc);
unsigned long long strtoull_l(const char *nptr, char **endptr, int base, locale_t loc);

// You will likely hit these next, so it’s worth providing them too:
long strtol_l(const char *nptr, char **endptr, int base, locale_t loc);
unsigned long strtoul_l(const char *nptr, char **endptr, int base, locale_t loc);

double strtod_l(const char *nptr, char **endptr, locale_t loc);
float strtof_l(const char *nptr, char **endptr, locale_t loc);
long double strtold_l(const char *nptr, char **endptr, locale_t loc);

#ifdef __cplusplus
}
#endif

#endif /* A8C4B50E_ACED_45B4_B568_371108491B34 */
