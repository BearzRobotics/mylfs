#ifndef MYLFS_CONFIG_H
#define MYLFS_CONFIG_H

#include <stdbool.h>

typedef struct {
    bool debug;
    short phase;
    char *packName;
    bool test;
    bool cleanup_src;
    char *lfs_tgt;
    bool keepLogs;
    char *buildPath;
    char *recipesPath;
    bool bootstrap;
    char *versionCheck;
} Config;

/* Declare the global config variable */
extern Config cfg;

#endif // MYLFS_CONFIG_H
