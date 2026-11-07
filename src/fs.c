#define _GNU_SOURCE
#define _XOPEN_SOURCE 700

#include <dirent.h>
#include <errno.h>
#include <ftw.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <unistd.h>

#include "config.h"
#include "dprint.h"
#include "fs.h"
#include "proc.h"

static const char *mounts[] = {
    "/dev",
    "/dev/pts",
    "/proc",
    "/sys",
    "/run",
};

// Decodes the octal escapes (\040 for a space) used in /proc/self/mountinfo
static void unescapeMount(char *s) {
    char *w = s;
    while (*s) {
        if (s[0] == '\\' && s[1] >= '0' && s[1] <= '7' && s[2] >= '0' && s[2] <= '7' &&
            s[3] >= '0' && s[3] <= '7') {
            *w++ = (char)(((s[1] - '0') << 6) | ((s[2] - '0') << 3) | (s[3] - '0'));
            s += 4;
        } else {
            *w++ = *s++;
        }
    }
    *w = '\0';
}

bool isMounted(const char *cpath) {
    FILE *fh = fopen("/proc/self/mountinfo", "re");
    if (!fh) {
        perror("fopen");
        return false;
    }

    char line[4096];
    bool found = false;

    while (fgets(line, sizeof line, fh)) {
        // mountinfo format:
        // id parent major:minor root mount_point options ... - fstype source superopts
        // We want the 5th field: mount_point
        char *save = NULL;
        char *tok = strtok_r(line, " ", &save); // 1: id
        for (int i = 0; i < 4 && tok; i++) {    // 2: parent 3: major:minor 4: root 5: mount_point
            tok = strtok_r(NULL, " ", &save);
        }
        if (!tok) continue;

        unescapeMount(tok);
        if (strcmp(tok, cpath) == 0) {
            found = true;
            break;
        }
    }

    fclose(fh);
    return found;
}

// bind mount /dev /dev/pts /proc /sys /run
bool mountBind(Config cfg, char *cpath) {
    size_t count = sizeof(mounts) / sizeof(mounts[0]);

    for (size_t i = 0; i < count; i++) {
        char target[PATH_MAX];
        snprintf(target, sizeof(target), "%s%s", cpath, mounts[i]);

        if (!isMounted(target)) {
            if (mount(mounts[i], target, NULL, MS_BIND, NULL) != 0) {
                perror("mount");
                failed("Could not bind mount needed filesystems\n");
                return false;
            }
            if (cfg.debug) {
                printf("Bind Mounted: %s -> %s\n", mounts[i], target);
            }
        } else {
            skip("Already mounted: %s\n", target);
        }
    }
    return true;
}

// unbind mount /dev /dev/pts /proc /sys /run
bool unmountBind(Config cfg, char *cpath) {
    size_t count = sizeof(mounts) / sizeof(mounts[0]);
    bool ok = true;

    // needs to be in reverse order
    for (size_t i = count; i-- > 0;) {
        char target[PATH_MAX];
        snprintf(target, sizeof(target), "%s%s", cpath, mounts[i]);

        if (!isMounted(target)) {
            continue;
        }
        if (umount2(target, MNT_DETACH) != 0) {
            perror("umount2");
            ok = false;
        } else if (cfg.debug) {
            printf("umounted: %s\n", target);
        }
    }
    return ok;
}

// Runs a small shell script with $LFS set. Output goes to logs/<log>.
static bool lfsScript(Config cfg, const char *script, const char *log) {
    mkdir("logs", 0755);

    char logPath[PATH_MAX];
    snprintf(logPath, sizeof logPath, "logs/%s", log);

    char lfsEnv[PATH_MAX + 8];
    snprintf(lfsEnv, sizeof lfsEnv, "LFS=%s", cfg.buildPath);
    char *envp[] = {
        lfsEnv,
        "PATH=/usr/sbin:/usr/bin:/sbin:/bin",
        "LC_ALL=POSIX",
        NULL,
    };
    char *argv[] = {"bash", NULL};

    RunSpec rs = {
        .path = hostBash(),
        .argv = argv,
        .envp = envp,
        .input = script,
        .logPath = logPath,
    };
    return runProcess(&rs) == 0;
}

bool vfsMounted(Config cfg) {
    char target[PATH_MAX];
    snprintf(target, sizeof target, "%s/dev", cfg.buildPath);
    if (isMounted(target)) return true;
    snprintf(target, sizeof target, "%s/proc", cfg.buildPath);
    return isMounted(target);
}

// LFS 12.3 section 7.3
bool mountVfs(Config cfg) {
    if (vfsMounted(cfg)) {
        if (cfg.debug) info("Virtual filesystems are already mounted\n");
        return true;
    }

    const char *cmd =
        "set -e\n"
        "mkdir -p $LFS/{dev,proc,sys,run}\n"
        "mount -v --bind /dev $LFS/dev\n"
        "mount -vt devpts devpts -o gid=5,mode=0620 $LFS/dev/pts\n"
        "mount -vt proc proc $LFS/proc\n"
        "mount -vt sysfs sysfs $LFS/sys\n"
        "mount -vt tmpfs tmpfs $LFS/run\n"
        "if [ -h $LFS/dev/shm ]; then\n"
        "    install -v -d -m 1777 $LFS$(realpath /dev/shm)\n"
        "else\n"
        "    mount -vt tmpfs -o nosuid,nodev tmpfs $LFS/dev/shm\n"
        "fi\n";

    if (!lfsScript(cfg, cmd, "004_mount_tmpfs.log")) {
        failed("could not mount the virtual filesystems, see logs/004_mount_tmpfs.log\n");
        // Don't leave half of them mounted.
        unmountVfs(cfg);
        return false;
    }
    return true;
}

bool unmountVfs(Config cfg) {
    const char *cmd =
        "umount -v $LFS/dev/shm || umount -vl $LFS/dev/shm\n"
        "umount -v $LFS/run     || umount -vl $LFS/run\n"
        "umount -v $LFS/sys     || umount -vl $LFS/sys\n"
        "umount -v $LFS/proc    || umount -vl $LFS/proc\n"
        "umount -v $LFS/dev/pts || umount -vl $LFS/dev/pts\n"
        "umount -v $LFS/dev     || umount -vl $LFS/dev\n";

    // Every umount is expected to fail when that mount isn't there, so
    // check the real state afterwards instead of the script's exit code.
    lfsScript(cfg, cmd, "009_umount_tmpfs.log");
    return !vfsMounted(cfg);
}

static bool isIgnoredEntry(const char *name) {
    // .gitkeep keeps mnt/lfs in the repo. It's not a build artifact.
    return strcmp(name, ".") == 0 || strcmp(name, "..") == 0 || strcmp(name, ".gitkeep") == 0;
}

bool isDirEmpty(Config cfg, const char *path) {
    if (cfg.debug) {
        printf("[debug] isDirEmpty Path: %s\n", path);
    }

    DIR *dir = opendir(path);
    if (!dir) {
        perror("opendir");
        return false;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (isIgnoredEntry(entry->d_name)) {
            continue;
        }
        if (cfg.debug) {
            printf("[debug] isDirEmpty File contents: %s\n", entry->d_name);
        }
        closedir(dir);
        return false; // found something
    }

    closedir(dir);
    return true;
}

// Callback function for nftw()
static int unlink_cb(const char *fpath, const struct stat *sb, int typeflag, struct FTW *ftwbuf) {
    (void)sb; (void)typeflag; (void)ftwbuf;
    int rv = remove(fpath); // remove() works for both files and empty directories
    if (rv) perror(fpath);
    return rv;
}

// rm -rf. FTW_PHYS never follows symlinks and FTW_MOUNT never leaves the
// filesystem, so a bind mounted /dev can't be wiped by accident.
bool removeTree(const char *path) {
    struct stat st;
    if (lstat(path, &st) != 0) {
        return errno == ENOENT;
    }
    return nftw(path, unlink_cb, 64, FTW_DEPTH | FTW_PHYS | FTW_MOUNT) == 0;
}

bool deleteDirContents(Config cfg, const char *path) {
    if (cfg.debug) {
        printf("[debug] deleteDirContents Path: %s\n", path);
    }

    // Deleting through a mounted /dev would delete the host's devices.
    if (vfsMounted(cfg)) {
        failed("%s still has the virtual filesystems mounted. Refusing to delete. Unmount them first.\n", path);
        return false;
    }

    DIR *dir = opendir(path);
    if (!dir) {
        perror("opendir");
        return false;
    }

    bool ok = true;
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (isIgnoredEntry(entry->d_name)) continue;

        char child[PATH_MAX];
        snprintf(child, sizeof child, "%s/%s", path, entry->d_name);

        // An empty mount point (dev, proc..) is deleted by removeTree, a
        // populated one would be refused by FTW_MOUNT leaving it behind.
        if (!removeTree(child)) ok = false;
    }
    closedir(dir);
    return ok;
}

bool makeDirs(const char *path, mode_t mode) {
    char tmp[PATH_MAX];
    snprintf(tmp, sizeof tmp, "%s", path);

    for (char *p = tmp + 1; *p; p++) {
        if (*p != '/') continue;
        *p = '\0';
        if (mkdir(tmp, 0755) != 0 && errno != EEXIST) {
            perror(tmp);
            return false;
        }
        *p = '/';
    }
    if (mkdir(tmp, mode) != 0 && errno != EEXIST) {
        perror(tmp);
        return false;
    }
    // mkdir is masked by umask
    return chmod(tmp, mode) == 0;
}

static uid_t chownUid;
static gid_t chownGid;

static int chown_cb(const char *fpath, const struct stat *sb, int typeflag, struct FTW *ftwbuf) {
    (void)sb; (void)typeflag; (void)ftwbuf;
    if (lchown(fpath, chownUid, chownGid) != 0) {
        perror(fpath);
    }
    return 0;
}

// chown -R that stays on one filesystem
bool chownTree(const char *path, uid_t uid, gid_t gid) {
    chownUid = uid;
    chownGid = gid;
    return nftw(path, chown_cb, 64, FTW_PHYS | FTW_MOUNT) == 0;
}

bool copyRecipeDir(Config cfg) {
    if (cfg.debug) {
        printf("[Debug] copyRecipeDir()\n");
    }

    char dest[PATH_MAX];
    snprintf(dest, sizeof dest, "%s/recipes", cfg.buildPath);

    // cp -a into an existing dir would nest the recipes inside it
    if (!removeTree(dest)) {
        failed("Could not remove the old %s\n", dest);
        return false;
    }

    char *argv[] = {"cp", "-a", (char *)cfg.recipesPath, dest, NULL};
    RunSpec rs = {.argv = argv};

    if (runProcess(&rs) != 0) {
        failed("Failed to copy recipes\n");
        return false;
    }
    passed("Completed copying %s -> %s\n", cfg.recipesPath, dest);
    return true;
}

bool doesFileExist(Config cfg, const char *file) {
    if (cfg.debug) {
        printf("[Debug] doesFileExist()\n");
    }
    return access(file, F_OK) == 0;
}

bool download(Config cfg, const char *url, const char *path) {
    if (cfg.debug) {
        printf("[Debug] download(%s -> %s)\n", url, path);
    }

    // Download to a temp name so an interrupted download is never
    // mistaken for a finished tarball on the next run.
    char part[PATH_MAX];
    snprintf(part, sizeof part, "%s.part", path);

    char *argv[] = {"curl", "-L", "--fail", "--retry", "3", "-sS", "-o", part, (char *)url, NULL};
    RunSpec rs = {.argv = argv};

    if (runProcess(&rs) != 0) {
        unlink(part);
        failed("Failed to download %s with curl!\n", url);
        return false;
    }
    if (rename(part, path) != 0) {
        perror("rename");
        return false;
    }
    return true;
}
