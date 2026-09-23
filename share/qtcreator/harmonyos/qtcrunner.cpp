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
// One directory per Qt that was handed over, named after the tag the IDE knows it by, and
// a file naming the one the last run used.
static const char *QtDirectory = "/data/storage/el2/base/files/qtcrun/qt";
static const char *QtCurrent = "/data/storage/el2/base/files/qtcrun/qt/current";

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
    std::string qtDirectory;
};

static const size_t ArgumentsAnnouncement = 0xffffffffu;
static const size_t QtAnnouncement = 0xfffffffeu;
static const size_t ArgumentsLimit = 1024u * 1024;
static const size_t NameLimit = 256u;
static const size_t FileLimit = 512u * 1024 * 1024;

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

static bool receiveArguments(int fd, Launch *launch, std::string *error)
{
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
    return true;
}

// A length and that many bytes, as everything on this channel is.
static bool readText(int fd, std::string *text, std::string *error)
{
    size_t bytes = 0;
    if (!readSize(fd, &bytes) || bytes == 0 || bytes > NameLimit) {
        *error = "the announced name is not plausible";
        return false;
    }
    text->assign(bytes, '\0');
    if (!readFully(fd, &(*text)[0], bytes)) {
        *error = "the channel closed inside a name";
        return false;
    }
    return true;
}

// The names on the channel become paths here, so nothing but a plain file name is taken.
static bool isPlainName(const std::string &name)
{
    return !name.empty() && name != "." && name != ".."
           && name.find('/') == std::string::npos;
}

static bool isDirectory(const std::string &path)
{
    struct stat status = {};
    return ::stat(path.c_str(), &status) == 0 && S_ISDIR(status.st_mode);
}

static bool receiveFile(int fd, const std::string &path, size_t size, std::string *error)
{
    const int out = ::open(path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (out < 0) {
        *error = "cannot write " + path + " (errno " + std::to_string(errno) + ")";
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
            *error = "short write on " + path;
            ::close(out);
            return false;
        }
        left -= want;
    }
    ::close(out);
    return true;
}

static bool fileOfSize(const std::string &path, size_t size)
{
    struct stat status = {};
    return ::stat(path.c_str(), &status) == 0 && S_ISREG(status.st_mode)
           && size_t(status.st_size) == size;
}

// The Qt the application was built against. A package carries the Qt it was built with and
// nothing else, and an IDE running on the device cannot put another one there, so what the
// application needs beyond that has to come over this channel - once, because it is tens of
// megabytes. The IDE offers the whole set by name and size, and this side asks only for
// what it can neither get from the platform nor find in the directory an earlier run of the
// same Qt left behind. Each file is written under a name of its own and moved into place, so
// that a transfer that breaks off leaves nothing that looks complete.
static bool receiveQt(int fd, Launch *launch, std::string *error)
{
    std::string tag;
    if (!readText(fd, &tag, error))
        return false;
    if (!isPlainName(tag)) {
        *error = "the announced Qt is not named plausibly";
        return false;
    }
    size_t count = 0;
    if (!readSize(fd, &count) || count == 0 || count > 4096) {
        *error = "the announced Qt is not plausible";
        return false;
    }
    std::vector<std::string> names(count);
    std::vector<size_t> sizes(count);
    std::string wanted(count, 'H');
    ::mkdir(RunDirectory, 0755);
    ::mkdir(QtDirectory, 0755);
    const std::string directory = std::string(QtDirectory) + '/' + tag;
    ::mkdir(directory.c_str(), 0755);
    launch->qtDirectory = directory;

    for (size_t index = 0; index < count; ++index) {
        if (!readText(fd, &names[index], error) || !readSize(fd, &sizes[index]))
            return false;
        if (!isPlainName(names[index]) || sizes[index] > FileLimit) {
            *error = "a library on the channel is not named plausibly";
            return false;
        }
        if (fileOfSize(directory + '/' + names[index], sizes[index]))
            continue;
        if (QtcLoad::installed(names[index].c_str()))
            continue;
        wanted[index] = 'S';
    }
    if (::write(fd, wanted.c_str(), count) != ssize_t(count)) {
        *error = "cannot answer which libraries are needed";
        return false;
    }

    size_t received = 0;
    size_t total = 0;
    for (size_t index = 0; index < count; ++index) {
        if (wanted[index] != 'S')
            continue;
        const std::string path = directory + '/' + names[index];
        const std::string incoming = path + ".part";
        if (!receiveFile(fd, incoming, sizes[index], error)) {
            ::unlink(incoming.c_str());
            return false;
        }
        if (::rename(incoming.c_str(), path.c_str()) != 0) {
            *error = "cannot move " + incoming + " into place (errno "
                     + std::to_string(errno) + ")";
            ::unlink(incoming.c_str());
            return false;
        }
        ++received;
        total += sizes[index];
    }
    note("Qt %s: %zu of %zu libraries received, %zu bytes", tag.c_str(), received, count,
         total);
    ::dprintf(fd, "runner: Qt %s: %zu of %zu libraries received, %zu bytes\n", tag.c_str(),
              received, count, total);
    return true;
}

static bool receiveApplication(int fd, size_t size, std::string *error)
{
    if (size == 0 || size > FileLimit) {
        *error = "the announced size is not plausible: " + std::to_string(size);
        return false;
    }
    ::mkdir(RunDirectory, 0755);
    if (!receiveFile(fd, Application, size, error))
        return false;
    note("received %zu bytes", size);
    ::dprintf(fd, "runner: received %zu bytes\n", size);
    return true;
}

// The IDE announces the application as a length and that many bytes, and can send the
// arguments and a Qt ahead of it, each announced by a length no application can have.
static bool receive(int fd, Launch *launch, std::string *error)
{
    for (;;) {
        size_t size = 0;
        if (!readSize(fd, &size)) {
            *error = "no application on the channel";
            return false;
        }
        if (size == ArgumentsAnnouncement) {
            if (!receiveArguments(fd, launch, error))
                return false;
            continue;
        }
        if (size == QtAnnouncement) {
            if (!receiveQt(fd, launch, error))
                return false;
            continue;
        }
        return receiveApplication(fd, size, error);
    }
}

// Which Qt a run that nobody is holding the channel open for maps: the one the last run
// used. Remembered here rather than guessed from what is on disk, because a device can
// hold several.
static void rememberQt(const std::string &directory)
{
    const int out = ::open(QtCurrent, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (out < 0)
        return;
    const std::string text = directory + "\n";
    (void)::write(out, text.c_str(), text.size());
    ::close(out);
}

static std::string rememberedQt()
{
    const int in = ::open(QtCurrent, O_RDONLY);
    if (in < 0)
        return {};
    char buffer[1024] = {0};
    const ssize_t got = ::read(in, buffer, sizeof(buffer) - 1);
    ::close(in);
    if (got <= 0)
        return {};
    std::string directory(buffer, size_t(got));
    while (!directory.empty() && (directory.back() == '\n' || directory.back() == '\r'))
        directory.pop_back();
    return isDirectory(directory) ? directory : std::string();
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

    if (launch.qtDirectory.empty())
        launch.qtDirectory = rememberedQt();
    else
        rememberQt(launch.qtDirectory);

    QtcLoad::World world;
    if (!launch.qtDirectory.empty()) {
        world.directories.push_back(launch.qtDirectory);
        note("mapping Qt from %s", launch.qtDirectory.c_str());
    }

    QtcLoad::Image image;
    std::string error;
    if (!QtcLoad::load(Application, &image, &error, &world)) {
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
