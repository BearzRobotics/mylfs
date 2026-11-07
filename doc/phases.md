# Phases

The phases mimic the stages of a normal LFS build, but nothing in mylfs is
specific to them. A recipe is built in whichever phase its `phase:` says, so in
theory the same program can build a clang + musl system by only changing the
recipes. The phase names below may then not match what is actually happening.

| Phase | Name | Runs | Environment |
|-------|------|------|-------------|
| 1 | Cross toolchain | on the host as `lfs` | `$LFS`, `$LFS_TGT`, `$LFS/tools/bin` first in PATH |
| 2 | Temporary tools | on the host as `lfs` | same as phase 1 |
| 3 | Temporary system | in the chroot as root | first entry into the chroot |
| 4 | Base LFS system | in the chroot as root | |
| 5 | Extra packages | in the chroot as root | everything beyond the base system |

`$MAKEFLAGS` is not passed in phases 1 and 2. See
[template.md](template.md#variables-available-to-buildsteps).

## Failure handling

* Phases 1 and 2 stop at the first failure.
* Phases 3 and 4 stop on a package marked `critical: true`, otherwise they log
  the failure and continue.
* Phase 5 never stops. A failed recipe is reported, and any recipe that needs it
  is shown as `[SKIP]`.

## Phase 5 and the rebuild of phase 4

Phase 4 is ordered by `order:`, phase 5 by `builddeps`. In phase 5 a recipe is skipped when:

* it is already built (recorded in `recipes/build_state.db`)
* one of its builddeps failed or has no recipe

Raising the `release:` of a built recipe rebuilds it and everything that
builddeps on it. Phase 4 recipes with no builddeps are recorded as built when
phase 4 finishes. The ones with builddeps are rebuilt in phase 5, so optional
features that need extra libraries get enabled.

## Resuming

A plain run starts at phase 1. Use `--start-phase <1-5>` to resume, and
`--start-package <name>...` to start at a package. In phases 1-4 the build
continues from the named package. In phase 5 only the named packages plus their
builddeps and rundeps are built. See [mylfs.md](mylfs.md).
