// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

// The runner package's libqohos.so, which carries no Qt, so that one package runs
// applications built against any Qt. ArkTS imports it as the "qohos" module, and it maps the
// real libqohos.so of the Qt last handed over, which then registers its own module with us
// rather than with the platform. Every Init the platform calls on us (the import, and each
// XComponent naming "qohos") goes on to that one.

#define NoteTag "qtcshim"
#include "qtcshim.h"

#include <atomic>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cerrno>
#include <cstring>
#include <string>

#include <dlfcn.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include <inputmethod/inputmethod_text_editor_proxy_capi.h>
#include <napi/native_api.h>
#include <window_manager/oh_window_event_filter.h>

static QtcLoad::World &world()
{
    static QtcLoad::World theWorld;
    return theWorld;
}

static napi_module *realModule = nullptr;

static void captureModule(napi_module *module)
{
    note("the mapped Qt registers its module \"%s\"",
         module && module->nm_modname ? module->nm_modname : "?");
    realModule = module;
}

// The pasteboard checks the callbacks it calls with CFI, which knows only what the platform's
// loader mapped and aborts on a function in the mapped Qt. It gets ours, which forward.
using PasteboardNotify = void (*)(void *context, int type);
using PasteboardFinalize = void (*)(void *context);
using PasteboardSetData = int (*)(void *observer, void *context, PasteboardNotify,
                                  PasteboardFinalize);

struct PasteboardForward
{
    void *context;
    PasteboardNotify notify;
    PasteboardFinalize finalize;
};

static void forwardNotify(void *context, int type)
{
    auto forward = static_cast<PasteboardForward *>(context);
    if (forward->notify)
        forward->notify(forward->context, type);
}

static void forwardFinalize(void *context)
{
    auto forward = static_cast<PasteboardForward *>(context);
    if (forward->finalize)
        forward->finalize(forward->context);
    delete forward;
}

static int forwardSetData(void *observer, void *context, PasteboardNotify notify,
                          PasteboardFinalize finalize)
{
    static const auto real = []() -> PasteboardSetData {
        void *handle = ::dlopen("libpasteboard.so", RTLD_NOW | RTLD_LOCAL);
        return handle ? reinterpret_cast<PasteboardSetData>(
                   ::dlsym(handle, "OH_PasteboardObserver_SetData"))
                      : nullptr;
    }();
    if (!real) {
        note("the pasteboard has no OH_PasteboardObserver_SetData");
        return -1;
    }
    auto forward = new PasteboardForward{context, notify, finalize};
    const int result = real(observer, forward, &forwardNotify, &forwardFinalize);
    if (result != 0)
        delete forward;
    return result;
}

// The same for the record provider that hands out the clipboard contents when they are read.
using ProviderGetData = void *(*)(void *context, const char *type);
using ProviderFinalize = void (*)(void *context);
using ProviderSetData = int (*)(void *provider, void *context, ProviderGetData,
                                ProviderFinalize);

struct ProviderForward
{
    void *context;
    ProviderGetData getData;
    ProviderFinalize finalize;
};

static void *forwardGetData(void *context, const char *type)
{
    auto forward = static_cast<ProviderForward *>(context);
    return forward->getData ? forward->getData(forward->context, type) : nullptr;
}

static void forwardProviderFinalize(void *context)
{
    auto forward = static_cast<ProviderForward *>(context);
    if (forward->finalize)
        forward->finalize(forward->context);
    delete forward;
}

static int forwardProviderSetData(void *provider, void *context, ProviderGetData getData,
                                  ProviderFinalize finalize)
{
    static const auto real = []() -> ProviderSetData {
        void *handle = ::dlopen("libudmf.so", RTLD_NOW | RTLD_LOCAL);
        return handle ? reinterpret_cast<ProviderSetData>(
                   ::dlsym(handle, "OH_UdmfRecordProvider_SetData"))
                      : nullptr;
    }();
    if (!real) {
        note("UDMF has no OH_UdmfRecordProvider_SetData");
        return -1;
    }
    auto forward = new ProviderForward{context, getData, finalize};
    const int result = real(provider, forward, &forwardGetData, &forwardProviderFinalize);
    if (result != 0)
        delete forward;
    return result;
}

// And for the callbacks that get no context: the input method's text editor proxy, which only
// passes the proxy, and the window's event filters. Each kind forwards to the one function set
// last, which is what the plugin does anyway: it hands the same function to every proxy and window.
template<int Kind, typename Setter>
struct SetterForward;

template<int Kind, typename Error, typename Owner, typename Result, typename... Args>
struct SetterForward<Kind, Error (*)(Owner, Result (*)(Args...))>
{
    using Func = Result (*)(Args...);
    using Setter = Error (*)(Owner, Func);

    static inline Setter real = nullptr;
    static inline std::atomic<Func> target = nullptr;

    static Result call(Args... args) { return target.load()(args...); }

    static Error set(Owner owner, Func func)
    {
        target = func;
        return real(owner, func ? &call : nullptr);
    }
};

template<int Kind, typename Setter>
static void forwardSetter(void *library, const char *name)
{
    using Forward = SetterForward<Kind, Setter>;
    Forward::real = reinterpret_cast<Setter>(::dlsym(library, name));
    if (Forward::real)
        QtcLoad::interposers()[name] = reinterpret_cast<void *>(&Forward::set);
    else
        note("there is no %s", name);
}

#define FORWARD(Setter) forwardSetter<__LINE__, decltype(&Setter)>(library, #Setter)

static void forwardSetters()
{
    if (void *library = ::dlopen("libohinputmethod.so", RTLD_NOW | RTLD_LOCAL)) {
        FORWARD(OH_TextEditorProxy_SetGetTextConfigFunc);
        FORWARD(OH_TextEditorProxy_SetInsertTextFunc);
        FORWARD(OH_TextEditorProxy_SetDeleteForwardFunc);
        FORWARD(OH_TextEditorProxy_SetDeleteBackwardFunc);
        FORWARD(OH_TextEditorProxy_SetSendKeyboardStatusFunc);
        FORWARD(OH_TextEditorProxy_SetSendEnterKeyFunc);
        FORWARD(OH_TextEditorProxy_SetMoveCursorFunc);
        FORWARD(OH_TextEditorProxy_SetHandleSetSelectionFunc);
        FORWARD(OH_TextEditorProxy_SetHandleExtendActionFunc);
        FORWARD(OH_TextEditorProxy_SetGetLeftTextOfCursorFunc);
        FORWARD(OH_TextEditorProxy_SetGetRightTextOfCursorFunc);
        FORWARD(OH_TextEditorProxy_SetGetTextIndexAtCursorFunc);
        FORWARD(OH_TextEditorProxy_SetReceivePrivateCommandFunc);
        FORWARD(OH_TextEditorProxy_SetSetPreviewTextFunc);
        FORWARD(OH_TextEditorProxy_SetFinishTextPreviewFunc);
    } else {
        note("cannot open the input method: %s", ::dlerror());
    }
    if (void *library = ::dlopen("libnative_window_manager.so", RTLD_NOW | RTLD_LOCAL)) {
        FORWARD(OH_NativeWindowManager_RegisterKeyEventFilter);
        FORWARD(OH_NativeWindowManager_RegisterMouseEventFilter);
        FORWARD(OH_NativeWindowManager_RegisterTouchEventFilter);
    } else {
        note("cannot open the window manager: %s", ::dlerror());
    }
}

#undef FORWARD

static bool exists(const std::string &path)
{
    struct stat info;
    return ::stat(path.c_str(), &info) == 0 && S_ISREG(info.st_mode);
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
    removeStrayDirectories(directory);
    const std::string pluginDirectory = directory + '/' + PluginDirectory;
    launch->qtDirectory = directory;

    for (size_t index = 0; index < count; ++index) {
        if (!readText(fd, &names[index], error) || !readSize(fd, &sizes[index]))
            return false;
        const bool plugin = isPluginName(names[index]);
        if ((!plugin && !isPlainName(names[index])) || sizes[index] > FileLimit) {
            *error = "a library on the channel is not named plausibly";
            return false;
        }
        const std::string path = (plugin ? pluginDirectory : directory) + '/' + names[index];
        if (plugin) {
            ::mkdir(pluginDirectory.c_str(), 0755);
            ::mkdir((pluginDirectory + '/' + names[index].substr(0, names[index].find('/')))
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
        const std::string path = (isPluginName(names[index]) ? pluginDirectory : directory)
                                 + '/' + names[index];
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
    launch->received = received;
    return true;
}

// A header that announces a Qt is the IDE's offer, which comes before anything else.
static bool receiveUpToTheApplication(int fd, QtcShim *shim, std::string *error)
{
    size_t size = 0;
    if (!readSize(fd, &size)) {
        *error = "no application on the channel";
        return false;
    }
    if (size != QtAnnouncement) {
        shim->headerPending = true;
        shim->header = size;
        return true;
    }
    return receiveQt(fd, shim->launch, error);
}

static bool mapQt()
{
    static Launch launch;
    static QtcShim shim;
    shim.launch = &launch;
    shim.channel = channel();
    if (shim.channel < 0) {
        note("no channel on port %d (errno %d), using what is already here", ChannelPort,
             errno);
    } else {
        std::string error;
        if (!receiveUpToTheApplication(shim.channel, &shim, &error)) {
            note("%s", error.c_str());
            ::dprintf(shim.channel, "runner: %s\n", error.c_str());
            ::close(shim.channel);
            return false;
        }
    }
    // Receiving took the time the platform gives the ability to come up, so this start
    // is lost anyway: end it, and let the next one find the Qt in place.
    if (launch.received > 0) {
        rememberQt(launch.qtDirectory);
        note("staged %zu libraries, the next start runs", launch.received);
        ::dprintf(shim.channel, "runner: staged, start again\n");
        ::close(shim.channel);
        ::_exit(0);
    }
    if (launch.qtDirectory.empty()) {
        launch.qtDirectory = rememberedQt();
        if (!launch.qtDirectory.empty()) {
            removeStrayDirectories(launch.qtDirectory);
            launch.plugins = rememberedPlugins(launch.qtDirectory);
        }
    } else {
        rememberQt(launch.qtDirectory);
    }
    const std::string directory = launch.qtDirectory;
    if (directory.empty()) {
        note("no Qt has been handed over yet (%s)", QtCurrent);
        return false;
    }
    std::string path = directory + "/libqohos.so";
    if (!exists(path))
        path = directory + '/' + PluginDirectory + "/platforms/libqohos.so";
    if (!exists(path)) {
        note("%s has no libqohos.so", directory.c_str());
        return false;
    }
    shim.world = &world();
    shim.load = &QtcLoad::load;
    shim.registerPlugin = &QtcLoad::registerPlugin;
    shim.lookup = &QtcLoad::lookup;
    shim.protect = &QtcLoad::protect;
    char pointer[32];
    ::snprintf(pointer, sizeof(pointer), "%p", static_cast<void *>(&shim));
    ::setenv(ShimVariable, pointer, 1);
    world().directories.push_back(directory);
    QtcLoad::interposers()["napi_module_register"] = reinterpret_cast<void *>(&captureModule);
    QtcLoad::interposers()["OH_PasteboardObserver_SetData"]
        = reinterpret_cast<void *>(&forwardSetData);
    QtcLoad::interposers()["OH_UdmfRecordProvider_SetData"]
        = reinterpret_cast<void *>(&forwardProviderSetData);
    forwardSetters();

    note("mapping %s", path.c_str());
    world().images.push_back(std::make_unique<QtcLoad::Image>());
    QtcLoad::Image *image = world().images.back().get();
    image->name = "libqohos.so";
    std::string error;
    if (!QtcLoad::load(path.c_str(), image, &error, &world())) {
        note("cannot map it: %s", error.c_str());
        for (const std::string &name : image->missing)
            note("  missing %s", name.c_str());
        return false;
    }
    for (const std::unique_ptr<QtcLoad::Image> &mapped : world().images)
        note("  mapped %s, %zu bytes", mapped->name.c_str(), mapped->size);
    if (!realModule)
        note("it did not register a module");
    return realModule != nullptr;
}

static napi_value Init(napi_env env, napi_value exports)
{
    static const bool mapped = mapQt();
    if (!mapped || !realModule || !realModule->nm_register_func) {
        note("Init: nothing to forward to");
        return exports;
    }
    note("Init: forwarding");
    return realModule->nm_register_func(env, exports);
}

extern "C" __attribute__((constructor)) void RegisterEntryModule(void)
{
    static napi_module module = {
        .nm_version = 1,
        .nm_flags = 0,
        .nm_filename = nullptr,
        .nm_register_func = Init,
        .nm_modname = "qohos",
        .nm_priv = nullptr,
        .reserved = {nullptr},
    };
    napi_module_register(&module);
}
