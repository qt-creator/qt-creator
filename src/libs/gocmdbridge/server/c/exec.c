// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
//
// Exec and signal command handlers, and the registry of running processes.
// Starting, signalling and killing a process is platform specific and is
// included below.
// Included by cmdbridge.c - do not compile separately.

/* ================================================================== */
/*  Exec -- run external command                                      */
/* ================================================================== */

/* ================================================================== */
/*  Running process registry (command Id -> pid/handle)               */
/* ================================================================== */

/* Both members are live on Windows: the handle is needed to wait on the
   process, the pid to walk its children. They must not share storage. */
typedef struct
{
    int pid;
#ifdef _WIN32
    HANDLE h;
#endif
} exec_val_t;

/* Command Id -> the process it started. Grows as needed: a fixed table would
   silently refuse to record further processes once full, and cancelling one of
   them would then find nothing to kill. */
static imap exec_jobs;
static pthread_mutex_t exec_mutex = PTHREAD_MUTEX_INITIALIZER;

static void exec_registry_init(void)
{
    imap_init(&exec_jobs);
}

static void exec_register(int id, exec_val_t val)
{
    exec_val_t *job = (exec_val_t *) malloc(sizeof(*job));
    if (!job) {
        fprintf(stderr, "cmdbridge: cannot register process for command %d\n", id);
        return;
    }
    *job = val;

    pthread_mutex_lock(&exec_mutex);
    if (!imap_put(&exec_jobs, id, job)) {
        pthread_mutex_unlock(&exec_mutex);
        free(job);
        fprintf(stderr, "cmdbridge: cannot register process for command %d\n", id);
        return;
    }
    pthread_mutex_unlock(&exec_mutex);
}

/* Drops the registration. The process handle stored on Windows is *not* closed
   here: exec_run() owns it and still needs it after a cancel to read the exit
   code, and closing it in both places closed an unrelated handle that had
   meanwhile been given the same number. */
static void exec_unregister(int id)
{
    pthread_mutex_lock(&exec_mutex);
    exec_val_t *job = (exec_val_t *) imap_get(&exec_jobs, id);
    if (job)
        imap_remove(&exec_jobs, id);
    pthread_mutex_unlock(&exec_mutex);
    free(job);
}

static exec_val_t exec_find(int id)
{
    exec_val_t v;
#ifdef _WIN32
    v.pid = 0;
    v.h = NULL;
#else
    v.pid = -1;
#endif

    pthread_mutex_lock(&exec_mutex);
    const exec_val_t *job = (const exec_val_t *) imap_get(&exec_jobs, id);
    if (job)
        v = *job;
    pthread_mutex_unlock(&exec_mutex);
    return v;
}

/* ------------------------------------------------------------------
 * Process handling for this platform, built on the registry above:
 *
 *   bool exec_run(value *cmd, value *exec_map, value *cmd_args, int *code)
 *       Runs the command and streams its output as execdata packets,
 *       storing the exit code in *code. Returns false when it has
 *       already answered with an error and no execresult should follow.
 *   int  exec_kill(int id)
 *       Terminates the process registered for `id`, including any
 *       children, and returns its exit code (or -1).
 *   int  plat_signal(int pid, const char *sig)
 *       Sends the named signal to `pid`. Returns 0, or -1 with errno
 *       set; EINVAL means the name is not one the protocol defines.
 * ------------------------------------------------------------------ */

#include "exec_posix.c"
#include "exec_win.c"

static void h_exec(value *cmd)
{
    value *exec_map = mfind(cmd, "Exec");
    value *cmd_args = exec_map ? mfind(exec_map, "Args") : mfind(cmd, "Args");
    if (!cmd_args || cmd_args->type != V_ARRAY) {
        send_err(mkey(cmd, "Id"), "missing Args");
        return;
    }

    int code = -1;
    if (!exec_run(cmd, exec_map, cmd_args, &code))
        return;

    value *m = mk3("Type", vs("execresult"), "Id", vi(mkey(cmd, "Id")), "Code", vi(code));
    size_t l;
    uint8_t *c = encode(m, &l);
    if (c) {
        send_pkt(c, l);
        free(c);
    }
    mfreekeys(m);
    vfree(m);
}

static void h_signal(value *cmd)
{
    value *sig_map = mfind(cmd, "Signal");
    const char *sig = sig_map ? mstr(sig_map, "Signal") : mstr(cmd, "Signal");
    int pid = sig_map ? mkey(sig_map, "Pid") : mkey(cmd, "Pid");
    if (!sig) {
        send_err(mkey(cmd, "Id"), "missing Signal");
        return;
    }
    if (plat_signal(pid, sig) != 0) {
        if (errno == EINVAL)
            send_err(mkey(cmd, "Id"), "unknown signal");
        else
            send_os_err(mkey(cmd, "Id"), strerror(errno), errno);
        return;
    }
    send_void(mkey(cmd, "Id"), "signalsuccess");
}
