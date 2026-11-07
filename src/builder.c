#define _GNU_SOURCE

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include "buildLog.h"
#include "builder.h"
#include "config.h"
#include "dprint.h"
#include "fs.h"
#include "graph.h"
#include "proc.h"
#include "state.h"
#include "recipe.h"
#include "strlist.h"
#include "user.h"

#define LOG_DIR "logs"

// Prepended to every recipe: no hashing, sane permissions, stop on error.
static const char *PREBUILD = "set +h\numask 022\nset -e\n";

// Phase 1 and 2 are built outside of the chroot as the lfs user.
static bool isHostPhase(BuildPhase phase) {
    return phase == CROSS_TOOLS || phase == TEMP_TOOLS;
}

/* ---------- phase.yaml ---------- */
// This file only has a single int, and doesn't need to use yaml at all.
// However, to maintain compatiblity i'm leaving the .yaml extension
short getPhase(Config cfg) {
    char path[PATH_MAX];
    snprintf(path, sizeof(path), "%s/recipes/phase.yaml", cfg.buildPath);

    FILE *fp = fopen(path, "r");
    if (!fp) {
        return 0;   // fresh build
    }

    short phase = 0;
    if (fscanf(fp, "%hd", &phase) != 1) phase = 0;
    fclose(fp);
    return phase;
}

bool setPhase(Config cfg, short phase) {
    char path[PATH_MAX];
    snprintf(path, sizeof(path), "%s/recipes/phase.yaml", cfg.buildPath);

    FILE *fp = fopen(path, "w");
    if (!fp) {
        perror("fopen");
        return false;
    }
    fprintf(fp, "%hd\n", phase);
    fclose(fp);
    return true;
}

/* ---------- helpers ---------- */
static bool isBlank(const char *s) {
    if (!s) return true;
    while (*s) {
        if (*s != ' ' && *s != '\t' && *s != '\n') return false;
        s++;
    }
    return true;
}

// Adds KEY=value to an environment list when value isn't empty
static void envAddIf(StrList *env, const char *key, const char *value) {
    if (!isBlank(value)) strlistPushf(env, "%s=%s", key, value);
}

// Compilers/make flags + RUN_TESTS shared by every phase.
static void envAddCommon(Config cfg, StrList *env, BuildPhase phase) {
    strlistPushf(env, "RUN_TESTS=%d", cfg.test ? 1 : 0);
    // config.yml: make flags are not passed during the first two phases
    if (!isHostPhase(phase)) envAddIf(env, "MAKEFLAGS", cfg.makeFlags);
    envAddIf(env, "CFLAGS", cfg.CFLAGS);
    envAddIf(env, "CXXFLAGS", cfg.CXXFLAGS);
    envAddIf(env, "RUSTFLAGS", cfg.RUSTFLAGS);
}

static char *makeLogPath(BuildPhase phase, int count, const Recipe *r) {
    char *p = NULL;
    if (asprintf(&p, LOG_DIR "/p%d_%d_%s_%s.log", (int)phase + 1, count, r->name, r->version) < 0) {
        return NULL;
    }
    return p;
}

// Absolute path of the recipe's source dir as seen from inside the chroot
static char *chrootPath(Config cfg, const Recipe *r) {
    size_t n = strlen(cfg.buildPath);
    const char *rel = r->recipeSource;
    if (strncmp(rel, cfg.buildPath, n) == 0) rel += n;
    char *p = NULL;
    if (asprintf(&p, "%s%s", *rel == '/' ? "" : "/", rel) < 0) return NULL;
    return p;
}

/* ---------- extracting ---------- */
static bool endsWith(const char *s, const char *suffix) {
    size_t a = strlen(s), b = strlen(suffix);
    return a >= b && strcmp(s + a - b, suffix) == 0;
}

static bool isTarball(const char *name) {
    static const char *suffixes[] = {
        ".tar.gz", ".tgz", ".tar.bz2", ".tar.xz", ".tar.zst", ".tar", NULL
    };
    for (int i = 0; suffixes[i]; i++) {
        if (endsWith(name, suffixes[i])) return true;
    }
    return false;
}

// Makes sure recipeSource exists. In phase 1 and 2 it has to be writable by lfs
static bool ensureSourceDir(const Recipe *r) {
    return makeDirs(r->recipeSource, 0777);
}

// Extract the main tarball into recipeSource. Phase 1 and 2 extract as lfs
// (so it can write in there) and later phases as root, like the python edition.
static bool extractTarball(Config cfg, const Recipe *r, BuildPhase phase) {
    if (cfg.debug) {
        printf("    [debug] tarball name: [%s] Tarball path: [%s]\n", r->tarballName, r->tarballPath);
    }

    if (access(r->tarballPath, R_OK) != 0) {
        failed("Tarball not found: %s\n", r->tarballPath);
        return false;
    }

    // Start from a clean dir
    if (!removeTree(r->recipeSource) || !ensureSourceDir(r)) {
        failed("Could not prepare %s\n", r->recipeSource);
        return false;
    }

    RunSpec rs = {.user = isHostPhase(phase) ? "lfs" : NULL};
    char *tarArgv[] = {"tar", "-xf", r->tarballPath, "-C", r->recipeSource,
                       "--strip-components=1", NULL};
    char *zipArgv[] = {"unzip", "-q", r->tarballPath, "-d", r->recipeSource, NULL};

    if (endsWith(r->tarballName, ".zip")) {
        rs.argv = zipArgv;
    } else if (isTarball(r->tarballName)) {
        rs.argv = tarArgv;
    } else {
        failed("Unknown tarball format: %s\n", r->tarballName);
        return false;
    }

    if (runProcess(&rs) != 0) {
        failed("Could not extract tarball: [%s]\n", r->tarballPath);
        return false;
    }
    return true;
}

// Delete the extracted source after a good build
static void cleanupSource(Config cfg, const Recipe *r) {
    if (!r->cleanup || !cfg.cleanup_src || r->urls.size == 0) {
        if (cfg.debug) printf("    [debug] keeping the source of %s\n", r->name);
        return;
    }
    if (cfg.debug) printf("    [debug] cleanup: %s\n", r->recipeSource);
    removeTree(r->recipeSource);
}

/* ---------- building one recipe ---------- */
static bool buildRecipe(Config cfg, BuildPhase phase, const Recipe *r, int count) {
    mkdir(LOG_DIR, 0755);
    char *logPath = makeLogPath(phase, count, r);
    if (!logPath) return false;

    if (r->tarballPath) {
        if (!extractTarball(cfg, r, phase)) {
            free(logPath);
            return false;
        }
    } else if (!ensureSourceDir(r)) {
        free(logPath);
        return false;
    }

    // Anything extracted/created by root has to be writable for lfs
    if (isHostPhase(phase)) {
        uid_t uid;
        gid_t gid;
        if (lookupUser("lfs", &uid, &gid)) chownTree(r->recipeSource, uid, gid);
    }

    StrList env;
    strlistInit(&env);
    char *script = NULL;
    RunSpec rs = {.logPath = logPath};

    if (isHostPhase(phase)) {
        // Section 4.4 of the book: a clean environment where the cross tools come first
        strlistPushf(&env, "HOME=/home/lfs");
        strlistPushf(&env, "TERM=xterm");
        strlistPushf(&env, "LFS=%s", cfg.buildPath);
        strlistPushf(&env, "LC_ALL=POSIX");
        strlistPushf(&env, "LFS_TGT=%s", cfg.lfs_tgt);
        strlistPushf(&env, "PATH=%s/tools/bin:/usr/bin:/bin:/sbin:/usr/sbin", cfg.buildPath);
        strlistPushf(&env, "CONFIG_SITE=%s/usr/share/config.site", cfg.buildPath);
        envAddCommon(cfg, &env, phase);
        strlistPush(&env, NULL);

        if (asprintf(&script, "%s%s", PREBUILD, r->buildsteps) < 0) script = NULL;

        char *argv[] = {"bash", NULL};
        rs.path = hostBash();
        rs.argv = argv;
        rs.envp = env.data;
        rs.input = script;
        rs.cwd = r->recipeSource;
        rs.user = "lfs";

        int rc = script ? runProcess(&rs) : -1;
        free(script);
        strlistFree(&env);
        free(logPath);
        return rc == 0;
    }

    // Inside the chroot. LFS is deliberately not set: the recipes use $LFS for
    // the host side path and inside the chroot the root is just /
    char *cwd = chrootPath(cfg, r);
    strlistPushf(&env, "HOME=/root");
    strlistPushf(&env, "TERM=xterm");
    strlistPushf(&env, "PS1=\\u:\\w\\$ ");
    strlistPushf(&env, "PATH=/usr/bin:/usr/sbin:/bin:/sbin");
    envAddCommon(cfg, &env, phase);
    strlistPush(&env, NULL);

    if (!cwd || asprintf(&script, "%scd '%s'\n%s", PREBUILD, cwd, r->buildsteps) < 0) script = NULL;

    char *argv[] = {"bash", "--login", NULL};
    rs.path = "/usr/bin/bash";
    rs.argv = argv;
    rs.envp = env.data;
    rs.input = script;
    rs.chroot = cfg.buildPath;

    int rc = script ? runProcess(&rs) : -1;
    free(script);
    free(cwd);
    strlistFree(&env);
    free(logPath);
    return rc == 0;
}

/* ---------- phase 1 - 4 ---------- */
static bool builderBootstrap(Config cfg, BuildPhase phase, const RecipeList *all, const StrList *startPkgs) {
    RecipeList todo;
    sortRecipes(all, (int)phase, &todo);

    if (todo.size == 0) {
        warn("No recipes found for phase %d\n", (int)phase + 1);
        free(todo.data);
        return true;
    }

    // Resume at --start-package
    const char *startPkg = (startPkgs && startPkgs->size) ? startPkgs->data[0] : NULL;
    size_t first = 0;
    if (startPkg) {
        bool found = false;
        for (size_t i = 0; i < todo.size; i++) {
            if (strcmp(todo.data[i].name, startPkg) == 0) {
                first = i;
                found = true;
                break;
            }
        }
        if (!found) {
            warn("Start package '%s' not found in phase %d, building all.\n", startPkg, (int)phase + 1);
        }
    }

    BuildState state;
    stateLoad(cfg, &state);

    bool ok = true;
    int count = 0;

    for (size_t i = 0; i < todo.size; i++) {
        const Recipe *r = &todo.data[i];
        count++;

        if (i < first) {
            skip("%-30s resuming at %s\n", r->name, startPkg);
            continue;
        }

        bool good = buildRecipe(cfg, phase, r, count);

        BuildRecord rec = {
            .timestamp = time(NULL),
            .name = r->name,
            .version = r->version,
            .phase = phase,
            .built = good,
        };
        buildLogAppend(cfg, &rec);

        if (good) {
            cleanupSource(cfg, r);
            built(count, "%s\n", r->name);

            // A phase 4 recipe without builddeps is final. The ones with builddeps are
            // rebuilt in phase 5 against whatever got installed since.
            if (phase == LFS_BASE && r->builddeps.size == 0) {
                stateRecord(&state, r, true);
            }
        } else {
            failed("could not build package %s  (see %s/p%d_%d_%s_%s.log)\n", r->name, LOG_DIR,
                   (int)phase + 1, count, r->name, r->version);
            // Keep going on a non critical package, but the base can't be built without
            // the critical ones.
            if (r->critical || isHostPhase(phase)) {
                ok = false;
                break;
            }
        }
    }

    stateFree(&state);
    free(todo.data);   // shallow copies, the strings belong to `all`
    return ok;
}

/* ---------- phase 5 ---------- */
static bool builderExtra(Config cfg, const RecipeList *all, const StrList *startPkgs) {
    // every phase 4 and 5 recipe
    RecipeList cand;
    recipelistInit(&cand);
    for (size_t i = 0; i < all->size; i++) {
        if (all->data[i].phase == LFS_BASE || all->data[i].phase == EXTRA_PACKAGES) {
            recipelistPush(&cand, &all->data[i]);
        }
    }

    RecipeList order;
    if (!resolveBuildOrder(&cand, &order)) {
        free(cand.data);
        return false;
    }

    // --start-package: those and their direct builddeps/rundeps
    if (startPkgs && startPkgs->size) {
        StrList needed;
        strlistInit(&needed);
        bool any = false;

        for (size_t i = 0; i < order.size; i++) {
            const Recipe *r = &order.data[i];
            if (!strlistContains(startPkgs, r->name)) continue;
            any = true;
            if (!strlistContains(&needed, r->name)) strlistPush(&needed, strdup(r->name));
            for (size_t d = 0; d < r->builddeps.size; d++)
                if (!strlistContains(&needed, r->builddeps.data[d])) strlistPush(&needed, strdup(r->builddeps.data[d]));
            for (size_t d = 0; d < r->rundeps.size; d++)
                if (!strlistContains(&needed, r->rundeps.data[d])) strlistPush(&needed, strdup(r->rundeps.data[d]));
        }

        if (!any) {
            warn("Start package(s) not found, building all.\n");
        } else {
            RecipeList filtered;
            recipelistInit(&filtered);
            for (size_t i = 0; i < cand.size; i++) {
                if (strlistContains(&needed, cand.data[i].name)) recipelistPush(&filtered, &cand.data[i]);
            }
            free(order.data);
            free(cand.data);
            cand = filtered;
            if (!resolveBuildOrder(&cand, &order)) {
                strlistFree(&needed);
                free(cand.data);
                return false;
            }
        }
        strlistFree(&needed);
    }

    BuildState state;
    stateLoad(cfg, &state);

    // Handy to look at when a build goes wrong
    mkdir(LOG_DIR, 0755);
    FILE *bo = fopen(LOG_DIR "/p5_00_build_order.log", "a");
    if (bo) {
        for (size_t i = 0; i < order.size; i++) fprintf(bo, "%s\n", order.data[i].name);
        fclose(bo);
    }

    int count = 0;

    for (size_t i = 0; i < order.size; i++) {
        const Recipe *r = &order.data[i];
        int relNum = r->release ? atoi(r->release) : 1;

        if (stateWasBuilt(&state, r->name)) {
            if (stateNewRelease(&state, r->name, relNum)) {
                // Bumped release: rebuild it and everything that was built against it.
                StrList deps = getAllDependents(&cand, r->name);
                for (size_t d = 0; d < deps.size; d++) stateMarkDirty(&state, deps.data[d]);
                strlistFree(&deps);
                stateMarkDirty(&state, r->name);
            } else {
                skip("%-33s already built\n", r->name);
                continue;
            }
        }

        StrList missing;
        if (stateMissingDeps(&state, r, &missing)) {
            char *list = strdup("");
            for (size_t m = 0; m < missing.size; m++) {
                char *n = NULL;
                if (asprintf(&n, "%s%s%s", list, m ? ", " : "", missing.data[m]) >= 0) {
                    free(list);
                    list = n;
                }
            }
            skip("%-33s missing/failed deps -> %s\n", r->name, list);
            free(list);
            strlistFree(&missing);
            continue;
        }
        strlistFree(&missing);

        stateRecord(&state, r, false);
        bool good = buildRecipe(cfg, EXTRA_PACKAGES, r, count);

        BuildRecord rec = {
            .timestamp = time(NULL), .name = r->name, .version = r->version,
            .phase = EXTRA_PACKAGES, .built = good,
        };
        buildLogAppend(cfg, &rec);

        if (good) {
            cleanupSource(cfg, r);
            built(count, "%s\n", r->name);
        } else {
            failed("could not build package %s  (see %s/p5_%d_%s_%s.log)\n", r->name, LOG_DIR,
                   count, r->name, r->version);
        }
        stateRecord(&state, r, good);
        count++;   // like the python edition, only packages we tried are counted
    }

    stateFree(&state);
    free(order.data);
    free(cand.data);
    return true;   // a failed package only makes the ones depending on it skip
}

bool builderPhase(Config cfg, BuildPhase phase, const RecipeList *all, const StrList *startPkgs) {
    if (phase == EXTRA_PACKAGES) {
        return builderExtra(cfg, all, startPkgs);
    }
    return builderBootstrap(cfg, phase, all, startPkgs);
}
