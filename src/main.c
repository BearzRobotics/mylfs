#include <stdio.h>
#include <string.h>
#include <yaml.h>

short int debug = 0;


int main(int argc, char* argv[]) {

    for (int i = 0; i < argc; i++) {
        if (strcmp(argv[i], "--debug") == 0) {
            debug = 1;
        }
    }

    if (debug == 1) {
        printf("Debug mode enabled!\n");
    }

    return 0;
}