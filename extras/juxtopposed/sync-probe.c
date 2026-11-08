/* Copyright (C) 2026 VLC authors and VideoLAN. GPL version 2 or later.
 *
 * A probe of what reaches the disk, for extras/juxtopposed/savefile-sync-test.cpp:
 * preloaded (LD_PRELOAD), it notes each fsync and fdatasync (the file of
 * the descriptor), each rename and each linkat, in their order, then calls
 * the real ones.
 * The test reads the notes through sync_probe_log().
 *   gcc -shared -fPIC -O1 extras/juxtopposed/sync-probe.c -o sync-probe.so -ldl
 */
#define _GNU_SOURCE
#include <dlfcn.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static char notes[1 << 16];
static size_t used;

static void note(const char *what, const char *a, const char *b)
{
    const int n = snprintf(notes + used, sizeof(notes) - used, "%s %s%s%s\n", what, a, b ? " -> " : "", b ? b : "");
    if (n > 0 && used + (size_t)n < sizeof(notes))
        used += (size_t)n;
}

static void noteFd(const char *what, int fd)
{
    char link[64], path[PATH_MAX];
    snprintf(link, sizeof(link), "/proc/self/fd/%d", fd);
    const ssize_t length = readlink(link, path, sizeof(path) - 1);
    path[length > 0 ? length : 0] = '\0';
    note(what, path, NULL);
}

const char *sync_probe_log(void)
{
    return notes;
}

int fsync(int fd)
{
    static int (*real)(int);
    if (!real)
        real = (int (*)(int))dlsym(RTLD_NEXT, "fsync");
    noteFd("sync", fd);
    return real(fd);
}

int fdatasync(int fd)
{
    static int (*real)(int);
    if (!real)
        real = (int (*)(int))dlsym(RTLD_NEXT, "fdatasync");
    noteFd("sync", fd);
    return real(fd);
}

int rename(const char *from, const char *to)
{
    static int (*real)(const char *, const char *);
    if (!real)
        real = (int (*)(const char *, const char *))dlsym(RTLD_NEXT, "rename");
    note("rename", from, to);
    return real(from, to);
}

int renameat(int fromDir, const char *from, int toDir, const char *to)
{
    static int (*real)(int, const char *, int, const char *);
    if (!real)
        real = (int (*)(int, const char *, int, const char *))dlsym(RTLD_NEXT, "renameat");
    note("rename", from, to);
    return real(fromDir, from, toDir, to);
}

/* (an O_TMPFILE file made visible: linkat of /proc/self/fd/N, or of the
 * descriptor itself with AT_EMPTY_PATH; noted by the name of that file) */
int linkat(int fromDir, const char *from, int toDir, const char *to, int flags)
{
    static int (*real)(int, const char *, int, const char *, int);
    if (!real)
        real = (int (*)(int, const char *, int, const char *, int))dlsym(RTLD_NEXT, "linkat");
    char path[PATH_MAX];
    ssize_t length = -1;
    if (strncmp(from, "/proc/self/fd/", 14) == 0)
        length = readlink(from, path, sizeof(path) - 1);
    else if (from[0] == '\0') {
        char link[64];
        snprintf(link, sizeof(link), "/proc/self/fd/%d", fromDir);
        length = readlink(link, path, sizeof(path) - 1);
    }
    if (length > 0)
        path[length] = '\0';
    else
        snprintf(path, sizeof(path), "%s", from);
    note("link", path, to);
    return real(fromDir, from, toDir, to, flags);
}

int renameat2(int fromDir, const char *from, int toDir, const char *to, unsigned int flags)
{
    static int (*real)(int, const char *, int, const char *, unsigned int);
    if (!real)
        real = (int (*)(int, const char *, int, const char *, unsigned int))dlsym(RTLD_NEXT, "renameat2");
    note("rename", from, to);
    return real(fromDir, from, toDir, to, flags);
}
