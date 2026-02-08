# SPDX-License-Identifier: GPL-2.0-only
#
# Copyright (C) 2025 Dakota James Owen Keeler
#
# This file is part of mylfs-py.
#
# mylfs-py is free software: you can redistribute it and/or modify
# it under the terms of version 2 of the GNU General Public License
# as published by the Free Software Foundation.
#
# mylfs-py is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.
from pathlib import Path
from typing import List, Optional
import subprocess
import shutil
import os

# 3rd party
import yaml

# My local imports
from config import GlobalConfig
from recipes import Recipe
from util import ConsoleMSG 
from buildphase import *
from graph import *

# if phase is 0-3 ask if encourge user to delete build dir
def checkIfBuildDirisEmpty(config: GlobalConfig):
    build_dir = Path(config.build_path)
    EXCLUDE_DIRS = {'dev', 'sys', 'run', 'proc'}
    
    if (build_dir.exists() and any(build_dir.iterdir())):
        ConsoleMSG.warn(f"Your build dir: {build_dir} is not empty! You should start fresh when building from scratch")
        response = input("Delete contents? [y/n] ")
        if response.lower() == 'y':
            for item in build_dir.iterdir():
                if item.name in EXCLUDE_DIRS:
                    ConsoleMSG.warn(f"Skipping bind-mounted dir: {item}")
                    continue
                try:
                    if item.is_dir():
                        shutil.rmtree(item)
                    else:
                        item.unlink()
                except Exception as e:
                    ConsoleMSG.failed(f"Failed to delete {item}: {e}")
        else:
            ConsoleMSG.warn("Continuing at your own risk!")

# recipes/phase.yaml
def get_phase_state(config: GlobalConfig) -> int:
    phase_path = Path(config.recipes_path)
    
    if phase_path.exists():
        phaseyml = phase_path / "phase.yaml"
        with phaseyml.open() as f:
            data = yaml.safe_load(f)
            return data.get("phase", 0)
    return 0 # Default to a fresh build

def set_phase_state(config: GlobalConfig, n: int):
    phase_path = Path(config.recipes_path)
    
    if phase_path.exists():
        phaseyml = phase_path / "phase.yaml"
        with phaseyml.open("w") as f:
            yaml.safe_dump({"phase": n}, f)

def sort_order(recipes: list[Recipe], phase: int) -> list[Recipe]:
    return sorted(
        [r for r in recipes if r.phase == phase],
        key=lambda r: r.order or 999)
    
# get all phase 4 and 5 recipes
def get_phase5_recipes(all_recipes: List[Recipe]) -> List[Recipe]:
    return [r for r in all_recipes if r.phase in (4, 5)]

def clean_up(config: GlobalConfig, recipe: Recipe):
    if not recipe.cleanup:
        if config.debug:
            print(f"Skipping cleanup for {recipe.name}: cleanup is set to False")
        return None
    
    if (config.cleanup_sources and recipe.urls) :
        source_dir = Path(recipe.recipe_source)
        
        if (config.debug):
            print (f"clean_up: {str(source_dir)}")
        
        if source_dir.exists() and source_dir.is_dir():
            shutil.rmtree(source_dir)
    else:
        if (config.debug):
            print("nothing to delelte - Either static or cleanup = False")

# if the first file of a list of urls 
def extract_tarball(config: GlobalConfig, recipe: Recipe, phase: int):
    if (config.debug):
        print(recipe.tarball_path)
    file = Path(recipe.tarball_path)
    name = Path(recipe.recipe_source)


    if not file.exists():
        ConsoleMSG.failed(f"Tarball not found: {file}")
        return False

    # Ensure the destination directory is clean
    if name.exists():
        shutil.rmtree(name)
    name.mkdir(parents=True, exist_ok=True)
    
    # Set full rwx permissions for user/group/other (777)
    os.chmod(name, 0o777)

    try:
        if file.suffix == ".zip":
            subprocess.run(["unzip", "-q", str(file), "-d", str(name)], check=True)
        elif file.suffix in [".tar", ".gz", ".bz2", ".xz", ".zst", ".tgz"]:
            # Support compound extensions like .tar.gz, .tar.xz, etc.
            if file.name.endswith((".tar.gz", ".tgz", ".tar.bz2", ".tar.xz", ".tar.zst", ".tar")):
                if (phase <= 1):
                    subprocess.run([
                        "sudo", "--preserve-env", "-u", "lfs", 
                        "tar", "-xf", str(file),
                        "-C", str(name),
                        "--strip-components=1"
                    ], check=True)
                else:
                   subprocess.run([
                        "tar", "-xf", str(file),
                        "-C", str(name),
                        "--strip-components=1"
                    ], check=True) 
                
            else:
                raise ValueError("Unsupported tar format")
        else:
            raise ValueError("Unsupported archive format")

        if (config.debug):
            ConsoleMSG.passed(f"Extracted {file.name} to {name}")
        return True

    except subprocess.CalledProcessError as e:
        ConsoleMSG.failed(f"Failed to extract {file.name}: {e}")
        return False
    except Exception as e:
        ConsoleMSG.failed(f"Error: {e}")
        return False

def filter_start_package(recipes: List[Recipe], start_pkgs: List[str] | None) -> List[Recipe]:
    if not start_pkgs:
        return recipes

    matched = [r for r in recipes if r.name in start_pkgs]

    if not matched:
        ConsoleMSG.warn(f"Start package(s) {start_pkgs} not found, building all.")
        return recipes

    return matched


def get_chroot_cwd(config, recipe):
    path = Path(recipe.recipe_source)
    return Path("/") / path.relative_to(config.build_path)

### Thoughts phase 1 & 2 are built outside of the chroot
### Phase 2 & 4 (5 if you're going beyond base) are build inside chroot.
### Phase 5 rebuild all phase 4 and 5 packages with deps. 

# phase 1 and 2
def buildPhase12(config: GlobalConfig, recipes: List[Recipe]):
    phase = 0 # dumby variable so user can set phase equal for sort_order
    # We need to check what pahse were in.
    if (get_phase_state(config) == 0):
        # Once phase one is done it will set phase to 1
        phase = 0 
    elif (get_phase_state(config) == 1):
        # Once phase two is done it will set phase to 2
        phase = 1
    else:
        ConsoleMSG.warn("buildPhase12 can only build phases 1 and 2")
        return
    
    # need to call our sort recipes
    # in template.yml phase starts at 1 not 0
    phase12R = sort_order(recipes, phase + 1)
    
    if config.start_package is not None:
        if phase == config.start_phase - 1:
            phaseR = filter_start_package(phase12R, config.start_package)
        else:
            if config.debug:
                print(f"Ignoring start_package '{config.start_package}' in phase {phase + 1}, only applies to phase {config.start_phase}")

        
    # we need to setup some envs
    env = {
        "TERM": "xterm", 
        "LFS": str(Path(config.build_path).resolve()),
        "LC_ALL": "POSIX",
        "LFS_TGT": str(config.lfs_tgt),
        "PATH": f"{Path(config.build_path).resolve()}/tools/bin:/usr/bin:/bin:/sbin:/usr/sbin",
        "CONFIG_SITE": f"{Path(config.build_path).resolve()}/usr/share/config.site",
        "MAKEFLAGS": str(config.make_flags),
        "RUN_TESTS": "1" if config.run_test else "0",
        # Build generic binaries
        "CFLAGS"  : "-march=x86-64 -mtune=generic -O2",
        "CXXFLAGS" : "-march=x86-64 -mtune=generic -O2",
        "RUSTFLAGS":"-C target-cpu=x86-64",
    }

    if (config.debug):
        print("=== Environment Variables ===")
        for key, value in env.items():
            print(f"{key}={value}")

    prebuild_cmd = "set +h \n umask 022 \n set -e \n"
    pkg_count = 0
     
    for recipe in phase12R:
        pkg_count += 1
        log_file = Path(f"{config.build_path}logs/p{phase + 1}_{pkg_count}_{recipe.name}_{recipe.version}.log")
        log_file.parent.mkdir(parents=True, exist_ok=True)
        
        cmd = prebuild_cmd + recipe.buildsteps
        
        if recipe.tarball_path is None:
            if (config.debug):
                print("don't extract")
        else:
            extract_tarball(config, recipe, phase)
        
        process = subprocess.run(
            ["sudo", "--preserve-env=LFS,LFS_TGT,PATH,LC_ALL,MAKEFLAGS,CONFIG_SITE,RUN_TESTS", "-u", "lfs", "bash"],
            #["bash"],
            env=env,
            input=cmd,
            cwd=Path(recipe.recipe_source),
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT  # merge both
        )
        
        with log_file.open("w") as f:    
            # lets write sone log files
            for line in process.stdout:
                f.write(line)
                
        if process.returncode == 0:
            # clean up extract source dir
            clean_up(config, recipe)
            ConsoleMSG.print_building(pkg_count, recipe.name)
        else:
            ConsoleMSG.failed(f"could not build package {recipe.name}")
            # by not having anything after this it allowed the build to continute after a
            # failed package. This could be part of implementing not critical
            return False 

    
    # If it made it this far without crashing or me exit() set phase
    if (phase == 0):
        set_phase_state(config, 1)
    elif (phase == 1):
        set_phase_state(config, 2)
    else:
        ConsoleMSG.failed("What are you doing here! buildPhase12 is only for the first two\n phases. This should have been caught before it got this far!")
        exit(1) 


# phase 2 and 4
# package allows the user to pick up from a package 
def buildPhase34(config: GlobalConfig, recipes: List[Recipe]):
    session = load_database(config)
    phase = 0
    
    # We need to check what pahse were in.
    if (get_phase_state(config) == 2):
        # Once phase three is done it will set phase to 3
        phase = 2
    elif (get_phase_state(config) == 3):
        # Once phase four is done it will set phase to 4
        phase = 3
    else:
        ConsoleMSG.warn("buildPhase34 can only build phases 3 and 4")
        return
    
    # need to call our sort recipes
    # in template.yml phase starts at 1 not 0
    phase34R = sort_order(recipes, phase + 1)
    
    if config.start_package is not None:
        if phase == config.start_phase - 1:
            phaseR = filter_start_package(phase34R, config.start_package)
        else:
            if config.debug:
                print(f"Ignoring start_package '{config.start_package}' in phase {phase + 1}, only applies to phase {config.start_phase}")
    
    # we need to setup some envs
    env = os.environ.copy()
    env["LFS"] = str(Path(config.build_path).resolve())
    if (config.run_test):
        env["RUN_TESTS"] = "1"

    # Build generic binaries
    env["CFLAGS"] = "-march=x86-64 -mtune=generic -O2"
    env["CXXFLAGS"] = "-march=x86-64 -mtune=generic -O2"
    env["RUSTFLAGS"]= "-C target-cpu=x86-64"
    
    if (config.debug):
        print("=== Environment Variables ===")
        for key, value in env.items():
            print(f"{key}={value}")

    prebuild_cmd = "set +h \n umask 022 \n set -e \n"
    pkg_count = 0
     
    for recipe in phase34R:
        pkg_count += 1
        log_file = Path(f"{config.build_path}logs/p{phase + 1}_{pkg_count}_{recipe.name}_{recipe.version}.log")
        log_file.parent.mkdir(parents=True, exist_ok=True)
        
        # if these works - It's really hacky
        cmd = prebuild_cmd + "cd " + str(get_chroot_cwd(config, recipe)) + "\n" + recipe.buildsteps
        
        if recipe.tarball_path is None:
            if (config.debug):
                print("don't extract")
        else:
            extract_tarball(config, recipe, phase)
        
        process = subprocess.run(
            ["chroot", env["LFS"],
            "/usr/bin/env", "-i",
            "HOME=/root",
            "TERM=xterm",
            "PS1=\\u:\\w\\$ ",
            "PATH=/usr/bin:/usr/sbin:/bin:/sbin",
            "bash", "--login"
            ],
            env=env,
            input=cmd,
            #cwd=Path(get_chroot_cwd(config, recipe)),
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT  # merge both
        )
        
        with log_file.open("w") as f:    
            # lets write sone log files
            for line in process.stdout:
                f.write(line)
                
        if process.returncode == 0:
            # clean up extract source dir
            clean_up(config, recipe)
            ConsoleMSG.print_building(pkg_count, recipe.name)
            
            # If it's phase 4 (3) and there are no builddeps: add it to the database so it's not rebuilt 
            if (phase == 3):
                if (not recipe.builddeps):
                    
                    entry = BuildEntry(
                    name=recipe.name,
                    version=recipe.version,
                    release=recipe.release,
                    built=True,
                    phase=recipe.phase,
                    builddeps=",".join(recipe.builddeps) if recipe.builddeps else None,
                    rundeps=",".join(recipe.rundeps) if recipe.rundeps else None,
                    package="Not Implemented"
                    )
                    save_or_update_entry(session, entry)
                    
        else:
            ConsoleMSG.failed(f"could not build package {recipe.name}")
            # by not having anything after this it allowed the build to continute after a
            # failed package. This could be part of implementing not critical
            # return False 
            
    session.close()
    
    # If it made it this far without crashing or me exit() set phase
    if (phase == 2):
        set_phase_state(config, 3)
    elif (phase == 3):
        set_phase_state(config, 4)
    else:
        ConsoleMSG.failed("What are you doing here! buildPhase32 is only for the second two\n phases. This should have been caught before it got this far!")
        exit(1) 

# builder for phase 5
# This will build one package
def chroot_builder(config: GlobalConfig, recipe: Recipe, pkg_count: int): 
    if (config.debug):
        ConsoleMSG.info("Inside chroot_builder")
    log_file = Path(f"{config.build_path}logs/p5_{pkg_count}_{recipe.name}_{recipe.version}.log")
    log_file.parent.mkdir(parents=True, exist_ok=True)
    
    # if these works - It's really hacky
    cmd = "set +h \n umask 022 \n set -e \n" + "cd " + str(get_chroot_cwd(config, recipe)) + "\n" + recipe.buildsteps
    
    # we need to setup some envs
    env = os.environ.copy()
    env["LFS"] = str(Path(config.build_path).resolve())
    if (config.run_test):
        env["RUN_TESTS"] = "1"

    # Build generic binaries
    env["CFLAGS"] = "-march=x86-64 -mtune=generic -O2"
    env["CXXFLAGS"] = "-march=x86-64 -mtune=generic -O2"
    env["RUSTFLAGS"]= "-C target-cpu=x86-64"

    if (config.debug):
        print("=== Environment Variables ===")
        for key, value in env.items():
            print(f"{key}={value}")
    
    
    if recipe.tarball_path is None:
        if (config.debug):
            print("don't extract")
    else:
        extract_tarball(config, recipe, phase=4)
        
    process = subprocess.run(
        ["chroot", env["LFS"],
        "/usr/bin/env", "-i",
        "HOME=/root",
        "TERM=xterm",
        "PS1=\\u:\\w\\$ ",
        "PATH=/usr/bin:/usr/sbin",
        "bash", "--login"
        ],
        env=env,
        input=cmd,
        #cwd=Path(get_chroot_cwd(config, recipe)),
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT  # merge both
    )
        
    with log_file.open("w") as f:    
        # lets write sone log files
        for line in process.stdout:
            f.write(line)
                
    if process.returncode == 0:
        clean_up(config, recipe)
        ConsoleMSG.print_building(pkg_count, recipe.name)
        return True
    else:
        ConsoleMSG.failed(f"could not build package {recipe.name}")
        # by not having anything after this it allowed the build to continute after a
        # failed package. This could be part of implementing not critical
        if (recipe.critical):
            return False
        
# rebuild all phase 4 and 5 packages with graph
# dep support. -- May become it's own file.
def buildAllPhase5(config: GlobalConfig, recipes: List[Recipe]):
    session = load_database(config)
    
    # get all phase 5
    phase5 = get_phase5_recipes(recipes)
    
    # get the build order taking into account deps
    phase5 = resolveBO(config, phase5)
    # get a list of recipes in their build order.
    
    if config.start_package is not None:
        if (config.start_phase - 1 == 4 or config.start_phase - 1 == 5):
            oldlist = phase5
            filtered = filter_start_package(phase5, config.start_package)
            
            needed = set(r.name for r in filtered)
            for recipe in filtered:
                for dep in recipe.builddeps + recipe.rundeps:
                    needed.add(dep)
            
            phase5 = [r for r in oldlist if r.name in needed]
            phase5 = resolveBO(config, phase5)
        else:
            if config.debug:
                print(f"Ignoring start_package '{config.start_package}' in phase {phase + 1}, only applies to phase {config.start_phase}")

    pkg_count = 0
    
    # Running into a problem where a yaml feild isn't present and I can't get it's name. 
    # we are going to iterate over the enter list and log it's order to p5_00_build_order.log
    blog_file = Path(f"{config.build_path}logs/p5_00_build_order.log")
    blog_file.parent.mkdir(parents=True, exist_ok=True)
    
    for re in phase5:
        with blog_file.open("a") as f:
            f.write(f"{re.name}\n")
    
    for recipe in phase5:
        if (config.debug):
            ConsoleMSG.info(f"buildAllPhase5 inside for loop: {recipe.name}")
        if (was_built(session, recipe.name)):
            # if you bump the revision of a package. This should build a reverse dep
            # map that sets all packages that rely on this to built=false (o in the db)
            # that way they will get rebuilt agianst the update.
            
            # This code is buggy. If you set the release higher than 1 it complains
            if (new_release(session, recipe.name, recipe.release)):
                rev_map = build_reverse_dep_map(phase5)
                to_rebuild = get_all_dependents(recipe.name, rev_map)
                mark_dependents_dirty(session, to_rebuild)
            else:
                # Otherwise, if it was already built and no new release skip it.
                # .ljust(20) built in str method to left justify optons
                print(f"    [SKIP] {recipe.name.ljust(33)} already built")
                #  I need to preserved the order that It would have built
                #pkg_count += 1 
                #log_file = Path(f"{config.build_path}logs/p5_{pkg_count}_{recipe.name}_SKIPPED.log")
                #log_file.parent.mkdir(parents=True, exist_ok=True)
                #with log_file.open("w") as f:    
                #    f.write("skip")
                continue
        
        failed_deps = all_deps_built(recipe, session)
        if failed_deps:
            print(f"    [SKIP] {recipe.name.ljust(33)} missing/failed deps -> \x1b[33m {', '.join(failed_deps)}\x1b[0m ")
            continue
        
        entry = BuildEntry(
            name=recipe.name,
            version=recipe.version,
            release=recipe.release,
            built=False,
            phase=recipe.phase,
            builddeps=",".join(recipe.builddeps) if recipe.builddeps else None,
            rundeps=",".join(recipe.rundeps) if recipe.rundeps else None,
            package="Not Implemented"
        )
        
        if(chroot_builder(config, recipe, pkg_count)):
            entry.built = True
            save_or_update_entry(session, entry)
            pkg_count += 1 
        else:
            save_or_update_entry(session, entry)
            pkg_count += 1 

            
    session.close()
    
# This function should update the running system
def updateSystem(config: GlobalConfig):
    pass