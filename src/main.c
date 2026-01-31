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
#include <dirent.h>

#include <yaml.h>

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


void print_yaml_event(yaml_event_t *event) {
    switch (event->type) {
        case YAML_NO_EVENT:       printf("YAML_NO_EVENT\n"); break;
        case YAML_STREAM_START_EVENT:  printf("STREAM_START\n"); break;
        case YAML_STREAM_END_EVENT:    printf("STREAM_END\n"); break;
        case YAML_DOCUMENT_START_EVENT: printf("DOC_START\n"); break;
        case YAML_DOCUMENT_END_EVENT:   printf("DOC_END\n"); break;
        case YAML_MAPPING_START_EVENT:  printf("MAPPING_START\n"); break;
        case YAML_MAPPING_END_EVENT:    printf("MAPPING_END\n"); break;
        case YAML_SEQUENCE_START_EVENT: printf("SEQ_START\n"); break;
        case YAML_SEQUENCE_END_EVENT:   printf("SEQ_END\n"); break;
        case YAML_SCALAR_EVENT:
            printf("SCALAR: %s\n", event->data.scalar.value);
            break;
        default:
            printf("UNKNOWN_EVENT: %d\n", event->type);
            break;
    }
}


static bool parse_bool(const char *v) {
    return strcmp(v, "true") == 0 || strcmp(v, "yes") == 0 || strcmp(v, "1") == 0;
}

// without the pointer (*) I only get a copy of the Config struct
// not the actually config Struct
int loadConfig(Config *cfg, const char *cfpath){
    // cfg.debug won't work here because of where it's called  
    // printf("[debug] loadConfig() cfpath: %s", cfpath);

    FILE *fh = fopen(cfpath, "r");
    if (!fh) {
        perror("fopen");
        exit(-1);
    }

    yaml_parser_t parser;
    yaml_event_t event;

    if (!yaml_parser_initialize(&parser)) {
        fprintf(stderr, "Failed to initialize YAML parser\n");
        fclose(fh);
        exit(-1);
    }

    yaml_parser_set_input_file(&parser, fh);
    char *current_key = NULL;

    while (1) {
        if (!yaml_parser_parse(&parser, &event)) {
            fprintf(stderr, "YAML parse error\n");
            goto error;
        }

        // cfg.debug won't work here because of where it's called            
        //print_yaml_event(&event);
      
        if (event.type == YAML_SCALAR_EVENT) {
            char *value = (char *)event.data.scalar.value;

            if (!current_key) {
                current_key = strdup(value);
            } else {
                // This scalar is a VALUE
                if (strcmp(current_key, "lfs_tgt") == 0) {
                    cfg->lfs_tgt = strdup(value);
                } else if (strcmp(current_key, "test") == 0) {
                    cfg->test = parse_bool(value);
                } else if (strcmp(current_key, "cleanup_sources") == 0) {
                    cfg->cleanup_src = parse_bool(value);
                } else if (strcmp(current_key, "keep_logs") == 0) {
                    cfg->keepLogs = parse_bool(value);
                } else if (strcmp(current_key, "build_path") == 0) {
                    cfg->buildPath = strdup(realpath(value, NULL));
                } else if (strcmp(current_key, "recipes_path") == 0) {
                    cfg->recipesPath = strdup(realpath(value, NULL));
                } else if (strcmp(current_key, "bootstrap_only") == 0) {
                    cfg->bootstrap = parse_bool(value);
                } else if (strcmp(current_key, "version_check") == 0) {
                    cfg->versionCheck = strdup(value);
                }

                free(current_key);
                current_key = NULL;
            }
        }

        if (event.type == YAML_STREAM_END_EVENT) {
            yaml_event_delete(&event);
            break;
        }

        yaml_event_delete(&event);
    }

    yaml_parser_delete(&parser);
    fclose(fh);
    return 0;

    error:
    yaml_event_delete(&event);
    yaml_parser_delete(&parser);
    fclose(fh);
    return -1;
}


bool versionCheck(Config cfg) {
    pid_t pid = fork();
    if (pid <0) {
        perror("fork");
        return false;
    }

    if (pid == 0) {
        char *argv[] = {
            "bash",
            "-c",
            cfg.versionCheck,
            NULL
        };


        if (execve("/usr/bin/bash", argv ,environ) != 0) {
            perror("execve");
            _exit(1);
        }

        _exit(0);
    }

    int status;
    wait(&status);

    if (status == 0) {
        return true;
    } else {
        return false;
    }

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

bool deleteDirContents(Config cfg, const char *path){
    if (cfg.debug) {
        printf("[debug] deleteDirContents Path: %s\n", path);
    }

    DIR *dir = opendir(path);
    if (!dir) {
        perror("opendir");
        return false;
    }

    // I need a dirent sturct to read the contents of my opendir
    struct dirent *entry;
  

    closedir(dir);
    return true;
}

int main(int argc, char* argv[]) {
    if (getuid() != 0) {
        failed("mylfs requires root privalges! {chroot, bind mount, umount} Run at your own peril\n");
        exit(1);
    }

    // Stack can contain random non-zero data that was causing my 
    // bool debug to always evaluate as true. Initializing it like
    // with the {0} fixed it.
    Config cfg = {0, .phase=0};

    //load our yaml config. By loading it here it will allow our command line options to 
    // override it.
    loadConfig(&cfg, "./config.yml");

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
                    perror("execve\n");
                    _exit(1);
                }
                
                _exit(0);
            }
            
            int status;
            wait(&status);

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


    if (cfg.debug) {
        printf("[Debug] cfg.versionCheck: %s\n", cfg.versionCheck);
    }


    if (versionCheck(cfg) == false) {
        failed("System does not meet the required build deps\n");
        exit(1);
    }
    

    // need to check for required tools
    // check if builderDir is empty 
    //      If it isn't ask if they want to delete it
    if (isDirEmpty(cfg, cfg.buildPath)) {
        warn("Build dir is not empty. Do you want to delete the contents y/n: ");
        int input;
        input = getchar(); 
        if (input == 'y')  {
            deleteDirContents(cfg, cfg.buildPath);
        }
    }


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


