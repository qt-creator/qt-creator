// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "utils_global.h"

#include "filepath.h"

#include <QDateTime>
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
    QDateTime buildTime; // can be invalid if compiled without time stamp

    FilePath plugins;
    FilePath userPluginsRoot;

    FilePath luaPlugins;
    FilePath userLuaPlugins;

    FilePath resources;
    FilePath userResources;
    FilePath crashReports;

    FilePath libexec;
    FilePath documentation;
};

QTCREATOR_UTILS_EXPORT const AppInfo &appInfo();
QTCREATOR_UTILS_EXPORT QString compilerString();

namespace Internal {
QTCREATOR_UTILS_EXPORT void setAppInfo(const AppInfo &info);
} // namespace Internal

} // namespace Utils
