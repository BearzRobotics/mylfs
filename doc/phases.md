# Phases
The phase in this program are meant to mimic the phases during a typicall LFS 
build session. However, there is nothing special to them. If the occosiated 
files exist as described in recipes.md they will be ran. 

In theory one could change the recipes to support build a LFS system with 
clang + musl without needing to modify the code to mylfs at all. Merely the
phase headers may not match up what is actually being done under the hood.


# Phases discriptions 

Phase 1:
   Cross toolchain

Phase 2:
   Temp tools /tools

Phase 3:
   (Entering chroot for first time)
   Temp system

Phase 4:
   This phase builds the base LFS system. 

Phase 5
   As described in recipes.md this phase will build all user add recipes beyond
   the base system. These recipes are located in recipes/[0-9] or [a-z].

   If a recipe fails to build it will merely move on. If another recipe relys on
   a recipe that failed to build it will be marked as [skip] at the prompt. 

   Any phase 4 recipe with a builddeps: option listed will be rebuilt in
   phase 5. As some packages can have optional build deps that enable more
   features if they're present at build time.
