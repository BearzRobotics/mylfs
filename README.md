# mylfs
This is inspired by [MyLFS](https://github.com/TheKingKerellos/MyLFS), I forked 
it to my github and built a fully working x windows stack.

If a package fails to download and it's from the linux kernel git site. I had to 
manually download them first in a browser and then wget worked. -- It's a bot 
protection mechanism they're using. -- If you download the right version and 
drop it in place the program will just pick up like normal. As if it was already 
downloaded.

https://git.kernel.org
https://youtu.be/3MTyv7hystI?si=jcm_VvVByv3tWD6Q

On the other hand, some packages connect to the internet to download things. 
If they fail most of the time, restarting them works.

# build
mylfs requires libyaml to be installed on the system and curl in the path. CC 
and CXX are set to clang and clang++ respetivly.

```sh
    make
```

```sh
sudo ./mylfs
```

This is a hack. However, in phase5 it's build function has logic to use 
--start-package List[str] and pulls out it builds deps
> **Note:** Output below is from the Python edition. C port in progress.
```sh
./mylfs --start-phase 5 --start-package nwipe

mylfs-py edition
========================================
Warn: skipping tests
Passed: Required tools found
Initializing recipes from /data/code/py/mylfs-py/dkrecipes
Info: Copying DB from previous build!
Passed: Preserved DB from previous build!
Passed: Loaded 631 recipes.
Passed: Created user and group lfs
Passed: Created lfs user and group
Failed: could not chown: /mnt/lfs/ lfs user and group

Phase 5 - Building final system
========================================
    [SKIP] make                              already built
    [SKIP] glibc                             already built
    [SKIP] pkgconfig                         already built
    [SKIP] ncurses                           already built
    [SKIP] libeconf                          already built
    [SKIP] libconfig                         already built
    [SKIP] libpcap                           already built
    [SKIP] parted                            already built
    [SKIP] libgudev                          already built
    [SKIP] libblockdev                       already built
    [SKIP] nwipe                             already built
Passed: unmounted tmp fs
```

If a dep failed to build or there isn't a recipe for it, it will skip it
missing/failed deps.

If you changed to dkrecipes this will build batteries included distro. With 
anything I or my uncle might want to use or may want to explore. ~ This is a 
huge build.

If a build crashes you can restart that phase with the below command. This is 
often handy in phase 5 when you add packages to your recipes directory.
```sh
sudo ./mylfs --start-phase <number>
```

While it's building you may want to see what is going on inside.

```sh
ps aux | grep mylfs
```

# Why C?
I feel like in this day and age I need to put this here, so people don't ask 
why it's not rust. -- First off I don't like programming in rust. It gives me a 
major migraine. Preferably I would choose zig, but two factors are against it. 
My primary user prefers C, so it that is the driving factor. With zigs IO gate 
changing the way programs are written it's not worth dumping a lot of effort
into zig until that settles.

# build deps
[] - GNU Make
[] - libyaml
[] - working c compiler

# runtime deps
[] - Rsync
[] - tar
[] - unzip
[] - gnu make
[] - autotools
[] - coreutils
[] - qemu
[] - wget

# Things I hate about C!
   * glibc is a nightmare to program with. With access unix features only working
   with a _GNU_SOURCE even though it has other options like _POSIX_C_SOURCE, etc
   * Scoping: when including dprint.h for my print wrappers or anyother header I can
   just run passed() {myfunc} without any reference to where it's from. What if glibc
   or another dep created a function named passed()!
   * short, int, long, long long, double, etc WTF!
   * header guarding your .h files so the compiler know to only include one copy of it
   * Lack of a proper build system!
   * Prototyping functions! That is they must be in .h files and declared before use. 
   * No built in bool value! stdbool.h should not be a requirement to use bool
   * errors as return numbers
   * C switch statements are ugly compared to zig switch or rust match