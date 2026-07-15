// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
//
// Windows process execution, signalling and tree termination. Implements
// exec_run(), exec_kill() and plat_signal() as declared in exec.c.
// Included by cmdbridge.c — do not compile separately.

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

static void enumerate_children(DWORD ppid, proc_entry_t *entries, int max_entries, int *count, int depth)
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

static bool exec_run(value *cmd, value *exec_map, value *cmd_args, int *out_code)
{
    int code = -1;
    /* Zeroed up front: the error paths below jump to win_cleanup, which
       inspects these handles, and a goto must not skip their initialisation. */
    PROCESS_INFORMATION pi;
    ZeroMemory(&pi, sizeof(pi));

    /* Build wide command line with proper argument quoting */
    wchar_t cmd_line[8192];
    cmd_line[0] = L'\0';
    size_t pos = 0;
    size_t cmd_cap = sizeof(cmd_line) / sizeof(wchar_t);
    for (size_t i = 0; i < cmd_args->nkids && pos < cmd_cap - 10; i++) {
        if (cmd_args->kids[i]->type == V_STRING) {
            wchar_t warg[4096];
            /* The -1 source length makes the conversion include the
               terminator, so the character count is one less. */
            int converted = MultiByteToWideChar(
                CP_UTF8, 0, cmd_args->kids[i]->str, -1, warg, sizeof(warg) / sizeof(warg[0]));
            if (converted <= 1)
                continue;
            int len = converted - 1;
            /* Check if quoting is needed: spaces, tabs, quotes, or leading/trailing spaces */
            bool need_quote = false;
            for (int j = 0; warg[j] && j < len; j++) {
                if (warg[j] == L' ' || warg[j] == L'\t' || warg[j] == L'"') {
                    need_quote = true;
                    break;
                }
            }
            if (!need_quote && len > 0) {
                if (warg[0] == L' ' || warg[0] == L'\t')
                    need_quote = true;
                if ((!need_quote) && (warg[len - 1] == L' ' || warg[len - 1] == L'\t'))
                    need_quote = true;
            }
            if (pos > 0) {
                if (pos < cmd_cap) {
                    cmd_line[pos++] = L' ';
                } else {
                    break;
                }
            }
            if (need_quote) {
                if (pos >= cmd_cap - 2)
                    break;
                cmd_line[pos++] = L'"';
                for (int j = 0; warg[j] && j < len && pos < cmd_cap - 2; j++) {
                    if (warg[j] == L'\\' || warg[j] == L'"') {
                        /* Count consecutive backslashes */
                        int bs = 0;
                        int k = j;
                        while (k < len && warg[k] == L'\\') {
                            bs++;
                            k++;
                        }
                        if (warg[k] == L'"') {
                            /* Escape all backslashes before the quote */
                            for (int b = 0; b < bs; b++) {
                                if (pos >= cmd_cap - 1)
                                    break;
                                cmd_line[pos++] = L'\\';
                            }
                        }
                        j = k - 1; /* advance to last backslash */
                    }
                    if (pos < cmd_cap - 1)
                        cmd_line[pos++] = warg[j];
                }
                if (pos < cmd_cap)
                    cmd_line[pos++] = L'"';
            } else {
                for (int j = 0; warg[j] && j < len && pos < cmd_cap - 1; j++) {
                    cmd_line[pos++] = warg[j];
                }
            }
        }
    }
    if (pos < cmd_cap)
        cmd_line[pos] = L'\0';

    /* Read stdin data */
    value *stdin_d = exec_map ? mfind(exec_map, "Stdin") : mfind(cmd, "Stdin");

    /* Create pipes: stdout, stderr, and optionally stdin */
    HANDLE hReadStdout = NULL, hWriteStdout = NULL;
    HANDLE hReadStderr = NULL, hWriteStderr = NULL;
    HANDLE hReadStdin = NULL, hWriteStdin = NULL;
    SECURITY_ATTRIBUTES sa = {sizeof(SECURITY_ATTRIBUTES), NULL, TRUE};

    if (!CreatePipe(&hReadStdout, &hWriteStdout, &sa, 0)
        || !CreatePipe(&hReadStderr, &hWriteStderr, &sa, 0)) {
        send_err(mkey(cmd, "Id"), "pipe creation failed");
        goto win_cleanup;
    }
    SetHandleInformation(hReadStdout, HANDLE_FLAG_INHERIT, 0);
    SetHandleInformation(hReadStderr, HANDLE_FLAG_INHERIT, 0);

    /* Create stdin pipe if Stdin data is provided */
    if (stdin_d && stdin_d->type == V_BYTES && stdin_d->nkids > 0) {
        if (!CreatePipe(&hReadStdin, &hWriteStdin, &sa, 0)) {
            send_err(mkey(cmd, "Id"), "stdin pipe creation failed");
            goto win_cleanup;
        }
        SetHandleInformation(hReadStdin, HANDLE_FLAG_INHERIT, 0);
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

    if (!CreateProcessW(NULL, cmd_line, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi)) {
        send_err(mkey(cmd, "Id"), "exec failed");
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
        /* Write stdin data and close the write end */
        DWORD written;
        WriteFile(hWriteStdin, stdin_d->bytes, (DWORD) stdin_d->nkids, &written, NULL);
        CloseHandle(hWriteStdin);
        hWriteStdin = NULL;
    }

    /* Read stdout and stderr using PeekNamedPipe + non-blocking reads */
    uint8_t buf[1024];
    DWORD n;
    bool stdout_done = false, stderr_done = false;
    int loop_count = 0;

    while (!stdout_done || !stderr_done) {
        loop_count++;
        if (!stdout_done) {
            if (!PeekNamedPipe(hReadStdout, NULL, 0, NULL, &n, NULL)) {
                /* Pipe closed or error — mark as done */
                stdout_done = true;
            } else if (n > 0) {
                if (ReadFile(hReadStdout, buf, sizeof(buf), &n, NULL) && n > 0) {
                    value *m = mk3(
                        "Type", vs("execdata"), "Id", vi(mkey(cmd, "Id")), "Stdout", vy(buf, n));
                    size_t l;
                    uint8_t *c = encode(m, &l);
                    if (c) {
                        send_pkt(c, l);
                        free(c);
                    }
                    mfreekeys(m);
                    vfree(m);
                } else {
                    stdout_done = true;
                }
            }
        }
        if (!stderr_done) {
            if (!PeekNamedPipe(hReadStderr, NULL, 0, NULL, &n, NULL)) {
                /* Pipe closed or error — mark as done */
                stderr_done = true;
            } else if (n > 0) {
                if (ReadFile(hReadStderr, buf, sizeof(buf), &n, NULL) && n > 0) {
                    value *m = mk3(
                        "Type", vs("execdata"), "Id", vi(mkey(cmd, "Id")), "Stderr", vy(buf, n));
                    size_t l;
                    uint8_t *c = encode(m, &l);
                    if (c) {
                        send_pkt(c, l);
                        free(c);
                    }
                    mfreekeys(m);
                    vfree(m);
                } else {
                    stderr_done = true;
                }
            }
        }

        if (loop_count % 100 == 0)
            Sleep(1);

        /* Check for cancellation */
        if (is_cancelled(mkey(cmd, "Id"))) {
            code = exec_kill(mkey(cmd, "Id"));
            break;
        }

        /* Check if process has exited */
        DWORD waitResult = WaitForSingleObject(pi.hProcess, 0);
        if (waitResult == WAIT_OBJECT_0) {
            /* Process exited — drain any remaining data */
            while (!stdout_done && PeekNamedPipe(hReadStdout, NULL, 0, NULL, &n, NULL) && n > 0) {
                if (ReadFile(hReadStdout, buf, sizeof(buf), &n, NULL) && n > 0) {
                    value *m = mk3(
                        "Type", vs("execdata"), "Id", vi(mkey(cmd, "Id")), "Stdout", vy(buf, n));
                    size_t l;
                    uint8_t *c = encode(m, &l);
                    if (c) {
                        send_pkt(c, l);
                        free(c);
                    }
                    mfreekeys(m);
                    vfree(m);
                } else {
                    stdout_done = true;
                }
            }
            while (!stderr_done && PeekNamedPipe(hReadStderr, NULL, 0, NULL, &n, NULL) && n > 0) {
                if (ReadFile(hReadStderr, buf, sizeof(buf), &n, NULL) && n > 0) {
                    value *m = mk3(
                        "Type", vs("execdata"), "Id", vi(mkey(cmd, "Id")), "Stderr", vy(buf, n));
                    size_t l;
                    uint8_t *c = encode(m, &l);
                    if (c) {
                        send_pkt(c, l);
                        free(c);
                    }
                    mfreekeys(m);
                    vfree(m);
                } else {
                    stderr_done = true;
                }
            }
            break;
        }
    }

    /* Get exit code */
    DWORD exitCode;
    if (pi.hProcess)
        GetExitCodeProcess(pi.hProcess, &exitCode);
    else
        exitCode = 1;
    code = (int) exitCode;

    clear_cancelled(mkey(cmd, "Id"));

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
    *out_code = code;
    return true;
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
