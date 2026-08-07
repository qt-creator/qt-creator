// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
//
// Readfile handler.
// Included by cmdbridge.c -- do not compile separately.

#include <fcntl.h>

static void h_readfile(value *cmd)
{
    value *rf = mfind(cmd, "ReadFile");
    const char *path = rf ? mstr(rf, "Path") : mstr(cmd, "Path");
    int64_t offset = rf ? mint(rf, "Offset", 0) : mint(cmd, "Offset", 0);
    int64_t limit = rf ? mint(rf, "Limit", -1) : mint(cmd, "Limit", -1);
    if (!path) {
        send_err(mkey(cmd, "Id"), "missing Path");
        return;
    }
    file_t fd = plat_open(path, O_RDONLY, 0);
    if (fd == INVALID_FILE) {
        send_os_err(mkey(cmd, "Id"), strerror(errno), errno);
        return;
    }
    if (plat_lseek(fd, offset, SEEK_SET) == (off_t) -1) {
        plat_close(fd);
        send_err(mkey(cmd, "Id"), "seek failed");
        return;
    }
    off_t fsize = plat_lseek(fd, 0, SEEK_END);
    if (fsize == (off_t) -1) {
        plat_close(fd);
        send_err(mkey(cmd, "Id"), "seek failed");
        return;
    }
    if (plat_lseek(fd, offset, SEEK_SET) == (off_t) -1) {
        plat_close(fd);
        send_err(mkey(cmd, "Id"), "seek failed");
        return;
    }
    int64_t to_read = (limit == -1) ? (fsize - offset) : limit;
    uint8_t buf[4096];
    while (to_read > 0) {
        size_t chunk = (size_t) (to_read > 4096 ? 4096 : to_read);
        ssize_t n = plat_read(fd, buf, chunk);
        if (n < 0) {
            if (errno == EINTR)
                continue;
            send_os_err(mkey(cmd, "Id"), strerror(errno), errno);
            plat_close(fd);
            return;
        }
        if (n == 0)
            break;
        value *m = mk3(
            "Type", vs("readfiledata"), "Id", vi(mkey(cmd, "Id")), "Contents", vy(buf, (size_t) n));
        size_t l;
        uint8_t *c = encode(m, &l);
        if (c) {
            send_pkt(c, l);
            free(c);
        }
        mfreekeys(m);
        vfree(m);
        to_read -= n;
    }
    plat_close(fd);
    send_void(mkey(cmd, "Id"), "readfiledone");
}
