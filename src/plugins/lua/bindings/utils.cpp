// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "../luaengine.h"
#include "../luaqttypes.h"

#include <utils/async.h>
#include <utils/futuresynchronizer.h>
#include <utils/hostosinfo.h>

#include <QDesktopServices>
#include <QTimer>
#include <QUuid>

using namespace Utils;

namespace Lua::Internal {

void setupUtilsModule()
{
    registerProvider(
        "Utils",
        [futureSync = Utils::FutureSynchronizer()](sol::state_view lua) mutable -> sol::object {
            const ScriptPluginSpec *pluginSpec = lua.get<ScriptPluginSpec *>("PluginSpec");

            auto async = lua.script("return require('async')", "_utils_").get<sol::table>();

            sol::table utils = lua.create_table();

            utils.set_function(
                "waitms_cb",
                [guard = pluginSpec->connectionGuard.get()](int ms, const sol::function &cb) {
                    QTimer::singleShot(ms, guard, [cb]() { cb(); });
                });

            auto dirEntries_cb = [&futureSync, guard = pluginSpec->connectionGuard.get()](
                                     const FilePath &p,
                                     const sol::table &options,
                                     const sol::function &cb) {
                const QStringList nameFilters = options.get_or<QStringList>("nameFilters", {});
                QDir::Filters fileFilters
                    = (QDir::Filters) options.get_or<int>("fileFilters", QDir::NoFilter);
                QDirIterator::IteratorFlags flags
                    = (QDirIterator::IteratorFlags)
                          options.get_or<int>("flags", QDirIterator::NoIteratorFlags);

                FileFilter filter(nameFilters, fileFilters, flags);

                QFuture<FilePath> future = Utils::asyncRun([p, filter](QPromise<FilePath> &promise) {
                    p.iterateDirectory(
                        [&promise](const FilePath &item) {
                            if (promise.isCanceled())
                                return IterationPolicy::Stop;

                            promise.addResult(item);
                            return IterationPolicy::Continue;
                        },
                        filter);
                });

                futureSync.addFuture<FilePath>(future);

                Utils::onFinished<FilePath>(future, guard, [cb](const QFuture<FilePath> &future) {
                    cb(future.results());
                });
            };

            auto searchInPath_cb = [&futureSync,
                                    guard = pluginSpec->connectionGuard
                                                .get()](const FilePath &p, const sol::function &cb) {
                QFuture<FilePath> future = Utils::asyncRun(
                    [p](QPromise<FilePath> &promise) { promise.addResult(p.searchInPath()); });

                futureSync.addFuture<FilePath>(future);

                Utils::onFinished<FilePath>(future, guard, [cb](const QFuture<FilePath> &future) {
                    cb(future.result());
                });
            };

            utils.set_function("__dirEntries_cb__", dirEntries_cb);
            utils.set_function("__searchInPath_cb__", searchInPath_cb);

            utils.set_function("createUuid", []() { return QUuid::createUuid().toString(); });

            sol::function wrap = async["wrap"].get<sol::function>();

            utils["waitms"] = wrap(utils["waitms_cb"]);

            utils["pid"] = QCoreApplication::applicationPid();

            auto hostOsInfoType = utils.new_usertype<HostOsInfo>("HostOsInfo");
            hostOsInfoType["isWindowsHost"] = &HostOsInfo::isWindowsHost;
            hostOsInfoType["isMacHost"] = &HostOsInfo::isMacHost;
            hostOsInfoType["isLinuxHost"] = &HostOsInfo::isLinuxHost;
            hostOsInfoType["os"] = sol::var([]() {
                if (HostOsInfo::isMacHost())
                    return "mac";
                else if (HostOsInfo::isLinuxHost())
                    return "linux";
                else if (HostOsInfo::isWindowsHost())
                    return "windows";
                else
                    return "unknown";
            }());
            hostOsInfoType["architecture"] = sol::var([]() {
                switch (HostOsInfo::hostArchitecture()) {
                case OsArchUnknown:
                    return "unknown";
                case OsArchX86:
                    return "x86";
                case OsArchAMD64:
                    return "x86_64";
                case OsArchItanium:
                    return "itanium";
                case OsArchArm:
                    return "arm";
                case OsArchArm64:
                    return "arm64";
                default:
                    return "unknown";
                }
            }());

            sol::usertype<FilePath> filePathType = utils.new_usertype<FilePath>(
                "FilePath",
                sol::call_constructor,
                sol::constructors<FilePath()>(),
                sol::meta_function::to_string,
                &FilePath::toUserOutput,
                "fromUserInput",
                &FilePath::fromUserInput,
                "exists",
                &FilePath::exists,
                "resolveSymlinks",
                &FilePath::resolveSymlinks,
                "isExecutableFile",
                &FilePath::isExecutableFile,
                "isDir",
                &FilePath::isDir,
                "nativePath",
                &FilePath::nativePath,
                "toUserOutput",
                &FilePath::toUserOutput,
                "fileName",
                &FilePath::fileName,
                "currentWorkingPath",
                &FilePath::currentWorkingPath,
                "parentDir",
                &FilePath::parentDir,
                "suffix",
                &FilePath::suffix,
                "completeSuffix",
                &FilePath::completeSuffix,
                "isAbsolutePath",
                &FilePath::isAbsolutePath,
                "resolvePath",
                sol::overload(
                    [](const FilePath &p, const QString &path) { return p.resolvePath(path); },
                    [](const FilePath &p, const FilePath &path) { return p.resolvePath(path); }));

            utils["FilePath"]["dirEntries_cb"] = utils["__dirEntries_cb__"];
            utils["FilePath"]["dirEntries"] = wrap(utils["__dirEntries_cb__"]);

            utils["FilePath"]["searchInPath_cb"] = utils["__searchInPath_cb__"];
            utils["FilePath"]["searchInPath"] = wrap(utils["__searchInPath_cb__"]);

            utils.new_usertype<QTimer>(
                "Timer",
                "create",
                [guard = pluginSpec](int timeout, bool singleShort, sol::function callback)
                    -> std::unique_ptr<QTimer> {
                    auto timer = std::make_unique<QTimer>();
                    timer->setInterval(timeout);
                    timer->setSingleShot(singleShort);
                    QObject::connect(
                        timer.get(), &QTimer::timeout, guard->connectionGuard.get(), [callback] {
                            void_safe_call(callback);
                        });

                    return timer;
                },
                "start",
                [](QTimer *timer) { timer->start(); },
                "stop",
                [](QTimer *timer) { timer->stop(); });

            utils["openExternalUrl"] = [](const QString &url) {
                QDesktopServices::openUrl(QUrl::fromEncoded(url.toUtf8()));
            };

            return utils;
        });
}

} // namespace Lua::Internal
