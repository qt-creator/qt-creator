/****************************************************************************
**
** Copyright (C) 2020 Alexis Jeandet.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/
#include "kitdata.h"
#include <projectexplorer/kit.h>
#include <projectexplorer/kitinformation.h>
#include <utils/macroexpander.h>
#include <utils/qtcassert.h>
#include <versionhelper.h>
#include <QString>
#pragma once
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
    data.qtVersion = Utils::QtVersion::None;
    auto version = Version::fromString(data.qtVersionStr);
    if (version.isValid) {
        switch (version.major) {
        case 4:
            data.qtVersion = Utils::QtVersion::Qt4;
            break;
        case 5:
            data.qtVersion = Utils::QtVersion::Qt5;
            break;
        default:
            data.qtVersion = Utils::QtVersion::Unknown;
        }
    }
    return data;
}

} // namespace KitHelper

} // namespace Internal
} // namespace MesonProjectManager
