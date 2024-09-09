// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "utils_global.h"

#include "filepath.h"

#include <QString>

namespace Utils {

class QTCREATOR_UTILS_EXPORT AppInfo
{
public:
    QString author;
    QString copyright;
    QString displayVersion;
    QString id;
    QString revision;
    QString revisionUrl;
    QString userFileExtension;

    FilePath plugins;

    /*! Local plugin path: <localappdata>/plugins
        where <localappdata> is e.g.
        "%LOCALAPPDATA%\QtProject\qtcreator" on Windows Vista and later
        "$XDG_DATA_HOME/data/QtProject/qtcreator" or "~/.local/share/data/QtProject/qtcreator" on Linux
        "~/Library/Application Support/QtProject/Qt Creator" on Mac
    */
    FilePath userPluginsRoot;

    FilePath luaPlugins;
    FilePath userLuaPlugins;

    FilePath resources;
    FilePath userResources;
};

QTCREATOR_UTILS_EXPORT const AppInfo &appInfo();

namespace Internal {
QTCREATOR_UTILS_EXPORT void setAppInfo(const AppInfo &info);
} // namespace Internal

} // namespace Utils
