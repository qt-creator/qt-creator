// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
//
// Stat, readlink, fileid, freespace, group, owner, remove handlers.
// Included by cmdbridge.c — do not compile separately.

/* ================================================================== */
/*  Command handlers                                                  */
/* ================================================================== */

static void h_stat(value *cmd)
{
    value *stat_map = mfind(cmd, "Stat");
    const char *path = stat_map ? mstr(stat_map, "Path") : mstr(cmd, "Path");
    if (!path) {
        send_err(mkey(cmd, "Id"), "missing Path");
        return;
    }
    struct stat st;
    if (plat_lstat(path, &st) != 0) {
        send_os_err(mkey(cmd, "Id"), strerror(errno), errno);
        return;
    }
    uint32_t um = 0;
    if (is_readable(path))
        um |= 0x400;
    if (is_writable(path))
        um |= 0x200;
    if (is_executable(path))
        um |= 0x100;
    value *m = mk8(
        "Type",
        vs("statresult"),
        "Id",
        vi(mkey(cmd, "Id")),
        "Size",
        vi((int64_t) st.st_size),
        "Mode",
        vu(go_file_mode(st.st_mode)),
        "UserMode",
        vu(um),
        "ModTime",
        vi((int64_t) st.st_mtime),
        "IsDir",
        vb(S_ISDIR(st.st_mode)),
        "NumHardLinks",
        vi(nlinks(path)));
    size_t l;
    uint8_t *c = encode(m, &l);
    if (c) {
        send_pkt(c, l);
        free(c);
    }
    mfreekeys(m);
    vfree(m);
}

static void h_readlink(value *cmd)
{
    const char *path = mstr(cmd, "Path");
    if (!path) {
        send_err(mkey(cmd, "Id"), "missing Path");
        return;
    }
    char buf[PATH_MAX];
    ssize_t n = plat_readlink(path, buf, sizeof(buf) - 1);
    if (n < 0) {
        send_os_err(mkey(cmd, "Id"), strerror(errno), errno);
        return;
    }
    buf[n] = '\0';
    value *m = mk3("Type", vs("readlinkresult"), "Id", vi(mkey(cmd, "Id")), "Target", vs(buf));
    size_t l;
    uint8_t *c = encode(m, &l);
    if (c) {
        send_pkt(c, l);
        free(c);
    }
    mfreekeys(m);
    vfree(m);
}

static void h_fileid(value *cmd)
{
    const char *path = mstr(cmd, "Path");
    if (!path) {
        send_err(mkey(cmd, "Id"), "missing Path");
        return;
    }
    char *f = fid(path);
    value *m = mk3("Type", vs("fileidresult"), "Id", vi(mkey(cmd, "Id")), "FileId", vs(f));
    free(f);
    size_t l;
    uint8_t *c = encode(m, &l);
    if (c) {
        send_pkt(c, l);
        free(c);
    }
    mfreekeys(m);
    vfree(m);
}

static void h_freespace(value *cmd)
{
    const char *path = mstr(cmd, "Path");
    if (!path) {
        send_err(mkey(cmd, "Id"), "missing Path");
        return;
    }
    value *m = mk3(
        "Type", vs("freespaceresult"), "Id", vi(mkey(cmd, "Id")), "FreeSpace", vu(fspace(path)));
    size_t l;
    uint8_t *c = encode(m, &l);
    if (c) {
        send_pkt(c, l);
        free(c);
    }
    mfreekeys(m);
    vfree(m);
}

static void h_group(value *cmd)
{
    const char *path = mstr(cmd, "Path");
    if (!path) {
        send_err(mkey(cmd, "Id"), "missing Path");
        return;
    }
    char *g = fgroup(path);
    value *m = mk3("Type", vs("groupresult"), "Id", vi(mkey(cmd, "Id")), "Group", vs(g));
    free(g);
    size_t l;
    uint8_t *c = encode(m, &l);
    if (c) {
        send_pkt(c, l);
        free(c);
    }
    mfreekeys(m);
    vfree(m);
}

static void h_group_id(value *cmd)
{
    const char *path = mstr(cmd, "Path");
    if (!path) {
        send_err(mkey(cmd, "Id"), "missing Path");
        return;
    }
    value *m
        = mk3("Type", vs("groupidresult"), "Id", vi(mkey(cmd, "Id")), "GroupId", vi(fgroup_id(path)));
    size_t l;
    uint8_t *c = encode(m, &l);
    if (c) {
        send_pkt(c, l);
        free(c);
    }
    mfreekeys(m);
    vfree(m);
}

static void h_owner(value *cmd)
{
    const char *path = mstr(cmd, "Path");
    if (!path) {
        send_err(mkey(cmd, "Id"), "missing Path");
        return;
    }
    char *o = fowner(path);
    value *m = mk3("Type", vs("ownerresult"), "Id", vi(mkey(cmd, "Id")), "Owner", vs(o));
    free(o);
    size_t l;
    uint8_t *c = encode(m, &l);
    if (c) {
        send_pkt(c, l);
        free(c);
    }
    mfreekeys(m);
    vfree(m);
}

static void h_owner_id(value *cmd)
{
    const char *path = mstr(cmd, "Path");
    if (!path) {
        send_err(mkey(cmd, "Id"), "missing Path");
        return;
    }
    value *m
        = mk3("Type", vs("owneridresult"), "Id", vi(mkey(cmd, "Id")), "OwnerId", vi(fowner_id(path)));
    size_t l;
    uint8_t *c = encode(m, &l);
    if (c) {
        send_pkt(c, l);
        free(c);
    }
    mfreekeys(m);
    vfree(m);
}

static void h_remove(value *cmd)
{
    const char *path = mstr(cmd, "Path");
    if (!path) {
        send_err(mkey(cmd, "Id"), "missing Path");
        return;
    }
    if (plat_unlink(path) != 0) {
        send_os_err(mkey(cmd, "Id"), strerror(errno), errno);
        return;
    }
    send_void(mkey(cmd, "Id"), "removeresult");
}

/* Removes the contents of `dir`, then `dir` itself. Returns 0 on success, -1
   with errno set on the first failure, so that removeall can report it.
   Implemented per platform in fileaccess.c / fileaccess_win.c. */
static int plat_rmtree(const char *dir);

static void h_remove_all(value *cmd)
{
    const char *path = mstr(cmd, "Path");
    if (!path) {
        send_err(mkey(cmd, "Id"), "missing Path");
        return;
    }

    struct stat st;
    if (plat_lstat(path, &st) != 0) {
        /* Matches os.RemoveAll: removing a missing path is not an error. */
        if (errno == ENOENT) {
            send_void(mkey(cmd, "Id"), "removeallresult");
            return;
        }
        send_os_err(mkey(cmd, "Id"), strerror(errno), errno);
        return;
    }

    errno = 0;
    if (S_ISDIR(st.st_mode) && !S_ISLNK(st.st_mode)) {
        if (plat_rmtree(path) != 0) {
            send_os_err(mkey(cmd, "Id"), strerror(errno), errno);
            return;
        }
    } else if (plat_unlink(path) != 0) {
        send_os_err(mkey(cmd, "Id"), strerror(errno), errno);
        return;
    }
    send_void(mkey(cmd, "Id"), "removeallresult");
}

static void h_ensure_file(value *cmd)
{
    const char *path = mstr(cmd, "Path");
    if (!path) {
        send_err(mkey(cmd, "Id"), "missing Path");
        return;
    }
    struct stat st;
    if (plat_stat(path, &st) != 0) {
        file_t fd = plat_open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd == INVALID_FILE) {
            send_os_err(mkey(cmd, "Id"), strerror(errno), errno);
            return;
        }
        plat_close(fd);
    }
    send_void(mkey(cmd, "Id"), "ensureexistingfileresult");
}
