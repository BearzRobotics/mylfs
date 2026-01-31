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
#include <sys/mount.h>
#include <sys/wait.h>
#include <limits.h>

#include "config.h"
#include "dprint.h"



// mounts is an array that holds these paths
const char *mounts[] = {
    "/dev",
    "/dev/pts",
    "/proc",
    "/sys",
    "/run",
};


void help(void) {
const char* help_mesg =
    "usage: mylfs\nBy default mylfs will load config.yml to operate\n\n"
    "Options:"
    "   --bootstrap\n      Only built phase 1-4\n"
    "   --chroot <PATH>\n   Open a chroot at your build path\n"
    "   --debug\n   Prints out debug values\n"
    "   --help\n    Prints out this help menu\n"
    "   --test\n    Tells whether to run the test if a recipe supports it\n"
    "   --start-package <Package name>\n        Takes the name of a recipe\n"
    "   --start-pahse   <phase number>\n        Accpets 1-5 for what phase to start in\n"
    "   --version\n Prints the current version number of the program\n\n"

    "See the config.yml for information on the config file.\n\n"
    "Author: Dakota James Owen Keeler\n"
    "Email: DakotaJKeeler@protonmail.com\n";


    printf("%s", help_mesg);
}


void version(void) {
    printf("0.0.6\n");
}

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




int main(int argc, char* argv[]) {

    /*
    // I don't remember why I needed root privalges for mylfs-py -- Duh chroot
    */
    if (getuid() != 0) {
        failed("mylfs requires root privalges! Run at your own peril\n");
        exit(1);
    }

    // Stack can contain random non-zero data that was causing my 
    // bool debug to always evaluate as true. Initializing it like
    // with the {0} fixed it.
    Config cfg = {0, .phase=0};

    for (int i = 0; i < argc; i++) {
        if (strcmp(argv[i], "--debug") == 0) {
            cfg.debug = true;
        } else if (strcmp(argv[i], "--help") == 0) {
            help();
            exit(0);
        } else if (strcmp(argv[i], "--version") == 0) {
            version();
            exit(0);
        } else if (strcmp(argv[i], "--bootstrap") == 0) {
           cfg.bootstrap = true;
        } else if (strcmp(argv[i], "--chroot") == 0) {
            // unistd.h _GNU_SOURCE
            // chroot syscall only changes the root dir of the current process
            char oldcwd[PATH_MAX];
            char *resolved = realpath(argv[i+1], NULL);

            if (!resolved) {
                perror("realpath");
                exit(1);
            }

            if (mountBind(cfg, resolved) == false) {
                failed("Could not mount the bind dirs! dev:dev/pts:sys:proc:run\n ");
                //exit(1); The system can build without these bind mounted
            }



            
            pid_t pid = fork();
            if (pid < 0) {
                perror("fork");
                exit(1);
            }

            
            if (pid == 0) {
                if (chroot(resolved) != 0) {
                    perror("chroot to chrootPath Failed!\n");
                    exit(1);
                }

                if (chdir("/") != 0) {
                    perror("chdir");
                    exit(1);
                }
                // man exec(3) -- _GNU_SOURCE

                char *sh_argv[] = {
                    "sh",
                    "--login",
                    NULL
                };

                if (execve("/bin/sh", sh_argv, environ) != 0) {
                    perror("execl\n");
                    _exit(1);
                }
                
                _exit(0);
            }
            
            int status;
            wait(&status);

            if (!getcwd(oldcwd, sizeof(oldcwd))) {
                perror("getcwd");
                exit(1);
            }

            if (unmountBind(cfg, resolved) == false) {
                failed("Could not unmount the bind dirs! dev:dev/pts:sys:proc:run\n ");
                exit(1);
            }
            free(resolved);
            exit(0);
           
        } else if (strcmp(argv[i], "--test") == 0) {
           cfg.test = true;
        } else if (strcmp(argv[i], "--start-package") == 0) {
           cfg.packName = argv[i+1];
        } else if (strcmp(argv[i], "--start-phase") == 0) {
           //cfg.phase = argv[i+1];
            cfg.phase = (short)atoi(argv[i + 1]);
        }
    }



    header("mylfs-c edition");
    if (cfg.debug == true) {
        printf("[Debug] Mode has been enabled!\n");
    }

    // Load my yaml config
    // need to check for required tools
    // check if builderDir is empty 
    //      If it isn't ask if they want to delete it
    // mount bind dirs - I also need to umount bind dirs
    // initalize recipes 
    //      Download packages
    // Copies recipes over
    // Create lfs user and group
    // chown buildDir to lfs user and group

    if (cfg.phase == 0) {
        header("Phase 1 - Cross Tools");
        cfg.phase += 1;
    }

    if (cfg.phase == 1) {
        header("Phase 2 - Temp Tools");
        cfg.phase += 1;
    }

    if (cfg.phase == 2) {
        header("Phase 3 - Temp system");
        cfg.phase += 1;
    }

    if (cfg.phase == 3) {
        header("Phase 4 - LFS Base System");
        cfg.phase += 1;
    }

    if (cfg.phase == 4 && cfg.bootstrap == false) {
        header("Phase 5 - Building extra packages per the recipes!");
    }

    return 0;
}


