// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

// Runs what an IDE on the device just built, in a process a debugger may trace. Neither
// comes for free: the device refuses to execute a file an application wrote, and it grants
// tracing per application, so a binary started from the terminal that compiled it cannot be
// traced at all. This program comes from the IDE's own package, so it may be started, and
// it maps the binary into itself instead of executing it.
//
// Where the image landed is only known here, and the debugger cannot see a mapping the
// platform did not make, so it is reported over the IDE's gate on the device's loopback.
// The same connection holds main() back until the IDE says the debugger is in place; a gate
// nothing listens on means an ordinary run.
//
// Installed under the name of one of Qt's build tools (moc, rcc, uic, ...) it stands in for
// that tool instead: the Qt a project on the device builds against has its tools in its own
// tree, which the device does not execute either, so the tool of that name in the directory
// QTC_QT_HOST_LIBEXECS (or, for qmake, QTC_QT_HOST_BINS) names is mapped and run in its
// place, together with the libraries it needs from the directories QTC_QT_HOST_LIBS lists:
// the tool has to run on the Qt it belongs to, not on the one this package carries.

#include "qtcload.h"

#include <cinttypes>
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

using Main = int (*)(int, char **);

// Written by the debugger, read here: the IDE's byte says the IDE is there, this says the
// debugger has the mapping and its breakpoints. Read while running, which a traced process
// only does once the debugger let it go, so every breakpoint it holds is in place by then.
volatile int qtcHostRelease = 0;

// Called while waiting for that: the debugger stops here, where it can write.
extern "C" __attribute__((visibility("default"), noinline)) void qtcHostWaiting()
{
    asm volatile("");
}

static int connectToGate(int port)
{
    const int fd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0)
        return -1;
    sockaddr_in address = {};
    address.sin_family = AF_INET;
    address.sin_port = ::htons(uint16_t(port));
    address.sin_addr.s_addr = ::htonl(INADDR_LOOPBACK);
    if (::connect(fd, reinterpret_cast<sockaddr *>(&address), sizeof(address)) != 0) {
        ::close(fd);
        return -1;
    }
    return fd;
}

static int runTool(const char *name, int argc, char **argv)
{
    const char *directory = std::getenv("QTC_QT_HOST_LIBEXECS");
    if (!directory || !*directory) {
        std::fprintf(stderr, "%s: QTC_QT_HOST_LIBEXECS does not name the directory of the "
                             "Qt tools to run\n", name);
        return 1;
    }
    // A Qt built for HarmonyOS makes even its tools loadable modules.
    std::string path;
    for (const char *variable : {"QTC_QT_HOST_LIBEXECS", "QTC_QT_HOST_BINS"}) {
        const char *candidate = std::getenv(variable);
        if (!candidate || !*candidate)
            continue;
        for (const std::string &file : {std::string(candidate) + "/lib" + name + ".so",
                                        std::string(candidate) + '/' + name}) {
            if (::access(file.c_str(), F_OK) == 0) {
                directory = candidate;
                path = file;
                break;
            }
        }
        if (!path.empty())
            break;
    }
    if (path.empty())
        path = std::string(directory) + '/' + name;

    // A Qt whose tools are these copies would otherwise have this start itself forever.
    char self[PATH_MAX];
    char target[PATH_MAX];
    if (::realpath("/proc/self/exe", self) && ::realpath(path.c_str(), target)
        && std::strcmp(self, target) == 0) {
        std::fprintf(stderr, "%s: %s is this program, not the tool\n", name, path.c_str());
        return 1;
    }

    QtcLoad::World world;
    if (const char *libraries = std::getenv("QTC_QT_HOST_LIBS")) {
        for (const char *start = libraries; *start;) {
            const char *end = std::strchr(start, ':');
            const size_t length = end ? size_t(end - start) : std::strlen(start);
            if (length > 0)
                world.directories.emplace_back(start, length);
            start += end ? length + 1 : length;
        }
    }

    QtcLoad::Image image;
    std::string error;
    if (!QtcLoad::load(path.c_str(), &image, &error, &world)) {
        std::fprintf(stderr, "%s: cannot load %s: %s\n", name, path.c_str(), error.c_str());
        return 1;
    }
    for (const std::string &missing : image.missing)
        std::fprintf(stderr, "%s: %s is missing %s\n", name, path.c_str(), missing.c_str());

    const Main entry = reinterpret_cast<Main>(QtcLoad::lookup(image, "main"));
    if (!entry) {
        std::fprintf(stderr, "%s: %s has no main()\n", name, path.c_str());
        return 1;
    }
    // In the tool's directory, for Qt to find its prefix, under the name the tool reports.
    std::string tool = std::string(directory) + '/' + name;
    argv[0] = tool.data();
    return entry(argc, argv);
}

int main(int argc, char **argv)
{
    const char *slash = std::strrchr(argv[0], '/');
    const char *name = slash ? slash + 1 : argv[0];
    if (std::strcmp(name, "qtchost") != 0)
        return runTool(name, argc, argv);

    if (argc < 3) {
        std::fprintf(stderr, "usage: qtchost <gate-port> <binary> [arguments]\n");
        return 2;
    }

    const int port = std::atoi(argv[1]);
    const char *path = argv[2];

    QtcLoad::Image image;
    std::string error;
    if (!QtcLoad::load(path, &image, &error, nullptr, true)) {
        std::fprintf(stderr, "qtchost: cannot load %s: %s\n", path, error.c_str());
        return 1;
    }
    for (const std::string &missing : image.missing)
        std::fprintf(stderr, "qtchost: %s is missing %s\n", path, missing.c_str());

    const Main entry = reinterpret_cast<Main>(QtcLoad::lookup(image, "main"));
    if (!entry) {
        std::fprintf(stderr, "qtchost: %s has no main()\n", path);
        return 1;
    }

    const uintptr_t slide = uintptr_t(image.base) - uintptr_t(image.lowest);
    const int gate = connectToGate(port);
    if (gate < 0) {
        std::printf("qtchost: nothing waits on port %d, running %s at slide 0x%" PRIxPTR "\n",
                    port, path, slide);
    } else {
        ::dprintf(gate, "pid %d slide 0x%" PRIxPTR "\n", int(::getpid()), slide);
        char released = 0;
        ::read(gate, &released, 1);
        ::close(gate);
        // The IDE writes that byte when its run control starts, which is before the
        // debugger has this process, so it cannot be what protection waits for.
        for (int waited = 0; !qtcHostRelease && waited < 30000; waited += 100) {
            qtcHostWaiting();
            ::usleep(100 * 1000);
        }
    }
    // Only now, with the breakpoints in place: the platform lets nobody write text that
    // the jit prctl has made executable.
    if (!QtcLoad::protect(&image)) {
        std::fprintf(stderr, "qtchost: cannot start %s: %s\n", path, error.c_str());
        return 1;
    }
    std::fflush(stdout);

    return entry(argc - 2, argv + 2);
}
