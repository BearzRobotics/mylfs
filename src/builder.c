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
#include "recipe.h"

const char * getTarballSuffix(const char *tarball){
    static const char *suffixes[] = {
        ".tar.gz", ".tgz", ".tar.bz2", ".tar.xz", ".tar.zst", ".tar", ".zip",
        NULL  // sentinel to mark end of array
    };


}


bool extractTarball(Config cfg, Recipe recipe, short int phase) {
    if (cfg.debug) {
        printf("    [debug] tarball name: [%s] Tarball path: [%s]\n", recipe.tarballName, recipe.tarballPath);
    }
    
    // Setup our command buffer
    char cmd[4096]; // Large to account for unkown tarball path/name size

    // Get the suffix to know what command to run on it
    const char *suffix = getTarballSuffix(recipe.tarballName);
    if (suffix == NULL) {
        failed("Unknown tarball format: %s", recipe.tarballName);
    } else if (suffix == ".zip") {
        snprintf(cmd, sizeof(cmd), "unzip -q %s -d %s", recipe.tarballPath);
    } else {
        // This might be bad practice but I'm for now assuming that everything else is tar
        if (phase <= 1) {
            // lfs is a hard coded user for building the system
            snprintf(cmd, sizeof(cmd), "sudo --preserve-env -u lfs tar -xf %s -C %s --strip-components=1", recipe.tarballPath, recipe.recipeSource);
        } else {
            // Done inside the chroot from phase 2 and on.
            snprintf(cmd, sizeof(cmd), "tar -xf %s -C %s --strip-components=1", recipe.tarballPath, recipe.recipeSource);
        }

    }


    if (system(cmd) != 0) {
        failed("Could not extract tarball: [%s]\n", recipe.tarballPath);
        return false;
    } else {
        return true;
    }
}

// load phase.yaml and grab it
// This file only has a single int, and doesn't need to use yaml at all.
// However, to maintain compatiblity i'm leaving the .yaml extension
short getPhase(Config cfg) {
    // generate string for fopen() call
    char path[4096];
    snprintf(path, sizeof(path), "%s/recipes/phase.yaml", cfg.buildPath);

    FILE *fp = fopen(path, "r");
    if (!fp) {
        perror("fopen");
    }

    short phase = 0;
    fscanf(fp, "%hd", &phase);

    fclose(fp);
    return phase;
}

// Updates the phase number in the phase.yaml
bool setPhase(Config cfg, short phase) {
    // generate string for fopen() call
    char path[4096];
    snprintf(path, sizeof(path), "%s/recipes/phase.yaml", cfg.buildPath);

    FILE *fp = fopen(path, "w");
    if (!fp) {
        perror("fopen");
        return false;
    }

    fprintf(fp, "%hd", phase);
    fclose(fp);
    return true;
}

// deletes dir
bool cleanup(Config cfg) {
    return true;
}

bool writeLogs(Config cfg) {
    return true;
}

bool builderBootstrap(Config cfg) {   
    return true;
} 


bool builderPhase5(Config cfg) {
    return true;
}
