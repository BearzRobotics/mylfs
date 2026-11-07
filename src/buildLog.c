#define _GNU_SOURCE
#include <limits.h>
#include <stdio.h>

#include "buildLog.h"
#include "dprint.h"

bool buildLogAppend(Config cfg, const BuildRecord *rec) {
    char path[PATH_MAX];
    snprintf(path, sizeof path, "%s/recipes/build_state.log", cfg.buildPath);

    FILE *fp = fopen(path, "a");
    if (!fp) {
        perror(path);
        return false;
    }
    fprintf(fp, "%ld|%s|%s|%d|%s\n", (long)rec->timestamp, rec->name, rec->version,
            (int)rec->phase, rec->built ? "true" : "false");
    fclose(fp);
    return true;
}
