#ifndef BB1B3E10_6CC6_4A64_8B40_D9BB5E73E37F
#define BB1B3E10_6CC6_4A64_8B40_D9BB5E73E37F
/*
    This is probably bad practice, but I seem to have a lot of functions I would like to reuse but
    don't want to justify a header/file for each of these.
*/


typedef enum {
    CROSS_TOOLS = 0,
    TEMP_TOOLS = 1,
    TEMP_SYSTEM = 2,
    LFS_BASE = 3,
    EXTRA_PACKAGES = 4,
} BuildPhase;

#endif /* BB1B3E10_6CC6_4A64_8B40_D9BB5E73E37F */
