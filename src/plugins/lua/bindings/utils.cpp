// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "../luaengine.h"
#include "../luaqttypes.h"

#include <utils/hostosinfo.h>

#include <QTimer>

using namespace Utils;

namespace Lua::Internal {

void addUtilsModule()
{
    LuaEngine::registerProvider("__utils", [](sol::state_view lua) -> sol::object {
        sol::table utils = lua.create_table();

        utils.set_function("waitms_cb", [](int ms, sol::function cb) {
            QTimer *timer = new QTimer();
            timer->setSingleShot(true);
            timer->setInterval(ms);
            QObject::connect(timer, &QTimer::timeout, timer, [cb, timer]() {
                cb();
                timer->deleteLater();
            });
            timer->start();
        });

        return utils;
    });

    LuaEngine::registerProvider("Utils", [](sol::state_view lua) -> sol::object {
        sol::table utils = lua.script(
                                  R"(
local u = require("__utils")
local a = require("async")

return {
    waitms_cb = u.waitms_cb,
    waitms = a.wrap(u.waitms_cb)
}
)",
                                  "_utils_")
                               .get<sol::table>();

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

        auto filePathType = utils.new_usertype<FilePath>(
            "FilePath",
            sol::call_constructor,
            sol::constructors<FilePath()>(),
            "fromUserInput",
            &FilePath::fromUserInput,
            "searchInPath",
            [](const FilePath &self) { return self.searchInPath(); },
            "exists",
            &FilePath::exists,
            "isExecutableFile",
            &FilePath::isExecutableFile,
            "dirEntries",
            [](sol::this_state s, const FilePath &p, sol::table options) -> sol::table {
                sol::state_view lua(s);
                sol::table result = lua.create_table();
                const QStringList nameFilters = options.get_or<QStringList>("nameFilters", {});
                QDir::Filters fileFilters
                    = (QDir::Filters) options.get_or<int>("fileFilters", QDir::NoFilter);
                QDirIterator::IteratorFlags flags
                    = (QDirIterator::IteratorFlags)
                          options.get_or<int>("flags", QDirIterator::NoIteratorFlags);

                FileFilter filter(nameFilters);
                p.iterateDirectory(
                    [&result](const FilePath &item) {
                        result.add(item);
                        return IterationPolicy::Continue;
                    },
                    FileFilter(nameFilters, fileFilters, flags));

                return result;
            },
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

        return utils;
    });
}

} // namespace Lua::Internal
