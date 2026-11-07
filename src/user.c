#define _XOPEN_SOURCE 700 // From the Musl features.h should work instead of _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L // From the Musl features.h should work instead of _GNU_SOURCE
#define _POSIX_SOURCE
#define _GNU_SOURCE // Chroot fails to compile without this  - Hopefully this won't create an issue with musl!

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <unistd.h>

#include "dprint.h"
#include "config.h"
#include "fs.h"
#include "user.h"

bool lookupUser(const char *name, uid_t *uid, gid_t *gid) {
    FILE *fp = fopen("/etc/passwd", "re");
    if (!fp) {
        return false;
    }

    // name:passwd:uid:gid:gecos:home:shell
    char line[1024];
    bool found = false;
    size_t nlen = strlen(name);

    while (fgets(line, sizeof line, fp)) {
        if (strncmp(line, name, nlen) != 0 || line[nlen] != ':') continue;

        char *save = NULL;
        strtok_r(line, ":", &save);               // name
        strtok_r(NULL, ":", &save);               // passwd
        char *u = strtok_r(NULL, ":", &save);     // uid
        char *g = strtok_r(NULL, ":", &save);     // gid
        if (!u || !g) continue;

        if (uid) *uid = (uid_t)strtoul(u, NULL, 10);
        if (gid) *gid = (gid_t)strtoul(g, NULL, 10);
        found = true;
        break;
    }

    fclose(fp);
    return found;
}

// Test whether the LFS user exist
bool lfsUserExist(void) {
    return lookupUser("lfs", NULL, NULL);
}

// Create the LFS user
bool creatLfsUser(void) {
    // system technincally runs a system shell. 
    // It would probably become more efficent to fork it.
    // useradd -- adds a user to the system
    // -k /dev/null tells the useradd command to use /dev/null as the skel dir
    // -m creates the home dir
    // -d /home/lfs sets the explicit path of the home dir
    // -s /bin/bash sets the users default shell
    // lfs is the user name
    if (system("useradd -k /dev/null -m -d /home/lfs -s /bin/bash lfs") != 0) {
        fprintf(stderr, "Failed to create user\n");
        return false;
    }

    return true;
}

// This creates a .bashrc, .bash_profile and chowns them all
// with the build dir
bool chownLfsFiles(Config cfg) {
    char data_profile[] = "exec env -i HOME=$HOME TERM=$TERM PS1='\\u:\\w\\$ ' /bin/bash";
    char datarc[4096];
    snprintf(datarc, sizeof(datarc), "set +h \n umask 022 \n LFS=%s \n LC_ALL=POSIX \n LFS_TGT=%s \n PATH=/usr/bin  \n if [ ! -L /bin ]; then PATH=/bin:$PATH; fi \n PATH=$LFS/tools/bin:$PATH \n CONFIG_SITE=$LFS/usr/share/config.site \n export LFS LC_ALL LFS_TGT PATH CONFIG_SITE\n", cfg.buildPath, cfg.lfs_tgt);

    //create our bashrc file
    FILE *frc = fopen("/home/lfs/.bashrc", "w");
    if (!frc) {
        perror("fopen");
        return false;
    }
    fputs(datarc, frc);
    fclose(frc);

    // create our profile file
        //create our bashrc file
    FILE *fbpr = fopen("/home/lfs/.bash_profile", "w");
    if (!fbpr) {
        perror("fopen");
        return false;
    }
    fputs(data_profile, fbpr);
    fclose(fbpr);

    // We need to make sure our user lfs owns those files
    uid_t uid;
    gid_t gid;
    if (!lookupUser("lfs", &uid, &gid)) {
        failed("User lfs could not be found!\n This should not be happing because we just created it!\n");
        return false;
    }

    if (chown("/home/lfs/.bash_profile", uid, gid) != 0 || chown("/home/lfs/.bashrc", uid, gid) != 0) {
        perror("chown");
    }

    // Everything under the build dir (tools, recipes, ...) has to be writable
    // by lfs for phase 1 and 2. This does not cross into mounted filesystems.
    if (!chownTree(cfg.buildPath, uid, gid)) {
        failed("could not chown: %s to the lfs user\n", cfg.buildPath);
        return false;
    }

    passed("chowned %s to the lfs user\n", cfg.buildPath);
    return true;
}

// Delete the LFS user for cleanup
bool deleteLfsUser(void) {
    // userdel deletes a user account
    // -r removes the home dir
    // lfs is the user account
    if (!lfsUserExist()) {
        return true;
    }
    if (system("userdel -r lfs") != 0) {
        fprintf(stderr, "Failed to delete user\n");
        return false;
    }
    
    return true;
}

