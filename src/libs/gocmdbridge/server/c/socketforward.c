// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
//
// Socket forwarding handlers (Unix domain socket server + per-conn threads).
// Included by cmdbridge.c - do not compile separately.

#ifndef _WIN32
#include <poll.h>
#include <sys/socket.h>
#include <sys/un.h>
#endif

/* ================================================================== */
/*  Socket forwarding (Unix domain socket server + per-conn threads)  */
/* ================================================================== */

/* Send functions shared by Unix and Windows implementations */

static void send_socket_connect(int forwardId, int connId)
{
    value *m = mk3("Type", vs("socketconnect"), "Id", vi(forwardId), "ConnId", vi(connId));
    size_t l;
    uint8_t *c = encode(m, &l);
    if (c) {
        send_pkt(c, l);
        free(c);
    }
    mfreekeys(m);
    vfree(m);
}

static void send_socket_data(int forwardId, int connId, const uint8_t *data, size_t len)
{
    value *m = mk4(
        "Type", vs("socketdata"), "Id", vi(forwardId), "ConnId", vi(connId), "Data", vy(data, len));
    size_t l;
    uint8_t *c = encode(m, &l);
    if (c) {
        send_pkt(c, l);
        free(c);
    }
    mfreekeys(m);
    vfree(m);
}

static void send_socket_close(int forwardId, int connId)
{
    value *m = mk3("Type", vs("socketclose"), "Id", vi(forwardId), "ConnId", vi(connId));
    size_t l;
    uint8_t *c = encode(m, &l);
    if (c) {
        send_pkt(c, l);
        free(c);
    }
    mfreekeys(m);
    vfree(m);
}

static void send_forward_ready(int id, const char *path)
{
    value *m = mk3("Type", vs("forwardlocalsocketserverready"), "Id", vi(id), "Path", vs(path));
    size_t l;
    uint8_t *c = encode(m, &l);
    if (c) {
        send_pkt(c, l);
        free(c);
    }
    mfreekeys(m);
    vfree(m);
}

static void send_forward_stopped(int id)
{
    value *m = mk2("Type", vs("forwardserverstopped"), "Id", vi(id));
    size_t l;
    uint8_t *c = encode(m, &l);
    if (c) {
        send_pkt(c, l);
        free(c);
    }
    mfreekeys(m);
    vfree(m);
}

/* ================================================================== */
/*  Platform helpers - callers use these instead of raw syscalls      */
/* ================================================================== */

/* Sockets are kept out of spawned children, as pipes and files are: a command
   running on another thread forks, and its child would otherwise hold the
   forwarded socket open after this side has closed it. Windows has no fork and
   does not inherit handles unless asked to, so there it is a no-op. */
#if defined(_WIN32) || !defined(SOCK_CLOEXEC)
#define SOCK_CLOEXEC_FLAG 0
#else
#define SOCK_CLOEXEC_FLAG SOCK_CLOEXEC
#endif

static int s_accept_cloexec(int fd, struct sockaddr *addr, socklen_t *len)
{
#if defined(_WIN32) || !defined(SOCK_CLOEXEC)
    return accept(fd, addr, len);
#else
    return accept4(fd, addr, len, SOCK_CLOEXEC);
#endif
}

static int s_close(int fd)
{
#if _WIN32
    return closesocket(fd);
#else
    return close(fd);
#endif
}

static int s_unlink(const char *path)
{
#if _WIN32
    return DeleteFileA(path) ? 0 : -1;
#else
    return unlink(path);
#endif
}

/* These helpers let the rest of the file stay #ifdef-free. */

static ssize_t s_read(int fd, void *buf, size_t n)
{
#if _WIN32
    return recv(fd, (char *) buf, (int) n, 0);
#else
    return read(fd, buf, n);
#endif
}

static ssize_t s_write(int fd, const void *buf, size_t n)
{
#if _WIN32
    return send(fd, (const char *) buf, (int) n, 0);
#else
    return write(fd, buf, n);
#endif
}

/* poll/select wrapper - nfds_t is provided by fileaccess_win.c on Windows. */
static int s_poll_wrapper(struct pollfd *fds, nfds_t nfds, int timeout)
{
#if _WIN32
    struct timeval tv = { .tv_sec = timeout / 1000, .tv_usec = (timeout % 1000) * 1000 };
    fd_set rset;
    FD_ZERO(&rset);
    /* A closed connection is marked with -1, which is INVALID_SOCKET here. It
       has to be left out: select() would fail outright on it, and then nothing
       set revents and the caller read whatever was on the stack. Comparing
       against 0 did not catch it, SOCKET being unsigned. */
    bool watching = false;
    for (nfds_t i = 0; i < nfds; i++) {
        fds[i].revents = 0;
        if (fds[i].fd != INVALID_SOCKET) {
            FD_SET(fds[i].fd, &rset);
            watching = true;
        }
    }
    if (!watching) {
        if (timeout > 0)
            Sleep((DWORD) timeout);
        return 0;
    }
    int rc = select(0, &rset, NULL, NULL, timeout == -1 ? NULL : &tv);
    if (rc > 0) {
        for (nfds_t i = 0; i < nfds; i++) {
            if (fds[i].fd != INVALID_SOCKET && FD_ISSET(fds[i].fd, &rset))
                fds[i].revents = POLLIN;
        }
    }
    return rc;
#else
    return poll(fds, nfds, timeout);
#endif
}

/* ================================================================== */
/*  Shared constants & types                                          */
/* ================================================================== */

#define SOCKET_DATA_BUF 32768

/* --- Per-forward-server state --- */

typedef struct
{
    int id;
    int listenerFd;
    char path[PATH_MAX];
    imap conns; /* connection Id -> connState * */
    int nextConnId;
    int stop;
    pthread_t acceptTid;
} socketForwardState;

/* --- Per-connection command --- */

typedef struct
{
    int kind; /* 1=data, 2=close */
    uint8_t *data;
    size_t data_len;
} socketForwardCmd;

/* --- Per-connection state --- */

struct connState
{
    int fd;
    int id;
    socketForwardState *fwd;
    fifo cmdCh; /* of socketForwardCmd * */
    pthread_mutex_t cmdMu;
    pthread_cond_t cmdCv;
    int done;
    int closedByCpp;
    int refcount;
    pthread_t tid;
};
typedef struct connState connState;

static void conn_retain(connState *cs)
{
    if (cs)
        __sync_fetch_and_add(&cs->refcount, 1);
}

static void conn_release(connState *cs)
{
    if (!cs)
        return;
    if (__sync_fetch_and_sub(&cs->refcount, 1) == 1) {
        pthread_mutex_destroy(&cs->cmdMu);
        pthread_cond_destroy(&cs->cmdCv);
        free(cs);
    }
}

/* --- Global state --- */

/* Forward Id -> server. Both this and the per-server connection map grow as
   needed, so neither the number of forwarded sockets nor the number of
   connections to one of them is capped at a compile-time constant. */
static imap forwards;
static pthread_mutex_t forwardMu = PTHREAD_MUTEX_INITIALIZER;

static void socket_forward_init(void)
{
    imap_init(&forwards);
}

static socketForwardState *find_forward(int id)
{
    return (socketForwardState *) imap_get(&forwards, id);
}

static connState *find_conn(socketForwardState *fwd, int connId)
{
    return (connState *) imap_get(&fwd->conns, connId);
}

/* --- Drain pending commands and write to socket --- */

static void drain_cmds(connState *cs)
{
    socketForwardCmd **items = NULL;
    int count = 0;

    pthread_mutex_lock(&cs->cmdMu);
    if (cs->cmdCh.count > 0) {
        items = (socketForwardCmd **) xmalloc(cs->cmdCh.count * sizeof(socketForwardCmd *));
        socketForwardCmd *item;
        while ((item = (socketForwardCmd *) fifo_pop(&cs->cmdCh)) != NULL)
            items[count++] = item;
    }
    pthread_mutex_unlock(&cs->cmdMu);

    for (int i = 0; i < count; i++) {
        socketForwardCmd *cmd = items[i];
        if (cmd->kind == 1 && cmd->data) {
            ssize_t total = 0;
            while ((size_t) total < cmd->data_len) {
                ssize_t w = s_write(cs->fd, cmd->data + total, cmd->data_len - (size_t) total);
                if (w <= 0)
                    break;
                total += w;
            }
            if (total < (ssize_t) cmd->data_len) {
                free(cmd->data);
                free(cmd);
                free(items);
                cs->done = 1;
                cs->closedByCpp = 1;
                return;
            }
        } else if (cmd->kind == 2) {
            cs->done = 1;
            cs->closedByCpp = 1;
        }
        free(cmd->data);
        free(cmd);
    }
    free(items);
}

/* --- Per-connection goroutine --- */

static void *conn_thread(void *arg)
{
    connState *cs = (connState *) arg;
    uint8_t readBuf[SOCKET_DATA_BUF];

    send_socket_connect(cs->fwd->id, cs->id);

    for (;;) {
        struct pollfd pfd;
        pfd.fd = cs->fd;
        pfd.events = POLLIN;
        int pr = s_poll_wrapper(&pfd, 1, 10);

        if (pr > 0 && (pfd.revents & POLLIN)) {
            ssize_t n = s_read(cs->fd, readBuf, sizeof(readBuf));
            if (n > 0)
                send_socket_data(cs->fwd->id, cs->id, readBuf, (size_t) n);
            if (n <= 0) {
                if (!cs->closedByCpp)
                    send_socket_close(cs->fwd->id, cs->id);
                break;
            }
        }

        drain_cmds(cs);
        if (cs->done) {
            /* Do not echo a close back to the side that requested it. */
            if (!cs->closedByCpp)
                send_socket_close(cs->fwd->id, cs->id);
            break;
        }
    }

    /* Whoever gets here first closes it. h_stop_forward() also shuts connections
       down, and closing twice would, in between, have closed whatever unrelated
       file or socket had meanwhile been handed the same descriptor number. */
    int fd = __sync_lock_test_and_set(&cs->fd, -1);
    if (fd >= 0)
        s_close(fd);
    cs->done = 1;

    pthread_mutex_lock(&forwardMu);
    if (cs->fwd && find_conn(cs->fwd, cs->id) == cs)
        imap_remove(&cs->fwd->conns, cs->id);
    pthread_mutex_unlock(&forwardMu);

    conn_release(cs);
    return NULL;
}

/* --- Accept thread --- */

static void *accept_thread(void *arg)
{
    socketForwardState *fwd = (socketForwardState *) arg;

    while (!fwd->stop) {
        /* Wait with a timeout rather than blocking in accept(): closing the
           listener from another thread does not reliably wake a blocked
           accept() (it does not on Linux), which would leave this thread
           unjoinable and hang stopforwardserver. Polling also means this
           thread alone owns the listener fd, so there is no window where it
           could be closed and its number reused underneath us. */
        struct pollfd pfd;
        pfd.fd = fwd->listenerFd;
        pfd.events = POLLIN;
        int pr = s_poll_wrapper(&pfd, 1, 100);
        if (pr <= 0)
            continue; /* timeout or error: re-check stop */

        struct sockaddr_un peer;
        socklen_t peer_len = sizeof(peer);
        int clientFd = s_accept_cloexec(fwd->listenerFd, (struct sockaddr *) &peer, &peer_len);
        if (clientFd < 0) {
            if (fwd->stop)
                break;
            continue;
        }

        connState *cs = (connState *) xmalloc(sizeof(connState));
        memset(cs, 0, sizeof(*cs));
        cs->fd = clientFd;
        cs->id = fwd->nextConnId++;
        cs->fwd = fwd;
        cs->done = 0;
        cs->refcount = 1;
        fifo_init(&cs->cmdCh);
        pthread_mutex_init(&cs->cmdMu, NULL);
        pthread_cond_init(&cs->cmdCv, NULL);

        pthread_mutex_lock(&forwardMu);
        bool registered = imap_put(&fwd->conns, cs->id, cs);
        pthread_mutex_unlock(&forwardMu);

        /* Unregistered the connection could never be addressed by ConnId, so
           refuse it rather than leaking the state and its thread. */
        if (!registered || pthread_create(&cs->tid, NULL, conn_thread, cs) != 0) {
            if (registered) {
                pthread_mutex_lock(&forwardMu);
                imap_remove(&fwd->conns, cs->id);
                pthread_mutex_unlock(&forwardMu);
            }
            s_close(cs->fd);
            conn_release(cs);
        }
    }

    /* This thread owns the listener, so it closes it. The socket file is
       removed by h_stop_forward once it has joined us, which is also what
       stops the freed slot from being handed to a new forward server whose
       path this thread would otherwise unlink. */
    s_close(fwd->listenerFd);
    return NULL;
}

/* --- Command handlers --- */

static void h_forward_server(value *cmd)
{
    const char *tmpdir = getenv("TMPDIR");
    if (!tmpdir)
#if _WIN32
        tmpdir = "C:\\Windows\\Temp";
#else
        tmpdir = "/tmp";
#endif

    char serverPath[PATH_MAX];
    int pathLen = snprintf(
        serverPath, sizeof(serverPath), "%s/cmdbridge-server-%d.sock", tmpdir, mkey(cmd, "Id"));

    /* sun_path is far shorter than PATH_MAX (108 bytes on Linux), so a long
       TMPDIR would silently bind a truncated path. Refuse instead. */
    struct sockaddr_un addr;
    if (pathLen < 0 || (size_t) pathLen >= sizeof(addr.sun_path)) {
        send_err(mkey(cmd, "Id"), "socket path too long");
        return;
    }

    s_unlink(serverPath);

    /* CLOEXEC: a command spawned from another thread must not inherit the
       listener, which would keep the forwarded socket alive after we close it. */
    int listenerFd = socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC_FLAG, 0);
#if _WIN32
    if (listenerFd < 0) {
#else
    if (listenerFd == -1) {
#endif
        send_err(mkey(cmd, "Id"), "socket creation failed");
        return;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    memcpy(addr.sun_path, serverPath, (size_t) pathLen + 1); /* length checked above */

#if _WIN32
    if (bind(listenerFd, (struct sockaddr *) &addr, sizeof(addr)) == SOCKET_ERROR) {
        s_close(listenerFd);
#else
    if (bind(listenerFd, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
        s_close(listenerFd);
#endif
        s_unlink(serverPath);
        send_err(mkey(cmd, "Id"), "socket bind failed");
        return;
    }

#if _WIN32
    if (listen(listenerFd, 128) == SOCKET_ERROR) {
        s_close(listenerFd);
#else
    if (listen(listenerFd, 128) < 0) {
        s_close(listenerFd);
#endif
        s_unlink(serverPath);
        send_err(mkey(cmd, "Id"), "socket listen failed");
        return;
    }

    socketForwardState *fwd = (socketForwardState *) malloc(sizeof(*fwd));
    if (!fwd) {
        s_close(listenerFd);
        s_unlink(serverPath);
        send_err(mkey(cmd, "Id"), "out of memory");
        return;
    }
    memset(fwd, 0, sizeof(*fwd));
    fwd->id = mkey(cmd, "Id");
    fwd->listenerFd = listenerFd;
    snprintf(fwd->path, sizeof(fwd->path), "%s", serverPath);
    fwd->nextConnId = 0;
    fwd->stop = 0;
    imap_init(&fwd->conns);

    pthread_mutex_lock(&forwardMu);
    bool added = imap_put(&forwards, fwd->id, fwd);
    pthread_mutex_unlock(&forwardMu);

    if (!added) {
        s_close(listenerFd);
        s_unlink(serverPath);
        free(fwd);
        send_err(mkey(cmd, "Id"), "could not register forward server");
        return;
    }

    send_forward_ready(fwd->id, serverPath);
    /* Joinable: h_stop_forward waits for it before releasing the slot. */
    if (pthread_create(&fwd->acceptTid, NULL, accept_thread, fwd) != 0) {
        s_close(listenerFd);
        s_unlink(serverPath);
        pthread_mutex_lock(&forwardMu);
        imap_remove(&forwards, fwd->id);
        pthread_mutex_unlock(&forwardMu);
        imap_free(&fwd->conns);
        free(fwd);
        send_err(mkey(cmd, "Id"), "could not start accept thread");
    }
}

static void h_socket_data(value *cmd)
{
    int forwardId = mkey(cmd, "Id");
    int connId = mkey(cmd, "ConnId");

    value *sd = mfind(cmd, "SocketData");
    value *dataVal = sd ? mfind(sd, "Data") : mfind(cmd, "Data");
    if (!dataVal || dataVal->type != V_BYTES) {
        send_err(forwardId, "missing Data");
        return;
    }

    connState *cs = NULL;
    pthread_mutex_lock(&forwardMu);
    {
        socketForwardState *fwd = find_forward(forwardId);
        if (fwd) {
            cs = find_conn(fwd, connId);
            if (cs)
                conn_retain(cs);
        }
    }
    pthread_mutex_unlock(&forwardMu);

    if (!cs)
        return;

    socketForwardCmd *cmdItem = (socketForwardCmd *) xmalloc(sizeof(socketForwardCmd));
    cmdItem->kind = 1;
    cmdItem->data = (uint8_t *) xmalloc(dataVal->nkids ? dataVal->nkids : 1);
    memcpy(cmdItem->data, dataVal->bytes, dataVal->nkids);
    cmdItem->data_len = dataVal->nkids;

    pthread_mutex_lock(&cs->cmdMu);
    bool queued = fifo_push(&cs->cmdCh, cmdItem);
    pthread_mutex_unlock(&cs->cmdMu);
    if (!queued) {
        free(cmdItem->data);
        free(cmdItem);
    }
    conn_release(cs);
}

static void h_socket_close(value *cmd)
{
    int forwardId = mkey(cmd, "Id");
    int connId = mkey(cmd, "ConnId");

    connState *cs = NULL;
    pthread_mutex_lock(&forwardMu);
    {
        socketForwardState *fwd = find_forward(forwardId);
        if (fwd) {
            cs = find_conn(fwd, connId);
            if (cs)
                conn_retain(cs);
        }
    }
    pthread_mutex_unlock(&forwardMu);

    if (!cs)
        return;

    socketForwardCmd *cmdItem = (socketForwardCmd *) malloc(sizeof(socketForwardCmd));
    cmdItem->kind = 2;
    cmdItem->data = NULL;
    cmdItem->data_len = 0;

    pthread_mutex_lock(&cs->cmdMu);
    bool queued = fifo_push(&cs->cmdCh, cmdItem);
    pthread_mutex_unlock(&cs->cmdMu);
    if (!queued) {
        free(cmdItem->data);
        free(cmdItem);
    }
    conn_release(cs);
}

static void h_stop_forward(value *cmd)
{
    int forwardId = mkey(cmd, "Id");

    pthread_mutex_lock(&forwardMu);
    socketForwardState *fwd = find_forward(forwardId);
    if (!fwd) {
        pthread_mutex_unlock(&forwardMu);
        return;
    }

    fwd->stop = 1;

    /* Ask every connection to finish and take a reference on each, so the
       thread objects stay alive until we have joined them. The join itself
       must happen with forwardMu released: conn_thread takes that same lock
       on its way out to deregister itself, so joining while holding it
       deadlocks both threads and leaves the mutex locked forever. */
    size_t nconn = imap_count(&fwd->conns);
    pthread_t *connTids = nconn ? (pthread_t *) xmalloc(nconn * sizeof(pthread_t)) : NULL;
    connState **conns = nconn ? (connState **) xmalloc(nconn * sizeof(connState *)) : NULL;
    size_t nconnTids = 0;
    {
        size_t it = 0;
        void *val;
        while (imap_next(&fwd->conns, &it, NULL, &val)) {
            connState *cs = (connState *) val;
            cs->done = 1;
            /* See conn_thread(): only one of us closes the descriptor. */
            int fd = __sync_lock_test_and_set(&cs->fd, -1);
            if (fd >= 0)
                s_close(fd);
            conn_retain(cs);
            conns[nconnTids] = cs;
            connTids[nconnTids] = cs->tid;
            nconnTids++;
        }
    }

    /* The accept thread polls with a timeout and closes the listener itself,
       so setting `stop` above is all that is needed to end it. */
    pthread_t acceptTid = fwd->acceptTid;
    pthread_mutex_unlock(&forwardMu);

    pthread_join(acceptTid, NULL);
    for (size_t i = 0; i < nconnTids; i++) {
        pthread_join(connTids[i], NULL);
        conn_release(conns[i]);
    }
    free(connTids);
    free(conns);

    /* Only now that no thread can touch the state may it be discarded. */
    pthread_mutex_lock(&forwardMu);
    s_unlink(fwd->path);
    imap_remove(&forwards, forwardId);
    pthread_mutex_unlock(&forwardMu);
    imap_free(&fwd->conns);
    free(fwd);

    send_forward_stopped(forwardId);
}
