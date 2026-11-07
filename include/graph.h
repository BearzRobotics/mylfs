#ifndef MYLFS_GRAPH_H
#define MYLFS_GRAPH_H

#include <stdbool.h>

#include "recipe.h"
#include "strlist.h"

// Orders `in` so every recipe comes after its builddeps (a builddep that has no
// recipe in `in` is ignored here, it fails later as a missing dependency). Ties are
// broken by `order` then name so the build order is the same every run.
// `out` gets shallow copies, free out->data only. Returns false on a cycle, after
// printing it.
bool resolveBuildOrder(const RecipeList *in, RecipeList *out);

// Every recipe name that directly or transitively builddeps on `name`.
// Caller strlistFree()s the result.
StrList getAllDependents(const RecipeList *recipes, const char *name);

#endif // MYLFS_GRAPH_H
