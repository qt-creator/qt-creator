/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/wait.h>
#ifdef __sun
# define PT_TRACE_ME 0
# define PT_DETACH 7
#else
# include <sys/ptrace.h>
#endif
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <assert.h>

/* For OpenBSD */
#ifndef EPROTO
# define EPROTO EINVAL
#endif

extern char **environ;

static int qtcFd;
static char *sleepMsg;
static int chldPipe[2];
static int isDebug;
static volatile int isDetached;
static volatile int chldPid;

static void __attribute__((noreturn)) doExit(int code)
{
    tcsetpgrp(0, getpid());
    puts(sleepMsg);
    fgets(sleepMsg, 2, stdin); /* Minimal size to make it wait */
    exit(code);
}

static void sendMsg(const char *msg, int num)
{
    int pidStrLen;
    int ioRet;
    char pidStr[64];

    pidStrLen = sprintf(pidStr, msg, num);
    if (!isDetached && (ioRet = write(qtcFd, pidStr, pidStrLen)) != pidStrLen) {
        fprintf(stderr, "Cannot write to creator comm socket: %s\n",
                        (ioRet < 0) ? strerror(errno) : "short write");
        isDetached = 2;
    }
}

enum {
    ArgCmd = 0,
    ArgAction,
    ArgSocket,
    ArgMsg,
    ArgDir,
    ArgEnv,
    ArgExe
};

/* Handle sigchld */
static void sigchldHandler(int sig)
{
    int chldStatus;
    /* Currently we have only one child, so we exit in case of error. */
    int waitRes;
    (void)sig;
    for (;;) {
        waitRes = waitpid(-1, &chldStatus, WNOHANG);
        if (!waitRes)
            break;
        if (waitRes < 0) {
            perror("Cannot obtain exit status of child process");
            doExit(3);
        }
        if (WIFSTOPPED(chldStatus)) {
            /* The child stopped. This can be only the result of ptrace(TRACE_ME). */
            /* We won't need the notification pipe any more, as we know that
             * the exec() succeeded. */
            close(chldPipe[0]);
            close(chldPipe[1]);
            chldPipe[0] = -1;
            /* If we are not debugging, just skip the "handover enabler".
             * This is suboptimal, as it makes us ignore setuid/-gid bits. */
            if (isDebug) {
                /* Stop the child after we detach from it, so we can hand it over to gdb.
                 * If the signal delivery is not queued, things will go awry. It works on
                 * Linux and MacOSX ... */
                kill(chldPid, SIGSTOP);
            }
#ifdef __linux__
            ptrace(PTRACE_DETACH, chldPid, 0, 0);
#else
            ptrace(PT_DETACH, chldPid, 0, 0);
#endif
            sendMsg("pid %d\n", chldPid);
            if (isDetached == 2 && isDebug) {
                /* qtcreator was not informed and died while debugging, killing the child */
                kill(chldPid, SIGKILL);
            }
        } else if (WIFEXITED(chldStatus)) {
            int errNo;

            /* The child exited normally. */
            if (chldPipe[0] >= 0) {
                /* The child exited before being stopped by ptrace(). That can only
                 * mean that the exec() failed. */
                switch (read(chldPipe[0], &errNo, sizeof(errNo))) {
                default:
                    /* Read of unknown length. Should never happen ... */
                    errno = EPROTO;
                    /* fallthrough */
                case -1:
                    /* Read failed. Should never happen, either ... */
                    perror("Cannot read status from child process");
                    doExit(3);
                case sizeof(errNo):
                    /* Child telling us the errno from exec(). */
                    sendMsg("err:exec %d\n", errNo);
                    doExit(3);
                }
            }
            sendMsg("exit %d\n", WEXITSTATUS(chldStatus));
            doExit(0);
        } else {
            sendMsg("crash %d\n", WTERMSIG(chldStatus));
            doExit(0);
        }
    }
}


/* syntax: $0 {"run"|"debug"} <pid-socket> <continuation-msg> <workdir> <env-file> <exe> <args...> */
/* exit codes: 0 = ok, 1 = invocation error, 3 = internal error */
int main(int argc, char *argv[])
{
    int errNo, hadInvalidCommand = 0;
    char **env = 0;
    struct sockaddr_un sau;
    struct sigaction act;

    memset(&act, 0, sizeof(act));

    if (argc < ArgEnv) {
        fprintf(stderr, "This is an internal helper of Qt Creator. Do not run it manually.\n");
        return 1;
    }
    sleepMsg = argv[ArgMsg];

    /* Connect to the master, i.e. Creator. */
    if ((qtcFd = socket(PF_UNIX, SOCK_STREAM, 0)) < 0) {
        perror("Cannot create creator comm socket");
        doExit(3);
    }
    memset(&sau, 0, sizeof(sau));
    sau.sun_family = AF_UNIX;
    strncpy(sau.sun_path, argv[ArgSocket], sizeof(sau.sun_path) - 1);
    if (connect(qtcFd, (struct sockaddr *)&sau, sizeof(sau))) {
        fprintf(stderr, "Cannot connect creator comm socket %s: %s\n", sau.sun_path, strerror(errno));
        doExit(1);
    }

    isDebug = !strcmp(argv[ArgAction], "debug");
    isDetached = 0;

    sendMsg("spid %ld\n", (long)getpid());

    if (*argv[ArgDir] && chdir(argv[ArgDir])) {
        /* Only expected error: no such file or direcotry */
        sendMsg("err:chdir %d\n", errno);
        return 1;
    }

    if (*argv[ArgEnv]) {
        FILE *envFd;
        char *envdata, *edp;
        long size;
        int count;
        if (!(envFd = fopen(argv[ArgEnv], "r"))) {
            fprintf(stderr, "Cannot read creator env file %s: %s\n",
                    argv[ArgEnv], strerror(errno));
            doExit(1);
        }
        fseek(envFd, 0, SEEK_END);
        size = ftell(envFd);
        rewind(envFd);
        envdata = malloc(size);
        if (fread(envdata, 1, size, envFd) != (size_t)size) {
            perror("Failed to read env file");
            doExit(1);
        }
        fclose(envFd);
        for (count = 0, edp = envdata; edp < envdata + size; ++count)
            edp += strlen(edp) + 1;
        env = malloc((count + 1) * sizeof(char *));
        for (count = 0, edp = envdata; edp < envdata + size; ++count) {
            env[count] = edp;
            edp += strlen(edp) + 1;
        }
        env[count] = 0;
    }


    /*
     * set up the signal handlers
     */
    {
        /* Ignore SIGTTOU. Without this, calling tcsetpgrp() from a background
         * process group (in which we will be, once as child and once as parent)
         * generates the mentioned signal and stops the concerned process. */
        act.sa_handler = SIG_IGN;
        if (sigaction(SIGTTOU, &act, 0)) {
            perror("sigaction SIGTTOU");
            doExit(3);
        }

        /* Handle SIGCHLD to keep track of what the child does without blocking */
        act.sa_handler = sigchldHandler;
        if (sigaction(SIGCHLD, &act, 0)) {
            perror("sigaction SIGCHLD");
            doExit(3);
        }
    }

    /* Create execution result notification pipe. */
    if (pipe(chldPipe)) {
        perror("Cannot create status pipe");
        doExit(3);
    }
    /* The debugged program is not supposed to inherit these handles. But we cannot
     * close the writing end before calling exec(). Just handle both ends the same way ... */
    fcntl(chldPipe[0], F_SETFD, FD_CLOEXEC);
    fcntl(chldPipe[1], F_SETFD, FD_CLOEXEC);
    switch ((chldPid = fork())) {
        case -1:
            perror("Cannot fork child process");
            doExit(3);
        case 0:
            close(qtcFd);

            /* Remove the SIGCHLD handler from the child */
            act.sa_handler = SIG_DFL;
            sigaction(SIGCHLD, &act, 0);

            /* Put the process into an own process group and make it the foregroud
             * group on this terminal, so it will receive ctrl-c events, etc.
             * This is the main reason for *all* this stub magic in the first place. */
            /* If one of these calls fails, the world is about to end anyway, so
             * don't bother checking the return values. */
            setpgid(0, 0);
            tcsetpgrp(0, getpid());

            /* Get a SIGTRAP after exec() has loaded the new program. */
#ifdef __linux__
            ptrace(PTRACE_TRACEME);
#else
            ptrace(PT_TRACE_ME, 0, 0, 0);
#endif

            if (env)
                environ = env;

            execvp(argv[ArgExe], argv + ArgExe);
            /* Only expected error: no such file or direcotry, i.e. executable not found */
            errNo = errno;
            write(chldPipe[1], &errNo, sizeof(errNo)); /* Only realistic error case is SIGPIPE */
            _exit(0);
        default:
            for (;;) {
                char buffer[100];
                int nbytes;

                nbytes = read(qtcFd, buffer, 100);
                if (nbytes <= 0) {
                    if (nbytes < 0 && errno == EINTR)
                        continue;
                    if (!isDetached) {
                        isDetached = 2;
                        if (nbytes == 0)
                            fprintf(stderr, "Lost connection to QtCreator, detaching from it.\n");
                        else
                            perror("Lost connection to QtCreator, detaching from it");
                    }
                    break;
                } else {
                    int i;
                    for (i = 0; i < nbytes; ++i) {
                        switch (buffer[i]) {
                        case 'k':
                            if (chldPid > 0) {
                                kill(chldPid, SIGTERM);
                                sleep(1);
                                kill(chldPid, SIGKILL);
                            }
                            break;
                        case 'd':
                            isDetached = 1;
                            break;
                        case 's':
                            exit(0);
                        default:
                            if (!hadInvalidCommand) {
                                fprintf(stderr, "Ignoring invalid commands from QtCreator.\n");
                                hadInvalidCommand = 1;
                            }
                        }
                    }
                }
            }
            if (isDetached) {
                for (;;)
                    pause(); /* will exit in the signal handler... */
            }
    }
    assert(0);
    return 0;
}
