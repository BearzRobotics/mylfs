#ifndef BE242EA5_33D7_4833_8E93_AE49272194BC
#define BE242EA5_33D7_4833_8E93_AE49272194BC

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "phase.h"
#include "config.h"

typedef struct {
    char   **items;
    size_t   len;
    size_t   cap;
} StrList;


typedef struct {
    /* Required */
    char  *name;        // YAML: name
    char  *version;     // YAML: version
    char  *buildsteps;  // YAML: buildsteps (treat as a single string/script)

    /* Optional scalars */
    char  *release;     // YAML: release (string is safest)
    char  *url;         // YAML: url
    char  *license;     // YAML: license
    char  *summary;     // YAML: summary
    char  *homepage;    // YAML: homepage
    char  *description; // YAML: description (may be multi-line)

    /* Optional typed fields */
    BuildPhase   phase;      // YAML: phase ("phase1"...)
    int32_t order;      // YAML: order (bootstrap only); use a sentinel if unset
    bool    has_order;

    /* Optional lists */
    StrList builddeps;  // YAML: builddeps
    StrList rundeps;    // YAML: rundeps

    /* Optional flags */
    bool cleanup;       // YAML: cleanup (default false)

    // These are not set in the template.yml but meta data about each template.yml
    char *path;           // absolute/real path to template.yml
    char *root;           // optional: recipe root directory (folder containing template.yml)
} Recipe;


typedef struct {
    Recipe *items;
    size_t  len;
    size_t  cap;
} RecipeList;

/* ---- helpers ---- */
void strlist_init(StrList *l);
void strlist_free(StrList *l);
bool strlist_push(StrList *l, const char *s);

void recipe_init(Recipe *r);
void recipe_free(Recipe *r);

void recipe_array_init(RecipeList *a);
void recipe_array_free(RecipeList *a);
Recipe *recipe_array_push(RecipeList *a); // returns pointer to new blank Recipe slot


#endif /* BE242EA5_33D7_4833_8E93_AE49272194BC */
