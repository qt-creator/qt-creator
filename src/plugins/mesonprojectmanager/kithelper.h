// Copyright (C) 2020 Alexis Jeandet.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "kitdata.h"
#include "versionhelper.h"

#include <projectexplorer/kit.h>
#include <projectexplorer/kitinformation.h>

#include <utils/macroexpander.h>
#include <utils/qtcassert.h>

namespace MesonProjectManager {
namespace Internal {

namespace KitHelper {
namespace details {

inline QString expand(const ProjectExplorer::Kit *kit, const QString &macro)
{
    return kit->macroExpander()->expand(macro);
}

} // namespace details

inline QString cCompilerPath(const ProjectExplorer::Kit *kit)
{
    QTC_ASSERT(kit, return {});
    return details::expand(kit, "%{Compiler:Executable:C}");
}

inline QString cxxCompilerPath(const ProjectExplorer::Kit *kit)
{
    QTC_ASSERT(kit, return {});
    return details::expand(kit, "%{Compiler:Executable:Cxx}");
}

inline QString qmakePath(const ProjectExplorer::Kit *kit)
{
    return details::expand(kit, "%{Qt:qmakeExecutable}");
}

inline QString cmakePath(const ProjectExplorer::Kit *kit)
{
    return details::expand(kit, "%{CMake:Executable:FilePath}");
}

inline QString qtVersion(const ProjectExplorer::Kit *kit)
{
    QTC_ASSERT(kit, return {});
    return details::expand(kit, "%{Qt:Version}");
}

inline KitData kitData(const ProjectExplorer::Kit *kit)
{
    QTC_ASSERT(kit, return {});
    KitData data;
    data.cCompilerPath = cCompilerPath(kit);
    data.cxxCompilerPath = cxxCompilerPath(kit);
    data.cmakePath = cmakePath(kit);
    data.qmakePath = qmakePath(kit);
    data.qtVersionStr = qtVersion(kit);
    data.qtVersion = Utils::QtMajorVersion::None;
    auto version = Version::fromString(data.qtVersionStr);
    if (version.isValid) {
        switch (version.major) {
        case 4:
            data.qtVersion = Utils::QtMajorVersion::Qt4;
            break;
        case 5:
            data.qtVersion = Utils::QtMajorVersion::Qt5;
            break;
        case 6:
            data.qtVersion = Utils::QtMajorVersion::Qt6;
            break;
        default:
            data.qtVersion = Utils::QtMajorVersion::Unknown;
        }
    }
    return data;
}

} // namespace KitHelper

} // namespace Internal
} // namespace MesonProjectManager
