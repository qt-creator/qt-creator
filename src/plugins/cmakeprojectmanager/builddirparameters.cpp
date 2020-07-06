/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "builddirparameters.h"

#include "cmakebuildconfiguration.h"
#include "cmakekitinformation.h"
#include "cmakeprojectplugin.h"
#include "cmakespecificsettings.h"
#include "cmaketoolmanager.h"

#include <projectexplorer/kitinformation.h>
#include <projectexplorer/project.h>
#include <projectexplorer/target.h>
#include <projectexplorer/toolchain.h>

#include <utils/qtcassert.h>

using namespace ProjectExplorer;

namespace CMakeProjectManager {
namespace Internal {

BuildDirParameters::BuildDirParameters() = default;

BuildDirParameters::BuildDirParameters(CMakeBuildConfiguration *bc)
{
    QTC_ASSERT(bc, return );

    const Utils::MacroExpander *expander = bc->macroExpander();

    initialCMakeArguments = Utils::transform(bc->initialCMakeArguments(),
                                             [expander](const QString &s) {
                                                 return expander->expand(s);
                                             });
    extraCMakeArguments = Utils::transform(bc->extraCMakeArguments(),
                                             [expander](const QString &s) {
                                                 return expander->expand(s);
                                             });

    const Target *t = bc->target();
    const Kit *k = t->kit();
    const Project *p = t->project();

    projectName = p->displayName();

    sourceDirectory = p->projectDirectory();
    buildDirectory = bc->buildDirectory();

    environment = bc->environment();
    // Disable distributed building for configuration runs. CMake does not do those in parallel,
    // so there is no win in sending data over the network.
    // Unfortunately distcc does not have a simple environment flag to turn it off:-/
    if (Utils::HostOsInfo::isAnyUnixHost())
        environment.set("ICECC", "no");

    CMakeSpecificSettings *settings = CMakeProjectPlugin::projectTypeSpecificSettings();
    if (!settings->ninjaPath().isEmpty()) {
        const Utils::FilePath setting = settings->ninjaPath();
        const Utils::FilePath path = setting.toFileInfo().isFile() ? setting.parentDir() : setting;
        environment.appendOrSetPath(path.toString());
    }

    cmakeToolId = CMakeKitAspect::cmakeToolId(k);
}

bool BuildDirParameters::isValid() const
{
    return cmakeTool();
}

CMakeTool *BuildDirParameters::cmakeTool() const
{
    return CMakeToolManager::findById(cmakeToolId);
}

BuildDirParameters::BuildDirParameters(const BuildDirParameters &) = default;
BuildDirParameters &BuildDirParameters::operator=(const BuildDirParameters &) = default;

} // namespace Internal
} // namespace CMakeProjectManager
