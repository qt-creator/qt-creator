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

#include "qtcload.h"

#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include <arpa/inet.h>
#include <cstdarg>
#include <dlfcn.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

static const int ChannelPort = QTC_CHANNEL_PORT;
static const char *RunDirectory = "/data/storage/el2/base/files/qtcrun";
static const char *Application = "/data/storage/el2/base/files/qtcrun/libapp.so";

// A runner that fails before the channel is open would otherwise be invisible: the
// platform sends an application's stdout nowhere. hilog is what the device does have, and
// the process already holds the library that writes to it.
static void note(const char *format, ...)
{
    char text[1024] = {0};
    va_list arguments;
    va_start(arguments, format);
    ::vsnprintf(text, sizeof(text) - 1, format, arguments);
    va_end(arguments);

    typedef int (*LogPrint)(int, int, unsigned int, const char *, const char *, ...);
    static LogPrint print = reinterpret_cast<LogPrint>(::dlsym(RTLD_DEFAULT, "OH_LOG_Print"));
    if (print)
        print(0 /*LOG_APP*/, 5 /*LOG_WARN*/, 0xA00000, "qtcrunner", "%s", text);
    ::printf("qtcrunner: %s\n", text);
}

// What the IDE hands over. It takes charge of the arguments where the launch cannot carry
// them - an implicit want has none, and leaves its own URI where the application expects
// its first argument.
struct Launch
{
    std::vector<std::string> arguments;
    bool argumentsGiven = false;
};

static const size_t ArgumentsAnnouncement = 0xffffffffu;
static const size_t ArgumentsLimit = 1024u * 1024;

static int channel()
{
    const int fd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0)
        return -1;
    sockaddr_in address = {};
    address.sin_family = AF_INET;
    address.sin_port = ::htons(ChannelPort);
    address.sin_addr.s_addr = ::htonl(INADDR_LOOPBACK);
    if (::connect(fd, reinterpret_cast<sockaddr *>(&address), sizeof(address)) != 0) {
        ::close(fd);
        return -1;
    }
    return fd;
}

static bool readFully(int fd, char *at, size_t left)
{
    while (left > 0) {
        const ssize_t got = ::read(fd, at, left);
        if (got <= 0)
            return false;
        at += got;
        left -= size_t(got);
    }
    return true;
}

static bool readSize(int fd, size_t *size)
{
    unsigned char header[4] = {0};
    if (!readFully(fd, reinterpret_cast<char *>(header), sizeof(header)))
        return false;
    *size = (size_t(header[0]) << 24) | (size_t(header[1]) << 16) | (size_t(header[2]) << 8)
            | size_t(header[3]);
    return true;
}

// The IDE announces the application as a length and that many bytes, and can send the
// arguments ahead of it, announced by a length no application can have.
static bool receive(int fd, Launch *launch, std::string *error)
{
    size_t size = 0;
    if (!readSize(fd, &size)) {
        *error = "no application on the channel";
        return false;
    }
    if (size == ArgumentsAnnouncement) {
        size_t bytes = 0;
        if (!readSize(fd, &bytes) || bytes > ArgumentsLimit) {
            *error = "the announced arguments are not plausible";
            return false;
        }
        std::string text(bytes, '\0');
        if (bytes > 0 && !readFully(fd, &text[0], bytes)) {
            *error = "the channel closed inside the arguments";
            return false;
        }
        for (size_t at = 0; at < bytes;) {
            const size_t end = text.find('\0', at);
            if (end == std::string::npos)
                break;
            launch->arguments.push_back(text.substr(at, end - at));
            at = end + 1;
        }
        launch->argumentsGiven = true;
        if (!readSize(fd, &size)) {
            *error = "no application on the channel";
            return false;
        }
    }
    if (size == 0 || size > 512u * 1024 * 1024) {
        *error = "the announced size is not plausible: " + std::to_string(size);
        return false;
    }

    ::mkdir(RunDirectory, 0755);
    const int out = ::open(Application, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (out < 0) {
        *error = "cannot write " + std::string(Application) + " (errno "
                 + std::to_string(errno) + ")";
        return false;
    }
    char buffer[65536];
    size_t left = size;
    while (left > 0) {
        const size_t want = left < sizeof(buffer) ? left : sizeof(buffer);
        if (!readFully(fd, buffer, want)) {
            *error = "the channel closed with " + std::to_string(left) + " bytes to go";
            ::close(out);
            return false;
        }
        if (::write(out, buffer, want) != ssize_t(want)) {
            *error = "short write";
            ::close(out);
            return false;
        }
        left -= want;
    }
    ::close(out);
    note("received %zu bytes", size);
    ::dprintf(fd, "runner: received %zu bytes\n", size);
    return true;
}

extern "C" __attribute__((visibility("default"))) int main(int argc, char **argv)
{
    note("main() entered, argc %d, connecting to port %d", argc, ChannelPort);
    Launch launch;
    const int fd = channel();
    if (fd < 0) {
        // Nothing is holding the channel open: run whatever was left here last time, so a
        // launch from the device's own launcher repeats the last run.
        note("no channel on port %d (errno %d), using what is already here", ChannelPort,
             errno);
    } else {
        std::string error;
        if (!receive(fd, &launch, &error)) {
            note("%s", error.c_str());
            ::dprintf(fd, "runner: %s\n", error.c_str());
            ::close(fd);
            return 1;
        }
    }

    QtcLoad::Image image;
    std::string error;
    if (!QtcLoad::load(Application, &image, &error)) {
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
    if (fd >= 0) {
        ::dprintf(fd, "runner: mapped at %p, %zu bytes, %zu frames, %zu dependencies\n",
                  image.base, image.size, image.frameCount, image.dependencies.size());
        for (const std::string &name : image.missing)
            ::dprintf(fd, "runner:   missing %s\n", name.c_str());
    }

    typedef int (*Main)(int, char **);
    Main entry = reinterpret_cast<Main>(QtcLoad::lookup(image, "main"));
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
    if (fd >= 0)
        ::dprintf(fd, "runner: calling main(), argc %d\n", argc);
    const int result = entry(argc, argv);
    note("the application's main() returned %d", result);
    if (fd >= 0) {
        ::dprintf(fd, "runner: main() returned %d\n", result);
        ::close(fd);
    }
    return result;
}
