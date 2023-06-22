// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "utils_global.h"

#include <QString>

namespace Utils {

class QTCREATOR_UTILS_EXPORT AppInfo
{
public:
    QString author;
    QString year;
    QString displayVersion;
    QString id;
    QString revision;
    QString revisionUrl;
    QString userFileExtension;
};

QTCREATOR_UTILS_EXPORT AppInfo appInfo();

namespace Internal {
QTCREATOR_UTILS_EXPORT void setAppInfo(const AppInfo &info);
} // namespace Internal

} // namespace Utils
