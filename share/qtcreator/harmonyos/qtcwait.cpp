// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

// Holds a HarmonyOS application at startup so that a debugger can be attached to one that
// has not run yet. Built for the device and added to the application library's DT_NEEDED
// entries, which the loader initializes before the library that names them - so before any
// static initializer of the application and before main(). Nothing here comes from the
// application or from Qt, so a plain C++ program is held the same way a Qt one is.
//
// The device refuses PTRACE_TRACEME, which leaves starting the debug server to the
// application itself, and there is no channel that tells a launch it should wait: the
// framework passes no environment, the parameter store is root-only, and nothing the host
// can write is readable this early. What is left is the device's own loopback. An hdc
// reverse forward leaves a listener on it, so a connect that succeeds means the tools are
// waiting on the other side, and one that is refused is an ordinary launch.

#include <cerrno>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <string>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

// Cleared by the debugger once it has attached and placed the modules.
extern "C" __attribute__((visibility("default"))) volatile bool qtc_waitForDebugger = true;

// Read straight out of the process by the debugger. Nothing else reports from here: hilog
// carries nothing this early, and the platform will stat a file in the application sandbox
// but not transfer it.
extern "C" __attribute__((visibility("default"))) char qtc_launchDump[1024] = {0};

static const int waitTimeoutMs = 60000;
static const int pollIntervalMs = 50;

static void note(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    const size_t at = ::strlen(qtc_launchDump);
    ::vsnprintf(qtc_launchDump + at, sizeof(qtc_launchDump) - at - 1, format, args);
    va_end(args);
}

static bool debuggerIsWaiting()
{
    const int fd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        note("gate: no socket (errno %d)\n", errno);
        return false;
    }
    sockaddr_in address = {};
    address.sin_family = AF_INET;
    address.sin_port = ::htons(QTC_GATE_PORT);
    address.sin_addr.s_addr = ::htonl(INADDR_LOOPBACK);
    const int result = ::connect(fd, reinterpret_cast<sockaddr *>(&address), sizeof(address));
    const int failure = errno;
    ::close(fd);
    note("gate: connect to port %d returned %d (errno %d)\n", QTC_GATE_PORT, result, failure);
    return result == 0;
}

static void startServer()
{
    const pid_t child = ::fork();
    if (child < 0) {
        note("server: no fork (errno %d)\n", errno);
        return;
    }
    if (child == 0) {
        const std::string listen = "*:" + std::to_string(QTC_SERVER_PORT);
        ::execl(QTC_SERVER_PATH, "lldb-server", "platform", "--listen", listen.c_str(), nullptr);
        ::execlp("lldb-server", "lldb-server", "platform", "--listen", listen.c_str(), nullptr);
        ::_exit(127);
    }
    note("server: %d listening on %d\n", int(child), QTC_SERVER_PORT);
}

__attribute__((constructor)) static void qtcWait()
{
    if (!debuggerIsWaiting()) {
        note("verdict: ordinary launch\n");
        return;
    }

    startServer();
    note("verdict: waiting to be released\n");
    for (int waited = 0; qtc_waitForDebugger && waited < waitTimeoutMs; waited += pollIntervalMs)
        ::usleep(pollIntervalMs * 1000);
    note(qtc_waitForDebugger ? "verdict: timed out\n" : "verdict: released\n");
}
