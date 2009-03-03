/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.3, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/

#define _WIN32_WINNT 0x0501 /* WinXP, needed for DebugActiveProcessStop() */

#include <windows.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <direct.h>

static FILE *qtcFd;
static wchar_t *sleepMsg;

/* Print some "press enter" message, wait for that, exit. */
static void doExit(int code)
{
    char buf[2];
    _putws(sleepMsg);
    fgets(buf, 2, stdin); /* Minimal size to make it wait */
    exit(code);
}

/* Print an error message for unexpected Windows system errors, wait, exit. */
static void systemError(const char *str)
{
    fprintf(stderr, str, GetLastError());
    doExit(3);
}

/* Send a message to the master. */
static void sendMsg(const char *msg, int num)
{
    int pidStrLen;
    char pidStr[64];

    pidStrLen = sprintf(pidStr, msg, num);
    if (fwrite(pidStr, pidStrLen, 1, qtcFd) != 1 || fflush(qtcFd)) {
        fprintf(stderr, "Cannot write to creator comm socket: %s\n",
                strerror(errno));
        doExit(3);
    }
}

/* Ignore the first ctrl-c/break within a second. */
static BOOL WINAPI ctrlHandler(DWORD dwCtrlType)
{
    static ULARGE_INTEGER lastTime;
    ULARGE_INTEGER thisTime;
    SYSTEMTIME sysTime;
    FILETIME fileTime;

    if (dwCtrlType == CTRL_C_EVENT || dwCtrlType == CTRL_BREAK_EVENT) {
        GetSystemTime(&sysTime);
        SystemTimeToFileTime(&sysTime, &fileTime);
        thisTime.LowPart = fileTime.dwLowDateTime;
        thisTime.HighPart = fileTime.dwHighDateTime;
        if (lastTime.QuadPart + 10000000 < thisTime.QuadPart) {
            lastTime.QuadPart = thisTime.QuadPart;
            return TRUE;
        }
    }
    return FALSE;
}

enum {
    ArgCmd = 0,
    ArgAction,
    ArgSocket,
    ArgDir,
    ArgEnv,
    ArgCmdLine,
    ArgMsg,
    ArgCount
};

/* syntax: $0 {"run"|"debug"} <pid-socket> <workdir> <env-file> <cmdline> <continuation-msg> */
/* exit codes: 0 = ok, 1 = invocation error, 3 = internal error */
int main()
{
    int argc;
    int creationFlags;
    wchar_t **argv;
    wchar_t *env = 0;
    STARTUPINFOW si;
    PROCESS_INFORMATION pi;
    DEBUG_EVENT dbev;

    argv = CommandLineToArgvW(GetCommandLine(), &argc);

    if (argc != ArgCount) {
        fprintf(stderr, "This is an internal helper of Qt Creator. Do not run it manually.\n");
        return 1;
    }
    sleepMsg = argv[ArgMsg];

    /* Connect to the master, i.e. Creator. */
    if (!(qtcFd = _wfopen(argv[ArgSocket], L"w"))) {
        fprintf(stderr, "Cannot connect creator comm pipe %S: %s\n",
                argv[ArgSocket], strerror(errno));
        doExit(1);
    }

    if (*argv[ArgDir] && !SetCurrentDirectoryW(argv[ArgDir])) {
        /* Only expected error: no such file or direcotry */
        sendMsg("err:chdir %d\n", GetLastError());
        return 1;
    }

    if (*argv[ArgEnv]) {
        FILE *envFd;
        long size;
        if (!(envFd = _wfopen(argv[ArgEnv], L"r"))) {
            fprintf(stderr, "Cannot read creator env file %S: %s\n",
                    argv[ArgEnv], strerror(errno));
            doExit(1);
        }
        fseek(envFd, 0, SEEK_END);
        size = ftell(envFd);
        rewind(envFd);
        env = malloc(size);
        if (fread(env, 1, size, envFd) != size) {
            perror("Failed to read env file");
            doExit(1);
        }
        fclose(envFd);
    }

    ZeroMemory(&pi, sizeof(pi));
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);

    creationFlags = CREATE_UNICODE_ENVIRONMENT;
    if (!wcscmp(argv[ArgAction], L"debug"))
        creationFlags |= DEBUG_ONLY_THIS_PROCESS;
    if (!CreateProcessW(0, argv[ArgCmdLine], 0, 0, FALSE, creationFlags, env, 0, &si, &pi)) {
        /* Only expected error: no such file or direcotry, i.e. executable not found */
        sendMsg("err:exec %d\n", GetLastError());
        doExit(1);
    }

    /* This is somewhat convoluted. What we actually want is creating a
       suspended process and letting gdb attach to it. Unfortunately,
       the Windows kernel runs amok when we attempt this.
       So instead we start a debugged process, eat all the initial
       debug events, suspend the process and detach from it. If gdb
       tries to attach *now*, everthing goes smoothly. Yay. */
    if (creationFlags & DEBUG_ONLY_THIS_PROCESS) {
        do {
            if (!WaitForDebugEvent (&dbev, INFINITE))
                systemError("Cannot fetch debug event, error %d\n");
            if (dbev.dwDebugEventCode == EXCEPTION_DEBUG_EVENT) {
                /* The first exception to be delivered is a trap
                   which indicates completion of startup. */
                if (SuspendThread(pi.hThread) == (DWORD)-1)
                    systemError("Cannot suspend debugee, error %d\n");
            }
            if (!ContinueDebugEvent(dbev.dwProcessId, dbev.dwThreadId, DBG_CONTINUE))
                systemError("Cannot continue debug event, error %d\n");
        } while (dbev.dwDebugEventCode != EXCEPTION_DEBUG_EVENT);
        if (!DebugActiveProcessStop(dbev.dwProcessId))
            systemError("Cannot detach from debugee, error %d\n");
    }

    SetConsoleCtrlHandler(ctrlHandler, TRUE);

    sendMsg("pid %d\n", pi.dwProcessId);

    if (WaitForSingleObject(pi.hProcess, INFINITE) == WAIT_FAILED)
        systemError("Wait for debugee failed, error %d\n");
    doExit(0);
    return 0;
}
