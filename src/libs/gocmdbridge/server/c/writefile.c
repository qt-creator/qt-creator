// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
//
// Writefile handler.
// Included by cmdbridge.c — do not compile separately.

#include <fcntl.h>

static void h_writefile(value *cmd)
{
    value *wf = mfind(cmd, "WriteFile");
    const char *path = wf ? mstr(wf, "Path") : mstr(cmd, "Path");
    value *contents = wf ? mfind(wf, "Contents") : mfind(cmd, "Contents");
    if (!path) {
        send_err(mkey(cmd, "Id"), "missing Path");
        return;
    }
    if (!contents || contents->type != V_BYTES) {
        send_err(mkey(cmd, "Id"), "missing Contents");
        return;
    }
    file_t fd = plat_open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd == INVALID_FILE) {
        send_os_err(mkey(cmd, "Id"), strerror(errno), errno);
        return;
    }
    size_t to_write = contents->nkids;
    const uint8_t *ptr = contents->bytes;
    while (to_write > 0) {
        ssize_t n = plat_write(fd, ptr, to_write);
        if (n <= 0)
            break;
        to_write -= (size_t) n;
        ptr += n;
    }
    plat_close(fd);
    /* Match Go: return expected size on success, actual on partial write */
    value *m = mk3(
        "Type",
        vs("writefileresult"),
        "Id",
        vi(mkey(cmd, "Id")),
        "WrittenBytes",
        vu(contents->nkids - to_write));
    size_t l;
    uint8_t *c = encode(m, &l);
    if (c) {
        send_pkt(c, l);
        free(c);
    }
    mfreekeys(m);
    vfree(m);
}
