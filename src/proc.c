#define _GNU_SOURCE

#include <errno.h>
#include <fcntl.h>
#include <grp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "proc.h"
#include "user.h"

const char *hostBash(void) {
    if (access("/usr/bin/bash", X_OK) == 0) {
        return "/usr/bin/bash";
    }
    return "/bin/bash";
}

// Runs in the child only. Never returns.
static void childExec(const RunSpec *s, int inFd) {
    // stdin
    if (inFd < 0) {
        inFd = open("/dev/null", O_RDONLY);
    }
    if (inFd >= 0) {
        dup2(inFd, STDIN_FILENO);
        if (inFd != STDIN_FILENO) close(inFd);
    }

    // stdout + stderr -> log
    if (s->logPath) {
        int lf = open(s->logPath, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (lf < 0) {
            perror(s->logPath);
            _exit(126);
        }
        dup2(lf, STDOUT_FILENO);
        dup2(lf, STDERR_FILENO);
        if (lf > STDERR_FILENO) close(lf);
    }

    // The passwd lookup has to happen before chroot(), afterwards the
    // user database is no longer reachable.
    uid_t uid = 0;
    gid_t gid = 0;
    if (s->user) {
        if (!lookupUser(s->user, &uid, &gid)) {
            fprintf(stderr, "mylfs: user %s does not exist\n", s->user);
            _exit(126);
        }
    }

    if (s->chroot) {
        if (chroot(s->chroot) != 0 || chdir("/") != 0) {
            perror("chroot");
            _exit(126);
        }
    }

    if (s->user) {
        if (setgroups(1, &gid) != 0 ||
            setgid(gid) != 0 ||
            setuid(uid) != 0) {
            perror("drop privileges");
            _exit(126);
        }
    }

    if (s->cwd && chdir(s->cwd) != 0) {
        fprintf(stderr, "mylfs: cannot chdir to %s: %s\n", s->cwd, strerror(errno));
        _exit(126);
    }

    if (s->path) {
        execve(s->path, s->argv, s->envp ? s->envp : environ);
    } else {
        execvp(s->argv[0], s->argv);
    }
    fprintf(stderr, "mylfs: exec %s failed: %s\n", s->path ? s->path : s->argv[0], strerror(errno));
    _exit(127);
}

int runProcess(const RunSpec *spec) {
    int pipefd[2] = {-1, -1};

    if (spec->input && pipe(pipefd) != 0) {
        perror("pipe");
        return -1;
    }

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        if (spec->input) { close(pipefd[0]); close(pipefd[1]); }
        return -1;
    }

    if (pid == 0) {
        if (spec->input) close(pipefd[1]);
        childExec(spec, pipefd[0]);
    }

    // parent
    if (spec->input) {
        close(pipefd[0]);
        const char *p = spec->input;
        size_t left = strlen(spec->input);
        // The child's output goes to a file and not a pipe, so blocking
        // here cannot deadlock. If the child exits early we get EPIPE.
        while (left > 0) {
            ssize_t n = write(pipefd[1], p, left);
            if (n < 0) {
                if (errno == EINTR) continue;
                break;
            }
            p += n;
            left -= (size_t)n;
        }
        close(pipefd[1]);
    }

    int status = 0;
    while (waitpid(pid, &status, 0) < 0) {
        if (errno != EINTR) {
            perror("waitpid");
            return -1;
        }
    }

    if (WIFEXITED(status)) return WEXITSTATUS(status);
    if (WIFSIGNALED(status)) return 128 + WTERMSIG(status);
    return -1;
}
