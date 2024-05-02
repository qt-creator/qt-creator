// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "../luaengine.h"
#include "../luaqttypes.h"

#include <utils/async.h>
#include <utils/futuresynchronizer.h>
#include <utils/hostosinfo.h>

#include <QTimer>

using namespace Utils;

namespace Lua::Internal {

void addUtilsModule()
{
    LuaEngine::registerProvider(
        "Utils",
        [futureSync = Utils::FutureSynchronizer()](sol::state_view lua) mutable -> sol::object {
            auto async = lua.script("return require('async')", "_utils_").get<sol::table>();

            sol::table utils = lua.create_table();

            utils.set_function("waitms_cb", [](int ms, const sol::function &cb) {
                QTimer *timer = new QTimer();
                timer->setSingleShot(true);
                timer->setInterval(ms);
                QObject::connect(timer, &QTimer::timeout, &LuaEngine::instance(), [cb, timer]() {
                    cb();
                    timer->deleteLater();
                });
                timer->start();
            });

            auto dirEntries_cb = [&futureSync](
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

                Utils::onFinished<FilePath>(
                    future, &LuaEngine::instance(), [cb](const QFuture<FilePath> &future) {
                        cb(future.results());
                    });
            };

            utils.set_function("__dirEntries_cb__", dirEntries_cb);

            sol::function wrap = async["wrap"].get<sol::function>();

            utils["waitms"] = wrap(utils["waitms_cb"]);

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

            sol::usertype<FilePath> filePathType = utils.new_usertype<FilePath>(
                "FilePath",
                sol::call_constructor,
                sol::constructors<FilePath()>(),
                sol::meta_function::to_string,
                &FilePath::toUserOutput,
                "fromUserInput",
                &FilePath::fromUserInput,
                "searchInPath",
                [](const FilePath &self) { return self.searchInPath(); },
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
                "resolvePath",
                sol::overload(
                    [](const FilePath &p, const QString &path) { return p.resolvePath(path); },
                    [](const FilePath &p, const FilePath &path) { return p.resolvePath(path); }));

            utils["FilePath"]["dirEntries_cb"] = utils["__dirEntries_cb__"];
            utils["FilePath"]["dirEntries"] = wrap(utils["__dirEntries_cb__"]);

            return utils;
        });
}

} // namespace Lua::Internal
