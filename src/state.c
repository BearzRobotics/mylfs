#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "state.h"

static char *joinList(const StrList *l) {
    size_t n = 1;
    for (size_t i = 0; i < l->size; i++) n += strlen(l->data[i]) + 1;
    char *out = malloc(n);
    if (!out) { perror("malloc"); exit(1); }
    out[0] = '\0';
    for (size_t i = 0; i < l->size; i++) {
        if (i) strcat(out, ",");
        strcat(out, l->data[i]);
    }
    return out;
}

static void entryFree(StateEntry *e) {
    free(e->name); free(e->version); free(e->builddeps); free(e->rundeps);
}

static StateEntry *statePush(BuildState *st) {
    if (st->size == st->cap) {
        st->cap = st->cap ? st->cap * 2 : 64;
        st->data = realloc(st->data, st->cap * sizeof(StateEntry));
        if (!st->data) { perror("realloc"); exit(1); }
    }
    StateEntry *e = &st->data[st->size++];
    *e = (StateEntry){0};
    return e;
}

bool stateLoad(Config cfg, BuildState *st) {
    *st = (BuildState){0};
    snprintf(st->path, sizeof st->path, "%s/recipes/build_state.db", cfg.buildPath);

    FILE *fp = fopen(st->path, "r");
    if (!fp) {
        return true;    // nothing built yet
    }

    char line[8192];
    while (fgets(line, sizeof line, fp)) {
        line[strcspn(line, "\n")] = '\0';
        if (!*line) continue;

        // strsep keeps empty fields, strtok would swallow them
        char *p = line;
        char *f[7];
        int n = 0;
        while (n < 7 && p) f[n++] = strsep(&p, "|");
        if (n < 7) continue;

        StateEntry *e = statePush(st);
        e->name = strdup(f[0]);
        e->version = strdup(f[1]);
        e->release = atoi(f[2]);
        e->phase = atoi(f[3]);
        e->built = strcmp(f[4], "true") == 0;
        e->builddeps = strdup(f[5]);
        e->rundeps = strdup(f[6]);
    }
    fclose(fp);
    return true;
}

bool stateSave(const BuildState *st) {
    // Write next to it and rename, a crash can't leave half a file behind
    char tmp[4200];
    snprintf(tmp, sizeof tmp, "%s.tmp", st->path);

    FILE *fp = fopen(tmp, "w");
    if (!fp) {
        perror(tmp);
        return false;
    }
    for (size_t i = 0; i < st->size; i++) {
        const StateEntry *e = &st->data[i];
        fprintf(fp, "%s|%s|%d|%d|%s|%s|%s\n", e->name, e->version, e->release, e->phase,
                e->built ? "true" : "false", e->builddeps, e->rundeps);
    }
    if (fclose(fp) != 0) {
        perror(tmp);
        return false;
    }
    return rename(tmp, st->path) == 0;
}

void stateFree(BuildState *st) {
    for (size_t i = 0; i < st->size; i++) entryFree(&st->data[i]);
    free(st->data);
    *st = (BuildState){0};
}

bool stateWasBuilt(const BuildState *st, const char *name) {
    for (size_t i = 0; i < st->size; i++) {
        if (st->data[i].built && strcmp(st->data[i].name, name) == 0) return true;
    }
    return false;
}

bool stateNewRelease(const BuildState *st, const char *name, int release) {
    for (size_t i = 0; i < st->size; i++) {
        if (st->data[i].built && strcmp(st->data[i].name, name) == 0 && st->data[i].release < release) {
            return true;
        }
    }
    return false;
}

void stateRecord(BuildState *st, const Recipe *r, bool built) {
    int phase = r->phase + 1;
    StateEntry *e = NULL;

    for (size_t i = 0; i < st->size; i++) {
        if (st->data[i].phase == phase && strcmp(st->data[i].name, r->name) == 0) {
            e = &st->data[i];
            entryFree(e);
            break;
        }
    }
    if (!e) e = statePush(st);

    e->name = strdup(r->name);
    e->version = strdup(r->version);
    e->release = r->release ? atoi(r->release) : 1;
    e->phase = phase;
    e->built = built;
    e->builddeps = joinList(&r->builddeps);
    e->rundeps = joinList(&r->rundeps);
    stateSave(st);
}

void stateMarkDirty(BuildState *st, const char *name) {
    bool changed = false;
    for (size_t i = 0; i < st->size; i++) {
        if (strcmp(st->data[i].name, name) == 0 && st->data[i].built) {
            st->data[i].built = false;
            changed = true;
        }
    }
    if (changed) stateSave(st);
}

bool stateMissingDeps(const BuildState *st, const Recipe *r, StrList *missing) {
    strlistInit(missing);
    for (size_t i = 0; i < r->builddeps.size; i++) {
        if (!stateWasBuilt(st, r->builddeps.data[i])) {
            strlistPush(missing, strdup(r->builddeps.data[i]));
        }
    }
    return missing->size > 0;
}
