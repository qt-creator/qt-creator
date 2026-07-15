// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
//
// File watching: registration bookkeeping, command handlers, and the choice
// of platform backend (included at the bottom of this file).
// Included by cmdbridge.c — do not compile separately.

/* ================================================================== */
/*  File watching                                                     */
/* ================================================================== */

/* fsnotify.Op values, which is what the client expects in EventType. */
#define WATCH_OP_NONE 0
#define WATCH_OP_REMOVE 1
#define WATCH_OP_CREATE 2
#define WATCH_OP_WRITE 4
#define WATCH_OP_RENAME 8
#define WATCH_OP_CHMOD 16

/* ------------------------------------------------------------------
 * Backend interface, implemented exactly once per platform:
 *
 *   bool watch_backend_supported(void)
 *       Whether this build can watch files at all.
 *   void watch_backend_start(void)
 *       Starts the event thread if it is not running yet. Idempotent.
 *   int  watch_backend_find(const char *path)
 *       Index of the watcher for `path`, or -1 if there is none.
 *   int  watch_backend_add(const char *path)
 *       Starts watching `path`. Returns its index, or -1 on failure.
 *   void watch_backend_remove(int idx)
 *       Stops watching `idx` and frees its slot.
 *
 * Watchers are identified by their index in the backend's own array. Those
 * indices are stable: a backend never moves a watcher to a different slot, so
 * registrations and any event thread holding an index stay valid for the
 * watchers that remain. Every function is called with watch_mutex held.
 * ------------------------------------------------------------------ */

/* ------------------------------------------------------------------
 * Registrations: several watch commands may share one watcher.
 * ------------------------------------------------------------------ */

/* Command Id -> watcher index. Several watch commands may share one watcher,
   so this is a many-to-one mapping and the reverse lookup is a scan. It grows
   as needed; a fixed table would silently stop registering watches, and the
   client would then simply never hear about those files again. */
static imap watch_ids;

static void watch_ids_init(void)
{
    imap_init(&watch_ids);
}

static bool watch_ids_add(int cmdId, int watcherIdx)
{
    return imap_put(&watch_ids, cmdId, IMAP_INT(watcherIdx));
}

/* Whether any registration still refers to `idx`. */
static bool watch_idx_in_use(int idx)
{
    size_t it = 0;
    void *val;
    while (imap_next(&watch_ids, &it, NULL, &val)) {
        if (IMAP_TO_INT(val) == idx)
            return true;
    }
    return false;
}

/* Sends a watchEvent to every command registered for `idx`. Backends call this
   from their event thread while holding watch_mutex. */
static void watch_emit(int idx, const char *path, int op)
{
    size_t it = 0;
    int cmdId;
    void *val;
    while (imap_next(&watch_ids, &it, &cmdId, &val)) {
        if (IMAP_TO_INT(val) != idx)
            continue;
        value *m = mk4(
            "Type", vs("watchEvent"), "Id", vi(cmdId), "Path", vs(path), "EventType", vi(op));
        size_t l;
        uint8_t *c = encode(m, &l);
        if (c) {
            send_pkt(c, l);
            free(c);
        }
        mfreekeys(m);
        vfree(m);
    }
}

/* ------------------------------------------------------------------
 * The backend for this platform. Included here rather than from
 * cmdbridge.c so that the choice lives next to the interface it
 * implements, and so the backends can call watch_emit() above without
 * needing forward declarations.
 * ------------------------------------------------------------------ */

#include "watcher_linux.c"
#include "watcher_apple.c"
#include "watcher_win.c"
#include "watcher_none.c"

/* ================================================================== */
/*  Command handlers                                                  */
/* ================================================================== */

static void send_watch_result(int id, const char *type, bool ok)
{
    value *m = mk3("Type", vs(type), "Id", vi(id), "Result", vb(ok));
    size_t l;
    uint8_t *c = encode(m, &l);
    if (c) {
        send_pkt(c, l);
        free(c);
    }
    mfreekeys(m);
    vfree(m);
}

static void h_watch(value *cmd)
{
    const char *path = mstr(cmd, "Path");
    if (!path) {
        send_err(mkey(cmd, "Id"), "missing Path");
        return;
    }
    int id = mkey(cmd, "Id");

    if (!watch_backend_supported()) {
        send_err(id, "file watching not supported on this platform");
        return;
    }

    pthread_mutex_lock(&watch_mutex);
    watch_backend_start();

    /* Reuse an existing watcher for the same path; only the registration is
       new in that case. */
    int idx = watch_backend_find(path);
    if (idx < 0)
        idx = watch_backend_add(path);
    if (idx < 0) {
        pthread_mutex_unlock(&watch_mutex);
        send_err(id, "failed to add watch");
        return;
    }
    watch_ids_add(id, idx);
    pthread_mutex_unlock(&watch_mutex);

    send_watch_result(id, "addwatchresult", true);
}

static void h_stopwatch(value *cmd)
{
    int id = mkey(cmd, "Id");

    pthread_mutex_lock(&watch_mutex);
    void **slot = imap_find(&watch_ids, id);
    if (!slot) {
        pthread_mutex_unlock(&watch_mutex);
        send_err_type(id, "watch not found", "Utils.WatchNotFoundError");
        return;
    }

    int idx = IMAP_TO_INT(*slot);
    imap_remove(&watch_ids, id);

    /* Drop the watcher itself once nothing refers to it any more. */
    if (!watch_idx_in_use(idx))
        watch_backend_remove(idx);
    pthread_mutex_unlock(&watch_mutex);

    send_watch_result(id, "removewatchresult", true);
}
