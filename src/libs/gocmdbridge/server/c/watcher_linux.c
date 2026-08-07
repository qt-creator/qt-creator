// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
//
// Linux file watching backend (inotify). Implements the interface declared in
// watcher.c. Included by cmdbridge.c - do not compile separately.

#ifdef __linux__
#include <sys/inotify.h>

typedef struct
{
    int wd;
    char path[PATH_MAX];
    bool is_file; /* true if watching a single file (not a directory) */
} watcher_t;

/* Watcher index -> watcher. Indices come from a counter and are never reused,
   which is what lets watcher.c hold on to them; the map grows as needed so the
   number of watched paths is not capped. */
static imap watchers;
static int next_watcher_idx = 0;
static int inotify_fd = -1;

/* IN_DELETE_SELF has to be asked for - inotify only ever delivers IN_IGNORED,
   IN_Q_OVERFLOW and IN_UNMOUNT unrequested. Without it, the watch on a file that
   an editor replaces went quiet with nothing to tell us so. */
static const uint32_t INOTIFY_MASK = IN_MODIFY | IN_CREATE | IN_DELETE | IN_MOVE | IN_MOVE_SELF
                                     | IN_DELETE_SELF | IN_ATTRIB;

static int inotify_to_op(uint32_t mask)
{
    if (mask & (IN_DELETE | IN_DELETE_SELF))
        return WATCH_OP_REMOVE;
    if (mask & IN_CREATE)
        return WATCH_OP_CREATE;
    if (mask & IN_MODIFY)
        return WATCH_OP_WRITE;
    if (mask & IN_MOVE)
        return WATCH_OP_RENAME;
    if (mask & IN_ATTRIB)
        return WATCH_OP_CHMOD;
    return WATCH_OP_NONE;
}

static void *watch_thread(void *arg)
{
    (void) arg;
    /* Large enough for a full read of several events, each of which carries a
       variable length name. */
    char buf[8192] __attribute__((aligned(__alignof__(struct inotify_event))));

    for (;;) {
        ssize_t n = read(inotify_fd, buf, sizeof(buf));
        if (n <= 0) {
            usleep(100000); /* the fd is non-blocking */
            continue;
        }
        for (ssize_t i = 0; i + (ssize_t) sizeof(struct inotify_event) <= n;) {
            struct inotify_event *ev = (struct inotify_event *) (buf + i);
            pthread_mutex_lock(&watch_mutex);
            size_t it = 0;
            int idx;
            void *val;
            while (imap_next(&watchers, &it, &idx, &val)) {
                watcher_t *w = (watcher_t *) val;
                if (w->wd != ev->wd)
                    continue;

                char full[PATH_MAX];
                int n;
                if (ev->len > 0)
                    n = snprintf(full, sizeof(full), "%s/%s", w->path, ev->name);
                else
                    n = snprintf(full, sizeof(full), "%s", w->path);
                /* A path that does not fit is one nothing could act on
                   anyway, so report the others and skip this event. */
                int op = inotify_to_op(ev->mask);
                if (op != WATCH_OP_NONE && n > 0 && (size_t) n < sizeof(full))
                    watch_emit(idx, full, op);

                /* A watch on a single file is lost when that file is replaced,
                   which editors do routinely; re-arm it. Matches watcher.go.
                   IN_IGNORED is the one event that says the watch is gone for
                   certain, and a rename over the file arrives as IN_MOVE_SELF;
                   watching only for IN_DELETE, which is reported for entries
                   *inside* a watched directory, meant this never ran and the
                   file stopped being watched after its first atomic save. */
                if (w->is_file
                    && (ev->mask & (IN_DELETE_SELF | IN_MOVE_SELF | IN_IGNORED)) != 0) {
                    struct stat st;
                    if (stat(w->path, &st) == 0 && !S_ISDIR(st.st_mode)) {
                        int new_wd = inotify_add_watch(inotify_fd, w->path, INOTIFY_MASK);
                        if (new_wd >= 0)
                            w->wd = new_wd;
                    }
                }
            }
            pthread_mutex_unlock(&watch_mutex);
            i += (ssize_t) sizeof(struct inotify_event) + ev->len;
        }
    }
    return NULL;
}

static bool watch_backend_supported(void)
{
    return true;
}

static void watch_backend_start(void)
{
    if (inotify_fd >= 0)
        return;
    imap_init(&watchers);
    inotify_fd = inotify_init1(IN_NONBLOCK | IN_CLOEXEC);
    if (inotify_fd < 0)
        return;
    pthread_t tid;
    if (pthread_create(&tid, NULL, watch_thread, NULL) == 0)
        pthread_detach(tid);
}

static int watch_backend_find(const char *path)
{
    size_t it = 0;
    int idx;
    void *val;
    while (imap_next(&watchers, &it, &idx, &val)) {
        if (strcmp(((watcher_t *) val)->path, path) == 0)
            return idx;
    }
    return -1;
}

static int watch_backend_add(const char *path)
{
    if (inotify_fd < 0)
        return -1;

    struct stat st;
    bool is_file = (lstat(path, &st) == 0 && !S_ISDIR(st.st_mode));
    int wd = inotify_add_watch(inotify_fd, path, INOTIFY_MASK);
    if (wd < 0)
        return -1;

    watcher_t *w = (watcher_t *) malloc(sizeof(*w));
    if (!w) {
        inotify_rm_watch(inotify_fd, wd);
        return -1;
    }
    w->wd = wd;
    snprintf(w->path, sizeof(w->path), "%s", path);
    w->is_file = is_file;

    int idx = next_watcher_idx++;
    if (!imap_put(&watchers, idx, w)) {
        inotify_rm_watch(inotify_fd, wd);
        free(w);
        return -1;
    }
    return idx;
}

static void watch_backend_remove(int idx)
{
    watcher_t *w = (watcher_t *) imap_get(&watchers, idx);
    if (!w)
        return;
    inotify_rm_watch(inotify_fd, w->wd);
    imap_remove(&watchers, idx);
    free(w);
}

#endif /* __linux__ */
