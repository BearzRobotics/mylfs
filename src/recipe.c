#include <stdbool.h>
#include <sys/types.h> // opendir() closedir() readdir()
#include <dirent.h> // opendir() closedir() readdir()  dirent struct

#include <yaml.h>

#include "recipe.h"
#include "config.h"
#include "strlist.h"

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

// Loads a single recipes
Recipe loadRecipe(Config cfg) {

}


// Finds all recipes files in buildDir
StrList findRecpies(Config cfg) {
    struct dirent *pDirent; // Creates a copy of the dirent struct to manage our files.
    DIR *bDir;
    bDir = opendir(cfg.recipesPath);
    if (bDir == NULL) {
        perror("opendir");
        fprintf(stderr, "Failed to opendir %s", cfg.recipesPath);
        exit(-1);
    }

    // Creates our recipe struct to hold all of our template.yml
    struct StrList *rList;


    
    // We need to loop through every dir and build a list of all template.yml
    // eg. recipes/b/bash/template.yml
    while ((pDirent = readdir(bDir)) != NULL) {
        printf("[%s]\n", pDirent->d_name);

    }


    closedir(bDir);
}

StrList sortRecipes(StrList RL, short int phase) {

}

StrList buildOrderBootStrap(Config cfg, StrList RL) {

}

StrList buildOrderP5(Config cfg, StrList RL) {
    
}