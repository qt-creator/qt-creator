// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

/*
 * CmdBridge C implementation — single compilation unit built from multiple
 * component files for readability.  The build system only references this
 * file; each component is #include'd here in dependency order so that
 * static functions are visible where needed.
 *
 * Include order: each file's static symbols must be available to the files
 * after it. Platform specific code lives in its own file per platform, and is
 * included by the module that owns it rather than from here -- see the bottom
 * of watcher.c, find.c, exec.c and fileaccess.c.
 *
 *   containers.c      - growable FIFO and int-keyed map
 *   cbor.c            - value types, encode/decode, map helpers, constructors
 *   fileaccess.c      - platform file access layer (posix / win)
 *   cancel.c          - command cancellation registry (exec and find)
 *   wire.c            - wire protocol and send functions
 *   environment.c     - environment command handler
 *   stat.c            - stat, readlink, fileid, freespace, group, owner, remove
 *   fileops.c         - createdir, copyfile, symlink, rename, temp, chmod
 *   is.c              - is and issamefile handlers
 *   readfile.c        - readfile handler
 *   writefile.c       - writefile handler
 *   find.c            - find command and output batcher (posix / win walk)
 *   exec.c            - exec/signal handlers and process registry (posix / win)
 *   watcher.c         - watch registrations and handlers (inotify / kqueue /
 *                       ReadDirectoryChangesW / unsupported)
 *   socketforward.c   - socket forwarding (AF_UNIX)
 */

/* Only what this file and the common core need; anything platform or feature
   specific is included by the file that uses it. */
#include <errno.h>
#include <limits.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#ifdef __APPLE__
#include <mach-o/dyld.h> /* _NSGetExecutablePath, for --deleteOnStart */
#endif

#include "containers.c"
#include "cbor.c"
#include "fileaccess.c"
#include "cancel.c"
#include "wire.c"
#include "environment.c"
#include "stat.c"
#include "fileops.c"
#include "is.c"
#include "readfile.c"
#include "writefile.c"
#include "find.c"
#include "exec.c"
#include "watcher.c"
#include "socketforward.c"

/* ================================================================== */
/*  Dispatch                                                          */
/* ================================================================== */

static void process(value *cmd)
{
    const char *t = mstr(cmd, "Type");
    if (!t)
        return;

    if (!strcmp(t, "ping")) { /* keepalive */
    } else if (!strcmp(t, "cancel")) {
        int id = mkey(cmd, "Id");
        mark_cancelled(id);
    } else if (!strcmp(t, "environment"))
        h_environment(cmd);
    else if (!strcmp(t, "stat"))
        h_stat(cmd);
    else if (!strcmp(t, "readlink"))
        h_readlink(cmd);
    else if (!strcmp(t, "fileid"))
        h_fileid(cmd);
    else if (!strcmp(t, "freespace"))
        h_freespace(cmd);
    else if (!strcmp(t, "group"))
        h_group(cmd);
    else if (!strcmp(t, "groupId"))
        h_group_id(cmd);
    else if (!strcmp(t, "owner"))
        h_owner(cmd);
    else if (!strcmp(t, "ownerid"))
        h_owner_id(cmd);
    else if (!strcmp(t, "remove"))
        h_remove(cmd);
    else if (!strcmp(t, "removeall"))
        h_remove_all(cmd);
    else if (!strcmp(t, "ensureexistingfile"))
        h_ensure_file(cmd);
    else if (!strcmp(t, "createdir"))
        h_createdir(cmd);
    else if (!strcmp(t, "copyfile"))
        h_copyfile(cmd);
    else if (!strcmp(t, "createsymlink"))
        h_symlink(cmd);
    else if (!strcmp(t, "renamefile"))
        h_rename(cmd);
    else if (!strcmp(t, "createtempdir"))
        h_mktmpdir(cmd);
    else if (!strcmp(t, "createtempfile"))
        h_mktmpfile(cmd);
    else if (!strcmp(t, "setpermissions"))
        h_chmod(cmd);
    else if (!strcmp(t, "signal"))
        h_signal(cmd);
    else if (!strcmp(t, "issamefile"))
        h_issamefile(cmd);
    else if (!strcmp(t, "is"))
        h_is(cmd);
    else if (!strcmp(t, "readfile"))
        h_readfile(cmd);
    else if (!strcmp(t, "writefile"))
        h_writefile(cmd);
    else if (!strcmp(t, "find"))
        h_find(cmd);
    else if (!strcmp(t, "watch"))
        h_watch(cmd);
    else if (!strcmp(t, "stopwatch"))
        h_stopwatch(cmd);
    else if (!strcmp(t, "exec"))
        h_exec(cmd);
    else if (!strcmp(t, "forwardlocalsocketserver"))
        h_forward_server(cmd);
    else if (!strcmp(t, "socketdata"))
        h_socket_data(cmd);
    else if (!strcmp(t, "socketclose"))
        h_socket_close(cmd);
    else if (!strcmp(t, "stopforwardserver"))
        h_stop_forward(cmd);
    else if (!strcmp(t, "error")) {
        const char *e = mstr(cmd, "Error");
        send_err(mkey(cmd, "Id"), e ? e : "unknown error");
    } else if (!strcmp(t, "exit"))
        exit(0);
}

/* ================================================================== */
/*  Thread-safe command queue for parallel execution                  */
/* ================================================================== */

#define DEFAULT_WORKER_THREADS 4
/* Handlers such as exec, find and readfile occupy their worker for as long as
   the operation runs, so a fixed pool would let a handful of long-running
   processes starve everything else -- including the cancel that would end
   them. The pool therefore grows on demand up to this many threads, which is
   the bounded equivalent of Go's goroutine per command. */
#define MAX_WORKER_THREADS 256

/* The queue grows as needed. It used to be a fixed ring that blocked the
   reader once full, which stalled the loop that resets the watchdog: with
   enough long-running commands in flight the bridge could be killed by its own
   watchdog while it was working perfectly well. It only ever holds pointers to
   commands the client has already sent, so its footprint is negligible next to
   the decoded commands themselves. */
typedef struct
{
    fifo commands; /* of value * */
    pthread_mutex_t mutex;
    pthread_cond_t not_empty;
    int shutdown;
    int busy_workers;  /* workers currently inside a handler */
    int total_workers; /* threads started so far */
} command_queue_t;

static void queue_init(command_queue_t *q)
{
    fifo_init(&q->commands);
    q->shutdown = 0;
    q->busy_workers = 0;
    q->total_workers = 0;
    pthread_mutex_init(&q->mutex, NULL);
    pthread_cond_init(&q->not_empty, NULL);
}

/* True when no idle worker is available to pick up further work. */
static bool queue_needs_worker(command_queue_t *q, int max_workers)
{
    pthread_mutex_lock(&q->mutex);
    bool need = !q->shutdown && q->total_workers < max_workers
                && q->busy_workers + (int) q->commands.count >= q->total_workers;
    pthread_mutex_unlock(&q->mutex);
    return need;
}

static void queue_destroy(command_queue_t *q)
{
    fifo_free(&q->commands);
    pthread_cond_destroy(&q->not_empty);
    pthread_mutex_destroy(&q->mutex);
}

/* Hands `cmd` to a worker. Returns -1 if the queue is shutting down or the
   command could not be queued, in which case the caller still owns it. */
static int queue_push(command_queue_t *q, value *cmd)
{
    pthread_mutex_lock(&q->mutex);
    if (q->shutdown || !fifo_push(&q->commands, cmd)) {
        pthread_mutex_unlock(&q->mutex);
        return -1;
    }
    pthread_cond_signal(&q->not_empty);
    pthread_mutex_unlock(&q->mutex);
    return 0;
}

static int queue_pop(command_queue_t *q, value **out_cmd)
{
    pthread_mutex_lock(&q->mutex);
    while (q->commands.count == 0 && !q->shutdown)
        pthread_cond_wait(&q->not_empty, &q->mutex);

    if (q->shutdown && q->commands.count == 0) {
        pthread_mutex_unlock(&q->mutex);
        return -1;
    }

    *out_cmd = (value *) fifo_pop(&q->commands);
    q->busy_workers++;
    pthread_mutex_unlock(&q->mutex);
    return 0;
}

static void queue_done(command_queue_t *q)
{
    pthread_mutex_lock(&q->mutex);
    q->busy_workers--;
    pthread_mutex_unlock(&q->mutex);
}

static void queue_shutdown(command_queue_t *q)
{
    pthread_mutex_lock(&q->mutex);
    q->shutdown = 1;
    pthread_cond_broadcast(&q->not_empty);
    pthread_mutex_unlock(&q->mutex);
}

/* ================================================================== */
/*  Worker threads for parallel command execution                     */
/* ================================================================== */

static void *worker_thread(void *arg)
{
    command_queue_t *queue = (command_queue_t *) arg;

    for (;;) {
        value *cmd = NULL;
        if (queue_pop(queue, &cmd) < 0)
            break;

        if (cmd) {
            process(cmd);
            mfreekeys(cmd);
            vfree(cmd);
        }
        queue_done(queue);
    }

    return NULL;
}

/* Adds one worker to the pool. Returns false if the thread could not start. */
static bool queue_spawn_worker(command_queue_t *q, pthread_t *slots)
{
    pthread_mutex_lock(&q->mutex);
    if (q->total_workers >= MAX_WORKER_THREADS) {
        pthread_mutex_unlock(&q->mutex);
        return false;
    }
    int idx = q->total_workers;
    pthread_mutex_unlock(&q->mutex);

    if (pthread_create(&slots[idx], NULL, (pthread_func_t) worker_thread, q) != 0)
        return false;

    pthread_mutex_lock(&q->mutex);
    q->total_workers++;
    pthread_mutex_unlock(&q->mutex);
    return true;
}

/* ================================================================== */
/*  Watchdog: exit on dead connection (runs in background thread)     */
/* ================================================================== */

static int watchdog_enabled = 0;
static time_t watchdog_timeout = 60;
static time_t watchdog_start_time = 0;
static pthread_mutex_t watchdog_mutex = PTHREAD_MUTEX_INITIALIZER;
/* Guarded by watchdog_mutex: "volatile sig_atomic_t" says nothing about
   visibility between threads, only about signal handlers. */
static bool watchdog_running = false;

static void set_watchdog_running(bool running)
{
    pthread_mutex_lock(&watchdog_mutex);
    watchdog_running = running;
    pthread_mutex_unlock(&watchdog_mutex);
}

static void *watchdog_thread_func(void *arg)
{
    (void) arg;

    for (;;) {
#ifdef _WIN32
        Sleep(1000);
#else
        usleep(1000000);
#endif

        pthread_mutex_lock(&watchdog_mutex);
        bool running = watchdog_running;
        time_t now = time(NULL);
        bool timeout = (now - watchdog_start_time > watchdog_timeout);
        pthread_mutex_unlock(&watchdog_mutex);

        if (!running)
            break;

        if (timeout) {
            fprintf(stderr, "Watchdog timeout, exiting.\n");
            fflush(stderr);
            exit(100);
        }
    }

    return NULL;
}

static void reset_watchdog(void)
{
    if (watchdog_enabled) {
        pthread_mutex_lock(&watchdog_mutex);
        watchdog_start_time = time(NULL);
        pthread_mutex_unlock(&watchdog_mutex);
    }
}

/* ================================================================== */
/*  Main loop: read CBOR packets from stdin and dispatch to workers  */
/* ================================================================== */

/* Start small: most commands are tiny. The buffer grows on demand for large
   ones -- writefile carries the whole file contents in a single command -- and
   shrinks again afterwards. */
#define RBUF_INITIAL (64 * 1024)

/* Executable self-deletion, used so a bridge copied onto a remote device
   leaves nothing behind. */
static void delete_self(void)
{
    char exe_path[PATH_MAX];
#ifdef _WIN32
    DWORD len = GetModuleFileNameA(NULL, exe_path, sizeof(exe_path));
    if (len > 0 && len < sizeof(exe_path))
        unlink(exe_path);
#elif defined(__APPLE__)
    uint32_t size = sizeof(exe_path);
    if (_NSGetExecutablePath(exe_path, &size) == 0)
        unlink(exe_path);
#else
    ssize_t len = readlink("/proc/self/exe", exe_path, sizeof(exe_path) - 1);
    if (len > 0) {
        exe_path[len] = '\0';
        unlink(exe_path);
    }
#endif
}

/* Matches one command line option, accepting every spelling Go's flag package
   accepts: "-name value", "--name value", "-name=value" and "--name=value".
   The client uses the Go spelling, so parsing only "--name=value" would
   silently ignore every option it passes. Returns the value, or NULL. */
static const char *match_opt(int argc, char **argv, int *i, const char *name)
{
    const char *arg = argv[*i];
    if (arg[0] != '-')
        return NULL;
    arg++;
    if (arg[0] == '-')
        arg++;

    size_t n = strlen(name);
    if (strncmp(arg, name, n) != 0)
        return NULL;
    if (arg[n] == '=')
        return arg + n + 1;
    if (arg[n] != '\0')
        return NULL;
    if (*i + 1 >= argc)
        return NULL;
    (*i)++;
    return argv[*i];
}

static bool match_flag(char **argv, int i, const char *name)
{
    const char *arg = argv[i];
    if (arg[0] != '-')
        return false;
    arg++;
    if (arg[0] == '-')
        arg++;
    return strcmp(arg, name) == 0;
}

int main(int argc, char **argv)
{
#ifdef _WIN32
    /* Set stdin/stdout to binary mode to prevent 0x1A (Ctrl-Z) from being
     * treated as EOF. This is critical for CBOR data which may contain 0x1A. */
    {
        unsigned long mode = 0x8000; /* _O_BINARY */
        _setmode(_fileno(stdin), (int) mode);
        _setmode(_fileno(stdout), (int) mode);
        _setmode(_fileno(stderr), (int) mode);
    }
    /* Initialize Winsock for socket forwarding */
    {
        WSADATA wsa;
        WSAStartup(MAKEWORD(2, 2), &wsa);
    }
#endif

    /* Matches Go: the watchdog is on by default so that a bridge whose client
       has gone away does not linger on the remote device forever. */
    watchdog_timeout = 60;
    watchdog_enabled = 1;

    int num_workers = DEFAULT_WORKER_THREADS;

    for (int i = 1; i < argc; i++) {
        const char *val;
        if ((val = match_opt(argc, argv, &i, "watchdogTimeout")) != NULL) {
            watchdog_timeout = atoi(val);
            watchdog_enabled = watchdog_timeout > 0;
        } else if ((val = match_opt(argc, argv, &i, "workers")) != NULL) {
            num_workers = atoi(val);
            if (num_workers < 1)
                num_workers = 1;
            if (num_workers > MAX_WORKER_THREADS)
                num_workers = MAX_WORKER_THREADS;
        } else if (match_flag(argv, i, "deleteOnStart")) {
            delete_self();
        } else {
            fprintf(stderr, "cmdbridge: ignoring unknown option \"%s\"\n", argv[i]);
        }
    }

    setbuf(stderr, NULL); /* unbuffered stderr for real-time output */

    pthread_mutex_init(&output_mutex, NULL);
    cancel_init();
    exec_registry_init();
    watch_ids_init();
    socket_forward_init();

    if (watchdog_enabled) {
        pthread_mutex_lock(&watchdog_mutex);
        watchdog_start_time = time(NULL);
        pthread_mutex_unlock(&watchdog_mutex);
        watchdog_running = true;
        pthread_t watchdog_tid;
        pthread_create(&watchdog_tid, NULL, (pthread_func_t) watchdog_thread_func, NULL);
        pthread_detach(watchdog_tid);
    }

    /* Initialize command queue */
    command_queue_t command_queue;
    queue_init(&command_queue);

    /* Start the initial worker threads; more are added on demand below. */
    pthread_t workers[MAX_WORKER_THREADS];
    for (int i = 0; i < num_workers; i++) {
        if (!queue_spawn_worker(&command_queue, workers))
            break;
    }

    /* Read raw CBOR from stdin (no magic marker prefix, matching Go implementation) */
    size_t rcap = RBUF_INITIAL;
    uint8_t *rb = (uint8_t *) xmalloc(rcap);
    size_t rpos = 0, rlen = 0;
    bool eof = false;

    while (!eof) {
        /* Consume every complete value currently in the buffer. */
        while (rlen > rpos) {
            size_t sz = 0;
            cbor_scan_result st = cbor_scan(rb + rpos, rlen - rpos, &sz);
            if (st == CBOR_INCOMPLETE)
                break; /* need more data, keep what we have */
            if (st == CBOR_INVALID) {
                /* Not the start of a value: resynchronise one byte at a time. */
                rpos++;
                continue;
            }

            value *cmd = decode(rb + rpos, sz);
            rpos += sz;
            if (!cmd)
                continue;

            if (queue_push(&command_queue, cmd) < 0) {
                mfreekeys(cmd);
                vfree(cmd);
                eof = true;
                break;
            }
            /* Grow the pool when every worker is occupied, so that long
               running commands cannot block the ones behind them. */
            if (queue_needs_worker(&command_queue, MAX_WORKER_THREADS))
                queue_spawn_worker(&command_queue, workers);
        }
        if (eof)
            break;

        /* Reclaim the consumed prefix. */
        if (rpos > 0) {
            memmove(rb, rb + rpos, rlen - rpos);
            rlen -= rpos;
            rpos = 0;
        }

        /* The buffer deliberately keeps its high-water mark. Shrinking it after
           a large command and growing it again for the next one measurably
           increases RSS: the allocator does not hand the released region back,
           and the next command's payload no longer fits where the previous one
           was. Keeping the buffer makes repeated large writes flat instead. */

        /* A single command carries whole file contents, so the buffer grows to
           whatever the peer actually sends -- there is no fixed ceiling, just
           as there was none in Go. Growth is driven by bytes that really
           arrived, never by a size a header merely claims. */
        if (rlen == rcap) {
            size_t ncap = rcap * 2;
            uint8_t *nb = (ncap > rcap) ? (uint8_t *) realloc(rb, ncap) : NULL;
            if (!nb) {
                /* Report it rather than dropping the command silently: the
                   client would otherwise wait for a reply that never comes. */
                fprintf(
                    stderr,
                    "cmdbridge: cannot buffer a command of %zu bytes, discarding\n",
                    rcap);
                send_err_type(
                    0, "command too large to buffer", "Utils.CommandTooLargeError");
                rlen = 0;
                rpos = 0;
                continue;
            }
            rb = nb;
            rcap = ncap;
        }

        ssize_t n = read(STDIN_FILENO, rb + rlen, rcap - rlen);
        if (n < 0) {
            if (errno == EINTR)
                continue;
            break;
        }
        if (n == 0)
            break; /* EOF */
        rlen += (size_t) n;
        reset_watchdog();
    }

    /* Shutdown: wait for queue to drain, then stop workers */
    queue_shutdown(&command_queue);
    for (int i = 0; i < command_queue.total_workers; i++)
        pthread_join(workers[i], NULL);
    queue_destroy(&command_queue);

    if (watchdog_enabled)
        set_watchdog_running(false);

    free(rb);
    return 0;
}
