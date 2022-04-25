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
#include "cmakebuildsystem.h"
#include "cmakekitinformation.h"
#include "cmakeprojectconstants.h"
#include "cmakeprojectplugin.h"
#include "cmakespecificsettings.h"
#include "cmaketoolmanager.h"

#include <projectexplorer/kitinformation.h>
#include <projectexplorer/project.h>
#include <projectexplorer/target.h>
#include <projectexplorer/toolchain.h>

#include <utils/algorithm.h>
#include <utils/qtcassert.h>

using namespace ProjectExplorer;

namespace CMakeProjectManager {
namespace Internal {

BuildDirParameters::BuildDirParameters() = default;

BuildDirParameters::BuildDirParameters(CMakeBuildSystem *buildSystem)
{
    QTC_ASSERT(buildSystem, return);
    auto bc = buildSystem->cmakeBuildConfiguration();
    QTC_ASSERT(bc, return);

    const Utils::MacroExpander *expander = bc->macroExpander();

    const QStringList expandedArguments = Utils::transform(bc->initialCMakeArguments(),
                                                           [expander](const QString &s) {
                                                               return expander->expand(s);
                                                           });
    initialCMakeArguments = Utils::filtered(expandedArguments,
                                            [](const QString &s) { return !s.isEmpty(); });
    configurationChangesArguments = Utils::transform(bc->configurationChangesArguments(),
                                                     [expander](const QString &s) {
                                                         return expander->expand(s);
                                                     });
    additionalCMakeArguments = Utils::transform(bc->additionalCMakeArguments(),
                                                [expander](const QString &s) {
                                                    return expander->expand(s);
                                                });
    const Target *t = bc->target();
    const Kit *k = t->kit();
    const Project *p = t->project();

    projectName = p->displayName();

    sourceDirectory = bc->sourceDirectory();
    if (sourceDirectory.isEmpty())
        sourceDirectory = p->projectDirectory();
    buildDirectory = bc->buildDirectory();

    cmakeBuildType = bc->cmakeBuildType();

    environment = bc->environment();
    // Disable distributed building for configuration runs. CMake does not do those in parallel,
    // so there is no win in sending data over the network.
    // Unfortunately distcc does not have a simple environment flag to turn it off:-/
    if (Utils::HostOsInfo::isAnyUnixHost())
        environment.set("ICECC", "no");

    CMakeSpecificSettings *settings = CMakeProjectPlugin::projectTypeSpecificSettings();
    if (!settings->ninjaPath.filePath().isEmpty()) {
        const Utils::FilePath ninja = settings->ninjaPath.filePath();
        environment.appendOrSetPath(ninja.isFile() ? ninja.parentDir() : ninja);
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
