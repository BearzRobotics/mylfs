# template.yml

Every recipe is a directory with a `template.yml` in it. mylfs finds them by
scanning the recipes directory for files with exactly that name (not
`template.yaml`). The file is a flat YAML mapping of the keys below.

Most keys only describe the package and have no effect on the build. They are
there so that later, when you forget what a package is, the recipe tells you.
Only `name`, `version` and `buildsteps` are required. A recipe missing one of
them is reported and not loaded.

Tip: for a lot of new packages, give an LLM an existing template and the
project's link to get a first draft. The build commands and descriptions are not
always right, so read them, but it is much faster than writing every file by
hand.

## Keys

| Key | Required | Type | Default | Used by mylfs |
|-----|----------|------|---------|---------------|
| `name` | yes | string | | yes |
| `version` | yes | string | | yes |
| `buildsteps` | yes | multi-line string | | yes |
| `release` | no | integer | `1` | phase 5 |
| `url` | no | string or list | none | yes |
| `phase` | no | `1`-`5` or `phaseN` | unset | yes |
| `order` | no | integer | `999` | phases 1-4 |
| `critical` | no | bool | `false` | yes |
| `cleanup` | no | bool | `true` | yes |
| `builddeps` | no | list | empty | phase 5 |
| `rundeps` | no | list | empty | phase 5 |
| `license` | no | string | | metadata only |
| `summary` | no | string | | metadata only |
| `homepage` | no | string | | metadata only |
| `description` | no | multi-line string | | metadata only |

Booleans accept `true`, `yes` or `1`. Anything else is false.

### name
The package name. It is what `builddeps`, `rundeps` and
`--start-package` refer to, and what shows up in the build output and log
file names.

### version
The package version. Keep it a plain string (`2.39`, `1.4.1-rc1`). Available in `buildsteps` as `{version}`.

### release
Your own revision number for the recipe. In phase 5 mylfs records
`name|version|release` in `recipes/build_state.db`. If you raise `release` of a
package that was already built, it is rebuilt along with every package that
lists it in `builddeps`. Use it when you change the buildsteps or want to force
a rebuild. A missing release counts as `1`.

### url
Where to download the source. Either one string or a list:

```yaml
url: https://ftp.gnu.org/gnu/glibc/glibc-2.39.tar.xz
```
```yaml
url:
    - https://ftp.gnu.org/gnu/gcc/gcc-16.2.0/gcc-16.2.0.tar.xz
    - https://ftp.gnu.org/gnu/mpfr/mpfr-4.2.2.tar.xz
```

The **first** url is the main tarball. It is extracted into `source/` with the
top directory stripped (`--strip-components=1`), and `source/` is where the
buildsteps start. Supported formats are `.tar.gz`, `.tgz`, `.tar.bz2`,
`.tar.xz`, `.tar.zst`, `.tar` and `.zip`.

Every **other** url is only downloaded, into the recipe directory next to
`template.yml`. The buildsteps extract them themselves (see gcc's mpfr, gmp and
mpc, they use `tar -xf ../mpfr-4.2.2.tar.xz`).

Files are downloaded with curl only if they are not already there. Drop the file
in the recipe directory yourself and mylfs will use it (needed for
git.kernel.org, which blocks bots).

A recipe with **no url** has nothing to extract. Its buildsteps start in the
recipe's `static/` directory instead, which is the right shape for recipes that
only install files you ship.

### buildsteps
A shell script (bash), usually written as a YAML literal block (`|`). It runs
with `set +h`, `umask 022` and `set -e` in front, so the first failing command
fails the package. The working directory is `source/`.

Phases 1 and 2 run on the host as the `lfs` user. Phases 3 and later run inside
the chroot as root.

Before the script runs, `{name}` and `{version}` are replaced with the recipe's
values so you do not have to repeat a version number:

```yaml
buildsteps: |
    cp -r Documentation/* /usr/share/doc/linux-{version}
```

#### Variables available to buildsteps
Anything else must be defined by the script itself.

| Variable | Where | Value |
|----------|-------|-------|
| `$LFS` | phases 1-2 only | the build path (`build_path`, e.g. `/mnt/lfs`). Inside the chroot `/` is the root so it is deliberately not set. |
| `$LFS_TGT` | phases 1-2 only | `lfs_tgt` from config.yml, e.g. `x86_64-lfs-linux-gnu` |
| `$RUN_TESTS` | all phases | `1` when `--test` is passed or `test: true` is set, otherwise `0` |
| `$MAKEFLAGS` | phases 3+ only | `make_flags` from config.yml. Left unset in phases 1 and 2 to avoid complications. |
| `$CFLAGS` | all phases | `cflags` from config.yml. Unset if blank. |
| `$CXXFLAGS` | all phases | `cxxflags` from config.yml. Unset if blank. |
| `$RUSTFLAGS` | all phases | `RUSTFLAGS` from config.yml. Unset if blank. |

To honor the test switch in a recipe:

```sh
if [ "$RUN_TESTS" = "1" ]; then
    set +e
    make check      # or whatever the package's test command is
    set -e
fi
```
The `set +e` keeps a failing test suite from failing the package (the LFS book
expects some failures).

### phase
Which phase builds the recipe: `1` to `5`. `phase4` is accepted too. A recipe
with no phase is never built. See [phases.md](phases.md) for what each phase is.

### order
Sort position inside phases 1-4, lowest first. Recipes with no order get `999`,
and ties are broken by name so every run builds in the same order. Space your
numbers (1, 5, 10, 15...) so you can add a package later without renumbering.
Phase 5 is ordered by `builddeps`, not by this field.

### critical
`critical: true` means a failure stops the build. Phases 1 and 2 always stop on
a failure no matter what this says. In phases 3, 4 and 5 a non-critical package
that fails is logged and the build moves on. Phase 5 never stops, critical or not.

### cleanup
After a successful build mylfs deletes the extracted `source/` directory. Set
`cleanup: false` to keep it for this recipe (gcc and linux do, since later
recipes need their trees). The global `cleanup_sources` in config.yml must also
be true for the delete to happen. Recipes without a url never delete anything.

Note that `--start-phase` refreshes the recipes on disk and so removes any
extracted `source/`. To debug a failed build, use the log and a chroot:

```sh
sudo ./mylfs --chroot mnt/lfs
```

### builddeps
List of recipe names that must be built first (phase 5):

```yaml
builddeps:
    - linux-headers
    - gcc
```

Only list what the base LFS system does not already give you. Technically you do
not have to list `make` or `gcc`, but listing everything keeps a clear record of
what the package needed. It matters for ordering and skipping:

* phase 5 recipes are built in dependency order
* a recipe is skipped if a builddep failed or has no recipe
* a phase 4 recipe that has builddeps is rebuilt in phase 5 against them, which
  is how optional features get enabled once their libraries exist
* raising `release` of a dependency rebuilds the recipes that depend on it

### rundeps
List of packages needed at run time (phase 5). They are built before the
package when you pass `--start-package`, and they document what the package
needs to run.

### license, summary, homepage, description
Free text. Nothing reads them, they are for you. `description` is normally a
literal block (`|`).

## Example

```yaml
name: glibc
version: 2.39
release: 1
url: https://ftp.gnu.org/gnu/glibc/glibc-2.39.tar.xz
homepage: https://www.gnu.org/software/libc/

license: LGPL-2.1-or-later
summary: GNU C Library
description: |
    The GNU C Library provides the core libraries for the GNU system and
    GNU/Linux systems. This includes libc, libm, and others.

phase: 4
critical: true
builddeps:
    - linux-headers
    - gcc

buildsteps: |
    mkdir build
    cd build
    ../configure --prefix=/usr
    make
    if [ "$RUN_TESTS" = "1" ]; then
        set +e
        make check
        set -e
    fi
    make install
```

A blank template to copy is in [template.yml](template.yml).
