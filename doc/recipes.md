# Recipes

A recipe is a directory holding a [template.yml](template.md) plus the files the
package needs. mylfs looks for recipes in `recipes_path` from config.yml (or
`--recipes-path`), relative to where you run it:

```console
$ ls
config.yml  logs  mnt  recipes/  mylfs
```

The recipe sets shipped here are `recipes/LFS13_1` (systemd, the default),
`recipes/LFS12_3`, and `dkrecipes`, a much bigger set that builds a full desktop
distro.

## Layout

Inside a recipe set, the directory a recipe lives in is only for your own
organization. mylfs finds every `template.yml` under the set, and the `phase:`
key inside decides when it is built. By convention:

* `bootstrap/phase1`, `phase2`, `phase3` hold the hard-coded base build
* `[0-9]` and `[a-z]` hold everything else, by first letter of the name

```
recipes/LFS13_1/
├── bootstrap/
│   └── phase1/
│       └── gcc/
│           ├── template.yml    # required
│           ├── source/         # created by mylfs
│           ├── gcc-16.2.0.tar.xz   # downloaded from url
│           ├── patches/        # optional
│           └── static/         # optional
├── b/
│   └── bash/
│       ├── template.yml
│       └── bash-5.3.tar.gz
└── l/
    └── linux/
        ├── template.yml
        └── config          # optional kernel config
```

| Entry | Meaning |
|-------|---------|
| `template.yml` | Required. Describes and builds the package. |
| `<tarball>` | The file(s) from `url`. Downloaded on first run, or put there by hand. |
| `source/` | Where the first tarball is extracted and where the buildsteps start. Created by mylfs and deleted after a good build unless `cleanup: false`. |
| `patches/` | Optional. Patch files for the buildsteps, used as `patch -Np1 -i ../patches/x.patch`. |
| `static/` | Optional. Files to install as-is (configs, unit files). A recipe with no `url` starts its buildsteps here. |
| `config` | Optional. Used by the linux recipe, see Kernel below. |

Because the buildsteps start in `source/`, the recipe directory is `..` from
there: `../patches/`, `../static/` and extra tarballs are all reached that way.

## Ordering

* **Phases 1-4** build in the order given by `order:`, lowest first. Use numbers
  spaced by 5 so you can add a package without renumbering. These phases are
  the base LFS build.
* **Phase 5** is sorted from `builddeps:`. The order can differ between runs
  where packages are independent of each other.

In phase 5 a recipe is skipped when it is already built (recorded in
`recipes/build_state.db` inside the build dir) or when one of its builddeps
failed or has no recipe. A package that cannot build is not retried by
whatever depended on it. See [phases.md](phases.md) for details.

## Kernel

To build the kernel with your own config, extract the kernel source for the
version you want and run:

```sh
make menuconfig
```

Save, then copy the resulting `.config` into the linux recipe directory
(`l/linux/`) as `config`, without the dot. The linux recipe copies it into
place and runs `make olddefconfig` if the file exists, otherwise it builds a
minimal `defconfig`.

## Adding a recipe

1. Make `<recipes>/<first letter>/<name>/template.yml`
   (start from [template.yml](template.yml)).
2. Set `phase: 5` for something that goes on top of the base system.
3. List what it needs in `builddeps`.
4. Run `sudo ./mylfs --start-phase 5 --start-package <name>` to build just it
   and its dependencies.

Downloads that fail because a site blocks bots (git.kernel.org) can be fetched
in a browser and dropped into the recipe directory, mylfs will use them.
