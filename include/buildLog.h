#ifndef ADAE6796_9022_48E8_8C7A_E4F5D6CEF296
#define ADAE6796_9022_48E8_8C7A_E4F5D6CEF296

#include <stdbool.h>
#include <time.h>
#include "config.h"
#include "phase.h"

typedef struct {
    time_t timestamp;
    char *name;
    char *version;
    BuildPhase phase;
    bool built;
} BuildRecord;

// Append only log of every package we attempted:
// <buildPath>/recipes/build_state.log
// timestamp | name | version | phase int (0-4) | built (true/false)
bool buildLogAppend(Config cfg, const BuildRecord *rec);

#endif /* ADAE6796_9022_48E8_8C7A_E4F5D6CEF296 */
