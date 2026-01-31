# mylfs
This is inspired by [MyLFS](https://github.com/TheKingKerellos/MyLFS), I forked it to
my github and built a fully working x windows stack.


The shell script version is not future proof and tedious to port to new versions.


by default mylfs looks for recipes logs and mnt in the same directory that it was launched.
These can be configured.


The original plan was to write this in zig. However, after playing around with it for almost a week
I can not get the yaml files to parse with any of the native zig yaml libraries. -- After my hike, the  landscape has seen good development. I would like to return to this.


NOT all config options are implemented yet! Patches are welcome if anyone wants to implement a feature not there yet!

If a package fails to download and it's from the linux kernel git site. I had to manually download them first in a browser and then wget worked. -- It's a bot protection mechanism they're using. -- If you download the right version and drop it in place the program will just pick up like normal. As if it was already downloaded.

https://git.kernel.org
https://youtu.be/3MTyv7hystI?si=jcm_VvVByv3tWD6Q


On the other hand, some packages connect to the internet to download things. If they fail most of the time, restarting them works.

PS. My spelling is bad... I'm aware. To the best of my ablity I used Google doc to spell check this readme. However, through out you'll probably find a lot of miss spelled stuff.

# build
This will build the base system with a handful of extra packages in phase 5 (including efi support).


```console
sudo ./mylfs
```

This is a hack. However, in phase5 it's build function has logic to use --start-package List[str] and pulls out it builds deps

```console
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
missing/failed deps ->  djok

There is a know bug, where if you pass in a package to --start-package that doesn't exist: djok, mylfs-py will just build all packages. However, if you pass it in with another valid package say ./mylfs --start-phase 5 --start-package nwipe djok nwipe. It will build nwipe and all of its deps. However in either case it never tells your that recipe djok doesn't exist.

or edit config.yml and change recipes_path: "recipes/" to recipes_path: "dkrecipes/"

If you changed to dkrecipes this will build batteries included distro. With anything I or my uncle might want to use or may want to explore. ~ This is a huge build.


If a build crashes you can restart that phase with the below command. This is often handy in phase 5 when you add packages to your recipes directory.
```console
sudo ./mylfs --start-phase <number>
```


While it's building you may want to see what is going on inside.


In the example below I was running mylfs on virtial terminal 3. The below command will print out
all processes running on that terminal. -- You will need to change 3 with the virtual terminal you're running
on or TTY.


virtual terminals
ls /dev/pts*


on solus /dev/pts/*


you can grep each one from ps aux until your find the one you want
```console
ps aux | grep pts/3
```

If this doesn't work, you either need to manually unmount each instance of our bind mounts (dev, dev/{pts,shm}, proc, sys, run). Or you can just reboot. (type findmnt and you'll see why)


# Img file
If you want to create an image file to load your distro in instead of installing to real hardware start here. Otherwise skip to Installation.


First we must create an image file. (I'm doing 40G as my dkrecipes currently is ~35G as a complete build)


status=progress is optionally.
```Console
dd if=/dev/zero of=lfs.img bs=1M count=40960 status=progress
```
or if your filesystem supports it
```Console
truncate -s 40G lfs.img
```

Now we need to mount the  img file.
```Console
sudo losetup -fP lfs.img
losetup -a
```

losetup -a should give you an output like:
/dev/loop0: []: (/home/dakota/lfs.img)

format your drive. I'm using cfdisk. However, you can use fdisk
or any other tool. For simplicity the drive should be MBR (dos) with one ext[2-4] partition.
```Console
sudo cfdisk /dev/loop0
```

Format the file system
```Console
sudo mkfs.ext4 /dev/loop0p1
```

Create a place to mount our image
```Console
sudo mkdir -vp /mnt/lfs-image
sudo mount /dev/loop0p1 /mnt/lfs-image
```

Once done Continue on with Installation as if it was on a real drive.


# Installation

If you did not build in place. Either by mounting your drive to mnt/lfs or changing the relative build path to where your new drive is mounted you will need to transfer your files over. In this example we mounted our newroot partition to /mnt/usb, change it to wherever you mounted your new root fs.

As this command is set up it needs to be executed in the root of the mylfs-py directory.


```console
sudo rsync -aAXHv --delete --info=progress2 \
 --exclude=/dev/* \
 --exclude=/proc/* \
 --exclude=/sys/* \
 --exclude=/run/* \
 mnt/lfs/ /mnt/usb/
```

Then you need to bind mount {/dev/{pts,shm},sys, run, proc} inside your mount point.

```console
mount --bind /dev  /mnt/usb/dev
mount --bind /proc /mnt/usb/proc
mount --bind /sys  /mnt/usb/sys
mount --bind /run  /mnt/usb/run
```

if /dev/pts exist it should be mounted as well.

```console
mount --bind /dev/pts  /mnt/usb/dev/pts
```

Once done chroot inside.
```console
chroot /mnt/usb /usr/bin/bash --login
```

When done make sure to unmount your bind mounts


# configure
fstab & hosts need to be configured inside. This is a good time to make any last minute configuration to other files as well. 

brave souls can try genfstab, however, I can't guarantee that it will work. Otherwise refer to the LFS book for setting up this file. This modified version also allows for a -C to be used with -P to allow for a clean style LFS fstab setup. --- Experimental


```console
genfstab -LP > /etc/fstab
```

Make sure to set a root password.

NOTE: This should be patched. However, when I first booted I could not set the root password. Doing this seems to have fixed that issue.


# grub2
In our Chroot we need to install grub2 to our drive. My drive is /dev/sdc make sure you properly
identify which drive you are actually using.

If you want to install legacy mode for qemu or older hardware and your system supports UEFI you
need to pass --target=i386-pc

With a gpt formatted disk, to install legacy mode you need a 1-2 mb unformatted space at the beginning of the disk that you mark as bios-grub. This will also allow you to make a medium that can boot both in legacy and UEFI as long as the kernel supports it. To do this after you install grub to the root of the drive you would then install grub normally for an efi setup with a fat32 (vfat) fs as your esp/boot/efi depending on the setup that is placed right after the 1-2 mb unformatted space for legacy grub. -- Make sure that your mount your EFI partition inside your chroot before installing grub if your using a efi setup.

If you're in the mounted image this should look like /dev/loop0
```console
grub-install /dev/sdc
```


example grub2 config from the book to be placed in  /boot/grub/grub.cfg
---
# Begin /boot/grub/grub.cfg
set default=0
set timeout=5


insmod part_gpt
insmod ext2
set root=(hd0,2)
set gfxpayload=1024x768x32


menuentry "GNU/Linux, Linux 6.13.9-lfs-12.3" {
       linux   /boot/vmlinuz-6.13.9-lfs-12.3 root=/dev/sda2 ro
}
---


Grub config from the bash script MyLFS that I based this off.
You can get the Part UUID from running blkid and passing in your drive with it's partition id.
```console
blkid /dev/sdc4
```

In it's output you'll see PARTUUID="". What ever is in the string is your part uuid and would go in the grub config. (Best success using this)

Make sure your partition is labled with LFSROOT or update the --label with your root fs label
---
# Begin /boot/grub/grub.cfg
set default=0
set timeout=5

insmod ext2

menuentry "GNU/Linux, Linux 6.13.9-lfs-12.3 nomodeset"{
  search --no-floppy --label LFSROOT --set=root
  linux   /boot/vmlinuz-6.13.9-lfs-12.3 rootwait root=PARTUUID=ecb913d1-02 ro net.ifnames=0 biosdevname=0 nomodeset
}

menuentry "GNU/Linux, Linux 6.13.9-lfs-12.3" {
  search --no-floppy --label LFSROOT --set=root
  linux   /boot/vmlinuz-6.13.9-lfs-12.3 rootwait root=PARTUUID=ecb913d1-02 ro net.ifnames=0 biosdevname=0
}


---

if you build dkrecipes you should have grub-mkconfig command.  I like to use UUIDs instead of /dev/sdX notation.


cat > /etc/default/grub << "EOF"
GRUB_DISABLE_LINUX_UUID="false"
GRUB_TIMEOUT=5
GRUB_DISTRIBUTOR="GNU/Linux"
GRUB_CMDLINE_LINUX_DEFAULT=""
GRUB_CMDLINE_LINUX=""
EOF


```console
grub-mkconfig -o /boot/grub/grub.cfg
```


Now we need to unmount the bind mounts. If you also mounted dev/pts that needs to be unmounted before
/mnt/usb/dev is.
```console
umount -l /mnt/usb/dev
umount -l /mnt/usb/proc
umount -l /mnt/usb/sys
umount -l /mnt/usb/run
umount -l /mnt/usb/
sync
```


# Trying the system in qemu


To try this in qemu you must also umount your new root fs.
```console
umount /mnt/usb
```

if you were working with the image file also run
```console
losetup -d /dev/loop0
```
Remember my drive was /dev/sdc. Make sure to pass in the right drive or lfs.img.

When creating the grub config, the system /dev/ is mounted. However, when booting the drive directly in qemu like done below it becomes the only drive. Thus I had to edit the grub config and change /dev/sdc4 to /dev/sda4. eg. linux /boot/vmlinuz-6.13.9-lfs-12.3 root=/dev/sda3 ro


```console
qemu-system-x86_64 -m 2048 -enable-kvm -cpu host -hda /dev/sdc -boot order=d -vga std
```

If you hardware supports it you can add in place of "-vga std"
```console
qemu-system-x86_64 -m 2048 -enable-kvm -cpu host -hda /dev/sdc -boot order=d  -device virtio-vga-gl -display gtk,gl=on
```

# Generic Binaries
in builder.py you'll find these set 
    
    # Commet these lines out if you want to build packages for your system
    "CFLAGS"  : "-march=x86-64 -mtune=generic -O2",
    "CXXFLAGS" : "-march=x86-64 -mtune=generic -O2",


    # Build generic binaries
    env["CFLAGS"] = "-march=x86-64 -mtune=generic -O2"
    env["CXXFLAGS"] = "-march=x86-64 -mtune=generic -O2"


# Phases
Phase 1:
   Cross toolchain

Phase 2:
   Temp tools /tools

Phase 3:
   (Entering chroot for first time)
   Temp system

Phase 4/5:
   By default the build tool will seek to build all package recipes.
   Packages in phases 1-3 are critical and the whole build system will crash.

   Optionally all packages in the recipes directory can have an optional critical tag. See
   any base package in LFS build. -- If present this tag will halt the whole build.

   Otherwise If a package fails to build, the build system will move on to the next
   package in the que. -- mylfs will skip all packages that had a required/runtime dep
   fail to build.

Phase 5
   All packages in the recipes directory beyond the basic LFS build are Phase 5. If you pass in
   recipes to the program or just replace the recipes folder with your own it will keep building
   all packages and skip failed packages and packages that have missing dependencies.

# Recipes

by default the mylfs will look for a recipes directory in the location it was executed from.
```console
$ ls 
logs mnt recipes mylfs
```

In the recipes folder are bootstrap, [0-9] & [A-z]

If you want a custom kernel config, place a .config as config [do not  dot] for the version of the kernel you are building  and place it next to the template.yml in the linux recipe directory..

recipes/
└── bootstrap/
   └── phase1/
       └──glibc
           ├── source               # required - Source is extract here
           ├── template.yaml        # required
           ├── source.tar.gz        # optional
           ├── patches/             # optional
           ├── static/              # optional
           └── config_templates/    # optional


recipes/
└── b/
   └── bash/
       ├── source               # required
       ├── template.yaml        # required
       ├── source.tar.gz        # optional
       ├── patches/             # optional
       ├── static/              # optional
       └── config_templates/    # optional


# template.yml


After doing 100 of these manually for the base install. What I would recommend is to pass an example in chatgpt and give it a link. The build commands and descriptions aren't always right. However, it's a lot faster than manually creating every template.yml file. Especially if you're adding a lot of packages in your recipes folder.


name:            # Required - package name
version:         # Required - package version
release:         # Optional - for internal versioning (e.g., release bump)
url:             # Optional - source tarball URL
sha256:          # Optional - checksum for source verification
license:         # Required or strongly recommended - license identifier
summary:         # Optional - short description
homepage:        # Optional - Link to their website
description:     # Optional - long multi-line description
phase:           # Optional - phase1, phase2, phase3, phase4, phase5
order:           # Optional - integer ordering (bootstrap only)
critical:        # Optional - boolean -- (I don't think this works in the current build)
builddeps:       # Optional - list of build dependencies
                             In phase 5 any phase 4 package with builddeps will be rebuilt taking into account build tools
                             capability to detect dependencies and use them. -- Phase 4 packages that omit it will not be rebuilt
rundeps:         # Optional - packages that need to be present at run time. (If package is marked
                             as critical if its rundeps are not built it will fail.)


                             This is ignored in phase 4. In phase 5 all packages will be rebuilt with
                             dependency awareness from the builddeps.
buildsteps:      # Required - preparation steps. Everything bellow is a sh script (bash) for building
cleanup: false   # Optional - Tells the build system to not delete the extract source on a successful build.
                             It should be noted if your run sudo ./mylfs-py --start-phase <number> It will refresh
                             the recipes folder and deleted the saved source. If it's important you should back it up.
                             If you don't have a clean up tag. The default is to cleanup extracted source

{name}           # Optional - Allows the name to be used as a variable
{version}        # Optional - Allows the version to be used as a variable

$var             # In the shell script section all vars must be defined. --
                  The appropriate LFS variables like $LFS or $LFS_TGT will be passed at each stage
                  to avoid repetition in each script


eg template.yml
---------------------------------------
name: glibc
version: 2.39
release: 1
url: https://ftp.gnu.org/gnu/glibc/glibc-2.39.tar.xz
homepage: https://www.gnu.org/software/binutils/


license: LGPL-2.1-or-later
summary: GNU C Library
description: |
   The GNU C Library provides the core libraries for the GNU system and GNU/Linux systems.
   This includes essential facilities like libc, crypt, math, and others.
phase: 2
order: 3
critical: true
builddeps:
   - linux-headers
   - gcc
buildsteps: |
   mkdir build
   cd build
   ../configure --prefix=/tools


   make
   make install
------

name:
version:
release:
url:
homepage:

license:
summary:
description: |

phase: 4                # Unless you want to modify the base LFS your packages should be phase 5
order:                  # phase 1-4 only
critical: true
builddeps:     []       # if need

buildsteps: |


# build deps
[] - GNU Make
[] - libyaml
[] - libcurl



# runtime deps
[] - Rsync
[] - tar
[] - unzip
[] - gnu make
[] - autotools
[] - zig

# Things I hate about C!
   * glibc is a nightmare to program with. With access unix features only working
   with a _GNU_SOURCE even though it has other options like _POSIX_C_SOURCE, etc
   * Scooping: when including dprint.h for my print wrappers or anyother header I can
   just run passed() {myfunc} without any reference to where it's from. What if glibc
   or another dep created a function named passed()!
   * short, int, long, long long, double, etc WTF!
   * header guarding your .h files so the compiler know to only include one copy of it
   * Lack of a proper build system!
   * Prototyping functions! That is they must be in .h files and delecared before use. 
   * No built in bool value! stdbool.h should not be a requirment to use bool
   * errors as return numbers
   * C switch statements are ugly compared to zig switch or rust match
   
