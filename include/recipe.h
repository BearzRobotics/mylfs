#ifndef BE242EA5_33D7_4833_8E93_AE49272194BC
#define BE242EA5_33D7_4833_8E93_AE49272194BC

#include <stddef.h>

typedef struct {
    char *name;
    char *version;
} Recipe;

typedef struct {
    Recipe *items;
    size_t len;
    size_t cap;
} RecipeList;


#endif /* BE242EA5_33D7_4833_8E93_AE49272194BC */
