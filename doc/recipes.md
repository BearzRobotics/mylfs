# Recipes Overview

by default the mylfs will look for a recipes directory in the location it was 
executed from. This can be configured in the config.yml located in the same dir
as mylfs was executed from. 

```console
$ ls 
logs mnt recipes mylfs
```

In the recipes directory they're a bootstrap directory, [0-9] & [a-z]

The bootstrap directory is broken up into the directories phase1, phase2 and 
phase3. These directories contains the recipes and template.yaml for each phase
of inital LFS build. “In the first 4 bootstrap phases, the build order is 
hard-coded. It is recommended to use numbers spaced by 5. (eg. 1, 5, 10, 15, 
etc) This allows you to go back later and add a package without having to modify
the order flag of every template.yaml.

The directories [0-9] and [a-z] all contains recipes directories following the 
template bellow. During phase5 mylfs will create the build order based off from
the builddeps value. If one of these fail to build the package depending on will
be skipped. As there is no point in trying to build a package we know cannot be 
built.

There is no way to guarantee that the build order in phase5 will be the same
each time mylfs is run.


# Recipes structure
recipes/
├── bootstrap/
│   └── phase1/
│       └── glibc/
│           ├── source/         # required
│           │                       If there is no source.tar.gz or other
│           │                       supported format this directory must exist 
│           │                       as its the default location the builder will 
│           │                       enter.
│           ├── template.yaml   # required  
│           ├── source.tar.gz   # optional   
│           ├── patches/        # optional  
│           ├── static/         # optional  
├── b/
│   └── bash/
│       ├── source/             # required
│       ├── template.yaml       # required
│       ├── source.tar.gz       # optional
│       │                           If a url is provided this  will be downloaded
│       │ 
│       ├── patches/            # optional  
│       ├── static/             # optional
├── l/
│   └── linux/
│       ├── source/             # required
│       ├── template.yaml       # required
│       ├── source.tar.gz       # optional
│       ├── patches/            # optional
│       └── config              # optional

# Kernel
If you desire to have a custom kernel config to enable different hardware 
support you can. First extract the kernel source code for the version you desire
to build. Run the following command to configure the kernel and make sure to 
save to generate a .config file.
```sh
make menuconfig
```

Once you've completed generating your config, copy it into the l/linux folder as 
config without the dot. The default recipe we check if it exist. If it does it
will copy it over in place and build the kernel with it. Otherwise the kernel 
build with a minimal defconfig.
