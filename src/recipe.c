#include <stdbool.h>

#include <yaml.h>

#include "recipe.h"
#include "config.h"



static char *xstrdup(const char *s) {
    if (!s) return NULL;
    size_t n = strlen(s) + 1;
    char *p = (char*)malloc(n);
    if (!p) return NULL;
    memcpy(p, s, n);
    return p;
}

/* ---------- StrList ---------- */
void strlist_init(StrList *l) { *l = (StrList){0}; }

bool strlist_push(StrList *l, const char *s) {
    if (l->len == l->cap) {
        size_t ncap = l->cap ? l->cap * 2 : 8;
        char **n = (char**)realloc(l->items, ncap * sizeof(char*));
        if (!n) return false;
        l->items = n;
        l->cap = ncap;
    }
    l->items[l->len] = xstrdup(s);
    if (!l->items[l->len]) return false;
    l->len++;
    return true;
}

void strlist_free(StrList *l) {
    for (size_t i = 0; i < l->len; i++) free(l->items[i]);
    free(l->items);
    *l = (StrList){0};
}

/* ---------- Recipe ---------- */
void recipe_init(Recipe *r) {
    *r = (Recipe){0};
    r->phase = CROSS_TOOLS;   // whatever your enum uses for “unset”
    strlist_init(&r->builddeps);
    strlist_init(&r->rundeps);
}

void recipe_free(Recipe *r) {
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

    strlist_free(&r->builddeps);
    strlist_free(&r->rundeps);

    *r = (Recipe){0};
}

/* ---------- RecipeList ---------- */
void recipe_array_init(RecipeList *a) { *a = (RecipeList){0}; }

Recipe *recipe_array_push(RecipeList *a) {
    if (a->len == a->cap) {
        size_t ncap = a->cap ? a->cap * 2 : 64;
        Recipe *n = (Recipe*)realloc(a->items, ncap * sizeof(Recipe));
        if (!n) return NULL;
        a->items = n;
        a->cap = ncap;
    }
    Recipe *slot = &a->items[a->len++];
    recipe_init(slot);
    return slot;
}

void recipe_array_free(RecipeList *a) {
    for (size_t i = 0; i < a->len; i++) recipe_free(&a->items[i]);
    free(a->items);
    *a = (RecipeList){0};
}

// Loads a single recipes
Recipe loadRecipe(Config cfg) {

}


// Finds all recipes files in buildDir
RecipeList findRecpies() {

}

RecipeList sortRecipes(RecipeList RL, short int phase) {

}

RecipeList buildOrderBootStrap(RecipeList RL) {

}

RecipeList buildOrderP5(RecipeList RL) {
    
}