#ifndef LARPER_EXEC_EMBEDDED_H
#define LARPER_EXEC_EMBEDDED_H

#define _GNU_SOURCE
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/wait.h>
#include <unistd.h>

/*
 * Executes an embedded bash script from memory (via memfd_create with /tmp fallback)
 * passing through all CLI arguments transparently.
 */
static inline int run_embedded_bash(const char *name,
                                    const unsigned char *script,
                                    size_t script_len,
                                    int argc, char **argv) {
    char path[128];
    int fd = -1;
    int used_tmp = 0;

#ifdef SYS_memfd_create
    fd = (int)syscall(SYS_memfd_create, name, 0);
    if (fd >= 0) {
        snprintf(path, sizeof(path), "/proc/self/fd/%d", fd);
    }
#endif
    if (fd < 0) {
        snprintf(path, sizeof(path), "/tmp/larper-%s-XXXXXX", name);
        fd = mkstemp(path);
        if (fd < 0) {
            perror(name);
            return 1;
        }
        used_tmp = 1;
    }

    size_t written = 0;
    while (written < script_len) {
        ssize_t n = write(fd, script + written, script_len - written);
        if (n < 0) {
            if (errno == EINTR) continue;
            perror(name);
            close(fd);
            if (used_tmp) unlink(path);
            return 1;
        }
        written += (size_t)n;
    }
    if (lseek(fd, 0, SEEK_SET) < 0) {
        /* ignore */
    }

    char **new_argv = calloc((size_t)argc + 3, sizeof(char *));
    if (!new_argv) {
        close(fd);
        if (used_tmp) unlink(path);
        return 1;
    }
    new_argv[0] = "bash";
    new_argv[1] = path;
    for (int i = 1; i < argc; i++) {
        new_argv[i + 1] = argv[i];
    }
    new_argv[argc + 1] = NULL;

    if (!used_tmp) {
        execvp("bash", new_argv);
        perror("execvp(bash)");
        free(new_argv);
        close(fd);
        return 127;
    }

    pid_t pid = fork();
    if (pid == 0) {
        close(fd);
        execvp("bash", new_argv);
        perror("execvp(bash)");
        _exit(127);
    }
    free(new_argv);
    close(fd);
    int status = 0;
    if (pid > 0) waitpid(pid, &status, 0);
    unlink(path);
    return WIFEXITED(status) ? WEXITSTATUS(status) : 1;
}

#endif /* LARPER_EXEC_EMBEDDED_H */
