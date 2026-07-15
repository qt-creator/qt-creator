// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
//
// Windows file access layer: plat_* wrappers, UTF-16 path helpers and file
// attribute helpers.
// Included by fileaccess.c — do not compile separately.

#ifdef _WIN32
// clang-format off
#include <winsock2.h>
#include <afunix.h>
#include <bcrypt.h>
// clang-format on
#include <direct.h>
#include <fcntl.h>
#include <io.h>
#include <process.h>
#include <shlwapi.h>
#include <tlhelp32.h>
#include <windows.h>
#include <winioctl.h>
#include <ws2tcpip.h>
#ifndef _SSIZE_T_DEFINED
typedef intptr_t ssize_t;
#define _SSIZE_T_DEFINED
#endif
#ifndef NFDS_T_DEFINED
typedef unsigned int nfds_t;
#define NFDS_T_DEFINED
#endif
#ifndef S_IFLNK
#define S_IFLNK 0xA000 // POSIX S_IFLNK: symlink file type
#endif
#ifndef S_ISLNK
#define S_ISLNK(m) (((m) & S_IFMT) == S_IFLNK)
#endif

/* Explicit definitions for zig cross-compilation */
#ifndef REPARSE_DATA_BUFFER
typedef struct _REPARSE_DATA_BUFFER
{
    ULONG ReparseTag;
    USHORT ReparseDataLength;
    USHORT Reserved;
    union {
        struct
        {
            USHORT SubstituteNameOffset;
            USHORT SubstituteNameLength;
            USHORT PrintNameOffset;
            USHORT PrintNameLength;
            ULONG Flags;
            WCHAR PathBuffer[1];
        } ReparseDataBuffer;
        struct
        {
            UCHAR DataBuffer[1];
        } GenericReparseBuffer;
    } DUMMYUNIONNAME;
} REPARSE_DATA_BUFFER, *PREPARSE_DATA_BUFFER;
#endif

#ifndef IO_REPARSE_TAG_SYMLINK
#define IO_REPARSE_TAG_SYMLINK 0xA000000CL
#endif

#ifndef IO_REPARSE_TAG_MOUNT_POINT
#define IO_REPARSE_TAG_MOUNT_POINT 0xA0000003L
#endif

typedef HANDLE file_t;
#define INVALID_FILE INVALID_HANDLE_VALUE

/* The CRT spells this _environ. */
#ifndef environ
#define environ _environ
#endif

/* Map Win32 GetLastError() to POSIX errno for strerror() compatibility. */
static int win_to_errno(DWORD err)
{
    switch (err) {
    case ERROR_FILE_NOT_FOUND:
    case ERROR_PATH_NOT_FOUND:
    case ERROR_INVALID_DRIVE:
        return ENOENT;
    case ERROR_ACCESS_DENIED:
    case ERROR_SHARING_VIOLATION:
        return EACCES;
    case ERROR_ALREADY_EXISTS:
        return EEXIST;
    case ERROR_FILENAME_EXCED_RANGE:
        return ENAMETOOLONG;
    case ERROR_HANDLE_DISK_FULL:
        return ENOSPC;
    case ERROR_NOT_ENOUGH_MEMORY:
        return ENOMEM;
    case ERROR_INVALID_PARAMETER:
        return EINVAL;
    default:
        return EIO;
    }
}

static wchar_t *utf8_to_utf16(const char *utf8)
{
    if (!utf8)
        return NULL;
    int len = MultiByteToWideChar(CP_UTF8, 0, utf8, -1, NULL, 0);
    if (len <= 0)
        return NULL;
    wchar_t *wstr = (wchar_t *) malloc(len * sizeof(wchar_t));
    if (!wstr)
        return NULL;
    MultiByteToWideChar(CP_UTF8, 0, utf8, -1, wstr, len);
    for (size_t i = 0; wstr[i]; i++) {
        if (wstr[i] == L'/')
            wstr[i] = L'\\';
    }
    return wstr;
}

/* Converts to a wide path, adding the \\?\ prefix that lifts the MAX_PATH
   limit. `force` adds it even for short paths, which callers need when they
   append further components afterwards: the result may exceed MAX_PATH even
   though the path handed in did not. */
static wchar_t *utf8_to_utf16_ex(const char *utf8, bool force)
{
    if (!utf8)
        return NULL;

    wchar_t *wpath = utf8_to_utf16(utf8);
    if (!wpath)
        return NULL;

    /* Already an extended path: nothing to add. */
    if (wcslen(wpath) >= 4 && wpath[0] == L'\\' && wpath[1] == L'\\' && wpath[2] == L'?'
        && wpath[3] == L'\\')
        return wpath;

    /* Resolve to a full canonical path using GetFullPathNameW.
     * Unlike the ANSI version, the Unicode version can handle paths
     * that exceed MAX_PATH (260 chars). */
    wchar_t *full = NULL;
    DWORD needed = GetFullPathNameW(wpath, 0, NULL, NULL);
    if (needed > 0) {
        full = (wchar_t *) malloc(needed * sizeof(wchar_t));
        if (full) {
            DWORD result = GetFullPathNameW(wpath, needed, full, NULL);
            if (result == 0 || result >= needed) {
                free(full);
                full = NULL;
            }
        }
    }

    wchar_t *target = full ? full : wpath;
    size_t target_len = wcslen(target);
    BOOL need_prefix = FALSE;

    /* Add the \\?\ prefix for excessively long paths, or when asked to. */
    if (force || target_len >= MAX_PATH) {
        need_prefix = TRUE;
    }

    wchar_t *result = NULL;
    if (need_prefix) {
        /* Check if target is already a valid extended path. */
        BOOL already_extended = FALSE;
        if (target[0] == L'\\' && target[1] == L'\\' && target[2] == L'?' && target[3] == L'\\') {
            already_extended = TRUE;
        }
        /* Handle \\.\ device paths: \\.\C:\ ... -> \\?\C:\ ... */
        else if (target[0] == L'\\' && target[1] == L'\\' && target[2] == L'.' && target[3] == L'\\') {
            target[2] = L'?';
            already_extended = TRUE;
        }

        if (already_extended) {
            result = target;
        } else if (target[0] == L'\\' && target[1] == L'\\') {
            /* UNC path: \\server\share -> \\?\UNC\server\share */
            size_t unc_len = 8 + (target_len - 2) + 1;
            result = (wchar_t *) malloc(unc_len * sizeof(wchar_t));
            if (result) {
                wcscpy(result, L"\\\\?\\UNC\\");
                wcscpy(result + 8, target + 2);
            }
        } else {
            /* Drive path: C:\ ... -> \\?\C:\ ... */
            size_t long_len = 4 + target_len + 1;
            result = (wchar_t *) malloc(long_len * sizeof(wchar_t));
            if (result) {
                wcscpy(result, L"\\\\?\\");
                wcscpy(result + 4, target);
            }
        }
    } else {
        result = wpath;
    }

    /* Free buffers that are not the result. */
    if (wpath && wpath != result)
        free(wpath);
    if (full && full != result && full != wpath)
        free(full);

    return result;
}

static wchar_t *utf8_to_utf16_long(const char *utf8)
{
    return utf8_to_utf16_ex(utf8, false);
}

static file_t plat_open(const char *path, int flags, mode_t mode)
{
    (void) mode;
    wchar_t *wpath = utf8_to_utf16_long(path);
    if (!wpath) {
        errno = win_to_errno(GetLastError());
        return INVALID_FILE;
    }
    DWORD access = 0;
    if (!(flags & O_WRONLY) || (flags & O_RDWR))
        access |= GENERIC_READ;
    if (flags & O_WRONLY || flags & O_RDWR)
        access |= GENERIC_WRITE;
    DWORD disposition = OPEN_EXISTING;
    if (flags & O_CREAT)
        disposition = CREATE_ALWAYS;
    file_t h = CreateFileW(
        wpath,
        access,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        NULL,
        disposition,
        FILE_FLAG_BACKUP_SEMANTICS,
        NULL);
    free(wpath);
    if (h == INVALID_FILE)
        errno = win_to_errno(GetLastError());
    return h;
}

static ssize_t plat_read(file_t h, void *buf, size_t n)
{
    DWORD read;
    if (!ReadFile(h, buf, n, &read, NULL)) {
        errno = win_to_errno(GetLastError());
        return -1;
    }
    return (ssize_t) read;
}

static ssize_t plat_write(file_t h, const void *buf, size_t n)
{
    DWORD written;
    if (!WriteFile(h, buf, n, &written, NULL)) {
        errno = win_to_errno(GetLastError());
        return -1;
    }
    return (ssize_t) written;
}

static off_t plat_lseek(file_t h, off_t offset, int origin)
{
    LARGE_INTEGER li;
    li.QuadPart = offset;
    LARGE_INTEGER prev;
    BOOL ok = FALSE;
    if (origin == SEEK_SET)
        ok = SetFilePointerEx(h, li, &prev, FILE_BEGIN);
    else if (origin == SEEK_CUR)
        ok = SetFilePointerEx(h, li, &prev, FILE_CURRENT);
    else if (origin == SEEK_END)
        ok = SetFilePointerEx(h, li, &prev, FILE_END);
    if (!ok) {
        errno = win_to_errno(GetLastError());
        return -1;
    }
    LARGE_INTEGER cur;
    LARGE_INTEGER zero = {0};
    if (!SetFilePointerEx(h, zero, &cur, FILE_CURRENT)) {
        errno = win_to_errno(GetLastError());
        return -1;
    }
    return cur.QuadPart;
}

static int plat_close(file_t h)
{
    if (!CloseHandle(h)) {
        errno = win_to_errno(GetLastError());
        return -1;
    }
    return 0;
}

static int plat_mkdir(const char *path, mode_t mode)
{
    (void) mode;
    wchar_t *wpath = utf8_to_utf16_long(path);
    if (!wpath) {
        errno = win_to_errno(GetLastError());
        return -1;
    }
    int result = CreateDirectoryW(wpath, NULL) ? 0 : -1;
    free(wpath);
    if (result < 0)
        errno = win_to_errno(GetLastError());
    return result;
}

static int plat_symlink(const char *target, const char *link)
{
#ifndef SYMBOLIC_LINK_FLAG_ALLOW_UNPRIVILEGED_CREATE
#define SYMBOLIC_LINK_FLAG_ALLOW_UNPRIVILEGED_CREATE 0x2
#endif
    wchar_t *wtarget = utf8_to_utf16_long(target);
    wchar_t *wlink = utf8_to_utf16_long(link);
    BOOL ok = FALSE;
    if (wtarget && wlink)
        ok = CreateSymbolicLinkW(wlink, wtarget, SYMBOLIC_LINK_FLAG_ALLOW_UNPRIVILEGED_CREATE);
    free(wtarget);
    free(wlink);
    if (!ok) {
        errno = win_to_errno(GetLastError());
        return -1;
    }
    return 0;
}

static int plat_rename(const char *src, const char *dst)
{
    wchar_t *wsrc = utf8_to_utf16_long(src);
    wchar_t *wdst = utf8_to_utf16_long(dst);
    BOOL ok = FALSE;
    if (wsrc && wdst)
        ok = MoveFileExW(wsrc, wdst, MOVEFILE_REPLACE_EXISTING);
    free(wsrc);
    free(wdst);
    if (!ok) {
        errno = win_to_errno(GetLastError());
        return -1;
    }
    return 0;
}

static int plat_unlink(const char *path)
{
    wchar_t *wpath = utf8_to_utf16_long(path);
    int result = -1;
    if (wpath) {
        /* DeleteFileW returns nonzero on success, zero on failure */
        result = DeleteFileW(wpath) ? 0 : -1;
        free(wpath);
    }
    if (result < 0)
        errno = win_to_errno(GetLastError());
    return result;
}

/* Windows with zig/mingw - use real pthreads from mingw */
#include <pthread.h>
typedef void *(*pthread_func_t)(void *);

/* ================================================================== */
/*  File attribute helpers used by the stat/is handlers               */
/* ================================================================== */

static bool is_readable(const char *path)
{
    (void) path;
    return true;
}
static bool is_writable(const char *path)
{
    (void) path;
    return true;
}
static bool is_executable(const char *path)
{
    (void) path;
    return true;
}
static int nlinks(const char *path)
{
    (void) path;
    return 0;
}
static char *fid(const char *path)
{
    wchar_t *wpath = utf8_to_utf16_long(path);
    if (!wpath)
        return strdup("");
    HANDLE h = CreateFileW(
        wpath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
    free(wpath);
    if (h == INVALID_HANDLE_VALUE)
        return strdup("");
    FILE_ID_INFO info;
    char *r = NULL;
    if (GetFileInformationByHandleEx(h, FileIdInfo, &info, sizeof(info))) {
        char buf[128];
        snprintf(
            buf,
            sizeof(buf),
            "0x%llx:%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X",
            (unsigned long long) info.VolumeSerialNumber,
            info.FileId.Identifier[0],
            info.FileId.Identifier[1],
            info.FileId.Identifier[2],
            info.FileId.Identifier[3],
            info.FileId.Identifier[4],
            info.FileId.Identifier[5],
            info.FileId.Identifier[6],
            info.FileId.Identifier[7],
            info.FileId.Identifier[8],
            info.FileId.Identifier[9],
            info.FileId.Identifier[10],
            info.FileId.Identifier[11],
            info.FileId.Identifier[12],
            info.FileId.Identifier[13],
            info.FileId.Identifier[14],
            info.FileId.Identifier[15]);
        r = strdup(buf);
    }
    CloseHandle(h);
    return r ? r : strdup("");
}

static uint64_t fspace(const char *path)
{
    wchar_t *wpath = utf8_to_utf16_long(path);
    if (!wpath)
        return 0;
    ULARGE_INTEGER fb;
    uint64_t res = GetDiskFreeSpaceExW(wpath, &fb, NULL, NULL) ? (uint64_t) fb.QuadPart : 0;
    free(wpath);
    return res;
}
static char *fowner(const char *path)
{
    (void) path;
    return strdup("");
}
static int fowner_id(const char *path)
{
    (void) path;
    return -2;
}
static char *fgroup(const char *path)
{
    (void) path;
    return strdup("");
}
static int fgroup_id(const char *path)
{
    (void) path;
    return -2;
}

/* Windows compatibility shims for POSIX functions */
static inline int plat_lstat(const char *path, struct stat *st)
{
    wchar_t *wpath = utf8_to_utf16_long(path);
    if (!wpath)
        return -1;
    HANDLE h = CreateFileW(
        wpath,
        0,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        NULL,
        OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT,
        NULL);
    free(wpath);
    if (h == INVALID_HANDLE_VALUE) {
        errno = ENOENT;
        return -1;
    }
    BY_HANDLE_FILE_INFORMATION info;
    if (!GetFileInformationByHandle(h, &info)) {
        CloseHandle(h);
        errno = EIO;
        return -1;
    }
    CloseHandle(h);

    memset(st, 0, sizeof(struct stat));
    st->st_mode = S_IFREG;
    if (info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        st->st_mode = S_IFDIR;
    if (info.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)
        st->st_mode = S_IFLNK;

    /* Parenthesise the whole expression: the cast binds tighter than "|", so
       the original truncated the high word wherever off_t is 32 bit. */
    st->st_size = (off_t) (((uint64_t) info.nFileSizeHigh << 32) | info.nFileSizeLow);
    st->st_mtime = (time_t) (info.ftLastWriteTime.dwLowDateTime
                             | ((uint64_t) info.ftLastWriteTime.dwHighDateTime << 32))
                   / 10000000;
    return 0;
}

static inline int plat_stat(const char *path, struct stat *st)
{
    wchar_t *wpath = utf8_to_utf16_long(path);
    if (!wpath)
        return -1;
    HANDLE h = CreateFileW(
        wpath,
        0,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        NULL,
        OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS,
        NULL);
    free(wpath);
    if (h == INVALID_HANDLE_VALUE) {
        errno = ENOENT;
        return -1;
    }
    BY_HANDLE_FILE_INFORMATION info;
    if (!GetFileInformationByHandle(h, &info)) {
        CloseHandle(h);
        errno = EIO;
        return -1;
    }
    CloseHandle(h);

    memset(st, 0, sizeof(struct stat));
    st->st_mode = S_IFREG;
    if (info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        st->st_mode = S_IFDIR;
    if (info.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)
        st->st_mode = S_IFLNK;

    /* Parenthesise the whole expression: the cast binds tighter than "|", so
       the original truncated the high word wherever off_t is 32 bit. */
    st->st_size = (off_t) (((uint64_t) info.nFileSizeHigh << 32) | info.nFileSizeLow);
    st->st_mtime = (time_t) (info.ftLastWriteTime.dwLowDateTime
                             | ((uint64_t) info.ftLastWriteTime.dwHighDateTime << 32))
                   / 10000000;
    return 0;
}

static inline ssize_t plat_readlink(const char *path, char *buf, size_t bufsiz)
{
    wchar_t *wpath = utf8_to_utf16_long(path);
    if (!wpath) {
        errno = ENOENT;
        return -1;
    }
    HANDLE h = CreateFileW(
        wpath,
        0,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        NULL,
        OPEN_EXISTING,
        FILE_FLAG_OPEN_REPARSE_POINT | FILE_FLAG_BACKUP_SEMANTICS,
        NULL);
    free(wpath);
    if (h == INVALID_HANDLE_VALUE) {
        errno = ENOENT;
        return -1;
    }
    unsigned char reparseBuf[4096];
    DWORD bytesReturned;
    BOOL ok = DeviceIoControl(
        h, FSCTL_GET_REPARSE_POINT, NULL, 0, reparseBuf, sizeof(reparseBuf), &bytesReturned, NULL);
    CloseHandle(h);
    if (!ok) {
        errno = EACCES;
        return -1;
    }
    if (bytesReturned < sizeof(REPARSE_DATA_BUFFER)) {
        errno = EACCES;
        return -1;
    }
    REPARSE_DATA_BUFFER *rdb = (REPARSE_DATA_BUFFER *) reparseBuf;
    if (rdb->ReparseTag != IO_REPARSE_TAG_SYMLINK && rdb->ReparseTag != IO_REPARSE_TAG_MOUNT_POINT) {
        errno = EACCES;
        return -1;
    }
    wchar_t *substituteName = (wchar_t *) ((char *) rdb
                                           + rdb->ReparseDataBuffer.SubstituteNameOffset);
    int wcsLen = rdb->ReparseDataBuffer.SubstituteNameLength / sizeof(wchar_t);
    if (wcsLen <= 0 || wcsLen >= (int) bufsiz) {
        errno = EACCES;
        return -1;
    }
    int utf8Len
        = WideCharToMultiByte(CP_UTF8, 0, substituteName, wcsLen, buf, (int) bufsiz - 1, NULL, NULL);
    if (utf8Len <= 0) {
        errno = EACCES;
        return -1;
    }
    buf[utf8Len] = '\0';
    return (ssize_t) utf8Len;
}




/* Reports whether two paths refer to the same file, by file id rather than
   inode. Returns 0, or -1 with errno set. */
static int plat_same_file(const char *p1, const char *p2, bool *same)
{
    wchar_t *w1 = utf8_to_utf16_long(p1);
    wchar_t *w2 = utf8_to_utf16_long(p2);
    HANDLE h1 = INVALID_HANDLE_VALUE, h2 = INVALID_HANDLE_VALUE;
    int rc = -1;

    if (!w1 || !w2) {
        errno = ENOENT;
        goto out;
    }
    h1 = CreateFileW(
        w1, 0, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS, NULL);
    h2 = CreateFileW(
        w2, 0, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS, NULL);
    if (h1 == INVALID_HANDLE_VALUE || h2 == INVALID_HANDLE_VALUE) {
        errno = win_to_errno(GetLastError());
        goto out;
    }

    FILE_ID_INFO i1, i2;
    if (!GetFileInformationByHandleEx(h1, FileIdInfo, &i1, sizeof(i1))
        || !GetFileInformationByHandleEx(h2, FileIdInfo, &i2, sizeof(i2))) {
        errno = win_to_errno(GetLastError());
        goto out;
    }
    *same = (i1.VolumeSerialNumber == i2.VolumeSerialNumber
             && memcmp(i1.FileId.Identifier, i2.FileId.Identifier, 16) == 0);
    rc = 0;

out:
    if (h1 != INVALID_HANDLE_VALUE)
        CloseHandle(h1);
    if (h2 != INVALID_HANDLE_VALUE)
        CloseHandle(h2);
    free(w1);
    free(w2);
    return rc;
}

/* Removes the contents of the directory `wdir` refers to, then the directory
   itself. Works on wide paths throughout: the CRT directory functions cannot
   handle paths past MAX_PATH or names outside the active code page, which is
   exactly what utf8_to_utf16_long() produces. */
static int win_rmtree_w(wchar_t *wdir, size_t wdir_len)
{
    size_t pattern_len = wdir_len + 3;
    wchar_t *pattern = (wchar_t *) malloc(pattern_len * sizeof(wchar_t));
    if (!pattern) {
        errno = ENOMEM;
        return -1;
    }
    memcpy(pattern, wdir, wdir_len * sizeof(wchar_t));
    pattern[wdir_len] = L'\\';
    pattern[wdir_len + 1] = L'*';
    pattern[wdir_len + 2] = L'\0';

    WIN32_FIND_DATAW fd;
    HANDLE h = FindFirstFileW(pattern, &fd);
    free(pattern);
    if (h == INVALID_HANDLE_VALUE) {
        errno = win_to_errno(GetLastError());
        return -1;
    }

    int rc = 0;
    int first_errno = 0;
    do {
        if (fd.cFileName[0] == L'.'
            && (fd.cFileName[1] == L'\0' || (fd.cFileName[1] == L'.' && fd.cFileName[2] == L'\0')))
            continue;

        size_t name_len = wcslen(fd.cFileName);
        size_t child_len = wdir_len + 1 + name_len;
        wchar_t *child = (wchar_t *) malloc((child_len + 1) * sizeof(wchar_t));
        if (!child) {
            rc = -1;
            first_errno = first_errno ? first_errno : ENOMEM;
            continue;
        }
        memcpy(child, wdir, wdir_len * sizeof(wchar_t));
        child[wdir_len] = L'\\';
        memcpy(child + wdir_len + 1, fd.cFileName, (name_len + 1) * sizeof(wchar_t));

        bool is_dir = (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
        bool is_link = (fd.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0;
        BOOL ok;
        if (is_dir && !is_link) {
            ok = win_rmtree_w(child, child_len) == 0;
        } else if (is_dir) {
            ok = RemoveDirectoryW(child); /* a directory symlink: unlink it */
        } else {
            /* Read-only files refuse to be deleted; clear the bit first. */
            if (fd.dwFileAttributes & FILE_ATTRIBUTE_READONLY)
                SetFileAttributesW(child, fd.dwFileAttributes & ~FILE_ATTRIBUTE_READONLY);
            ok = DeleteFileW(child);
        }
        if (!ok) {
            rc = -1;
            if (!first_errno)
                first_errno = win_to_errno(GetLastError());
        }
        free(child);
    } while (FindNextFileW(h, &fd));
    FindClose(h);

    if (!RemoveDirectoryW(wdir)) {
        rc = -1;
        if (!first_errno)
            first_errno = win_to_errno(GetLastError());
    }
    if (rc != 0)
        errno = first_errno ? first_errno : EIO;
    return rc;
}

static int plat_rmtree(const char *dir)
{
    /* Forced: the directory itself may be short while its contents are not. */
    wchar_t *wdir = utf8_to_utf16_ex(dir, true);
    if (!wdir) {
        errno = ENOENT;
        return -1;
    }
    int rc = win_rmtree_w(wdir, wcslen(wdir));
    int saved = errno;
    free(wdir);
    errno = saved;
    return rc;
}

/* Random bytes for temporary names. GetSystemTimeAsFileTime has a ~15 ms
   granularity, so a burst of names derived from the clock would differ only by
   a counter; the point of generating our own names instead of calling
   GetTempFileNameW is that they must not be guessable. */
static bool plat_random_bytes(unsigned char *out, size_t len)
{
    if (BCryptGenRandom(NULL, out, (ULONG) len, BCRYPT_USE_SYSTEM_PREFERRED_RNG) == 0)
        return true;

    /* Should not happen; degrade to something unique-per-call rather than
       failing the command outright. */
    static volatile LONG counter = 0;
    FILETIME ft;
    GetSystemTimeAsFileTime(&ft);
    uint64_t s = ((uint64_t) ft.dwHighDateTime << 32) | ft.dwLowDateTime;
    s ^= (uint64_t) GetCurrentProcessId() << 17;
    s ^= (uint64_t) GetCurrentThreadId() << 33;
    s ^= (uint64_t) (uint32_t) InterlockedIncrement(&counter) * 0x9E3779B97F4A7C15ull;
    for (size_t i = 0; i < len; i++) {
        s ^= s >> 30;
        s *= 0xBF58476D1CE4E5B9ull;
        s ^= s >> 27;
        out[i] = (unsigned char) (s >> 32);
    }
    return true;
}

/* Creates `path`, failing with EEXIST if it is taken. CREATE_NEW and
   CreateDirectoryW are both atomic, which is what makes the retry loop in
   fileaccess.c safe against another process racing for the same name. */
static int plat_create_new_file(const char *path)
{
    wchar_t *wpath = utf8_to_utf16_long(path);
    if (!wpath) {
        errno = EINVAL;
        return -1;
    }
    HANDLE h = CreateFileW(
        wpath, GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
    DWORD err = GetLastError();
    free(wpath);
    if (h == INVALID_HANDLE_VALUE) {
        errno = win_to_errno(err);
        return -1;
    }
    CloseHandle(h);
    return 0;
}

static int plat_create_new_dir(const char *path)
{
    wchar_t *wpath = utf8_to_utf16_long(path);
    if (!wpath) {
        errno = EINVAL;
        return -1;
    }
    BOOL ok = CreateDirectoryW(wpath, NULL);
    DWORD err = GetLastError();
    free(wpath);
    if (!ok) {
        errno = win_to_errno(err);
        return -1;
    }
    return 0;
}

#endif /* _WIN32 */
