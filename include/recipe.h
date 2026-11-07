#ifndef BE242EA5_33D7_4833_8E93_AE49272194BC
#define BE242EA5_33D7_4833_8E93_AE49272194BC

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

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
    char  *license;     // YAML: license
    char  *summary;     // YAML: summary
    char  *homepage;    // YAML: homepage
    char  *description; // YAML: description (may be multi-line)

    /* Optional typed fields */
    int     phase;      // YAML: phase (1-5 in the file). Stored as a BuildPhase
                        // (0-4) or -1 when the recipe doesn't set one.
    int32_t order;      // YAML: order (bootstrap only)
    bool    has_order;
    bool    critical;   // YAML: critical. A failed critical recipe stops the build.

    /* Optional lists */
    StrList urls;       // YAML: url (a single string or a list). The first one is
                        // the main tarball that gets extracted, the others are
                        // only downloaded next to it.
    StrList builddeps;  // YAML: builddeps
    StrList rundeps;    // YAML: rundeps

    /* Optional flags */
    bool cleanup;       // YAML: cleanup (default true)

    // These are not set in the template.yml but meta data about each template.yml
    char *path;           // path to template.yml
    char *root;           // recipe root directory (folder containing template.yml)
    char *tarballName;    // basename of the first url. NULL if there is no url
    char *tarballPath;    // root + tarballName. NULL if there is no url
    char *recipeSource;   // root/source if there is a url, otherwise root/static
} Recipe;

typedef struct {
    Recipe *data;
    size_t  size;
    size_t  cap;
} RecipeList;

/* ---- helpers ---- */
void recipeInit(Recipe *r);
void recipeFree(Recipe *r);

void recipelistInit(RecipeList *v);
void recipelistFree(RecipeList *v);
void recipelistPush(RecipeList *v, Recipe *value);   // takes ownership of *value

// Finds every template.yml under cfg.recipesPath
StrList scanRecpies(Config cfg);

// Parses one template.yml. Returns false (and prints why) on failure.
bool loadRecipe(Config cfg, const char *templatePath, Recipe *out);

// Scans and loads every recipe under cfg.recipesPath.
bool loadAllRecipes(Config cfg, RecipeList *out);

// Downloads the tarballs of every recipe with phase <= maxPhase that isn't already
// next to its template.yml. Returns false if any download failed.
bool downloadTarballs(Config cfg, const RecipeList *rl, int maxPhase);

// Fills `out` with copies of every recipe in `phase`, sorted by `order`. The
// copies share their strings with `all`, so only free `all`. Free `out`->data.
void sortRecipes(const RecipeList *all, int phase, RecipeList *out);

#endif /* BE242EA5_33D7_4833_8E93_AE49272194BC */
