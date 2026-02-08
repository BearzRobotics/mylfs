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
#include <errno.h>
#include <sys/stat.h> // mkdir
#include <unistd.h>

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

bool isMounted(char *cpath) {
    FILE *fh = fopen("/proc/self/mountinfo", "re");
    if (!fh) {
        perror("fopen");
        return false;
    }

    char line[4096];
    bool found = false;

    while(fgets(line, sizeof line, fh)) {
        // mountinfo format:
        // id parent major:minor root mount_point options ... - fstype source superopts
        // We want the 5th field: mount_point
        //
        // Fields are space-separated, but mount_point may contain escaped spaces as \040
        // (same for root).
        char *save = NULL;
        char *tok = strtok_r(line, " ", &save); // 1: id
        if (!tok) continue;
        tok = strtok_r(NULL, " ", &save);       // 2: parent
        tok = strtok_r(NULL, " ", &save);       // 3: major:minor
        tok = strtok_r(NULL, " ", &save);       // 4: root
        tok = strtok_r(NULL, " ", &save);       // 5: mount_point
        if (!tok) continue;

        // tok is the mount_point, possibly with escapes like \040 for space.
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
    //man 2 mount

    // We need to get the size of our mounts array
    size_t count = sizeof(mounts) / sizeof(mounts[0]);

    for (size_t i = 0; i < count; i++) {
        char target[PATH_MAX]; // limits.h

        snprintf(target, sizeof(target), "%s%s", cpath, mounts[i]);

        if (!isMounted(target)) {
            if (mount(mounts[i], target, NULL, MS_BIND, NULL) != 0) {
                perror("mount");
                failed("Could not bind mount needed filesystems\n");
                return false;
            } else {
                if (cfg.debug == true) {
                    printf("Bind Mounted: %s -> %s\n", mounts[i], target);
                }
            }
        } else {
            char buf[1024];
            snprintf(buf, sizeof buf, "Already mounted: %s", target);
            skip(buf);
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



bool copyRecipeDir(Config cfg) {
    if (cfg.debug) {
        printf("[Debug] copyRecipeDir()\n");
    }

    char cmd[4096];

    snprintf(cmd, sizeof cmd,
            "cp -a '%s' '%s/recipes'",
            cfg.recipesPath,
            cfg.buildPath);

    if (system(cmd) != 0) {
        failed("Failed to copy recipes\n");
    } else {
        passed("Passed: Completed copying %s -> %s/recipes\n", cfg.recipesPath, cfg.buildPath);
    }

}

bool doesFileExist(Config cfg, const char file) {
    if (cfg.debug) {
        printf("[Debug] doesFileExist()\n");
    }

    if (access(file, F_OK) == 0) {
        return true; // file exist
    }

    return false;
}
 
bool download(Config cfg, const char url, const char path) {
    if (cfg.debug) {
        printf("[Debug] download()\n");
    }

    char cmd[4096];


    snprintf(cmd, sizeof cmd,
            "curl -L --fail --retry 3 -o '%s' '%s'",
            path,
            url);

    if (system(cmd) != 0) {
        failed("Failed to downloaded packages with curl!\n");
        return false;
    } 

    return true;

}

#endif /* A5A9851A_DE99_493B_96A6_F58B147C5071 */
