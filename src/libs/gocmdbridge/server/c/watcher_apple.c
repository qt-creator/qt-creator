// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
//
// macOS and BSD file watching backend (kqueue). Implements the interface
// declared in watcher.c. Included by cmdbridge.c — do not compile separately.

#if defined(__APPLE__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
#include <fcntl.h>
#include <sys/event.h>
#include <sys/types.h>

typedef struct
{
    int fd;
    char path[PATH_MAX];
    bool is_file; /* true if watching a single file (not a directory) */
} kq_watcher_t;

/* Watcher index -> watcher. Indices come from a counter and are never reused,
   which is what lets watcher.c hold on to them; the map grows as needed so the
   number of watched paths is not capped. */
static imap kq_watchers;
static int next_kq_idx = 0;
static int kq_fd = -1;

static const unsigned KQ_NOTES
    = NOTE_WRITE | NOTE_DELETE | NOTE_EXTEND | NOTE_ATTRIB | NOTE_RENAME;

static int kq_note_to_op(uint32_t flags)
{
    if (flags & NOTE_DELETE)
        return WATCH_OP_REMOVE;
    if (flags & NOTE_WRITE)
        return WATCH_OP_WRITE;
    if (flags & NOTE_EXTEND)
        return WATCH_OP_WRITE; /* extend = modify */
    if (flags & NOTE_ATTRIB)
        return WATCH_OP_CHMOD;
    if (flags & NOTE_RENAME)
        return WATCH_OP_RENAME;
    return WATCH_OP_NONE;
}

/* Registers `fd` with the kqueue. Returns true on success. */
static bool kq_arm(int fd)
{
    struct kevent ev;
    EV_SET(&ev, fd, EVFILT_VNODE, EV_ADD | EV_CLEAR, KQ_NOTES, 0, 0);
    return kevent(kq_fd, &ev, 1, NULL, 0, NULL) >= 0;
}

static void *kq_watch_thread(void *arg)
{
    (void) arg;
    struct kevent events[64];

    for (;;) {
        int n = kevent(kq_fd, NULL, 0, events, 64, NULL);
        if (n <= 0) {
            usleep(100000);
            continue;
        }
        for (int i = 0; i < n; i++) {
            struct kevent *ev = &events[i];
            pthread_mutex_lock(&watch_mutex);
            size_t it = 0;
            int idx;
            void *val;
            while (imap_next(&kq_watchers, &it, &idx, &val)) {
                kq_watcher_t *w = (kq_watcher_t *) val;
                if (w->fd != (int) ev->ident)
                    continue;

                watch_emit(idx, w->path, kq_note_to_op(ev->fflags));

                /* kqueue watches an open fd, so a watch on a single file stops
                   producing events once that file is replaced; re-open it.
                   Matches watcher.go. */
                if (w->is_file && (ev->fflags & (NOTE_DELETE | NOTE_RENAME)) != 0) {
                    struct stat st;
                    if (stat(w->path, &st) == 0 && !S_ISDIR(st.st_mode)) {
                        int new_fd = open(w->path, O_RDONLY);
                        if (new_fd >= 0) {
                            if (kq_arm(new_fd)) {
                                close(w->fd);
                                w->fd = new_fd;
                            } else {
                                close(new_fd);
                            }
                        }
                    }
                }
            }
            pthread_mutex_unlock(&watch_mutex);
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
    if (kq_fd >= 0)
        return;
    kq_fd = kqueue();
    if (kq_fd < 0)
        return;
    imap_init(&kq_watchers);
    pthread_t tid;
    if (pthread_create(&tid, NULL, kq_watch_thread, NULL) == 0)
        pthread_detach(tid);
}

static int watch_backend_find(const char *path)
{
    size_t it = 0;
    int idx;
    void *val;
    while (imap_next(&kq_watchers, &it, &idx, &val)) {
        if (strcmp(((kq_watcher_t *) val)->path, path) == 0)
            return idx;
    }
    return -1;
}

static int watch_backend_add(const char *path)
{
    if (kq_fd < 0)
        return -1;

    int fd = open(path, O_RDONLY);
    if (fd < 0)
        return -1;
    if (!kq_arm(fd)) {
        close(fd);
        return -1;
    }

    kq_watcher_t *w = (kq_watcher_t *) malloc(sizeof(*w));
    if (!w) {
        close(fd);
        return -1;
    }
    struct stat st;
    w->fd = fd;
    snprintf(w->path, sizeof(w->path), "%s", path);
    w->is_file = (lstat(path, &st) == 0 && !S_ISDIR(st.st_mode));

    int idx = next_kq_idx++;
    if (!imap_put(&kq_watchers, idx, w)) {
        close(fd);
        free(w);
        return -1;
    }
    return idx;
}

static void watch_backend_remove(int idx)
{
    kq_watcher_t *w = (kq_watcher_t *) imap_get(&kq_watchers, idx);
    if (!w)
        return;
    close(w->fd); /* closing the fd removes it from the kqueue */
    imap_remove(&kq_watchers, idx);
    free(w);
}

#endif /* __APPLE__ || BSD */
