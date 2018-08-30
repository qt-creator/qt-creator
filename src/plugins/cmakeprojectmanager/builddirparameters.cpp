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
#include "cmaketoolmanager.h"

#include <projectexplorer/kit.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>

#include <utils/hostosinfo.h>

using namespace ProjectExplorer;

namespace CMakeProjectManager {
namespace Internal {

BuildDirParameters::BuildDirParameters() = default;

BuildDirParameters::BuildDirParameters(CMakeBuildConfiguration *bc)
{
    buildConfiguration = bc;

    const Kit *k = bc->target()->kit();

    projectName = bc->target()->project()->displayName();

    sourceDirectory = bc->target()->project()->projectDirectory();
    buildDirectory = bc->buildDirectory();

    environment = bc->environment();
    // Disable distributed building for configuration runs. CMake does not do those in parallel,
    // so there is no win in sending data over the network.
    // Unfortunately distcc does not have a simple environment flag to turn it off:-/
    if (Utils::HostOsInfo::isAnyUnixHost())
        environment.set("ICECC", "no");

    cmakeToolId = CMakeKitInformation::cmakeToolId(k);

    auto tc = ToolChainKitInformation::toolChain(k, Constants::CXX_LANGUAGE_ID);
    if (tc)
        cxxToolChainId = tc->id();
    tc = ToolChainKitInformation::toolChain(k, Constants::C_LANGUAGE_ID);
    if (tc)
        cToolChainId = tc->id();
    sysRoot = SysRootKitInformation::sysRoot(k);

    expander = k->macroExpander();

    configuration = bc->configurationForCMake();

    generator = CMakeGeneratorKitInformation::generator(k);
    extraGenerator = CMakeGeneratorKitInformation::extraGenerator(k);
    platform = CMakeGeneratorKitInformation::platform(k);
    toolset = CMakeGeneratorKitInformation::toolset(k);
    generatorArguments = CMakeGeneratorKitInformation::generatorArguments(k);
}

bool BuildDirParameters::isValid() const { return buildConfiguration && cmakeTool(); }

CMakeTool *BuildDirParameters::cmakeTool() const
{
    return CMakeToolManager::findById(cmakeToolId);
}

BuildDirParameters::BuildDirParameters(const BuildDirParameters &) = default;

} // namespace Internal
} // namespace CMakeProjectManager
