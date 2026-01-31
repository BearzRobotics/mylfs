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


#include "dprint.h"

// Test whether the LFS user exist
bool lfsUserExist(void) {
    // man getpwnam
    // when creating a variable that returns a pointer from a struct
    // you must use the struct keyword <nameofstruct> <type> = function that returns
    // the struct

    //getpwnam has mutliple backends and even in static linking will loaded the
    //appropriate backend. -- This can cause breakage if glibc version is incompatiable
    //with the one the static program was compiled against.
    struct passwd *pwd = getpwnam("lfs");
    if (pwd == NULL) {
        return false; // user doesn't exist
    } else {
        return true; // user does exist.
    }


}

// Create the LFS user
bool creatLfsUser(void) {
    // system technincally runs a system shell. 
    // It would probably become more efficent to fork it.
    if (system("useradd -m -s /bin/bash lfs") != 0) {
        fprintf(stderr, "Failed to create user\n");
        return false;
    }

    passed("User 'lfs' created successfully\n");
    return true;
}

// Delete the LFS user for cleanup
bool deleteLfsUser(void) {
    if (system("userdel -r -s lfs") != 0) {
        fprintf(stderr, "Failed to delete user\n");
        return false;
    }

    passed("User 'lfs' deleted successfully\n");
    return true;
}

