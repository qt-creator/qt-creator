// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
//
// Windows process execution, signalling and tree termination. Implements
// exec_run(), exec_kill() and plat_signal() as declared in exec.c.
// Included by cmdbridge.c - do not compile separately.

#ifdef _WIN32
/* Sends the named signal to `pid`. Windows has no signals, so every kind the
   protocol defines terminates the process, matching os.Process.Signal there.
   Returns 0, or -1 with errno set; EINVAL means an unknown name. */

/* Collects the descendants of `ppid` into `entries`, appending at *count.
   Every child has to advance the write position, otherwise siblings overwrite
   each other and only one branch of the tree is ever seen. */

/* --- Process tree termination --- */

typedef struct
{
    DWORD pid;
    DWORD ppid;
} proc_entry_t;

static void enumerate_children(
    DWORD ppid, proc_entry_t *entries, int max_entries, int *count, int depth)
{
    if (depth > 64 || *count >= max_entries)
        return;
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE)
        return;
    PROCESSENTRY32 pe;
    pe.dwSize = sizeof(pe);
    if (Process32First(snap, &pe)) {
        do {
            if (pe.th32ParentProcessID == ppid && pe.th32ProcessID != ppid) {
                if (*count >= max_entries)
                    break;
                entries[*count].pid = pe.th32ProcessID;
                entries[*count].ppid = ppid;
                (*count)++;
                enumerate_children(pe.th32ProcessID, entries, max_entries, count, depth + 1);
            }
        } while (Process32Next(snap, &pe));
    }
    CloseHandle(snap);
}

static void terminate_tree(DWORD pid)
{
    /* Snapshot the descendants *before* killing the root: once it is gone the
       children are reparented and can no longer be found through it. */
    enum { MAX_TREE_ENTRIES = 4096 };
    proc_entry_t *entries = (proc_entry_t *) calloc(MAX_TREE_ENTRIES, sizeof(proc_entry_t));
    int count = 0;
    if (entries)
        enumerate_children(pid, entries, MAX_TREE_ENTRIES, &count, 0);

    HANDLE h = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
    if (h) {
        TerminateProcess(h, 1);
        CloseHandle(h);
    }

    for (int i = 0; i < count; i++) {
        HANDLE ch = OpenProcess(PROCESS_TERMINATE, FALSE, entries[i].pid);
        if (ch) {
            TerminateProcess(ch, 1);
            CloseHandle(ch);
        }
    }
    free(entries);
}



static int exec_kill(int id)
{
    exec_val_t info = exec_find(id);
    if (info.h) {
        terminate_tree((DWORD) info.pid);
        exec_unregister(id);
        return 0;
    }
    return -1;
}

/* Writes the whole Stdin payload and closes the pipe, from a thread of its own.
   Doing it inline blocked exec for good once the payload outgrew the pipe
   buffer (64K) and the child was waiting for us to read its output. Owns its
   copy of the data: the command it came from is freed as soon as the handler
   returns, which can be long before the child has read everything. */
typedef struct
{
    HANDLE pipe;
    uint8_t *data;
    size_t len;
} exec_stdin_writer_t;

static void *exec_stdin_thread(void *arg)
{
    exec_stdin_writer_t *w = (exec_stdin_writer_t *) arg;
    size_t off = 0;
    while (off < w->len) {
        DWORD written = 0;
        if (!WriteFile(w->pipe, w->data + off, (DWORD) (w->len - off), &written, NULL)
            || written == 0)
            break; /* the child closed its end or died */
        off += written;
    }
    CloseHandle(w->pipe);
    free(w->data);
    free(w);
    return NULL;
}

/* --- Command line assembly --- */

/* A growable UTF-16 string. Always keeps room for a terminator past `len`. */
typedef struct
{
    wchar_t *w;
    size_t len;
    size_t cap;
} wbuf;

static bool wbuf_reserve(wbuf *b, size_t extra)
{
    if (b->w && b->len + extra + 1 <= b->cap)
        return true;
    size_t ncap = b->cap ? b->cap : 256;
    while (ncap < b->len + extra + 1)
        ncap *= 2;
    wchar_t *n = (wchar_t *) realloc(b->w, ncap * sizeof(wchar_t));
    if (!n)
        return false;
    b->w = n;
    b->cap = ncap;
    return true;
}

static bool wbuf_putc(wbuf *b, wchar_t c)
{
    if (!wbuf_reserve(b, 1))
        return false;
    b->w[b->len++] = c;
    return true;
}

/* Appends one argument, quoted so that CommandLineToArgvW() - which is what the
   child's own startup code uses - parses it back unchanged. */
static bool cmdline_append(wbuf *b, const char *utf8)
{
    int need = MultiByteToWideChar(CP_UTF8, 0, utf8, -1, NULL, 0);
    if (need <= 0)
        return false;
    wchar_t *warg = (wchar_t *) malloc((size_t) need * sizeof(wchar_t));
    if (!warg)
        return false;
    if (MultiByteToWideChar(CP_UTF8, 0, utf8, -1, warg, need) <= 0) {
        free(warg);
        return false;
    }
    size_t len = wcslen(warg);

    /* An empty argument has to be quoted, or it disappears entirely. */
    bool quote = (len == 0);
    for (size_t i = 0; i < len && !quote; i++) {
        if (warg[i] == L' ' || warg[i] == L'\t' || warg[i] == L'"')
            quote = true;
    }

    bool ok = (b->len == 0) || wbuf_putc(b, L' ');
    if (ok && quote)
        ok = wbuf_putc(b, L'"');
    for (size_t i = 0; ok && i < len; i++) {
        if (warg[i] == L'\\') {
            size_t bs = 0;
            while (i + bs < len && warg[i + bs] == L'\\')
                ++bs;
            /* Backslashes are literal unless they run into a quote - including
               the closing one added below. Then each of them needs doubling. */
            bool before_quote = (i + bs < len && warg[i + bs] == L'"')
                                || (i + bs == len && quote);
            for (size_t r = 0; ok && r < bs * (before_quote ? 2 : 1); ++r)
                ok = wbuf_putc(b, L'\\');
            i += bs - 1;
        } else if (warg[i] == L'"') {
            ok = wbuf_putc(b, L'\\') && wbuf_putc(b, L'"');
        } else {
            ok = wbuf_putc(b, warg[i]);
        }
    }
    if (ok && quote)
        ok = wbuf_putc(b, L'"');
    free(warg);
    return ok;
}

/* Forwards whatever `pipe` has ready as an execdata packet under `key`, which
   is "Stdout" or "Stderr". Sets *done once the pipe is finished. Returns whether
   anything was forwarded, so the caller can tell an idle moment from a busy one.
   One helper for both streams and both call sites: the four copies this replaces
   had already started to differ. */
static bool exec_forward_available(HANDLE pipe, int id, const char *key, bool *done)
{
    if (*done)
        return false;

    DWORD avail = 0;
    if (!PeekNamedPipe(pipe, NULL, 0, NULL, &avail, NULL)) {
        *done = true; /* closed, or the child is gone */
        return false;
    }
    if (avail == 0)
        return false;

    uint8_t buf[4096];
    DWORD got = 0;
    if (!ReadFile(pipe, buf, sizeof(buf), &got, NULL) || got == 0) {
        *done = true;
        return false;
    }

    value *m = mk3("Type", vs("execdata"), "Id", vi(id), key, vy(buf, got));
    size_t l;
    uint8_t *c = encode(m, &l);
    if (c) {
        send_pkt(c, l);
        free(c);
    }
    mfreekeys(m);
    vfree(m);
    return true;
}

/* Turns the Env array into the double-NUL terminated UTF-16 block
   CreateProcessW wants. Returns NULL when there is nothing to pass, which means
   "inherit", matching the POSIX side. */
static wchar_t *exec_build_env_block(value *env)
{
    if (!env || env->type != V_ARRAY)
        return NULL;

    /* Room for the two terminators that close the block even when it is empty,
       which is how "run with no environment at all" is spelled. */
    size_t wide_total = 2;
    for (size_t i = 0; i < env->nkids; i++) {
        if (env->kids[i]->type != V_STRING)
            continue;
        int n = MultiByteToWideChar(CP_UTF8, 0, env->kids[i]->str, -1, NULL, 0);
        if (n > 0)
            wide_total += (size_t) n;
    }

    wchar_t *block = (wchar_t *) malloc(wide_total * sizeof(wchar_t));
    if (!block)
        return NULL;

    size_t pos = 0;
    for (size_t i = 0; i < env->nkids; i++) {
        if (env->kids[i]->type != V_STRING)
            continue;
        int n = MultiByteToWideChar(
            CP_UTF8, 0, env->kids[i]->str, -1, block + pos, (int) (wide_total - pos));
        if (n > 0)
            pos += (size_t) n; /* includes the entry's own terminator */
    }
    block[pos] = L'\0';
    block[pos + 1] = L'\0';
    return block;
}

static bool exec_run(value *cmd, value *exec_map, value *cmd_args, int *out_code)
{
    int code = -1;
    /* False once an error packet has been sent, so that no execresult follows
       it - the client would otherwise see two answers to one command. */
    bool ok = true;
    /* Set when exec_kill() has taken the process handle away from us. */
    bool cancelled = false;
    /* Zeroed up front: the error paths below jump to win_cleanup, which
       inspects these handles, and a goto must not skip their initialisation. */
    HANDLE hReadStdout = NULL, hWriteStdout = NULL;
    HANDLE hReadStderr = NULL, hWriteStderr = NULL;
    HANDLE hReadStdin = NULL, hWriteStdin = NULL;
    PROCESS_INFORMATION pi;
    ZeroMemory(&pi, sizeof(pi));
    wchar_t *env_block = NULL;

    /* Build the command line. It grows as needed: the fixed buffers this
       replaces dropped any argument longer than 4096 characters outright and cut
       the line off at 8192, so a long compiler invocation ran with a silently
       shortened argument list. Windows itself allows 32767. */
    wbuf cmd_line = {NULL, 0, 0};
    for (size_t i = 0; i < cmd_args->nkids; i++) {
        if (cmd_args->kids[i]->type != V_STRING)
            continue;
        if (!cmdline_append(&cmd_line, cmd_args->kids[i]->str)) {
            send_err(mkey(cmd, "Id"), "could not build the command line");
            ok = false;
            goto win_cleanup;
        }
    }
    if (!cmd_line.w) {
        code = 127; /* nothing to run; answered as a failure to start */
        goto win_cleanup;
    }
    cmd_line.w[cmd_line.len] = L'\0'; /* wbuf_reserve() always keeps room */

    /* Read stdin data */
    value *stdin_d = exec_map ? mfind(exec_map, "Stdin") : mfind(cmd, "Stdin");

    /* Create pipes: stdout, stderr, and optionally stdin */
    SECURITY_ATTRIBUTES sa = {sizeof(SECURITY_ATTRIBUTES), NULL, TRUE};

    if (!CreatePipe(&hReadStdout, &hWriteStdout, &sa, 0)
        || !CreatePipe(&hReadStderr, &hWriteStderr, &sa, 0)) {
        send_err(mkey(cmd, "Id"), "pipe creation failed");
        ok = false;
        goto win_cleanup;
    }
    SetHandleInformation(hReadStdout, HANDLE_FLAG_INHERIT, 0);
    SetHandleInformation(hReadStderr, HANDLE_FLAG_INHERIT, 0);

    /* Create stdin pipe if Stdin data is provided. Only our own end is made
       non-inheritable: clearing the flag on the read end handed the child a
       handle it could not use, so its standard input was never connected, and
       left our write end inheritable so the child held it open itself. */
    if (stdin_d && stdin_d->type == V_BYTES && stdin_d->nkids > 0) {
        if (!CreatePipe(&hReadStdin, &hWriteStdin, &sa, 0)) {
            send_err(mkey(cmd, "Id"), "stdin pipe creation failed");
            ok = false;
            goto win_cleanup;
        }
        SetHandleInformation(hWriteStdin, HANDLE_FLAG_INHERIT, 0);
    }

    STARTUPINFOW si;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdOutput = hWriteStdout;
    si.hStdError = hWriteStderr;
    if (hReadStdin) {
        si.hStdInput = hReadStdin;
    } else {
        si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    }

    /* An explicit Env replaces the environment, as it does on POSIX and in Go;
       without it the child silently inherited ours. */
    env_block = exec_build_env_block(exec_map ? mfind(exec_map, "Env") : mfind(cmd, "Env"));

    if (!CreateProcessW(
            NULL,
            cmd_line.w,
            NULL,
            NULL,
            TRUE,
            env_block ? CREATE_UNICODE_ENVIRONMENT : 0,
            env_block,
            NULL,
            &si,
            &pi)) {
        /* Report a failure to start as an exit code, the way the POSIX side
           does for a program it cannot find: Go answers every exec with an
           execresult, and an error packet surfaces as an exception instead. */
        code = 127;
        goto win_cleanup;
    }

    /* Register for cancellation (store process handle for tree termination) */
    {
        exec_val_t val;
        val.pid = (int) pi.dwProcessId;
        val.h = pi.hProcess;
        exec_register(mkey(cmd, "Id"), val);
    }

    /* Register for cancellation */
    register_cancel(mkey(cmd, "Id"));

    /* Close write ends in parent */
    CloseHandle(hWriteStdout);
    hWriteStdout = NULL;
    CloseHandle(hWriteStderr);
    hWriteStderr = NULL;
    if (hWriteStdin) {
        /* Handed to a thread that owns the handle and its own copy of the data
           from here on, so a child that does not drain its input cannot stop us
           from reading its output. */
        exec_stdin_writer_t *writer = (exec_stdin_writer_t *) malloc(sizeof(*writer));
        uint8_t *copy = (uint8_t *) malloc(stdin_d->nkids);
        if (writer && copy) {
            memcpy(copy, stdin_d->bytes, stdin_d->nkids);
            writer->pipe = hWriteStdin;
            writer->data = copy;
            writer->len = stdin_d->nkids;
            pthread_t wtid;
            if (pthread_create(&wtid, NULL, exec_stdin_thread, writer) == 0) {
                pthread_detach(wtid);
                hWriteStdin = NULL; /* the thread closes it */
            } else {
                free(copy);
                free(writer);
            }
        } else {
            free(copy);
            free(writer);
        }
        if (hWriteStdin) {
            /* Could not hand it over; close so the child sees end of input. */
            CloseHandle(hWriteStdin);
            hWriteStdin = NULL;
        }
    }

    /* Forward whatever is readable on `pipe` right now. Returns false once the
       pipe is finished, so the caller can stop looking at it. */
    bool stdout_done = false, stderr_done = false;

    while (!stdout_done || !stderr_done) {
        bool moved = false;
        if (!stdout_done)
            moved |= exec_forward_available(hReadStdout, mkey(cmd, "Id"), "Stdout", &stdout_done);
        if (!stderr_done)
            moved |= exec_forward_available(hReadStderr, mkey(cmd, "Id"), "Stderr", &stderr_done);

        /* Check for cancellation */
        if (is_cancelled(mkey(cmd, "Id"))) {
            code = exec_kill(mkey(cmd, "Id"));
            cancelled = true;
            break;
        }

        /* Check if process has exited */
        if (WaitForSingleObject(pi.hProcess, 0) == WAIT_OBJECT_0) {
            /* Process exited - drain what is left and stop. */
            while (exec_forward_available(hReadStdout, mkey(cmd, "Id"), "Stdout", &stdout_done)) {
            }
            while (exec_forward_available(hReadStderr, mkey(cmd, "Id"), "Stderr", &stderr_done)) {
            }
            break;
        }

        /* Nothing arrived, so wait a little instead of spinning. Peeking a
           hundred times per millisecond kept a core busy for the whole run of
           any command that was merely slow. */
        if (!moved)
            Sleep(5);
    }

    if (!cancelled) {
        DWORD exitCode = 0;
        /* Only valid while we still own the handle, which is why the cancelled
           path above does not come through here: exec_kill() has closed it. */
        code = GetExitCodeProcess(pi.hProcess, &exitCode) ? (int) exitCode : -1;
    }

    clear_cancelled(mkey(cmd, "Id"));
    /* Drop the registry entry on every path, not just after a cancel, or it
       keeps one allocation and a stale pid for every command ever run. */
    exec_unregister(mkey(cmd, "Id"));

win_cleanup:
    if (hReadStdout)
        CloseHandle(hReadStdout);
    if (hWriteStdout)
        CloseHandle(hWriteStdout);
    if (hReadStderr)
        CloseHandle(hReadStderr);
    if (hWriteStderr)
        CloseHandle(hWriteStderr);
    if (hReadStdin)
        CloseHandle(hReadStdin);
    if (hWriteStdin)
        CloseHandle(hWriteStdin);
    if (pi.hProcess)
        CloseHandle(pi.hProcess);
    if (pi.hThread)
        CloseHandle(pi.hThread);
    free(env_block);
    free(cmd_line.w);
    *out_code = code;
    return ok;
}

static int plat_signal(int pid, const char *sig)
{
    if (strcmp(sig, "terminate") != 0 && strcmp(sig, "kill") != 0
        && strcmp(sig, "interrupt") != 0) {
        errno = EINVAL;
        return -1;
    }
    HANDLE h = OpenProcess(PROCESS_TERMINATE, FALSE, (DWORD) pid);
    if (!h) {
        errno = win_to_errno(GetLastError());
        return -1;
    }
    BOOL ok = TerminateProcess(h, 1);
    DWORD err = GetLastError();
    CloseHandle(h);
    if (!ok) {
        errno = win_to_errno(err);
        return -1;
    }
    return 0;
}


#endif /* _WIN32 */
