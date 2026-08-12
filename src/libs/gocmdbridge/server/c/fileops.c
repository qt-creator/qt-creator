// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
//
// Copyfile, symlink, rename, temp dir/file, chmod, createdir handlers.
// Included by cmdbridge.c — do not compile separately.

#include <fcntl.h>

static int mkdir_all(const char *path, mode_t mode)
{
    if (!path || !*path)
        return -1;

    char *tmp = strdup(path);
    if (!tmp)
        return -1;

    size_t len = strlen(tmp);
    if (tmp[len - 1] == '\\' || tmp[len - 1] == '/')
        tmp[--len] = '\0';

    for (char *p = tmp + 1; *p; p++) {
        if (*p == '\\' || *p == '/') {
            *p = '\0';
#ifdef _WIN32
            /* Skip Windows drive letter segments (e.g., "C:") */
            if (strlen(tmp) == 2 && tmp[1] == ':') {
                *p = '/';
                continue;
            }
#endif
            if (plat_mkdir(tmp, mode) != 0 && errno != EEXIST) {
                free(tmp);
                return -1;
            }
            *p = '/';
        }
    }
    if (plat_mkdir(tmp, mode) != 0 && errno != EEXIST) {
        free(tmp);
        return -1;
    }
    free(tmp);
    return 0;
}

static void h_createdir(value *cmd)
{
    const char *path = mstr(cmd, "Path");
    if (!path) {
        send_err(mkey(cmd, "Id"), "missing Path");
        return;
    }
#ifdef _WIN32
    /* Resolve 8.3 short names so all createdir calls use the same long path.
     * This ensures mkdir_all uses consistent \\?\ paths for all segments. */
    const char *resolved_path = path;
    int need_free = 0;
    {
        int len = MultiByteToWideChar(CP_UTF8, 0, path, -1, NULL, 0);
        if (len > 0) {
            wchar_t *wpath = (wchar_t *) malloc(len * sizeof(wchar_t));
            if (wpath) {
                MultiByteToWideChar(CP_UTF8, 0, path, -1, wpath, len);
                wchar_t *resolved = (wchar_t *) malloc(4096 * sizeof(wchar_t));
                if (resolved && GetLongPathNameW(wpath, resolved, 4096) > 0) {
                    char *utf8_resolved = (char *) malloc(4096);
                    if (utf8_resolved) {
                        int rlen = WideCharToMultiByte(
                            CP_UTF8, 0, resolved, -1, utf8_resolved, 4096, NULL, NULL);
                        if (rlen > 0) {
                            resolved_path = utf8_resolved;
                            need_free = 1;
                        } else {
                            free(utf8_resolved);
                        }
                    }
                }
                free(resolved);
                free(wpath);
            }
        }
    }

    if (mkdir_all(resolved_path, 0755) != 0) {
        send_os_err(mkey(cmd, "Id"), strerror(errno), errno);
        if (need_free)
            free((void *) resolved_path);
        return;
    }
    if (need_free)
        free((void *) resolved_path);
#else
    if (mkdir_all(path, 0755) != 0) {
        send_os_err(mkey(cmd, "Id"), strerror(errno), errno);
        return;
    }
#endif
    send_void(mkey(cmd, "Id"), "createdirresult");
}

static void h_copyfile(value *cmd)
{
    value *cf = mfind(cmd, "CopyFile");
    const char *src = cf ? mstr(cf, "Source") : mstr(cmd, "Source");
    const char *dst = cf ? mstr(cf, "Target") : mstr(cmd, "Target");
    if (!src || !dst) {
        send_err(mkey(cmd, "Id"), "missing Source or Target");
        return;
    }
    file_t infd = plat_open(src, O_RDONLY, 0);
    if (infd == INVALID_FILE) {
        send_os_err(mkey(cmd, "Id"), strerror(errno), errno);
        return;
    }
    file_t outfd = plat_open(dst, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (outfd == INVALID_FILE) {
        plat_close(infd);
        send_os_err(mkey(cmd, "Id"), strerror(errno), errno);
        return;
    }
    uint8_t buf[32768];
    for (;;) {
        ssize_t n = plat_read(infd, buf, sizeof(buf));
        if (n < 0) {
            if (errno == EINTR)
                continue;
            /* A source that cannot be read - a directory, say - is an error,
               not an empty copy. */
            int e = errno;
            plat_close(infd);
            plat_close(outfd);
            send_os_err(mkey(cmd, "Id"), strerror(e), e);
            return;
        }
        if (n == 0)
            break;
        ssize_t w = 0;
        while (w < n) {
            ssize_t wn = plat_write(outfd, buf + w, (size_t) (n - w));
            if (wn < 0 && errno == EINTR)
                continue;
            if (wn <= 0) {
                int e = wn < 0 ? errno : EIO;
                plat_close(infd);
                plat_close(outfd);
                send_os_err(mkey(cmd, "Id"), strerror(e), e);
                return;
            }
            w += wn;
        }
    }
    plat_close(infd);
    plat_close(outfd);
    /* Copy execute permissions from source (matches Go: copy 0111 bits) */
#ifdef _WIN32
    {
        wchar_t *ws = utf8_to_utf16_long(src);
        DWORD sa = ws ? GetFileAttributesW(ws) : 0;
        if (sa != INVALID_FILE_ATTRIBUTES) {
            bool src_exec = !(sa & FILE_ATTRIBUTE_READONLY);
            wchar_t *wd = utf8_to_utf16_long(dst);
            DWORD da = wd ? GetFileAttributesW(wd) : 0;
            if (da != INVALID_FILE_ATTRIBUTES) {
                if (!src_exec && (da & FILE_ATTRIBUTE_READONLY) == 0) {
                    SetFileAttributesW(wd, da | FILE_ATTRIBUTE_READONLY);
                } else if (src_exec && (da & FILE_ATTRIBUTE_READONLY)) {
                    SetFileAttributesW(wd, da & ~FILE_ATTRIBUTE_READONLY);
                }
            }
            free(wd);
        }
        free(ws);
    }
#else
    {
        struct stat ss;
        if (stat(src, &ss) == 0) {
            struct stat ds;
            if (stat(dst, &ds) == 0) {
                mode_t nm = (ds.st_mode & ~0111) | (ss.st_mode & 0111);
                chmod(dst, nm);
            }
        }
    }
#endif
    send_void(mkey(cmd, "Id"), "copyfileresult");
}

static void h_symlink(value *cmd)
{
    value *cs = mfind(cmd, "CreateSymLink");
    const char *target = cs ? mstr(cs, "Source") : mstr(cmd, "Source");
    const char *link = cs ? mstr(cs, "SymLink") : mstr(cmd, "SymLink");
    if (!target || !link) {
        send_err(mkey(cmd, "Id"), "missing Source or SymLink");
        return;
    }
    if (plat_symlink(target, link) != 0) {
        send_os_err(mkey(cmd, "Id"), strerror(errno), errno);
        return;
    }
    send_void(mkey(cmd, "Id"), "createsymlinkresult");
}

static void h_rename(value *cmd)
{
    value *rn = mfind(cmd, "RenameFile");
    const char *src = rn ? mstr(rn, "Source") : mstr(cmd, "Source");
    const char *dst = rn ? mstr(rn, "Target") : mstr(cmd, "Target");
    if (!src || !dst) {
        send_err(mkey(cmd, "Id"), "missing Source or Target");
        return;
    }
    if (plat_rename(src, dst) != 0) {
        send_os_err(mkey(cmd, "Id"), strerror(errno), errno);
        return;
    }
    send_void(mkey(cmd, "Id"), "renamefileresult");
}

static void h_mktmpdir(value *cmd)
{
    const char *path = mstr(cmd, "Path");
    if (!path) {
        send_err(mkey(cmd, "Id"), "missing Path");
        return;
    }
    char tmp[PATH_MAX];
    snprintf(tmp, sizeof(tmp), "%s", path);

    if (plat_mktemp_dir(tmp, sizeof(tmp)) != 0) {
        send_os_err(mkey(cmd, "Id"), strerror(errno), errno);
        return;
    }

    value *m = mk3("Type", vs("createtempdirresult"), "Id", vi(mkey(cmd, "Id")), "Path", vs(tmp));
    size_t l;
    uint8_t *c = encode(m, &l);
    if (c) {
        send_pkt(c, l);
        free(c);
    }
    mfreekeys(m);
    vfree(m);
}

static void h_mktmpfile(value *cmd)
{
    const char *path = mstr(cmd, "Path");
    if (!path) {
        send_err(mkey(cmd, "Id"), "missing Path");
        return;
    }
    char tmp[PATH_MAX];
    snprintf(tmp, sizeof(tmp), "%s", path);

    if (plat_mktemp_file(tmp, sizeof(tmp)) != 0) {
        send_os_err(mkey(cmd, "Id"), strerror(errno), errno);
        return;
    }

    value *m = mk3("Type", vs("createtempfileresult"), "Id", vi(mkey(cmd, "Id")), "Path", vs(tmp));
    size_t l;
    uint8_t *c = encode(m, &l);
    if (c) {
        send_pkt(c, l);
        free(c);
    }
    mfreekeys(m);
    vfree(m);
}

static void h_chmod(value *cmd)
{
#ifdef _WIN32
    /* Windows has no POSIX permissions — no-op, matching os.Chmod on Windows */
    send_void(mkey(cmd, "Id"), "setpermissionsresult");
#else
    value *sp = mfind(cmd, "SetPermissions");
    const char *path = sp ? mstr(sp, "Path") : mstr(cmd, "Path");
    uint32_t mode = (uint32_t) (sp ? muint(sp, "Mode", 0) : muint(cmd, "Mode", 0));
    if (!path) {
        send_err(mkey(cmd, "Id"), "missing Path");
        return;
    }
    if (chmod(path, (mode_t) mode) != 0) {
        send_os_err(mkey(cmd, "Id"), strerror(errno), errno);
        return;
    }
    send_void(mkey(cmd, "Id"), "setpermissionsresult");
#endif
}
