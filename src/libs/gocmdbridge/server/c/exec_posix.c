// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
//
// POSIX implementation of the exec command. Implements exec_run() as declared
// in exec.c. Included by cmdbridge.c - do not compile separately.

#ifndef _WIN32
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <sys/wait.h>

/* A pipe whose ends are not handed to unrelated children. Commands run on
   several threads at once, so between an unprotected pipe() here and the fork
   below, another thread's child would inherit these descriptors and hold the
   write end open - and this exec then waited for an end of output that could
   only come once that unrelated process exited. Spawned programs also had no
   business seeing the bridge's forwarded sockets and watch descriptors.

   dup2() clears the flag on the copies the child actually needs, so stdin,
   stdout and stderr still reach it. */
static int make_pipe(int fds[2])
{
#ifdef __APPLE__
    /* No pipe2() here; the two-step version leaves the same small window, but
       nothing better is available. */
    if (pipe(fds) < 0)
        return -1;
    for (int i = 0; i < 2; i++) {
        int flags = fcntl(fds[i], F_GETFD, 0);
        if (flags >= 0)
            fcntl(fds[i], F_SETFD, flags | FD_CLOEXEC);
    }
    return 0;
#else
    return pipe2(fds, O_CLOEXEC);
#endif
}

/* Forwards everything readable on `fd` as execdata packets under `key`, which is
   "Stdout" or "Stderr". Returns false once the pipe is at end of file or has
   failed, so the caller knows to stop polling it.

   The read must not block. The loops this replaces read until read() returned 0,
   which on a still-open pipe means waiting for the child to produce more - so a
   child that was itself waiting for the rest of its standard input never got it,
   and both sides sat there until the watchdog fired. */
static bool exec_forward_available(int fd, int id, const char *key)
{
    for (;;) {
        uint8_t buf[4096];
        ssize_t n = read(fd, buf, sizeof(buf));
        if (n > 0) {
            value *m = mk3("Type", vs("execdata"), "Id", vi(id), key, vy(buf, (size_t) n));
            size_t l;
            uint8_t *c = encode(m, &l);
            if (c) {
                send_pkt(c, l);
                free(c);
            }
            mfreekeys(m);
            vfree(m);
            continue;
        }
        if (n < 0 && errno == EINTR)
            continue;
        if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK))
            return true; /* nothing more for now, but the pipe is still open */
        return false;    /* end of file, or a broken pipe */
    }
}

/* Resolves a program name against PATH the way Go's exec.Command does: names
   containing a separator are used as-is, everything else is looked up in the
   PATH of *this* process. Returns a malloc'd path, or NULL if not found. */
static char *resolve_program(const char *name)
{
    if (!name || !*name)
        return NULL;
    if (strchr(name, '/'))
        return xstrdup(name);

    const char *path = getenv("PATH");
    if (!path || !*path)
        path = "/usr/local/bin:/usr/bin:/bin";

    size_t namelen = strlen(name);
    const char *p = path;
    while (*p) {
        const char *sep = strchr(p, ':');
        size_t dirlen = sep ? (size_t) (sep - p) : strlen(p);
        /* An empty entry means the current directory. */
        const char *dir = dirlen ? p : ".";
        size_t use = dirlen ? dirlen : 1;

        char *full = (char *) xmalloc(use + 1 + namelen + 1);
        memcpy(full, dir, use);
        full[use] = '/';
        memcpy(full + use + 1, name, namelen + 1);

        if (access(full, X_OK) == 0)
            return full;
        free(full);

        if (!sep)
            break;
        p = sep + 1;
    }
    return NULL;
}
static int exec_kill(int id)
{
    pid_t pid = (pid_t) exec_find(id).pid;
    if (pid > 0) {
        kill(pid, SIGTERM);
        for (int i = 0; i < 50; i++) {
            int status;
            if (waitpid(pid, &status, WNOHANG) > 0) {
                exec_unregister(id);
                if (WIFEXITED(status))
                    return WEXITSTATUS(status);
                if (WIFSIGNALED(status))
                    return 128 + WTERMSIG(status);
                return -1;
            }
            usleep(10000);
        }
        kill(pid, SIGKILL);
        int status = 0;
        if (waitpid(pid, &status, 0) > 0) {
            exec_unregister(id);
            if (WIFEXITED(status))
                return WEXITSTATUS(status);
            if (WIFSIGNALED(status))
                return 128 + WTERMSIG(status);
        }
        return -1;
    }
    return -1;
}


static bool exec_run(value *cmd, value *exec_map, value *cmd_args, int *out_code)
{
    int code = -1;
    /* Build argv. xmalloc: these are our own sizes, and there is nothing
       sensible to answer with if they cannot be had. */
    char **argv = (char **) xmalloc((cmd_args->nkids + 1) * sizeof(char *));
    for (size_t i = 0; i < cmd_args->nkids; i++)
        argv[i] = (cmd_args->kids[i]->type == V_STRING) ? cmd_args->kids[i]->str : (char *) " ";
    argv[cmd_args->nkids] = NULL;

    /* Build envp - when Env is present (even empty), use it explicitly;
       otherwise inherit parent environment. Matches Go: process.Env = cmd.Exec.Env */
    value *env = exec_map ? mfind(exec_map, "Env") : mfind(cmd, "Env");
    char **envp = NULL;
    if (env) {
        envp = (char **) xmalloc((env->nkids + 1) * sizeof(char *));
        for (size_t i = 0; i < env->nkids; i++)
            envp[i] = (env->kids[i]->type == V_STRING) ? env->kids[i]->str : (char *) " ";
        envp[env->nkids] = NULL;
    }

    /* Resolve the program against this process's PATH before forking, the way
       Go does, so that a supplied Env cannot break the lookup. */
    char *program = resolve_program(argv[0]);
    if (!program) {
        /* Report this as an exit code, not an error packet: that is what the
           client gets for every other startup failure (the child _exit(127)s
           when execve fails), and Go likewise always answers exec with an
           execresult. An error packet would instead surface as an exception. */
        free(argv);
        free(envp);
        *out_code = 127;
        return true;
    }

    value *stdin_d = exec_map ? mfind(exec_map, "Stdin") : mfind(cmd, "Stdin");
    bool have_stdin = stdin_d && stdin_d->type == V_BYTES && stdin_d->nkids > 0;

    int pout[2] = {-1, -1}, perr[2] = {-1, -1}, pin[2] = {-1, -1};
    if (make_pipe(pout) < 0 || make_pipe(perr) < 0 || (have_stdin && make_pipe(pin) < 0)) {
        for (int i = 0; i < 2; i++) {
            if (pout[i] >= 0)
                close(pout[i]);
            if (perr[i] >= 0)
                close(perr[i]);
            if (pin[i] >= 0)
                close(pin[i]);
        }
        free(program);
        free(argv);
        free(envp);
        send_err(mkey(cmd, "Id"), "pipe creation failed");
        return false;
    }
    pid_t pid = fork();
    if (pid < 0) {
        for (int i = 0; i < 2; i++) {
            if (pout[i] >= 0)
                close(pout[i]);
            if (perr[i] >= 0)
                close(perr[i]);
            if (pin[i] >= 0)
                close(pin[i]);
        }
        free(program);
        free(argv);
        free(envp);
        send_err(mkey(cmd, "Id"), "fork failed");
        return false;
    }
    if (pid == 0) {
        /* Only async-signal-safe calls between fork() and exec(). */
        dup2(pout[1], STDOUT_FILENO);
        dup2(perr[1], STDERR_FILENO);
        if (pin[0] >= 0) {
            dup2(pin[0], STDIN_FILENO);
            close(pin[0]);
            close(pin[1]);
        } else {
            /* No stdin data: give the child an empty stdin rather than the
               bridge's own command stream, which it must never read from. */
            int devnull = open("/dev/null", O_RDONLY);
            if (devnull >= 0) {
                dup2(devnull, STDIN_FILENO);
                close(devnull);
            } else {
                close(STDIN_FILENO);
            }
        }
        close(pout[0]);
        close(pout[1]);
        close(perr[0]);
        close(perr[1]);
        execve(program, argv, envp ? envp : environ);
        _exit(127);
    }
    close(pout[1]);
    pout[1] = -1;
    close(perr[1]);
    perr[1] = -1;
    /* Read without blocking, so that draining the child's output can never stop
       us from getting back to the poll loop that feeds its input. */
    fcntl(pout[0], F_SETFL, fcntl(pout[0], F_GETFL, 0) | O_NONBLOCK);
    fcntl(perr[0], F_SETFL, fcntl(perr[0], F_GETFL, 0) | O_NONBLOCK);
    if (pin[0] >= 0) {
        close(pin[0]);
        pin[0] = -1;
        /* Written from the poll loop below so that a child which does not read
           its input cannot block the bridge. */
        fcntl(pin[1], F_SETFL, fcntl(pin[1], F_GETFL, 0) | O_NONBLOCK);
    }
    size_t stdin_written = 0;
    free(program);

    /* Register for cancellation */
    exec_register(mkey(cmd, "Id"), ((exec_val_t) {.pid = (int) pid}));
    register_cancel(mkey(cmd, "Id"));

    /* Read stdout/stderr using poll() for cancellation support */
    bool done = false;
    /* The child is waited for in one of two places; this says which one has it
       and what it found, so the exit code never comes from an unset status. */
    int child_status = 0;
    bool reaped = false;

    while (!done) {
        struct pollfd pfd[3];
        int npfd = 0;
        int stdout_idx = -1, stderr_idx = -1, stdin_idx = -1;

        if (pout[0] >= 0) {
            stdout_idx = npfd;
            pfd[npfd].fd = pout[0];
            pfd[npfd].events = POLLIN;
            npfd++;
        }
        if (perr[0] >= 0) {
            stderr_idx = npfd;
            pfd[npfd].fd = perr[0];
            pfd[npfd].events = POLLIN;
            npfd++;
        }
        if (pin[1] >= 0) {
            stdin_idx = npfd;
            pfd[npfd].fd = pin[1];
            pfd[npfd].events = POLLOUT;
            npfd++;
        }

        int pr = poll(pfd, (nfds_t) npfd, 50); /* 50ms timeout for cancel responsiveness */
        if (pr == 0) {
            /* Timeout: check for cancel or process exit */
            if (is_cancelled(mkey(cmd, "Id"))) {
                code = exec_kill(mkey(cmd, "Id"));
                break;
            }
            if (waitpid(pid, &child_status, WNOHANG) > 0) {
                /* Reaped here, so the wait below must not run again: it would
                   fail with ECHILD and leave the status it reads unset. */
                reaped = true;
                done = true;
                break;
            }
            continue;
        }
        if (pr < 0) {
            if (errno == EINTR)
                continue;
            break;
        }

        /* Feed stdin. Closing the write end signals EOF to the child. */
        if (stdin_idx >= 0 && (pfd[stdin_idx].revents & (POLLOUT | POLLERR | POLLHUP))) {
            if (pfd[stdin_idx].revents & (POLLERR | POLLHUP)) {
                close(pin[1]);
                pin[1] = -1;
            } else {
                ssize_t w = write(
                    pin[1], stdin_d->bytes + stdin_written, stdin_d->nkids - stdin_written);
                if (w > 0)
                    stdin_written += (size_t) w;
                else if (w < 0 && errno != EAGAIN && errno != EINTR)
                    w = 0; /* treated as failure below */
                if (stdin_written >= stdin_d->nkids || (w == 0)) {
                    close(pin[1]);
                    pin[1] = -1;
                }
            }
        }

        /* Check stdout and stderr. POLLHUP can arrive with data still in the
           pipe, so both cases drain first and only then close. */
        if (stdout_idx >= 0
            && (pfd[stdout_idx].revents & (POLLIN | POLLHUP | POLLERR | POLLNVAL))) {
            if (!exec_forward_available(pout[0], mkey(cmd, "Id"), "Stdout")) {
                close(pout[0]);
                pout[0] = -1;
            }
            if (is_cancelled(mkey(cmd, "Id"))) {
                code = exec_kill(mkey(cmd, "Id"));
                break;
            }
        }

        if (stderr_idx >= 0
            && (pfd[stderr_idx].revents & (POLLIN | POLLHUP | POLLERR | POLLNVAL))) {
            if (!exec_forward_available(perr[0], mkey(cmd, "Id"), "Stderr")) {
                close(perr[0]);
                perr[0] = -1;
            }
            if (is_cancelled(mkey(cmd, "Id"))) {
                code = exec_kill(mkey(cmd, "Id"));
                break;
            }
        }

        /* Check if child has exited (both pipes closed) */
        if (pout[0] < 0 && perr[0] < 0) {
            done = true;
            break;
        }
    }

    if (pout[0] >= 0)
        close(pout[0]);
    if (perr[0] >= 0)
        close(perr[0]);
    if (pin[1] >= 0)
        close(pin[1]);

    if (!is_cancelled(mkey(cmd, "Id"))) {
        if (!reaped && waitpid(pid, &child_status, 0) > 0)
            reaped = true;
        exec_unregister(mkey(cmd, "Id"));
        if (reaped && WIFEXITED(child_status))
            code = WEXITSTATUS(child_status);
        else
            /* Match Go: return -1 for signal death (non-ExitError) */
            code = -1;
    }
    clear_cancelled(mkey(cmd, "Id"));
    free(argv);
    free(envp);
    *out_code = code;
    return true;
}

/* Sends the named signal to `pid`. Returns 0, or -1 with errno set; EINVAL
   means the name is not one the protocol defines. */
static int plat_signal(int pid, const char *sig)
{
    int signo;
    if (strcmp(sig, "terminate") == 0)
        signo = SIGTERM;
    else if (strcmp(sig, "kill") == 0)
        signo = SIGKILL;
    else if (strcmp(sig, "interrupt") == 0)
        signo = SIGINT;
    else {
        errno = EINVAL;
        return -1;
    }
    return kill(pid, signo);
}

#endif /* !_WIN32 */
