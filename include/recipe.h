#ifndef BE242EA5_33D7_4833_8E93_AE49272194BC
#define BE242EA5_33D7_4833_8E93_AE49272194BC

#define _XOPEN_SOURCE 500
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <ftw.h>

#include "phase.h"
#include "config.h"
#include "strlist.h"

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

void recipeInit(Recipe *r);
void recipeFree(Recipe *r);

void recipe_array_init(RecipeList *a);
void recipe_array_free(RecipeList *a);
Recipe *recipe_array_push(RecipeList *a); // returns pointer to new blank Recipe slot
static int findTemplates(const char *path, const struct stat *sb, int typeflag, struct FTW *ftwbuf);

StrList scanRecpies(Config cfg);
StrList buildOrderBootStrap(Config cfg, StrList RL);
StrList buildOrderP5(Config cfg, StrList RL);
void downloadTarballs(const char url);

#endif /* BE242EA5_33D7_4833_8E93_AE49272194BC */
