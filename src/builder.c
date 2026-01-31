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
#include <pwd.h>


#include "config.h"
#include "dprint.h"

int builder(Config cfg) {
    if (cfg.debug) {
        printf("[debug] Entered the builder phase\n");
    }




            if (chroot(cfg.buildPath) != 0) {
                perror("chroot to chrootPath Failed!\n");
                exit(1);
            }
        
            if (chdir("/") != 0) {
                perror("chdir");
                exit(1);
            }
            
            // man exec(3) -- _GNU_SOURCE
            // While for the --chroot this was alright, execl is not the right
            // one for the builder
            // TLPI pg 563 - 589
            if (execl("/bin/sh", "sh" "--login", NULL) != 0) {
                perror("execl\n");
                exit(1);
            }
           
            exit(0);

} 