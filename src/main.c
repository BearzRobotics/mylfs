#define _XOPEN_SOURCE 700 // From the Musl features.h should work instead of _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L // From the Musl features.h should work instead of _GNU_SOURCE
#define _POSIX_SOURCE
#define _GNU_SOURCE // Chroot fails to compile without this  - Hopefully this won't create an issue with musl!

#include <stdio.h> // remove rmdir unlink
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h> //getuid chown
#include <errno.h>
#include <limits.h>
#include <signal.h>
#include <sys/wait.h>

#include <yaml.h>

#include "config.h"
#include "dprint.h"
#include "phase.h"
#include "proc.h"
#include "builder.h"
#include "fs.h"
#include "recipe.h"
#include "user.h"

// mounts is an array that holds these paths
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
    "   --start-phase   <phase number>\n        Accpets 1-5 for what phase to start in\n"
    "   --version\n Prints the current version number of the program\n\n"

    "See the config.yml for information on the config file.\n\n"
    "Author: Dakota James Owen Keeler\n"
    "Email: DakotaJKeeler@protonmail.com\n";

    printf("%s", help_mesg);
}

void version(void) {
    printf("0.4.0\n");
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
                    char *rp = realpath(value, NULL);
                    if (!rp) {
                        perror("realpath");
                        failed("build_path does not exist: %s\n     Create it first (e.g. mkdir -p '%s')\n", value, value);
                        exit(1);
                    }

                    cfg->buildPath = rp;   // take ownership

                } else if (strcmp(current_key, "recipes_path") == 0) {
                    char *rp = realpath(value, NULL);
                    if (!rp) {
                        perror("realpath");
                        failed("recipes_path does not exist: %s\n", value);
                        exit(1);
                    }
                    cfg->recipesPath = rp;
                } else if (strcmp(current_key, "bootstrap_only") == 0) {
                    cfg->bootstrap = parse_bool(value);
                } else if (strcmp(current_key, "version_check") == 0) {
                    cfg->versionCheck = strdup(value);
                } else if (strcmp(current_key, "make_flags") == 0) {
                    cfg->makeFlags = strdup(value);
                } else if (strcmp(current_key, "cflags") == 0) {
                    cfg->CFLAGS = strdup(value);
                } else if (strcmp(current_key, "cxxflags") == 0) {
                    cfg->CXXFLAGS = strdup(value);
                } else if (strcmp(current_key, "RUSTFLAGS") == 0) {
                    cfg->RUSTFLAGS = strdup(value);
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
    // Create a pipe before forking
    int pipefd[2]; 
    // pipefd[0] = read end (parent)
    // pipefd[1] = write end (child)
    if (pipe(pipefd) == -1) {
        perror("pipe");
        return false;
    }
    
    pid_t pid = fork();
    if (pid <0) {
        perror("fork");
        return false;
    }

    if (pid == 0) {
        close(pipefd[0]); // child doesn't read 

        dup2(pipefd[1], STDOUT_FILENO);
        dup2(pipefd[1], STDERR_FILENO);
        close(pipefd[1]);

        char *argv[] = {
            "bash",
            "-c",
            cfg.versionCheck,
            NULL
        };


        if (execve(hostBash(), argv, environ) != 0) {
            perror("execve");
            _exit(1);
        }

        _exit(0);
    }

    close(pipefd[1]); // parent doesn't write

    char buffer[4096];
    size_t total = 0;
    char *output = strdup("");

    ssize_t n;
    while ((n = read(pipefd[0], buffer, sizeof(buffer))) > 0) {
        char *tmp = realloc(output, total + n + 1);
        if (!tmp) {
            perror("realloc");
            break;
        }
        output = tmp;
        memcpy(output + total, buffer, n);
        total += n;
        output[total] = '\0';
    }
    close(pipefd[0]);

    // the child closes its end when it exits, so this is after the read
    int status;
    waitpid(pid, &status, 0);
    bool ok = WIFEXITED(status) && WEXITSTATUS(status) == 0;

    if (ok && cfg.debug) {
        printf("%s", output);
    } else if (!ok) {
        printf("%s", output);
    }
    free(output);
    return ok;
}

// Runs a fresh/resumed build. Everything that has to be undone afterwards
// is undone by the caller.
static bool runBuild(Config *cfg, const StrList *startPkgs) {
    // Only the packages up to the last phase we build need to be downloaded.
    int lastPhase = cfg->bootstrap ? LFS_BASE : EXTRA_PACKAGES;

    // Download into the real recipes dir so the tarballs are kept between builds
    RecipeList src;
    loadAllRecipes(*cfg, &src);
    // Like the python edition this isn't fatal. A recipe whose tarball is missing
    // fails on its own when its turn comes, and a package depending on it is skipped.
    if (!downloadTarballs(*cfg, &src, lastPhase)) {
        warn("Some tarballs could not be downloaded. Recipes missing theirs will fail to build.\n");
    }
    recipelistFree(&src);

    // The chroot can only see what is inside the build dir.
    // build_state.log has to survive that, it's the record of what was built.
    char keep[PATH_MAX + 32], log[PATH_MAX + 32];
    snprintf(keep, sizeof keep, "%s/.build_state.log", cfg->buildPath);
    snprintf(log, sizeof log, "%s/recipes/build_state.log", cfg->buildPath);
    rename(log, keep);

    bool copied = copyRecipeDir(*cfg);
    rename(keep, log);
    if (!copied) return false;

    // From here on use the copy, that's the one the chroot sees as /recipes
    char copy[PATH_MAX];
    snprintf(copy, sizeof copy, "%s/recipes", cfg->buildPath);
    cfg->recipesPath = strdup(copy);

    RecipeList recipes;
    if (!loadAllRecipes(*cfg, &recipes)) {
        warn("Some recipes could not be loaded and will be ignored\n");
    }
    passed("Loaded %zu recipes.\n", recipes.size);

    bool ok = true;


    while (ok && (int)cfg->phase <= lastPhase) {
        BuildPhase phase = cfg->phase;
        const StrList *pkgs = (phase == cfg->startPhase) ? startPkgs : NULL;

        switch (phase) {
        case CROSS_TOOLS: header("Phase 1 - Cross Tools"); break;
        case TEMP_TOOLS:  header("Phase 2 - Temp Tools"); break;
        case TEMP_SYSTEM: header("Phase 3 - Temp system"); break;
        case LFS_BASE:    header("Phase 4 - LFS Base System"); break;
        case EXTRA_PACKAGES: header("Phase 5 - Building final system"); break;
        }

        // The lfs user only builds phase 1 and 2. Everything after that happens
        // as root inside the chroot.
        if (phase < TEMP_SYSTEM) {
            if (lfsUserExist()) {
                warn("lfs already existed!\n");
            } else if (!creatLfsUser()) {
                failed("Could not create the lfs user!\n");
                ok = false;
                break;
            } else {
                passed("lfs user succefully created\n");
            }
            if (!chownLfsFiles(*cfg)) {
                failed("To setup .bashrc, .bash_profile or chown %s\n", cfg->buildPath);
                ok = false;
                break;
            }
        } else {
            if (lfsUserExist() && !deleteLfsUser()) {
                warn("Could not delete the lfs user\n");
            }
            if (!mountVfs(*cfg)) {
                ok = false;
                break;
            }
        }

        ok = builderPhase(*cfg, phase, &recipes, pkgs);
        if (!ok) {
            failed("Phase %d has failed!\n", (int)phase + 1);
            break;
        }

        cfg->phase = phase + 1;
        setPhase(*cfg, (short)cfg->phase);
    }

    if (ok) {
        passed("Done! The system is built in %s\n", cfg->buildPath);
    }

    recipelistFree(&recipes);
    return ok;
}

int main(int argc, char* argv[]) {
    if (getuid() != 0) {
        failed("mylfs requires root privalges! {chroot, bind mount, umount} Run at your own peril\n");
        exit(1);
    }

    // A recipe that exits early would otherwise kill us when we write its script
    signal(SIGPIPE, SIG_IGN);

    // Stack can contain random non-zero data that was causing my 
    // bool debug to always evaluate as true. Initializing it like
    // with the {0} fixed it.
    Config cfg = {0};

    //load our yaml config. By loading it here it will allow our command line options to 
    // override it.
    loadConfig(&cfg, "./config.yml");

    if (!cfg.buildPath || !cfg.recipesPath || !cfg.lfs_tgt) {
        failed("config.yml must set build_path, recipes_path and lfs_tgt\n");
        exit(1);
    }

    bool startPhaseGiven = false;
    StrList startPkgs;
    strlistInit(&startPkgs);

    for (int i = 1; i < argc; i++) {
        bool hasArg = (i + 1 < argc);

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
            if (!hasArg) {
                failed("--chroot needs the path to chroot into\n");
                exit(1);
            }
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
                    "bash",
                    "--login",
                    NULL
                };

                if (execve("/bin/bash", sh_argv, environ) != 0) {
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
        } else if (strcmp(argv[i], "--start-package") == 0 && hasArg) {
           // takes one or more names: --start-package nwipe parted
           while (i + 1 < argc && strncmp(argv[i + 1], "--", 2) != 0) {
               strlistPush(&startPkgs, strdup(argv[++i]));
           }
        } else if (strcmp(argv[i], "--build-path") == 0 && hasArg) {
           char *rp = realpath(argv[++i], NULL);
           if (!rp) {
               failed("--build-path does not exist: %s\n", argv[i]);
               exit(1);
           }
           cfg.buildPath = rp;
        } else if (strcmp(argv[i], "--recipes-path") == 0 && hasArg) {
           char *rp = realpath(argv[++i], NULL);
           if (!rp) {
               failed("--recipes-path does not exist: %s\n", argv[i]);
               exit(1);
           }
           cfg.recipesPath = rp;
        } else if (strcmp(argv[i], "--start-phase") == 0 && hasArg) {
            int n = atoi(argv[++i]);
            if (n < 1 || n > 5) {
                failed("--start-phase takes a number between 1 and 5\n");
                exit(1);
            }
            startPhaseGiven = true;
            cfg.phase = (BuildPhase)(n - 1);   // the command line is 1 based
        } else {
            failed("Unknown option or missing argument: %s\n", argv[i]);
            help();
            exit(1);
        }
    }
    cfg.startPhase = cfg.phase;

    if (startPkgs.size > 0 && !startPhaseGiven) {
        failed("--start-package must be used with --start-phase\n");
        exit(1);
    }

    header("mylfs-c edition");
    if (cfg.debug == true) {
        printf("[Debug] Mode has been enabled!\n");
    }
    if (!cfg.test) {
        warn("skipping tests\n");
    }

    if (versionCheck(cfg) == false) {
        failed("System does not meet the required build deps\n");
        exit(1);
    } else {
        passed("Your system meets the mininum requirments!\n");
    }

    // Starting from scratch? Then the build dir should be empty.
    if (cfg.phase == CROSS_TOOLS) {
        if (isDirEmpty(cfg, cfg.buildPath) == false) {
            warn("Build dir %s is not empty. Do you want to delete the contents y/n: ", cfg.buildPath);
            fflush(stdout);
            int input = getchar();
            if (input == 'y')  {
                if (!deleteDirContents(cfg, cfg.buildPath)) {
                    failed("Could not empty %s\n", cfg.buildPath);
                    exit(1);
                }
            } else {
                warn("Continuing at your own risk!\n");
            }
        }
    }

    bool ok = runBuild(&cfg, &startPkgs);
    strlistFree(&startPkgs);

    // -----------------------------------------------------------
    // Cleanup
    // -----------------------------------------------------------
    if (vfsMounted(cfg)) {
        if (unmountVfs(cfg)) {
            passed("unmounted tmp fs\n");
        } else {
            failed("could not unmount tmpfs, check %s by hand\n", cfg.buildPath);
        }
    }

    if (lfsUserExist()) {
        if (deleteLfsUser() == false) {
            failed("Could not delete the LFS user\n");
        } else {
            passed("User 'lfs' deleted successfully\n");
        }
    }

    return ok ? 0 : 1;
}
