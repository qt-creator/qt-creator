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

#include "qtcchannel.h"
#include "qtcload.h"

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
        const bool plugin = isPluginName(names[index]);
        if ((!plugin && !isPlainName(names[index])) || sizes[index] > FileLimit) {
            *error = "a library on the channel is not named plausibly";
            return false;
        }
        const std::string path = directory + '/' + names[index];
        if (plugin) {
            ::mkdir((directory + '/' + names[index].substr(0, names[index].find('/')))
                        .c_str(), 0755);
            launch->plugins.push_back(path);
        }
        if (fileOfSize(path, sizes[index]))
            continue;
        // What the platform has answers for a library, and only for the very same one: a
        // copy of another size belongs to another Qt, and a plugin of that name in the
        // package is another plugin whatever its size.
        if (!plugin && QtcLoad::installed(names[index].c_str(), sizes[index]))
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


// Where the package's plugins are, which the platform plugin puts into the environment
// for the Qt the package carries.
static const char *PluginPaths[] = {"QT_QPA_PLATFORM_PLUGIN_PATH", "QT_PLUGIN_PATH"};

static void registerPlugins(const std::vector<std::string> &plugins, QtcLoad::World *world)
{
    size_t registered = 0;
    for (const std::string &path : plugins) {
        std::string error;
        if (QtcLoad::registerPlugin(path.c_str(), world, &error)) {
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

    if (launch.qtDirectory.empty()) {
        launch.qtDirectory = rememberedQt();
        if (!launch.qtDirectory.empty())
            launch.plugins = rememberedPlugins(launch.qtDirectory);
    } else {
        rememberQt(launch.qtDirectory);
    }

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

    // Before main(), because that is where the application asks Qt for a platform.
    registerPlugins(launch.plugins, &world);

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
