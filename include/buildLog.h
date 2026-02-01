#ifndef ADAE6796_9022_48E8_8C7A_E4F5D6CEF296
#define ADAE6796_9022_48E8_8C7A_E4F5D6CEF296

#include <stdbool.h>
#include <time.h>

#include "phase.h"

typedef struct {
    time_t timestamp;
    char *name;
    char *version;
    BuildPhase phase;
    bool built;
} BuildRecord;

#endif /* ADAE6796_9022_48E8_8C7A_E4F5D6CEF296 */
