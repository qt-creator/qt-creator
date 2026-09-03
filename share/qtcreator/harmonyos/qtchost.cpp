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

#include "qtcload.h"

#include <cinttypes>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

using Main = int (*)(int, char **);

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

int main(int argc, char **argv)
{
    if (argc < 3) {
        std::fprintf(stderr, "usage: qtchost <gate-port> <binary> [arguments]\n");
        return 2;
    }

    const int port = std::atoi(argv[1]);
    const char *path = argv[2];

    QtcLoad::Image image;
    std::string error;
    if (!QtcLoad::load(path, &image, &error)) {
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
        // Read, not sleep: the byte the IDE sends once the debugger holds the process is
        // the only thing that says breakpoints can be hit.
        char released = 0;
        ::read(gate, &released, 1);
        ::close(gate);
    }
    std::fflush(stdout);

    return entry(argc - 2, argv + 2);
}
