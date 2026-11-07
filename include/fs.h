#ifndef A5A9851A_DE99_493B_96A6_F58B147C5071
#define A5A9851A_DE99_493B_96A6_F58B147C5071

#include <stdbool.h>
#include <sys/types.h>

#include "config.h"

// mount info
bool isMounted(const char *path);

// bind mount /dev /dev/pts /proc /sys /run  (used by --chroot)
bool mountBind(Config cfg, char *cpath);
bool unmountBind(Config cfg, char *cpath);

// The virtual filesystems the LFS book mounts before entering the chroot
// (LFS 12.3 section 7.3). Used by phase 3 - 5.
bool mountVfs(Config cfg);
bool unmountVfs(Config cfg);
bool vfsMounted(Config cfg);

// directories / files
bool isDirEmpty(Config cfg, const char *path);
bool deleteDirContents(Config cfg, const char *path);
bool removeTree(const char *path);
bool makeDirs(const char *path, mode_t mode);
bool chownTree(const char *path, uid_t uid, gid_t gid);
bool doesFileExist(Config cfg, const char *file);

// recipes
bool copyRecipeDir(Config cfg);

// networking
bool download(Config cfg, const char *url, const char *path);

#endif /* A5A9851A_DE99_493B_96A6_F58B147C5071 */
