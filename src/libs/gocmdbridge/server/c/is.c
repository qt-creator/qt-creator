// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
//
// Is and issamefile handlers.
// Included by cmdbridge.c -- do not compile separately.

static void h_issamefile(value *cmd)
{
    value *sf = mfind(cmd, "IsSameFile");
    const char *p1 = sf ? mstr(sf, "Path1") : mstr(cmd, "Path1");
    const char *p2 = sf ? mstr(sf, "Path2") : mstr(cmd, "Path2");
    if (!p1 || !p2) {
        send_err(mkey(cmd, "Id"), "missing Path1 or Path2");
        return;
    }
    bool same = false;
    if (plat_same_file(p1, p2, &same) != 0) {
        send_os_err(mkey(cmd, "Id"), strerror(errno), errno);
        return;
    }
    value *m = mk3("Type", vs("issamefileresult"), "Id", vi(mkey(cmd, "Id")), "Result", vb(same));
    size_t l;
    uint8_t *c = encode(m, &l);
    if (c) {
        send_pkt(c, l);
        free(c);
    }
    mfreekeys(m);
    vfree(m);
}

static void h_is(value *cmd)
{
    value *is_map = mfind(cmd, "Is");
    const char *path = is_map ? mstr(is_map, "Path") : mstr(cmd, "Path");
    int check = is_map ? mkey(is_map, "Check") : mkey(cmd, "Check");
    if (!path) {
        send_err(mkey(cmd, "Id"), "missing Path");
        return;
    }
    struct stat st;
    bool exists = (plat_stat(path, &st) == 0);

    if (!exists) {
        /* Symlink check uses lstat */
        if (check == 8) {
            if (plat_lstat(path, &st) == 0) {
                value *m = mk3(
                    "Type",
                    vs("isresult"),
                    "Id",
                    vi(mkey(cmd, "Id")),
                    "Result",
                    vb(S_ISLNK(st.st_mode)));
                size_t l;
                uint8_t *c = encode(m, &l);
                if (c) {
                    send_pkt(c, l);
                    free(c);
                }
                mfreekeys(m);
                vfree(m);
            } else {
                value *m
                    = mk3("Type", vs("isresult"), "Id", vi(mkey(cmd, "Id")), "Result", vb(false));
                size_t l;
                uint8_t *c = encode(m, &l);
                if (c) {
                    send_pkt(c, l);
                    free(c);
                }
                mfreekeys(m);
                vfree(m);
            }
        } else {
            value *m = mk3("Type", vs("isresult"), "Id", vi(mkey(cmd, "Id")), "Result", vb(false));
            size_t l;
            uint8_t *c = encode(m, &l);
            if (c) {
                send_pkt(c, l);
                free(c);
            }
            mfreekeys(m);
            vfree(m);
        }
        return;
    }
    bool r = false;
    switch (check) {
    case 0:
        r = is_readable(path);
        break;
    case 1:
        r = is_writable(path);
        break;
    case 2:
        r = is_executable(path);
        break;
    case 3:
        r = S_ISDIR(st.st_mode) && is_readable(path);
        break;
    case 4:
        r = S_ISDIR(st.st_mode) && is_writable(path);
        break;
    case 5:
        r = !S_ISDIR(st.st_mode);
        break;
    case 6:
        r = S_ISDIR(st.st_mode);
        break;
    case 7:
        r = true;
        break;
    case 8:
        r = S_ISLNK(st.st_mode);
        break;
    default:
        r = true;
        break;
    }
    value *m = mk3("Type", vs("isresult"), "Id", vi(mkey(cmd, "Id")), "Result", vb(r));
    size_t l;
    uint8_t *c = encode(m, &l);
    if (c) {
        send_pkt(c, l);
        free(c);
    }
    mfreekeys(m);
    vfree(m);
}
