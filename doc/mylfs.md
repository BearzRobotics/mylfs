# NAME
        mylfs - Is a tool for bootstraping Linux From Scratch systems.

# SYNOPSIS
        ```sh
        mylfs
        ```
# DESCRIPTION
        If you ever built 

# OPTIONS
        --bootstrap
                This options runs only phase 1-4 which bootstraps a base LFS
                system using the default recipes.
        
        --chroot
                Accepts a path to a directory to chroot into. Most likely this 
                will be mnt/lfs

                The advantage of using this over the chroot command is it will
                auto bind mount /dev /dev/pts /pro /sys and /run allowing you to
                have access to all vitial filesystems for disk manipulation and 
                the installation of your desired bootloader.

                If the auto bind mount of these locations failed it will print
                that to the screen but still attempt to chroot. Under these 
                circumstances you won't be able to install a bootloader.

        --debug
                This prints debug message used to help develop and work issues 
                out of the program. It should be noted that phases during the 
                debug are 0 index and not 1 based. So phase 1 will be printed 
                out as 0

                The way I parse pramaters --debug should be passed first to
                enable the max amount of debug info.

        --help
                Prints a basic help menu for mylfs

        --test
                Enable a $RUN_TEST variable inside the chroot enviroment for
                build scripts that support it to run their test. Otherwise test 
                are skipped.
        
        --start-package <Package name> [<Package name> ...]
                This option is must be used with --start-phase <phase number>.
                The package name corilates to the "name:" tag inside of your 
                template.yml file and the phase.

                In phases 1-4 the build resumes at the first name given.
                In phase 5 only the named packages and their builddeps and
                rundeps are built.

        --build-path <path>
                Overrides build_path from config.yml.

        --recipes-path <path>
                Overrides recipes_path from config.yml.

        --start-phase <phase-number>
                This option accepts a number between 1 and 5.
                        phase 1 - cross tools
                        phase 2 - Temp Tools
                        phase 3 - Temp system 
                        phase 4 - LFS base system
                        phase 5 - building extra packages per the recipes!

        --version
                Prints the current version of mylfs as a simple string in the 
                format x.y.z.

# EXIT STATUS
        The program returns:
            - 0 on success
            - Non‑zero on error

# ENVIRONMENT
        <List any environment variables that affect the utility’s behavior, if applicable.>

# FILES
        mylfs can be configured with a config.yml in the same direcotry as 
        the program was exected from. -- There is an example config.yml provided
        for you at the root of the repo.

# EXAMPLES
        Builds LFS using the config.yml file to point to it's build dir and 
        recipes dir.
        ```sh
        sudo ./mylfs
        ```

        This command tells the program to resume in phase 4 at the package named
        bash.
        ```sh
        sudo ./mylfs --start-phase 4 --start-package bash
        ```

        This command tells the program to resume in phase 4 at the package named
        bash. While also printing out debug info
        ```sh
        sudo ./mylfs --debug --start-phase 4 --start-package bash
        ```

        Chroots into the desired dir with bind appropriate bindmounts.
        ```sh
        sudo ./mylfs --chroot mnt/lfs
        ```

        Only build phase 1-4
        ```sh
        sudo ./mylfs --bootstrap 
        ```

# NOTES
        mylfs is meant to be a fully static binary (see "static / musl build"
        in the README). It looks up the lfs user by reading /etc/passwd itself
        and never calls getpwnam(), so a static build does not depend on the
        libc it was built against. Building against musl is still preferred.

        The build state of phases 4 and 5 is kept in recipes/build_state.db
        inside the build dir, one line per package:
        name|version|release|phase|built|builddeps|rundeps
        This is a text file. The python edition's sqlite file of the same name
        is not read.

        A plain run always starts from phase 1. Use --start-phase to resume.

# AUTHOR
    Author: Dakota James Owen Keeler
    Email: DakotaJKeeler@protonmail.com

# REPORT BUGS
        Report all bugs to the following repo.
        <URL/form/etc>

        All Bug reports should follow this format

        Program version: <fill in>
        OS: <fill in>
        Description of bug: <fill in>
        How to reproduce: <fill in>
        Expected behavior: <fill in>
        Misc: <Anything else you feel is important>

# SEE ALSO
        <List related man pages, docs, or resources, e.g., other utilities or standards.>