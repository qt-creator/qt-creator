// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
//
// Wire protocol, send functions, and platform helpers.
// Included by cmdbridge.c — do not compile separately.

/* ================================================================== */
/*  Wire protocol                                                     */
/* ================================================================== */

static pthread_mutex_t output_mutex;
static pthread_mutex_t watch_mutex = PTHREAD_MUTEX_INITIALIZER;

/* The build passes the real marker; the fallback has to be the same string,
   otherwise a hand-built binary produces packets the client cannot find.
   Keep in sync with src/libs/gocmdbridge/CMakeLists.txt. */
#ifndef GOBRIDGE_MAGIC_PACKET_MARKER
#define GOBRIDGE_MAGIC_PACKET_MARKER "PkgMarkerGoBridgeMagicPacket"
#endif
#define MAGIC GOBRIDGE_MAGIC_PACKET_MARKER
#define MLG (sizeof(MAGIC) - 1)

static void write_all(int fd, const uint8_t *data, size_t len)
{
    size_t sent = 0;
    while (sent < len) {
        ssize_t w = write(fd, data + sent, len - sent);
        if (w < 0) {
            if (errno == EINTR)
                continue;
            return;
        }
        sent += (size_t) w;
    }
}

/* Writes one packet. The caller must hold output_mutex.
   A packet carries exactly one CBOR value: the client reads a single value per
   packet and ignores any trailing bytes. */
/* Converts a POSIX st_mode into the value the client expects in a "Mode"
   field, which is Go's io/fs.FileMode: the permission bits stay in the low 9
   bits, but the file type moves into named high bits. The client reads the type
   from those (see the fsMode enum in bridgedfileaccess.cpp), so sending a raw
   st_mode leaves every directory looking like a plain file -- the file dialog
   then lists folders as files and tries to open them instead of entering. */
#define GO_MODE_DIR 0x80000000u         /* 1 << 31 */
#define GO_MODE_APPEND 0x40000000u      /* 1 << 30 */
#define GO_MODE_EXCLUSIVE 0x20000000u   /* 1 << 29 */
#define GO_MODE_TEMPORARY 0x10000000u   /* 1 << 28 */
#define GO_MODE_SYMLINK 0x08000000u     /* 1 << 27 */
#define GO_MODE_DEVICE 0x04000000u      /* 1 << 26 */
#define GO_MODE_NAMED_PIPE 0x02000000u  /* 1 << 25 */
#define GO_MODE_SOCKET 0x01000000u      /* 1 << 24 */
#define GO_MODE_SETUID 0x00800000u      /* 1 << 23 */
#define GO_MODE_SETGID 0x00400000u      /* 1 << 22 */
#define GO_MODE_CHAR_DEVICE 0x00200000u /* 1 << 21 */
#define GO_MODE_STICKY 0x00100000u      /* 1 << 20 */
#define GO_MODE_IRREGULAR 0x00080000u   /* 1 << 19 */

static uint32_t go_file_mode(mode_t st_mode)
{
    uint32_t m = (uint32_t) (st_mode & 0777);

    switch (st_mode & S_IFMT) {
    case S_IFDIR:
        m |= GO_MODE_DIR;
        break;
    case S_IFLNK:
        m |= GO_MODE_SYMLINK;
        break;
    case S_IFIFO:
        m |= GO_MODE_NAMED_PIPE;
        break;
    case S_IFBLK:
        m |= GO_MODE_DEVICE;
        break;
    case S_IFCHR:
        m |= GO_MODE_DEVICE | GO_MODE_CHAR_DEVICE;
        break;
#ifdef S_IFSOCK
    case S_IFSOCK:
        m |= GO_MODE_SOCKET;
        break;
#endif
    case S_IFREG:
        break; /* a regular file carries no type bit */
    default:
        m |= GO_MODE_IRREGULAR;
        break;
    }

#ifdef S_ISUID
    if (st_mode & S_ISUID)
        m |= GO_MODE_SETUID;
#endif
#ifdef S_ISGID
    if (st_mode & S_ISGID)
        m |= GO_MODE_SETGID;
#endif
#ifdef S_ISVTX
    if (st_mode & S_ISVTX)
        m |= GO_MODE_STICKY;
#endif
    return m;
}

static void send_pkt_locked(const uint8_t *cbor, size_t len)
{
    write_all(STDERR_FILENO, (const uint8_t *) MAGIC, MLG);
    uint32_t be = (uint32_t) len;
    uint8_t b[4] = {(uint8_t) (be >> 24), (uint8_t) (be >> 16), (uint8_t) (be >> 8), (uint8_t) be};
    write_all(STDERR_FILENO, b, 4);
    write_all(STDERR_FILENO, cbor, len);
}

static void send_pkt(const uint8_t *cbor, size_t len)
{
    pthread_mutex_lock(&output_mutex);
    send_pkt_locked(cbor, len);
    pthread_mutex_unlock(&output_mutex);
}

static void send_err(int id, const char *msg)
{
    value *m = mk5(
        "Type",
        vs("error"),
        "Id",
        vi(id),
        "Error",
        vs(msg),
        "ErrorType",
        vs("os.PathError"),
        "Errno",
        vi(0));
    size_t l;
    uint8_t *c = encode(m, &l);
    if (c) {
        send_pkt(c, l);
        free(c);
    }
    mfreekeys(m);
    vfree(m);
}

static void send_os_err(int id, const char *msg, int os_errno)
{
    const char *errtype = "os.PathError";
#ifdef _WIN32
    (void) os_errno;
#else
    if (os_errno == ENOENT || os_errno == ENOTDIR)
        errtype = "os.ErrNotExist";
    else if (os_errno == EACCES || os_errno == EPERM || os_errno == EROFS)
        errtype = "os.ErrPermission";
    else if (os_errno == EEXIST)
        errtype = "os.ErrExist";
    else if (os_errno == EINVAL || os_errno == ENOTEMPTY)
        errtype = "os.PathError";
#endif
    value *m = mk5(
        "Type",
        vs("error"),
        "Id",
        vi(id),
        "Error",
        vs(msg),
        "ErrorType",
        vs(errtype),
        "Errno",
        vi(os_errno));
    size_t l;
    uint8_t *c = encode(m, &l);
    if (c) {
        send_pkt(c, l);
        free(c);
    }
    mfreekeys(m);
    vfree(m);
}

static void send_err_type(int id, const char *msg, const char *errtype)
{
    value *m = mk5(
        "Type", vs("error"), "Id", vi(id), "Error", vs(msg), "ErrorType", vs(errtype), "Errno", vi(0));
    size_t l;
    uint8_t *c = encode(m, &l);
    if (c) {
        send_pkt(c, l);
        free(c);
    }
    mfreekeys(m);
    vfree(m);
}

static void send_void(int id, const char *type)
{
    value *m = mk2("Type", vs(type), "Id", vi(id));
    size_t l;
    uint8_t *c = encode(m, &l);
    if (c) {
        send_pkt(c, l);
        free(c);
    }
    mfreekeys(m);
    vfree(m);
}
