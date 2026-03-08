#define _XOPEN_SOURCE 500

#include <stdbool.h>
#include <ftw.h>
#include <stdio.h>
#include <unistd.h>

#include <yaml.h>

#include "recipe.h"
#include "config.h"
#include "strlist.h"
#include "dprint.h"

static char *xstrdup(const char *s) {
    if (!s) return NULL;
    size_t n = strlen(s) + 1;
    char *p = (char*)malloc(n);
    if (!p) return NULL;
    memcpy(p, s, n);
    return p;
}

/* ---------- Recipe ---------- */
void recipeInit(Recipe *r) {
    *r = (Recipe){0};
    r->phase = CROSS_TOOLS;   // whatever your enum uses for “unset”
    strlistInit(&r->builddeps);
    strlistInit(&r->rundeps);
}

void recipeFree(Recipe *r) {
    free(r->name);
    free(r->version);
    free(r->buildsteps);
    free(r->release);
    free(r->url);
    free(r->license);
    free(r->summary);
    free(r->homepage);
    free(r->description);
    free(r->path);
    free(r->root);

    strlistFree(&r->builddeps);
    strlistFree(&r->rundeps);

    *r = (Recipe){0};
}


void recipelistInit(RecipeList *v) {
    v->size = 0;
    v->cap = 8;
    v->data = malloc(v->cap * sizeof(char*));
}
void recipelistFree(RecipeList *v) {
     free(v->data);
}
void recipelistPush(RecipeList *v, Recipe *value) {
    if (v->size == v->cap) {
        v->cap *= 2;                                          // missing - you must grow cap
        v->data = realloc(v->data, v->cap * sizeof(Recipe)); // realloc only, no malloc here
        if (!v->data) {
            perror("realloc");
            exit(1);
        }
    }
    v->data[v->size++] = *value;                             // dereference * as compiler told you
}

static StrList rList;  // global or pass via workaround since nftw callback has fixed signature

// Loads a single recipes
Recipe loadRecipe(Config cfg, const char *templatPath) {
    FILE *fh = fopen(templatPath, "r");

}

RecipeList loadAllRecipes(Config cfg) {}

static Config *currentCfg; // This is to enable debug in the findTemplates function
                           // Should not be used for any other function

// nftw has a strict fixed calling parameters.
static int findTemplates(const char *path, const struct stat *sb, int typeflag, struct FTW *ftwbuf) {
    if (typeflag == FTW_F && strcmp(path + ftwbuf->base, "template.yml") == 0) {
        if (currentCfg->debug) {
            fprintf(stderr, "recipe.c - findTemplates(): [%s]\n", path);
        }
        strlistPush(&rList, strdup(path));
    }
    return 0;  // return non-zero to stop traversal early
}

// Finds all recipes files in buildDir
StrList scanRecpies(Config cfg) {
    currentCfg = &cfg;
    strlistInit(&rList);
    nftw(cfg.recipesPath, findTemplates, 16, FTW_PHYS);
    return rList;
}

void downloadTarballs(const char *url) {
    // We need to construct the wget string to download packages
    char cmd[4096]; // This might seem big, but I don't want a url failing to download
                    // because it couldn't fit in the buffer

    // buffer, we need to get the size of our buffer
    snprintf(cmd, sizeof(cmd), "wget -nc %s", url);

    if (system(cmd) != 0) {
        failed("Failed to downloaded: [%s]", url);
    }
}


StrList sortRecipes(StrList RL, short int phase) {
    return RL;
}

StrList buildOrderBootStrap(Config cfg, StrList RL) {
    return RL;
}

StrList buildOrderP5(Config cfg, StrList RL) {
    return RL;   
}