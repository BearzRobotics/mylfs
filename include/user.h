#ifndef AEF1DD38_01E2_4C1D_84E2_6F10EC57774B
#define AEF1DD38_01E2_4C1D_84E2_6F10EC57774B

#include "config.h"

#include <sys/types.h>

// Reads /etc/passwd directly instead of getpwnam(). getpwnam goes through NSS
// which dlopen()s libc plugins even in a static binary, and that breaks when the
// libc we were built against differs from the one on the running system.
bool lookupUser(const char *name, uid_t *uid, gid_t *gid);

bool lfsUserExist(void);
bool creatLfsUser(void);
bool deleteLfsUser(void);
bool chownLfsFiles(Config cfg);


#endif /* AEF1DD38_01E2_4C1D_84E2_6F10EC57774B */
