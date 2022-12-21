// Copyright (C) 2020 Alexis Jeandet.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/cpplanguage_details.h>

#include <QString>

namespace MesonProjectManager {
namespace Internal {

struct KitData
{
    QString cCompilerPath;
    QString cxxCompilerPath;
    QString cmakePath;
    QString qmakePath;
    QString qtVersionStr;
    Utils::QtMajorVersion qtVersion;
};

} // namespace Internal
} // namespace MesonProjectManager
