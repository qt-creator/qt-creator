// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
//
// The platform file access layer that the command handlers are written
// against: file_t and INVALID_FILE, the plat_* wrappers around file I/O,
// paths, temporary names and recursive removal, plus the file attribute
// helpers (is_readable, fid, fspace, fowner, fgroup and friends).
//
// One of the two implementations below provides all of it.
// Included by cmdbridge.c -- do not compile separately.

#include "fileaccess_posix.c"
#include "fileaccess_win.c"

/* ================================================================== */
/*  Temporary names                                                   */
/* ================================================================== */

/* Appends a random suffix to `path` and creates it, retrying until the name is
   free. Keeps the caller's name as a prefix, the way os.CreateTemp does.

   The suffix comes from the platform's CSPRNG (see plat_random_bytes) rather
   than from libc's mkstemp or from GetTempFileNameW: a temporary in a shared
   directory whose name can be guessed lets another user pre-create the path.
   Creation is atomic and exclusive, so concurrent callers cannot end up with
   the same file even if they do pick the same name. */
#define TEMP_SUFFIX_LEN 8
#define TEMP_MAX_ATTEMPTS 128

static int plat_mktemp(char *path, size_t cap, int (*create)(const char *))
{
    static const char alphanum[] = "abcdefghijklmnopqrstuvwxyz0123456789";

    size_t base_len = strlen(path);
    if (base_len + TEMP_SUFFIX_LEN + 1 > cap) {
        errno = ENAMETOOLONG;
        return -1;
    }

    for (int attempt = 0; attempt < TEMP_MAX_ATTEMPTS; attempt++) {
        unsigned char rnd[TEMP_SUFFIX_LEN];
        if (!plat_random_bytes(rnd, sizeof(rnd))) {
            path[base_len] = '\0';
            errno = EIO;
            return -1;
        }
        for (size_t i = 0; i < sizeof(rnd); i++)
            path[base_len + i] = alphanum[rnd[i] % (sizeof(alphanum) - 1)];
        path[base_len + TEMP_SUFFIX_LEN] = '\0';

        if (create(path) == 0)
            return 0;
        if (errno != EEXIST) {
            path[base_len] = '\0';
            return -1;
        }
    }
    path[base_len] = '\0';
    errno = EEXIST;
    return -1;
}

static int plat_mktemp_file(char *path, size_t cap)
{
    return plat_mktemp(path, cap, plat_create_new_file);
}

static int plat_mktemp_dir(char *path, size_t cap)
{
    return plat_mktemp(path, cap, plat_create_new_dir);
}
