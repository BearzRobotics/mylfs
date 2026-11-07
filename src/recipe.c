#define _GNU_SOURCE
#define _XOPEN_SOURCE 700

#include <ctype.h>
#include <ftw.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <yaml.h>

#include "recipe.h"
#include "config.h"
#include "strlist.h"
#include "dprint.h"
#include "fs.h"

#define DEFAULT_ORDER 999

/* ---------- Recipe ---------- */
void recipeInit(Recipe *r) {
    *r = (Recipe){0};
    r->phase = -1;        // not set
    r->cleanup = true;
    strlistInit(&r->urls);
    strlistInit(&r->builddeps);
    strlistInit(&r->rundeps);
}

void recipeFree(Recipe *r) {
    free(r->name);
    free(r->version);
    free(r->buildsteps);
    free(r->release);
    free(r->license);
    free(r->summary);
    free(r->homepage);
    free(r->description);
    free(r->path);
    free(r->root);
    free(r->tarballName);
    free(r->tarballPath);
    free(r->recipeSource);

    strlistFree(&r->urls);
    strlistFree(&r->builddeps);
    strlistFree(&r->rundeps);

    *r = (Recipe){0};
}

void recipelistInit(RecipeList *v) {
    v->size = 0;
    v->cap = 8;
    v->data = malloc(v->cap * sizeof(Recipe));
    if (!v->data) {
        perror("malloc");
        exit(1);
    }
}

void recipelistFree(RecipeList *v) {
    for (size_t i = 0; i < v->size; i++) {
        recipeFree(&v->data[i]);
    }
    free(v->data);
    v->data = NULL;
    v->size = v->cap = 0;
}

void recipelistPush(RecipeList *v, Recipe *value) {
    if (v->size == v->cap) {
        v->cap *= 2;
        v->data = realloc(v->data, v->cap * sizeof(Recipe));
        if (!v->data) {
            perror("realloc");
            exit(1);
        }
    }
    v->data[v->size++] = *value;
}

/* ---------- template.yml parsing ---------- */
static bool parseBool(const char *v) {
    return strcmp(v, "true") == 0 || strcmp(v, "yes") == 0 || strcmp(v, "1") == 0;
}

static void setString(char **dst, const char *val) {
    free(*dst);
    *dst = strdup(val);
}

// "phase: 4", "phase: phase4" both mean index 3. Returns -1 if unusable.
static int parsePhase(const char *v) {
    while (*v && !isdigit((unsigned char)*v)) v++;
    if (!*v) return -1;
    int n = atoi(v);
    if (n < 1 || n > 5) return -1;
    return n - 1;
}

static void setScalar(Recipe *r, const char *key, const char *val) {
    if      (strcmp(key, "name") == 0)        setString(&r->name, val);
    else if (strcmp(key, "version") == 0)     setString(&r->version, val);
    else if (strcmp(key, "buildsteps") == 0)  setString(&r->buildsteps, val);
    else if (strcmp(key, "release") == 0)     setString(&r->release, val);
    else if (strcmp(key, "license") == 0)     setString(&r->license, val);
    else if (strcmp(key, "summary") == 0)     setString(&r->summary, val);
    else if (strcmp(key, "homepage") == 0)    setString(&r->homepage, val);
    else if (strcmp(key, "description") == 0) setString(&r->description, val);
    else if (strcmp(key, "phase") == 0)       r->phase = parsePhase(val);
    else if (strcmp(key, "order") == 0) {
        if (*val) {
            r->order = (int32_t)atoi(val);
            r->has_order = true;
        }
    }
    else if (strcmp(key, "critical") == 0)    r->critical = parseBool(val);
    else if (strcmp(key, "cleanup") == 0)     r->cleanup = parseBool(val);
    // A lone string where a list is expected, ie. url: https://...
    else if (strcmp(key, "url") == 0)         { if (*val) strlistPush(&r->urls, strdup(val)); }
    else if (strcmp(key, "builddeps") == 0)   { if (*val) strlistPush(&r->builddeps, strdup(val)); }
    else if (strcmp(key, "rundeps") == 0)     { if (*val) strlistPush(&r->rundeps, strdup(val)); }
}

static void addListItem(Recipe *r, const char *key, const char *val) {
    if      (strcmp(key, "url") == 0)       strlistPush(&r->urls, strdup(val));
    else if (strcmp(key, "builddeps") == 0) strlistPush(&r->builddeps, strdup(val));
    else if (strcmp(key, "rundeps") == 0)   strlistPush(&r->rundeps, strdup(val));
}

// Replaces every {name} and {version} in the build steps. This is how the
// recipes avoid repeating their version number.
static char *expandBuildSteps(const char *text, const Recipe *r) {
    size_t cap = strlen(text) + 1;
    char *out = malloc(cap);
    size_t len = 0;
    if (!out) return NULL;

    while (*text) {
        const char *rep = NULL;
        size_t skipLen = 0;

        if (strncmp(text, "{name}", 6) == 0)         { rep = r->name;    skipLen = 6; }
        else if (strncmp(text, "{version}", 9) == 0) { rep = r->version; skipLen = 9; }

        if (rep) {
            size_t rl = strlen(rep);
            cap += rl;
            out = realloc(out, cap);
            if (!out) return NULL;
            memcpy(out + len, rep, rl);
            len += rl;
            text += skipLen;
        } else {
            out[len++] = *text++;
        }
    }
    out[len] = '\0';
    return out;
}

// url -> file name. Drops any ?query and #fragment
static char *tarballBasename(const char *url) {
    char *copy = strdup(url);
    if (!copy) return NULL;
    char *cut = strpbrk(copy, "?#");
    if (cut) *cut = '\0';

    char *slash = strrchr(copy, '/');
    char *name = strdup(slash ? slash + 1 : copy);
    free(copy);
    return name;
}

bool loadRecipe(Config cfg, const char *templatePath, Recipe *out) {
    FILE *fh = fopen(templatePath, "r");
    if (!fh) {
        failed("Cannot open %s: ", templatePath);
        perror("");
        return false;
    }

    yaml_parser_t parser;
    yaml_event_t event;

    if (!yaml_parser_initialize(&parser)) {
        failed("Failed to initialize YAML parser\n");
        fclose(fh);
        return false;
    }
    yaml_parser_set_input_file(&parser, fh);

    Recipe r;
    recipeInit(&r);

    char *key = NULL;
    int depth = 0;       // mapping/sequence nesting
    bool inSeq = false;
    bool ok = true;
    bool done = false;

    while (!done) {
        if (!yaml_parser_parse(&parser, &event)) {
            failed("%s: YAML error: %s (line %zu)\n", templatePath,
                   parser.problem ? parser.problem : "unknown", parser.problem_mark.line + 1);
            ok = false;
            break;
        }

        switch (event.type) {
        case YAML_MAPPING_START_EVENT:
            depth++;
            break;
        case YAML_MAPPING_END_EVENT:
            depth--;
            break;
        case YAML_SEQUENCE_START_EVENT:
            depth++;
            inSeq = true;
            break;
        case YAML_SEQUENCE_END_EVENT:
            depth--;
            inSeq = false;
            free(key);
            key = NULL;
            break;
        case YAML_SCALAR_EVENT: {
            const char *val = (const char *)event.data.scalar.value;
            if (depth == 1 && !inSeq) {
                if (!key) {
                    key = strdup(val);          // this scalar is a key
                } else {
                    setScalar(&r, key, val);    // this scalar is its value
                    free(key);
                    key = NULL;
                }
            } else if (depth == 2 && inSeq && key) {
                addListItem(&r, key, val);
            }
            break;
        }
        case YAML_STREAM_END_EVENT:
            done = true;
            break;
        default:
            break;
        }

        yaml_event_delete(&event);
    }

    free(key);
    yaml_parser_delete(&parser);
    fclose(fh);

    if (ok && (!r.name || !*r.name || !r.version || !*r.version)) {
        failed("%s: name and version are required\n", templatePath);
        ok = false;
    }
    if (ok && (!r.buildsteps || !*r.buildsteps)) {
        failed("%s: buildsteps are required (recipe %s)\n", templatePath, r.name);
        ok = false;
    }
    if (!ok) {
        recipeFree(&r);
        return false;
    }

    // buildsteps {name} {version}
    char *expanded = expandBuildSteps(r.buildsteps, &r);
    if (expanded) {
        free(r.buildsteps);
        r.buildsteps = expanded;
    }

    // Where things are
    char *dirc = strdup(templatePath);
    char *slash = strrchr(dirc, '/');
    if (slash) {
        *slash = '\0';
    } else {
        strcpy(dirc, ".");
    }
    r.path = strdup(templatePath);
    r.root = dirc;

    // The main tarball is the first url and extracts into source/. Recipes
    // without a url only have static/ files that the packager provides.
    if (r.urls.size > 0) {
        r.tarballName = tarballBasename(r.urls.data[0]);
        if (asprintf(&r.tarballPath, "%s/%s", r.root, r.tarballName) < 0) r.tarballPath = NULL;
        if (asprintf(&r.recipeSource, "%s/source", r.root) < 0) r.recipeSource = NULL;
    } else {
        if (asprintf(&r.recipeSource, "%s/static", r.root) < 0) r.recipeSource = NULL;
    }

    if (cfg.debug) {
        printf("[debug] loadRecipe(): %s %s phase=%d order=%d tarball=%s\n", r.name, r.version,
               r.phase + 1, r.has_order ? (int)r.order : DEFAULT_ORDER,
               r.tarballPath ? r.tarballPath : "(none)");
    }

    *out = r;
    return true;
}

/* ---------- finding recipes ---------- */
static StrList rList;  // nftw's callback has a fixed signature so this is shared
static bool rDebug;

static int findTemplates(const char *path, const struct stat *sb, int typeflag, struct FTW *ftwbuf) {
    (void)sb;
    if (typeflag == FTW_F && strcmp(path + ftwbuf->base, "template.yml") == 0) {
        if (rDebug) {
            fprintf(stderr, "recipe.c - findTemplates(): [%s]\n", path);
        }
        strlistPush(&rList, strdup(path));
    }
    return 0;  // return non-zero to stop traversal early
}

// Finds all template.yml files in the recipes dir. Caller frees with strlistFree.
StrList scanRecpies(Config cfg) {
    rDebug = cfg.debug;
    strlistInit(&rList);
    if (nftw(cfg.recipesPath, findTemplates, 16, FTW_PHYS) != 0) {
        failed("Could not scan %s for recipes\n", cfg.recipesPath);
    }
    return rList;
}

bool loadAllRecipes(Config cfg, RecipeList *out) {
    StrList paths = scanRecpies(cfg);
    recipelistInit(out);
    bool ok = true;

    for (size_t i = 0; i < paths.size; i++) {
        Recipe r;
        if (loadRecipe(cfg, paths.data[i], &r)) {
            recipelistPush(out, &r);
        } else {
            ok = false;   // already reported. Keep loading the others.
        }
    }
    strlistFree(&paths);
    return ok;
}

bool downloadTarballs(Config cfg, const RecipeList *rl, int maxPhase) {
    bool ok = true;

    for (size_t i = 0; i < rl->size; i++) {
        const Recipe *r = &rl->data[i];
        if (r->phase < 0 || r->phase > maxPhase) continue;

        for (size_t u = 0; u < r->urls.size; u++) {
            char *name = tarballBasename(r->urls.data[u]);
            char *dest = NULL;
            if (!name || asprintf(&dest, "%s/%s", r->root, name) < 0) {
                free(name);
                ok = false;
                continue;
            }

            if (access(dest, F_OK) == 0) {
                if (cfg.debug) printf("[debug] %s already exists - skipping\n", name);
            } else {
                info("Downloading %s\n", name);
                if (!download(cfg, r->urls.data[u], dest)) ok = false;
            }
            free(name);
            free(dest);
        }
    }
    return ok;
}

/* ---------- ordering ---------- */
static int cmpRecipe(const void *a, const void *b) {
    const Recipe *x = a, *y = b;
    int ox = x->has_order ? x->order : DEFAULT_ORDER;
    int oy = y->has_order ? y->order : DEFAULT_ORDER;
    if (ox != oy) return ox < oy ? -1 : 1;
    return strcmp(x->name, y->name);   // keeps the order the same between runs
}

void sortRecipes(const RecipeList *all, int phase, RecipeList *out) {
    recipelistInit(out);
    for (size_t i = 0; i < all->size; i++) {
        if (all->data[i].phase == phase) {
            recipelistPush(out, &all->data[i]);
        }
    }
    qsort(out->data, out->size, sizeof(Recipe), cmpRecipe);
}
