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
#include <pwd.h> // password file
#include <unistd.h>
#include <pwd.h>
#include <grp.h>

#include "dprint.h"
#include "config.h"

// Test whether the LFS user exist
// NOTE: getpwnam requires glibc NSS at runtime even in static builds.
// This binary is intended for musl-based systems where this is not an issue.
bool lfsUserExist(void) {
    // man getpwnam
    // when creating a variable that returns a pointer from a struct
    // you must use the struct keyword <nameofstruct> <type> = function that returns
    // the struct

    //getpwnam has mutliple backends and even in static linking will loaded the
    //appropriate backend. -- This can cause breakage if glibc version is incompatiable
    //with the one the static program was compiled against.
    struct passwd *pw = getpwnam("lfs");
    if (pw == NULL) {
        return false; // user doesn't exist
    } else {
        return true; // user does exist.
    }
}

// Create the LFS user
bool creatLfsUser(Config cfg) {
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
    struct passwd *pw =getpwnam("lfs");
    if (!pw) {
        failed("User lfs could not be found!\n This should not be happing because we just created it!\n");
        return false;
    }

    chown("/home/lfs/.bash_profile", pw->pw_uid, pw->pw_gid);
    chown("/home/lfs/.bashrc", pw->pw_uid, pw->pw_gid);
    chown(cfg.buildPath, pw->pw_uid, pw->pw_gid);

    passed("User 'lfs' created successfully\n");
}

// Delete the LFS user for cleanup
bool deleteLfsUser(void) {
    // userdel deletes a user account
    // -r removes the home dir
    // lfs is the user account
    if (system("userdel -r lfs") != 0) {
        fprintf(stderr, "Failed to delete user\n");
        return false;
    }
    
    return true;
}

