// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
//
// Windows directory walk for the find command. Implements find_walk() as
// declared in find.c. Included by cmdbridge.c - do not compile separately.

#ifdef _WIN32
/* Matches one bracket expression, e.g. "[a-z]" or "[!0-9]". `pp` points just
   past the '[' and is advanced past the closing ']'. */
static bool find_glob_class(const char **pp, char c)
{
    const char *p = *pp;
    bool negated = false;
    if (*p == '!' || *p == '^') {
        negated = true;
        ++p;
    }
    bool matched = false;
    /* A ']' first in the list is a literal, as in fnmatch. */
    for (bool first = true; *p && (*p != ']' || first); first = false) {
        if (p[0] != ']' && p[1] == '-' && p[2] && p[2] != ']') {
            if ((unsigned char) c >= (unsigned char) p[0]
                && (unsigned char) c <= (unsigned char) p[2])
                matched = true;
            p += 3;
        } else {
            if (c == *p)
                matched = true;
            ++p;
        }
    }
    if (*p == ']')
        ++p;
    *pp = p;
    return matched != negated;
}

/* Glob match with '*', '?' and bracket expressions, standing in for fnmatch(),
   which mingw does not provide. Backtracks on the last '*' so that patterns
   like "*a*b" work. */
static bool find_glob_match(const char *pat, const char *name)
{
    const char *star = NULL;
    const char *retry = NULL;

    while (*name) {
        if (*pat == '*') {
            star = ++pat;
            retry = name;
            continue;
        }
        bool hit;
        if (*pat == '?') {
            hit = true;
            ++pat;
        } else if (*pat == '[') {
            const char *p = pat + 1;
            hit = find_glob_class(&p, *name);
            pat = p;
        } else if (*pat != '\0' && *pat == *name) {
            hit = true;
            ++pat;
        } else {
            hit = false;
        }
        if (hit) {
            ++name;
            continue;
        }
        if (!star)
            return false;
        /* Let the '*' swallow one more character and try again from there. */
        pat = star;
        name = ++retry;
    }
    while (*pat == '*')
        ++pat;
    return *pat == '\0';
}

/* True when `name` passes any of the filters, or when there are none. Every
   filter has to be tried: returning on the first one made "*.cpp" hide the
   matches of "*.h" behind it. */
static bool find_match_name(const char *name, const char **filters, int nfilt)
{
    if (nfilt <= 0)
        return true;
    for (int i = 0; i < nfilt; i++) {
        if (filters[i] && find_glob_match(filters[i], name))
            return true;
    }
    return false;
}

static void find_walk(
    const char *dir, const char **filters, int nfilt, int ffilt, int iflags, int id)
{
    char search_path[PATH_MAX];
    find_join(search_path, sizeof(search_path), dir, "*", '\\');

    wchar_t wsearch[PATH_MAX];
    int wlen = MultiByteToWideChar(CP_UTF8, 0, search_path, -1, NULL, 0);
    if (wlen <= 0)
        return;
    wlen = MultiByteToWideChar(CP_UTF8, 0, search_path, -1, wsearch, wlen);
    if (wlen <= 0)
        return;

    WIN32_FIND_DATAW fd;
    HANDLE h = FindFirstFileW(wsearch, &fd);
    if (h == INVALID_HANDLE_VALUE)
        return;

    while (true) {
        if (is_cancelled(id))
            break;

        if (fd.cFileName[0] == L'.'
            && (fd.cFileName[1] == '\0' || (fd.cFileName[1] == '.' && fd.cFileName[2] == '\0'))) {
            if (!FindNextFileW(h, &fd))
                break;
            continue;
        }

        bool is_dir = (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
        bool is_sym
            = ((fd.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0
               && fd.dwReserved0 == IO_REPARSE_TAG_SYMLINK);

        /* Resolve symlinks for directory check, matching Go find.go:135-143 */
        if (is_sym) {
            char nameA[8192];
            WideCharToMultiByte(CP_UTF8, 0, fd.cFileName, -1, nameA, sizeof(nameA), NULL, NULL);
            char pathA[PATH_MAX];
            find_join(pathA, sizeof(pathA), dir, nameA, '\\');
            wchar_t wfull[PATH_MAX];
            int wfulllen = MultiByteToWideChar(CP_UTF8, 0, pathA, -1, NULL, 0);
            if (wfulllen > 0) {
                wfulllen = MultiByteToWideChar(CP_UTF8, 0, pathA, -1, wfull, wfulllen);
                DWORD attr = GetFileAttributesW(wfull);
                if (attr != INVALID_FILE_ATTRIBUTES)
                    is_dir = (attr & FILE_ATTRIBUTE_DIRECTORY) != 0;
            }
        }

        /* Whether the entry is reported; see the POSIX branch for why this is
           kept apart from whether the walk descends into it. Note every filter
           below only ever clears the flag: an early `continue` here would skip
           the FindNextFileW at the end of the loop and spin forever. */
        bool report = true;

        /* File type filter */
        if (find_filtering(ffilt)) {
            if (ffilt & FF_TYPEMASK) {
                bool want_dirs = (ffilt & FF_DIRS) != 0;
                bool want_files = (ffilt & FF_FILES) != 0;
                if (!want_dirs && is_dir)
                    report = false;
                if (!want_files && !is_dir)
                    report = false;
            }
            /* NoSymLinks: skip symlinks */
            if (report && (ffilt & FF_NOSYMLINKS) != 0 && is_sym)
                report = false;
            /* Permission filters - always true on Windows, so nothing to do. */
        }

        /* Convert file name to UTF-8 */
        char nameA[8192];
        WideCharToMultiByte(CP_UTF8, 0, fd.cFileName, -1, nameA, sizeof(nameA), NULL, NULL);

        /* Name filter */
        if (report && !find_match_name(nameA, filters, nfilt))
            report = false;

        char full[PATH_MAX];
        find_join(full, sizeof(full), dir, nameA, '\\');

        if (report) {
            /* Mode: approximate POSIX mode from Windows attributes */
            uint32_t mode;
            if (is_dir) {
                mode = GO_MODE_DIR | 0755;
            } else {
                mode = 0444;
                if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_READONLY))
                    mode |= 0222; /* writable by owner/group/other */
            }
            if (is_sym) {
                /* 0777, like win_fill_stat: the permission bits of a reparse
                   point describe the name, not the file it points at. */
                mode = (mode & ~0777u) | GO_MODE_SYMLINK | 0777;
            }

            find_emit(
                id,
                full,
                (int64_t) ((uint64_t) fd.nFileSizeHigh << 32 | fd.nFileSizeLow),
                mode,
                is_dir,
                (int64_t) win_filetime_to_unix(fd.ftLastWriteTime));
        }

        /* See the POSIX branch: descended only when subdirectories were asked
           for, and never through a symlink. Reporting the directory itself is
           a separate decision, made above. */
        if (is_dir && !is_sym && (iflags & 2) != 0) {
            find_batch_flush();
            find_walk(full, filters, nfilt, ffilt, iflags, id);
        }

        if (!FindNextFileW(h, &fd))
            break;
    }

    FindClose(h);
}
/* Lists the system drives, which is what an empty root means on Windows. */
static bool find_list_drives(const char *dir, int ffilt, int id)
{
    if (!(*dir == '\0' || strcmp(dir, "/") == 0 || strcmp(dir, "\\") == 0))
        return false;
    {
        DWORD drives = GetLogicalDrives();
        bool want_dirs = (ffilt == 0) || (ffilt & FF_DIRS);
        for (int i = 0; i < 26; i++) {
            if ((drives & (1 << i)) && want_dirs) {
                char drive[4] = {'A' + (char) i, ':', '\\', '\0'};
                value *m = mk7(
                    "Type",
                    vs("finddata"),
                    "Id",
                    vi(id),
                    "Path",
                    vs(drive),
                    "Size",
                    vi((int64_t) 0),
                    "Mode",
                    vu(GO_MODE_DIR | 0555),
                    "IsDir",
                    vb(true),
                    "ModTime",
                    vi((int64_t) 0));
                size_t l;
                uint8_t *c = encode(m, &l);
                if (c) {
                    find_batch_add(c, l);
                }
                mfreekeys(m);
                vfree(m);
            }
            if (is_cancelled(id))
                break;
        }
        find_batch_flush();
        value *end = mk2("Type", vs("findend"), "Id", vi(id));
        size_t l;
        uint8_t *c = encode(end, &l);
        if (c) {
            find_batch_add(c, l);
        }
        mfreekeys(end);
        vfree(end);
        clear_cancelled(id);
        find_batch_flush();
        find_batch_free();
        return true;
    }
    return false;
}

#endif /* _WIN32 */
