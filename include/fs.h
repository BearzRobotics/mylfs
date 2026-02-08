#ifndef A5A9851A_DE99_493B_96A6_F58B147C5071
#define A5A9851A_DE99_493B_96A6_F58B147C5071

#include <stdio.h> // remove rmdir unlink
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/mount.h>
#include <sys/wait.h>
#include <limits.h>
#include <dirent.h> 
#include <ftw.h>
#include <sys/stat.h> // mkdir

#include "config.h"
#include "dprint.h"

const char *mounts[] = {
    "/dev",
    "/dev/pts",
    "/proc",
    "/sys",
    "/run",
};

// DJOK
// Need to check if bindmounts are already mounted. If so skip it

// bind mount /dev /dev/pts /proc /sys /run
bool mountBind(Config cfg, char *cpath) {
    //man 2 mount

    // We need to get the size of our mounts array
    size_t count = sizeof(mounts) / sizeof(mounts[0]);

    for (size_t i = 0; i < count; i++) {
        char target[PATH_MAX]; // limits.h

        snprintf(target, sizeof(target), "%s%s", cpath, mounts[i]);

        if (mount(mounts[i], target, NULL, MS_BIND, NULL) != 0) {
            perror("mount");
            failed("Could not bind mount needed filesystems\n");
            return false;
        } else {
            if (cfg.debug == true) {
                printf("Bind Mounted: %s -> %s\n", mounts[i], target);
            }
        }
    }
    return true;
}

// unbind mount /dev /dev/pts /proc /sys /run
bool unmountBind(Config cfg, char *cpath) {
    //man 2 umount

    // We need to get the size of our mounts array
    size_t count = sizeof(mounts) / sizeof(mounts[0]);

    // needs to be in reverse order
    for (size_t i = count; i-- > 0;) {
        char target[PATH_MAX]; // limits.h

        snprintf(target, sizeof(target), "%s%s", cpath, mounts[i]);

        if (umount2(target, MNT_DETACH) != 0) {
            perror("umount2");
            printf("umounted: %s\n", target);
            return false;
        }  else {
            if (cfg.debug == true) {
                printf("umounted: %s\n", target);
            }
        }
    }
    return true;
}


bool isDirEmpty(Config cfg, const char *path){
    if (cfg.debug) {
        printf("[debug] isDirEmpty Path: %s\n", path);
    }

    DIR *dir = opendir(path);
    if (!dir) {
        perror("opendir");
        return false;
    }

    // I need a dirent sturct to read the contents of my opendir
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0|| strcmp(entry->d_name, "..") == 0)  {
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
int unlink_cb(const char *fpath, const struct stat *sb, int typeflag, struct FTW *ftwbuf)
{
    int rv = remove(fpath); // remove() works for both files and empty directories

    if (rv)
        perror(fpath);

    return rv;
}

bool deleteDirContents(Config cfg, const char *path){
    if (cfg.debug) {
        printf("[debug] deleteDirContents Path: %s\n", path);
    }
    
    // remove(path) doesn't work if the dir if filled
    if (nftw(path, unlink_cb, 64, FTW_DEPTH | FTW_PHYS) != 0) {
        perror("remove");
        return false;
    }

    // Create the build dir

    // 777 allows all users to read and write to it
    if (mkdir(path, 777) != 0) {
        perror("mkdir");
        return false;
    }
    return true;
}

#endif /* A5A9851A_DE99_493B_96A6_F58B147C5071 */
