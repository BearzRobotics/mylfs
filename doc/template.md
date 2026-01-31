# template.yaml

After doing 100s of these manually for the base install. What I would recommend 
is to pass an example in chatgpt and give it a link. The build commands and 
descriptions aren't always right. However, its a lot faster than manually 
creating every template.yml file. Especially if you're adding a lot of packages 
in your recipes folder.

Most elements in the template.yaml files are optional and have no direct impact
on building LFS. These fields exist to provided further meta data about the
packages so later if you forget what a package does you can refer back to its 
meta data.


name:            # Required - package name

version:         # Required - package version

release:         # Optional - for internal versioning 
                              (e.g. release bump) [not implemented]

url:             # Optional - source tarball URL

license:         # Optional - Know the legal restrictions on the code

summary:         # Optional - short description

homepage:        # Optional - Link to their website

description:     # Optional - long multi-line description

phase:           # Optional - phase1, phase2, phase3, phase4, phase5

order:           # Optional - integer ordering (bootstrap only)

builddeps:       # Optional - list of build dependencies (phase 5 - only)
                              This option is only required if you package has
                              build deps beyond what the base LFS has installed.
                              On a technically level therefor you don't need to
                              include "make" or "autotools" as a build option.

                              However, I strongly recommend it for keeping a 
                              clear history of every package used need for it.
              
                              As some packages can have optional build deps to 
                              expose more functionality if they're present.
                              Any phase4 recipe that contains a builddeps option
                              will be rebuilt in phase5 against any new deps.

rundeps:         # Optional - (phase 5 - only) 
                              packages that need to be present at run time. This
                              is here to keep track of what all a package needs 
                              to run smooth. 


buildsteps:      # Required - preparation steps. Everything below is a sh 
                              script (bash) for building

cleanup: false   # Optional - Tells the build system to not delete the extract 
                              source on a successful build.
                              
                              It should be noted if your run the below it will 
                              refresh the recipes on the disk. 
                              
                              ```sh
                              sudo ./mylfs  --start-phase <number> 
                             ```
                             This will cause all extracted versions to be 
                             deleted. Thus it is recommend to use a combination
                             of the log output and chroot to debug the build.

                             ```sh
                             sudo ./mylfs --chroot <buildDir Path>
                             ```
                    

{name}           # Optional - Allows the name to be used as a variable
{version}        # Optional - Allows the version to be used as a variable

$var            In the shell script section all vars must be defined with a 
                few exceptions:
                   $RUN_TEST
                   $LFS
                   $LFS_TGT                  
                These values are auto populated. Though $RUN_TESTS is only
                populated if --test is passed or if test: false is set to true
                in the config.yml.

                To use $RUN_TEST variable add this to your buildsteps:

                if [ "$RUN_TESTS" = "1" ]; then
                    set +e
                    make check # Or any other test command as desired. 
                    set -e
                fi


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
   The GNU C Library provides the core libraries for the GNU system and 
   GNU/Linux systems. This includes essential facilities like libc, crypt, 
   math, and others.
phase: 2
order: 3
builddeps:
   - linux-headers
   - gcc
buildsteps: |
   mkdir build
   cd build
   ../configure --prefix=/tools

   make
   make install



