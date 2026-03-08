#ifndef BE242EA5_33D7_4833_8E93_AE49272194BC
#define BE242EA5_33D7_4833_8E93_AE49272194BC

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
    char *tarballName;    // Later we will extarct the tarball name into here.
    char *tarballPath;    // This should be path + tarballName
    char *recipeSource;   // Location to extract tarball too.
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
void recipelistPush(RecipeList *v, Recipe *value);

static int findTemplates(const char *path, const struct stat *sb, int typeflag, struct FTW *ftwbuf);

StrList scanRecpies(Config cfg);
StrList buildOrderBootStrap(Config cfg, StrList RL);
StrList buildOrderP5(Config cfg, StrList RL);
void downloadTarballs(const char *url);
Recipe loadRecipe(Config cfg, const char *templatPath);

#endif /* BE242EA5_33D7_4833_8E93_AE49272194BC */
