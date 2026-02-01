#ifndef MYLFS_CONFIG_H
#define MYLFS_CONFIG_H

#include <stdbool.h>
#include "phase.h"

typedef struct {
    bool debug;
    BuildPhase phase;
    char *packName;
    bool test;
    bool cleanup_src;
    char *lfs_tgt;
    bool keepLogs;
    char *buildPath;
    char *recipesPath;
    bool bootstrap;
    char *versionCheck;
    char *makeFlags;
    char *CFLAGS;
    char *CXXFLAGS;
    char *RUSTFLAGS;
} Config;

/* Declare the global config variable */
extern Config cfg;

#endif // MYLFS_CONFIG_H
