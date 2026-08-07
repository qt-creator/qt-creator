// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
//
// POSIX file access layer: plat_* wrappers and file attribute helpers.
// Included by fileaccess.c — do not compile separately.

#ifndef _WIN32
#include <dirent.h>
#include <fcntl.h>
#include <grp.h>
#include <pthread.h>
#include <pwd.h>
#include <sys/resource.h> /* getrlimit, for plat_raise_fd_limit() */

/* statfs, for fspace() */
#if defined(__linux__)
#include <sys/statfs.h>
#elif defined(__NetBSD__)
/* NetBSD has no statfs(), only the POSIX statvfs(). */
#include <sys/statvfs.h>
#else
#include <sys/mount.h>
#include <sys/param.h>
#endif

/* getentropy() is declared in <unistd.h> on Linux and the BSDs, but only in
   <sys/random.h> on macOS. */
#ifdef __APPLE__
#include <sys/random.h>
#endif

typedef void *(*pthread_func_t)(void *);

/* Not declared by <unistd.h> on every platform we build for. */
extern char **environ;

/* file_t: int on POSIX, HANDLE on Windows. Invalid = -1 on POSIX. */
typedef int file_t;
#define INVALID_FILE (-1)

/* Thin wrappers so callers always use plat_* names. */
static inline file_t plat_open(const char *p, int f, mode_t m) { return open(p, f, m); }
static inline ssize_t plat_read(file_t h, void *b, size_t n) { return read(h, b, n); }
static inline ssize_t plat_write(file_t h, const void *b, size_t n) { return write(h, b, n); }
static inline off_t plat_lseek(file_t h, off_t o, int w) { return lseek(h, o, w); }
static inline int plat_close(file_t h) { return close(h); }
static inline int plat_mkdir(const char *p, mode_t m) { return mkdir(p, m); }
static inline int plat_symlink(const char *t, const char *l) { return symlink(t, l); }
static inline int plat_rename(const char *s, const char *d) { return rename(s, d); }
static inline int plat_unlink(const char *p) { return unlink(p); }
static inline int plat_stat(const char *p, struct stat *st) { return stat(p, st); }
static inline int plat_lstat(const char *p, struct stat *st) { return lstat(p, st); }
static inline ssize_t plat_readlink(const char *p, char *buf, size_t n)
{
    return readlink(p, buf, n);
}

/* Reports whether two paths refer to the same file. Returns 0, or -1 with
   errno set. */
static int plat_same_file(const char *p1, const char *p2, bool *same)
{
    struct stat s1, s2;
    if (stat(p1, &s1) != 0 || stat(p2, &s2) != 0)
        return -1;
    *same = (s1.st_dev == s2.st_dev && s1.st_ino == s2.st_ino);
    return 0;
}

/* Removes the contents of `dir`, then `dir` itself. Returns 0, or -1 with
   errno set on the first failure. */
static int plat_rmtree(const char *dir)
{
    DIR *d = opendir(dir);
    if (!d)
        return -1;

    struct dirent *ent;
    char full[PATH_MAX];
    int rc = 0;
    int first_errno = 0;
    while ((ent = readdir(d)) != NULL) {
        if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0)
            continue;
        snprintf(full, sizeof(full), "%s/%s", dir, ent->d_name);

        struct stat s;
        int failed;
        if (lstat(full, &s) != 0)
            failed = 1;
        else if (S_ISDIR(s.st_mode)) /* lstat: a symlink is never S_ISDIR */
            failed = plat_rmtree(full) != 0;
        else
            failed = unlink(full) != 0;

        if (failed) {
            rc = -1;
            if (!first_errno)
                first_errno = errno;
        }
    }
    closedir(d);

    if (rmdir(dir) != 0) {
        rc = -1;
        if (!first_errno)
            first_errno = errno;
    }
    if (rc != 0)
        errno = first_errno ? first_errno : EIO;
    return rc;
}

/* Random bytes from the kernel.

   Note this deliberately does not use mkstemp()/mkdtemp(): the names those
   produce are only as good as the libc, and the Linux binary we ship is built
   against musl, whose __randname() draws from 32 characters and derives the
   last two of the six from addresses that do not change within a process --
   measured over 200 names, position 6 was constant and position 5 took five
   values. That is around 20 bits for a name that is supposed to be
   unguessable. Generating the name here and creating it with O_EXCL keeps the
   atomicity mkstemp gave us and makes the randomness the same everywhere. */
static bool plat_random_bytes(unsigned char *out, size_t len)
{
    if (getentropy(out, len) == 0)
        return true;

    int fd = open("/dev/urandom", O_RDONLY);
    if (fd < 0)
        return false;
    size_t got = 0;
    while (got < len) {
        ssize_t n = read(fd, out + got, len - got);
        if (n <= 0) {
            if (n < 0 && errno == EINTR)
                continue;
            break;
        }
        got += (size_t) n;
    }
    close(fd);
    return got == len;
}

/* Creates `path`, failing with EEXIST if it is taken. Both are 0700/0600 so a
   temporary is not readable by other users. */
static int plat_create_new_file(const char *path)
{
    int fd = open(path, O_RDWR | O_CREAT | O_EXCL, 0600);
    if (fd < 0)
        return -1;
    close(fd);
    return 0;
}

static int plat_create_new_dir(const char *path)
{
    return mkdir(path, 0700);
}

/* Raises the descriptor limit to the hard one, as the Go runtime does since
   Go 1.19. The kqueue watcher spends a descriptor per watched path, and the
   soft limit is low enough on some systems to cap the number of watches well
   below what a project needs: OpenBSD's default login class allows 128
   against a hard limit of 1024. */
static void plat_raise_fd_limit(void)
{
    struct rlimit rl;
    if (getrlimit(RLIMIT_NOFILE, &rl) != 0)
        return;
    if (rl.rlim_cur < rl.rlim_max) {
        rl.rlim_cur = rl.rlim_max;
        setrlimit(RLIMIT_NOFILE, &rl);
    }
}

/* ================================================================== */
/*  File attribute helpers used by the stat/is handlers               */
/* ================================================================== */

static bool is_readable(const char *p)
{
    return access(p, R_OK) == 0;
}
static bool is_writable(const char *p)
{
    return access(p, W_OK) == 0;
}
static bool is_executable(const char *p)
{
    return access(p, X_OK) == 0;
}
/* The caller's lstat already carries the count, so only a symlink needs the
   second call: the client asks about the file, not the name pointing at it,
   and Go resolves the link here too. */
static int nlinks(const char *path, const struct stat *st)
{
    if (!S_ISLNK(st->st_mode))
        return (int) st->st_nlink;
    struct stat s;
    return (stat(path, &s) == 0) ? (int) s.st_nlink : 0;
}
static char *fid(const char *path)
{
    struct stat s;
    if (stat(path, &s) != 0)
        return strdup("");
    char buf[64];
    snprintf(buf, sizeof(buf), "%x:%llx", (unsigned) s.st_dev, (unsigned long long) s.st_ino);
    return strdup(buf);
}
static uint64_t fspace(const char *path)
{
#ifdef __NetBSD__
    /* statvfs() counts f_bavail in f_frsize units, not f_bsize ones. */
    struct statvfs s;
    return (statvfs(path, &s) == 0) ? (uint64_t) s.f_bavail * (uint64_t) s.f_frsize : 0;
#else
    struct statfs s;
    return (statfs(path, &s) == 0) ? (uint64_t) s.f_bavail * (uint64_t) s.f_bsize : 0;
#endif
}
static char *fowner(const char *path)
{
    struct stat s;
    if (stat(path, &s) != 0)
        return strdup("");
    struct passwd *pw = getpwuid(s.st_uid);
    return pw ? strdup(pw->pw_name) : strdup("");
}
static int fowner_id(const char *path)
{
    struct stat s;
    return (stat(path, &s) == 0) ? (int) s.st_uid : -1;
}
static char *fgroup(const char *path)
{
    struct stat s;
    if (stat(path, &s) != 0)
        return strdup("");
    struct group *gr = getgrgid(s.st_gid);
    return gr ? strdup(gr->gr_name) : strdup("");
}
static int fgroup_id(const char *path)
{
    struct stat s;
    return (stat(path, &s) == 0) ? (int) s.st_gid : -1;
}

#endif /* !_WIN32 */
