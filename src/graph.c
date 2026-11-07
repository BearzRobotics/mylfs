#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dprint.h"
#include "graph.h"

static int findByName(const RecipeList *rl, const char *name) {
    for (size_t i = 0; i < rl->size; i++) {
        if (strcmp(rl->data[i].name, name) == 0) return (int)i;
    }
    return -1;
}

static int orderOf(const Recipe *r) {
    return r->has_order ? (int)r->order : 999;
}

// is a before b in the tie break?
static bool before(const Recipe *a, const Recipe *b) {
    if (orderOf(a) != orderOf(b)) return orderOf(a) < orderOf(b);
    return strcmp(a->name, b->name) < 0;
}

bool resolveBuildOrder(const RecipeList *in, RecipeList *out) {
    size_t n = in->size;
    bool *done = calloc(n ? n : 1, sizeof(bool));
    if (!done) { perror("calloc"); exit(1); }

    recipelistInit(out);

    for (size_t placed = 0; placed < n; placed++) {
        int best = -1;

        for (size_t i = 0; i < n; i++) {
            if (done[i]) continue;

            bool ready = true;
            for (size_t d = 0; d < in->data[i].builddeps.size; d++) {
                int di = findByName(in, in->data[i].builddeps.data[d]);
                if (di >= 0 && !done[di] && (size_t)di != i) { ready = false; break; }
                if (di >= 0 && (size_t)di == i) { ready = false; break; } // depends on itself
            }
            if (ready && (best < 0 || before(&in->data[i], &in->data[best]))) {
                best = (int)i;
            }
        }

        if (best < 0) {
            failed("Cycle detected in build dependencies, these depend on each other:\n");
            for (size_t i = 0; i < n; i++) {
                if (!done[i]) printf("     - %s\n", in->data[i].name);
            }
            free(done);
            free(out->data);
            out->data = NULL;
            out->size = out->cap = 0;
            return false;
        }

        done[best] = true;
        recipelistPush(out, &in->data[best]);
    }

    free(done);
    return true;
}

StrList getAllDependents(const RecipeList *recipes, const char *name) {
    StrList found, queue;
    strlistInit(&found);
    strlistInit(&queue);
    strlistPush(&queue, strdup(name));

    for (size_t q = 0; q < queue.size; q++) {
        const char *cur = queue.data[q];
        for (size_t i = 0; i < recipes->size; i++) {
            const Recipe *r = &recipes->data[i];
            if (!strlistContains(&r->builddeps, cur)) continue;
            if (strlistContains(&found, r->name)) continue;
            strlistPush(&found, strdup(r->name));
            strlistPush(&queue, strdup(r->name));
        }
    }
    strlistFree(&queue);
    return found;
}
