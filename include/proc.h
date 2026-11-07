#ifndef MYLFS_PROC_H
#define MYLFS_PROC_H

// Everything mylfs runs on behalf of a recipe goes through runProcess().
// It forks, optionally chroots, drops privileges to a user, feeds a script
// to stdin and sends stdout+stderr to a log file.
typedef struct {
    const char *path;       // Program to exec. NULL -> argv[0] searched in PATH
    char *const *argv;      // NULL terminated
    char *const *envp;      // NULL terminated. NULL -> inherit our environment
    const char *input;      // Written to the child's stdin. NULL -> /dev/null
    const char *cwd;        // chdir() into this (inside the chroot if set)
    const char *chroot;     // chroot() into this dir before exec
    const char *user;       // setuid to this user before exec (NULL -> stay root)
    const char *logPath;    // stdout+stderr go here (truncated). NULL -> inherit
} RunSpec;

// Returns the child's exit status, 128+signal if it was killed or -1 if it
// could not be started.
int runProcess(const RunSpec *spec);

// "/usr/bin/bash" or "/bin/bash" - whichever the host has.
const char *hostBash(void);

#endif // MYLFS_PROC_H
