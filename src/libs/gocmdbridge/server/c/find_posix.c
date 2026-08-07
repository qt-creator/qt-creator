// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
//
// POSIX directory walk for the find command. Implements find_walk() as
// declared in find.c. Included by cmdbridge.c - do not compile separately.

#ifndef _WIN32
#include <dirent.h>
#include <fnmatch.h>
static void find_walk(
    const char *dir, const char **filters, int nfilt, int ffilt, int iflags, int id)
{
    DIR *d = opendir(dir);
    if (!d)
        return;
    struct dirent *ent;
    while ((ent = readdir(d)) != NULL) {
        if (is_cancelled(id)) {
            closedir(d);
            return;
        }
        if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0)
            continue;
        char full[PATH_MAX];
        find_join(full, sizeof(full), dir, ent->d_name, '/');
        struct stat st;
        if (lstat(full, &st) != 0)
            continue;
        bool is_dir = S_ISDIR(st.st_mode);
        bool is_sym = S_ISLNK(st.st_mode);

        /* Resolve symlinks for permission filtering, matching Go find.go:135-143 */
        struct stat resolved_st;
        if (is_sym) {
            if (stat(full, &resolved_st) == 0) {
                st = resolved_st;
                is_dir = S_ISDIR(st.st_mode);
            }
        }

        /* Whether the entry is reported. A filter it does not pass must not
           end the walk below it: Go returns filepath.SkipDir only for a
           non-recursive listing, so a recursive find still descends into a
           directory it does not report. Deciding this separately is what keeps
           "all files below here" from stopping at the first directory. */
        bool report = true;

        /* File type filter (bitmask, matches Go FileFilters) */
        if (find_filtering(ffilt)) {
            /* Drives filter: skip device files unless Drives flag set */
            if ((ffilt & FF_DRIVES) == 0 && (S_ISCHR(st.st_mode) || S_ISBLK(st.st_mode)))
                report = false;
            /* Dirs/Files filter: if TypeMask bits are set, enforce them */
            if (report && (ffilt & FF_TYPEMASK)) {
                bool want_dirs = (ffilt & FF_DIRS) != 0;
                bool want_files = (ffilt & FF_FILES) != 0;
                if (!want_dirs && is_dir)
                    report = false;
                if (!want_files && !is_dir)
                    report = false;
            }
            /* NoSymLinks: skip symlinks (check original lstat result) */
            if (report && (ffilt & FF_NOSYMLINKS) != 0 && is_sym)
                report = false;
            /* Permission filters - use resolved stats when available */
            if (report && (ffilt & FF_READABLE) != 0 && !is_readable(full))
                report = false;
            if (report && (ffilt & FF_WRITABLE) != 0 && !is_writable(full))
                report = false;
            if (report && (ffilt & FF_EXECUTABLE) != 0 && !is_executable(full))
                report = false;
        }

        /* Name filter */
        if (report && nfilt > 0) {
            report = false;
            for (int i = 0; i < nfilt; i++)
                if (filters[i] && fnmatch(filters[i], ent->d_name, 0) == 0) {
                    report = true;
                    break;
                }
        }

        if (report)
            find_emit(id, full, (int64_t) st.st_size, go_file_mode(st.st_mode), is_dir,
                      (int64_t) st.st_mtime);

        /* Descend only when the client asked for subdirectories, and never
           through a symlink: a link pointing at an ancestor would otherwise
           loop until the path hits PATH_MAX. Go's walk does not follow them
           either. Note this is separate from whether the directory itself is
           reported above -- a non-recursive listing still names its
           subdirectories, which is how the file browser navigates. */
        if (is_dir && !is_sym && (iflags & 2) != 0) {
            find_batch_flush();
            find_walk(full, filters, nfilt, ffilt, iflags, id);
        }
    }
    closedir(d);
}
/* No drive roots outside Windows. */
static bool find_list_drives(const char *dir, int ffilt, int id)
{
    (void) dir;
    (void) ffilt;
    (void) id;
    return false;
}

#endif /* !_WIN32 */
