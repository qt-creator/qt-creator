// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

// The runner: what the platform starts instead of the application, so that running does
// not need a package. Installing one costs a minute of packaging, signing and installing
// per run, and the device refuses to dlopen a library that was not installed with it - so
// the application arrives over the channel the IDE holds open, is mapped into this process
// by hand, and its main() is called as if the loader had done it. An IDE running on the
// device itself would have no other option at all: it cannot install what it builds.
//
// Nothing here creates a QApplication: the application being run does that, exactly as it
// would if the platform had loaded it. Everything the Qt template set up - the Qt main
// thread, the ability, the surface the platform plugin draws on - is already in place and
// stays valid.
//
// Nor does it link Qt, or map one: the package's libqohos.so is the shim, which mapped the
// Qt that was handed over before the ability came up, and whose loader maps the application
// into that same world.

#include "qtcshim.h"

#include <cinttypes>
#include <csignal>

#include <sys/wait.h>

static QtcShim *shim()
{
    static QtcShim *theShim = []() -> QtcShim * {
        const char *pointer = ::getenv(ShimVariable);
        auto found = pointer ? reinterpret_cast<QtcShim *>(::strtoull(pointer, nullptr, 16))
                             : nullptr;
        return found && found->version == ShimVersion ? found : nullptr;
    }();
    return theShim;
}

// The IDE announces the application as a length and that many bytes, and can send the
// arguments ahead of it, announced by a length no application can have, as is a debugger
// waiting for it. The shim has read what came before, and maybe the first header after it.
static bool receive(int fd, QtcShim *shim, bool *debugging, std::string *error)
{
    for (;;) {
        size_t size = 0;
        if (shim->headerPending) {
            shim->headerPending = false;
            size = shim->header;
        } else if (!readSize(fd, &size)) {
            *error = "no application on the channel";
            return false;
        }
        if (size == ArgumentsAnnouncement) {
            if (!receiveArguments(fd, shim->launch, error))
                return false;
            continue;
        }
        if (size == DebugAnnouncement) {
            *debugging = true;
            continue;
        }
        return receiveApplication(fd, size, error);
    }
}

// Where the package's plugins are, which the platform plugin puts into the environment
// for the Qt the package carries.
static const char *PluginPaths[] = {"QT_QPA_PLATFORM_PLUGIN_PATH", "QT_PLUGIN_PATH"};

static void registerPlugins(const std::vector<std::string> &plugins, QtcLoad::World *world)
{
    size_t registered = 0;
    for (const std::string &path : plugins) {
        std::string error;
        if (shim()->registerPlugin(path.c_str(), world, &error)) {
            note("registered the plugin %s", fileName(path).c_str());
            ++registered;
        } else {
            note("%s", error.c_str());
        }
    }
    if (registered == 0)
        return;
    // A registered plugin is one Qt finds among the static ones, which it looks at after
    // the files: a plugin of the same name in the package therefore wins, and it is the
    // one that cannot work, because it belongs to the package's Qt. Qt asks for a name
    // rather than for all of them, so there is no falling back to the second answer once
    // the first fails to load. Take the package's plugins out of the search instead.
    for (const char *variable : PluginPaths) {
        if (::getenv(variable)) {
            ::unsetenv(variable);
            note("unset %s, which is where the package's plugins are", variable);
        }
    }
}

// Written by the debugger once it has the application and its breakpoints in place.
extern "C" __attribute__((visibility("default"))) volatile int qtcHostRelease = 0;

// Called while waiting for that: the debugger stops here, where it can write.
extern "C" __attribute__((visibility("default"), noinline)) void qtcHostWaiting()
{
    asm volatile("");
}

static uintptr_t slide(const QtcLoad::Image &image)
{
    return uintptr_t(image.base) - uintptr_t(image.lowest);
}

// The device refuses PTRACE_TRACEME, so the server is started from here, where attaching
// is allowed. Nothing the platform knows says where the application and its Qt are, so
// that goes over the channel, and the application waits with its text still writable:
// once protected, the platform refuses a breakpoint there, a tracer's write included.
static void holdForDebugger(int fd, const QtcLoad::Image &image, const QtcLoad::World &world)
{
    // The server would keep the channel open after the application ended.
    ::fcntl(fd, F_SETFD, FD_CLOEXEC);
    // The server serves one connection, so a probe whether it listens would take that. It
    // writes this file once it does, and the debugger is told to come only then.
    const std::string socketFile = std::string(RunDirectory) + "/server-port";
    ::unlink(socketFile.c_str());
    const pid_t child = ::fork();
    if (child == 0) {
        const std::string listen = "*:" + std::to_string(QTC_SERVER_PORT);
        ::execl(QTC_SERVER_PATH, "lldb-server", "platform", "--listen", listen.c_str(),
                "--socket-file", socketFile.c_str(), nullptr);
        const int error = errno;
        ::execlp("lldb-server", "lldb-server", "platform", "--listen", listen.c_str(),
                 "--socket-file", socketFile.c_str(), nullptr);
        ::dprintf(fd, "runner: cannot execute %s (errno %d)\n", QTC_SERVER_PATH, error);
        ::_exit(127);
    }
    if (child < 0) {
        note("cannot start the debug server (errno %d)", errno);
        ::dprintf(fd, "runner: cannot start the debug server (errno %d)\n", errno);
        return;
    }
    for (int waited = 0; ::access(socketFile.c_str(), F_OK) != 0; waited += 10) {
        int status = 0;
        const bool ended = ::waitpid(child, &status, WNOHANG) == child;
        if (ended || waited >= 10000) {
            note("the debug server did not start listening (status 0x%x)", ended ? status : -1);
            ::dprintf(fd, "runner: the debug server did not start listening (status 0x%x)\n",
                      ended ? status : -1);
            return;
        }
        ::usleep(10 * 1000);
    }
    for (const std::unique_ptr<QtcLoad::Image> &mapped : world.images) {
        ::dprintf(fd, "runner: mapped %s slide 0x%" PRIxPTR "\n", mapped->name.c_str(),
                  slide(*mapped));
    }
    ::dprintf(fd, "runner: holding pid %d slide 0x%" PRIxPTR "\n", int(::getpid()),
              slide(image));
    note("server %d started, waiting for the debugger", int(child));
    for (int waited = 0; !qtcHostRelease && waited < 60000; waited += 100) {
        qtcHostWaiting();
        ::usleep(100 * 1000);
    }
    note(qtcHostRelease ? "released by the debugger" : "no debugger came, running anyway");
}

// A crash takes the process down before main() returns, and with it the exit code, so it is
// told over the channel instead. The platform's own handlers, which write the crash log, run
// afterwards.
struct CrashSignal
{
    int number;
    const char *name;
    struct sigaction previous;
};

static CrashSignal crashSignals[] = {{SIGSEGV, "SIGSEGV", {}}, {SIGBUS, "SIGBUS", {}},
                                     {SIGABRT, "SIGABRT", {}}, {SIGILL, "SIGILL", {}},
                                     {SIGFPE, "SIGFPE", {}}};
static int crashChannel = -1;

static void reportCrash(int number)
{
    for (CrashSignal &crash : crashSignals) {
        if (crash.number != number)
            continue;
        if (crashChannel >= 0) {
            static const char prefix[] = "runner: crashed with ";
            ssize_t ignored = ::write(crashChannel, prefix, sizeof(prefix) - 1);
            ignored = ::write(crashChannel, crash.name, ::strlen(crash.name));
            ignored = ::write(crashChannel, "\n", 1);
            (void) ignored;
        }
        ::sigaction(number, &crash.previous, nullptr);
        ::raise(number);
        return;
    }
}

static void watchForCrashes(int fd)
{
    crashChannel = fd;
    for (CrashSignal &crash : crashSignals) {
        struct sigaction action = {};
        action.sa_handler = reportCrash;
        ::sigaction(crash.number, &action, &crash.previous);
    }
}

static void stopWatchingForCrashes()
{
    for (CrashSignal &crash : crashSignals)
        ::sigaction(crash.number, &crash.previous, nullptr);
    crashChannel = -1;
}

extern "C" __attribute__((visibility("default"))) int main(int argc, char **argv)
{
    note("main() entered, argc %d", argc);
    if (!shim()) {
        note("no shim of version %d in %s, so no Qt is mapped", ShimVersion, ShimVariable);
        return 1;
    }
    Launch &launch = *shim()->launch;
    const int fd = shim()->channel;
    bool debugging = false;
    if (fd >= 0) {
        std::string error;
        if (!receive(fd, shim(), &debugging, &error)) {
            note("%s", error.c_str());
            ::dprintf(fd, "runner: %s\n", error.c_str());
            ::close(fd);
            return 1;
        }
    }
    QtcLoad::World &world = *shim()->world;

    QtcLoad::Image image;
    std::string error;
    if (!shim()->load(Application, &image, &error, &world, debugging)) {
        note("cannot map the application: %s", error.c_str());
        for (const std::string &name : image.missing)
            note("  missing %s", name.c_str());
        if (fd >= 0) {
            ::dprintf(fd, "runner: cannot map the application: %s\n", error.c_str());
            for (const std::string &name : image.missing)
                ::dprintf(fd, "runner:   missing %s\n", name.c_str());
            ::close(fd);
        }
        return 1;
    }
    note("mapped at %p, %zu bytes, %zu frames (%s), %zu dependencies", image.base,
         image.size, image.frameCount,
         image.framesRegistered ? "unwinder took them" : "NO REGISTRATION - throws will die",
         image.dependencies.size());
    for (const std::unique_ptr<QtcLoad::Image> &mapped : world.images) {
        note("  mapped %s, %zu bytes", mapped->name.c_str(), mapped->size);
        for (const std::string &name : mapped->missing)
            note("    missing %s", name.c_str());
    }
    if (fd >= 0) {
        ::dprintf(fd, "runner: mapped at %p, %zu bytes, %zu frames, %zu dependencies\n",
                  image.base, image.size, image.frameCount, image.dependencies.size());
        if (!world.images.empty()) {
            ::dprintf(fd, "runner: mapped %zu libraries of the Qt handed over\n",
                      world.images.size());
        }
        for (const std::string &name : image.missing)
            ::dprintf(fd, "runner:   missing %s\n", name.c_str());
    }

    if (debugging)
        holdForDebugger(fd, image, world);
    if (debugging && !shim()->protect(&image)) {
        note("cannot start the application: %s", error.c_str());
        ::dprintf(fd, "runner: cannot start the application: %s\n", error.c_str());
        ::close(fd);
        return 1;
    }

    // Before main(), because that is where the application asks Qt for a platform.
    registerPlugins(launch.plugins, &world);

    typedef int (*Main)(int, char **);
    Main entry = reinterpret_cast<Main>(shim()->lookup(image, "main"));
    if (!entry) {
        note("the application has no main()");
        if (fd >= 0) {
            ::dprintf(fd, "runner: the application has no main()\n");
            ::close(fd);
        }
        return 1;
    }

    static char self[] = "qtcrunner";
    std::vector<char *> values;
    if (launch.argumentsGiven) {
        values.push_back(argc > 0 ? argv[0] : self);
        for (std::string &argument : launch.arguments)
            values.push_back(&argument[0]);
        values.push_back(nullptr);
        argc = int(values.size()) - 1;
        argv = values.data();
    }

    note("calling the application's main()");
    int savedOutput = -1;
    int savedError = -1;
    if (fd >= 0) {
        ::dprintf(fd, "runner: calling main(), argc %d\n", argc);
        // The platform sends the application's output nowhere, the IDE shows what comes
        // over the channel. Line by line, because the platform may end the process before
        // main() returns, and with it whatever a buffer still held.
        ::fflush(stdout);
        ::fflush(stderr);
        savedOutput = ::dup(STDOUT_FILENO);
        savedError = ::dup(STDERR_FILENO);
        ::dup2(fd, STDOUT_FILENO);
        ::dup2(fd, STDERR_FILENO);
        ::setvbuf(stdout, nullptr, _IOLBF, 0);
        watchForCrashes(fd);
    }
    const int result = entry(argc, argv);
    if (fd >= 0) {
        stopWatchingForCrashes();
        ::fflush(stdout);
        ::fflush(stderr);
        ::dup2(savedOutput, STDOUT_FILENO);
        ::dup2(savedError, STDERR_FILENO);
        ::close(savedOutput);
        ::close(savedError);
    }
    note("the application's main() returned %d", result);
    if (fd >= 0) {
        ::dprintf(fd, "runner: main() returned %d\n", result);
        ::close(fd);
    }
    return result;
}
