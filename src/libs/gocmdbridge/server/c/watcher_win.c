// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
//
// Windows file watching backend (ReadDirectoryChangesW). Implements the
// interface declared in watcher.c. Included by cmdbridge.c - do not compile
// separately.

#ifdef _WIN32

typedef struct
{
    int idx; /* the index watcher.c knows this watcher by */
    HANDLE dirHandle;
    char path[PATH_MAX]; /* what the client asked to watch */
    /* ReadDirectoryChangesW only works on directories, so watching a file means
       watching its parent and reporting just that one name - which is what
       fsnotify did, and what the client needs to hear about a file being saved
       from another program. Empty when `path` is itself a directory. */
    char filter[PATH_MAX];
    bool is_file;
    volatile long cancelled;
    HANDLE stopped; /* signalled by the thread once it is done with the entry */
} win_watcher_t;

/* Watcher index -> watcher. Indices come from a counter and are never reused,
   and each watcher is allocated separately so the pointer its thread holds
   stays valid however many others come and go. The map grows as needed, so the
   number of watched directories is not capped. */
static imap win_watchers;
static int next_win_idx = 0;

static int win_action_to_op(DWORD action)
{
    switch (action) {
    case FILE_ACTION_ADDED:
        return WATCH_OP_CREATE;
    case FILE_ACTION_REMOVED:
        return WATCH_OP_REMOVE;
    case FILE_ACTION_MODIFIED:
        return WATCH_OP_WRITE;
    case FILE_ACTION_RENAMED_OLD_NAME:
    case FILE_ACTION_RENAMED_NEW_NAME:
        return WATCH_OP_RENAME;
    default:
        return WATCH_OP_NONE;
    }
}

static void *win_watch_thread(void *arg)
{
    win_watcher_t *w = (win_watcher_t *) arg;
    char buf[65536];
    DWORD bytesReturned;

    while (!w->cancelled) {
        BOOL ok = ReadDirectoryChangesW(
            w->dirHandle,
            buf,
            sizeof(buf),
            FALSE, /* only watch this directory, not subdirectories */
            FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME | FILE_NOTIFY_CHANGE_SIZE
                | FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_ATTRIBUTES,
            &bytesReturned,
            NULL, /* no overlapped struct - synchronous */
            NULL);

        if (!ok)
            break; /* cancelled, or the directory went away */

        DWORD offset = 0;
        while (offset + sizeof(FILE_NOTIFY_INFORMATION) <= bytesReturned) {
            FILE_NOTIFY_INFORMATION *fni = (FILE_NOTIFY_INFORMATION *) (buf + offset);
            int op = win_action_to_op(fni->Action);

            if (op != WATCH_OP_NONE && fni->FileNameLength > 0) {
                wchar_t wname[4096];
                /* The bound is the element count, not the size in bytes. */
                int wcopies = (int) (fni->FileNameLength / sizeof(wchar_t));
                if (wcopies >= (int) (sizeof(wname) / sizeof(wname[0])))
                    wcopies = (int) (sizeof(wname) / sizeof(wname[0])) - 1;
                memcpy(wname, fni->FileName, (size_t) wcopies * sizeof(wchar_t));
                wname[wcopies] = L'\0';

                /* UTF-8 is longer than the wide-character count for anything
                   outside ASCII, so terminate at the converted length. */
                char nameA[8192];
                int nameALen = WideCharToMultiByte(
                    CP_UTF8, 0, wname, wcopies, nameA, (int) sizeof(nameA) - 1, NULL, NULL);
                if (nameALen > 0) {
                    nameA[nameALen] = '\0';
                    /* A file watch hears about every sibling in the directory;
                       only the one that was asked for is reported, under the
                       path the client used. Windows names are case
                       insensitive. */
                    bool wanted = !w->is_file || _stricmp(nameA, w->filter) == 0;
                    if (wanted) {
                        char full[PATH_MAX];
                        if (w->is_file)
                            snprintf(full, sizeof(full), "%s", w->path);
                        else
                            snprintf(full, sizeof(full), "%s\\%s", w->path, nameA);

                        pthread_mutex_lock(&watch_mutex);
                        watch_emit(w->idx, full, op);
                        pthread_mutex_unlock(&watch_mutex);
                    }
                }
            }

            if (fni->NextEntryOffset == 0)
                break;
            offset += fni->NextEntryOffset;
        }
    }

    CloseHandle(w->dirHandle);
    w->dirHandle = INVALID_HANDLE_VALUE;
    SetEvent(w->stopped);
    return NULL;
}

static bool watch_backend_supported(void)
{
    return true;
}

static void watch_backend_start(void)
{
    static bool initialised = false;
    if (!initialised) {
        imap_init(&win_watchers);
        initialised = true;
    }
    /* Each watcher runs its own thread, started in watch_backend_add(). */
}

static int watch_backend_find(const char *path)
{
    size_t it = 0;
    int idx;
    void *val;
    while (imap_next(&win_watchers, &it, &idx, &val)) {
        if (strcmp(((win_watcher_t *) val)->path, path) == 0)
            return idx;
    }
    return -1;
}

static int watch_backend_add(const char *path)
{
    wchar_t *wpath = utf8_to_utf16_long(path);
    if (!wpath)
        return -1;
    WIN32_FILE_ATTRIBUTE_DATA attr;
    if (!GetFileAttributesExW(wpath, GetFileExInfoStandard, &attr)) {
        free(wpath);
        return -1;
    }

    /* ReadDirectoryChangesW only works on directories, so a file is watched
       through its parent; the events are filtered back down to it. Refusing the
       request instead meant the client never heard about a watched file at all. */
    char watched_dir[PATH_MAX];
    char filter[PATH_MAX];
    bool is_file = (attr.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0;
    filter[0] = '\0';
    if (is_file) {
        snprintf(watched_dir, sizeof(watched_dir), "%s", path);
        char *sep = strrchr(watched_dir, '\\');
        char *fwd = strrchr(watched_dir, '/');
        if (fwd && (!sep || fwd > sep))
            sep = fwd;
        if (!sep) {
            free(wpath);
            return -1; /* a bare name has no parent to watch */
        }
        snprintf(filter, sizeof(filter), "%s", sep + 1);
        *sep = '\0';
        if (watched_dir[0] == '\0') {
            free(wpath);
            return -1;
        }
        free(wpath);
        wpath = utf8_to_utf16_long(watched_dir);
        if (!wpath)
            return -1;
    }

    HANDLE h = CreateFileW(
        wpath,
        FILE_LIST_DIRECTORY,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        NULL,
        OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS,
        NULL);
    free(wpath);
    if (h == INVALID_HANDLE_VALUE)
        return -1;

    win_watcher_t *w = (win_watcher_t *) malloc(sizeof(*w));
    if (!w) {
        CloseHandle(h);
        return -1;
    }
    w->idx = next_win_idx++;
    w->dirHandle = h;
    snprintf(w->path, sizeof(w->path), "%s", path);
    snprintf(w->filter, sizeof(w->filter), "%s", filter);
    w->is_file = is_file;
    w->cancelled = 0;
    w->stopped = CreateEventA(NULL, TRUE, FALSE, NULL);

    if (!imap_put(&win_watchers, w->idx, w)) {
        CloseHandle(h);
        if (w->stopped)
            CloseHandle(w->stopped);
        free(w);
        return -1;
    }

    pthread_t th;
    if (pthread_create(&th, NULL, win_watch_thread, w) != 0) {
        imap_remove(&win_watchers, w->idx);
        CloseHandle(h);
        if (w->stopped)
            CloseHandle(w->stopped);
        free(w);
        return -1;
    }
    pthread_detach(th);
    return w->idx;
}

static void watch_backend_remove(int idx)
{
    win_watcher_t *w = (win_watcher_t *) imap_get(&win_watchers, idx);
    if (!w)
        return;

    /* Wait for the thread to finish before freeing the entry it is reading
       from. Cancelling the blocking ReadDirectoryChangesW makes it return at
       once. */
    w->cancelled = 1;
    if (w->dirHandle != INVALID_HANDLE_VALUE)
        CancelIoEx(w->dirHandle, NULL);
    if (w->stopped) {
        WaitForSingleObject(w->stopped, 2000);
        CloseHandle(w->stopped);
        w->stopped = NULL;
    }

    imap_remove(&win_watchers, idx);
    free(w);
}

#endif /* _WIN32 */
