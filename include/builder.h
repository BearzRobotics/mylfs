#ifndef MYLFS_BUILDER_H
#define MYLFS_BUILDER_H

#include <stdbool.h>

#include "config.h"
#include "phase.h"
#include "recipe.h"

// Phase 1 and 2 (CROSS_TOOLS, TEMP_TOOLS). Built on the host as the lfs user.
// Phase 3 and 4 (TEMP_SYSTEM, LFS_BASE). Built inside the chroot as root.
// Phase 5 (EXTRA_PACKAGES). Every phase 4 and 5 recipe, in dependency order, inside the
// chroot. What is already built is skipped, and a recipe whose builddeps failed is too.
//
// `all` is every loaded recipe. `startPkgs` (may be NULL) is --start-package:
//   phase 1-4: resume the phase at the first name given.
//   phase 5:   build only those packages and their builddeps/rundeps.
// Returns false when the phase had to be abandoned. Phase 5 never abandons.
bool builderPhase(Config cfg, BuildPhase phase, const RecipeList *all, const StrList *startPkgs);

// Phase state kept in <buildPath>/recipes/phase.yaml
short getPhase(Config cfg);
bool  setPhase(Config cfg, short phase);

#endif // MYLFS_BUILDER_H
