#ifndef MYLFS_STATE_H
#define MYLFS_STATE_H

#include <stdbool.h>
#include <stddef.h>

#include "config.h"
#include "recipe.h"
#include "strlist.h"

// What has been built, kept in <buildPath>/recipes/build_state.db. Same data as the
// python edition's sqlite table, but a plain text file (one entry per line):
//   name|version|release|phase|built|builddeps(,)|rundeps(,)
// This lets phase 5 skip what is already built, rebuild phase 4 packages that have
// builddeps, and resume after a crash.
typedef struct {
    char *name;
    char *version;
    int   release;
    int   phase;      // 1-5, like the template.yml
    bool  built;
    char *builddeps;  // comma separated, "" if none
    char *rundeps;
} StateEntry;

typedef struct {
    StateEntry *data;
    size_t size;
    size_t cap;
    char path[4096];
} BuildState;

bool stateLoad(Config cfg, BuildState *st);
bool stateSave(const BuildState *st);
void stateFree(BuildState *st);

bool stateWasBuilt(const BuildState *st, const char *name);
// Python: new_release(). True when the recipe's release is higher than what we built.
bool stateNewRelease(const BuildState *st, const char *name, int release);
// Insert or update (keyed on name + phase) and save.
void stateRecord(BuildState *st, const Recipe *r, bool built);
// Marks every entry called `name` as not built
void stateMarkDirty(BuildState *st, const char *name);
// Dependencies of r that haven't built. Caller strlistFree()s `missing`.
bool stateMissingDeps(const BuildState *st, const Recipe *r, StrList *missing);

#endif // MYLFS_STATE_H
