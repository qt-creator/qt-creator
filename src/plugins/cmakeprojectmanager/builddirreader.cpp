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

#include "builddirreader.h"

#include "cmakebuildconfiguration.h"
#include "cmakekitinformation.h"
#include "servermodereader.h"
#include "tealeafreader.h"

#include <projectexplorer/kitinformation.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>

using namespace ProjectExplorer;

namespace CMakeProjectManager {
namespace Internal {

// --------------------------------------------------------------------
// BuildDirReader:
// --------------------------------------------------------------------

BuildDirReader::Parameters::Parameters() = default;

BuildDirReader::Parameters::Parameters(const CMakeBuildConfiguration *bc)
{
    const ProjectExplorer::Kit *k = bc->target()->kit();

    projectName = bc->target()->project()->displayName();

    sourceDirectory = bc->target()->project()->projectDirectory();
    buildDirectory = bc->buildDirectory();

    environment = bc->environment();

    CMakeTool *cmake = CMakeKitInformation::cmakeTool(k);
    cmakeVersion = cmake->version();
    cmakeHasServerMode = cmake->hasServerMode();
    cmakeExecutable = cmake->cmakeExecutable();

    pathMapper = cmake->pathMapper();
    isAutorun = cmake->isAutoRun();

    auto tc = ProjectExplorer::ToolChainKitInformation::toolChain(k, ProjectExplorer::Constants::CXX_LANGUAGE_ID);
    if (tc)
        cxxToolChainId = tc->id();
    tc = ProjectExplorer::ToolChainKitInformation::toolChain(k, ProjectExplorer::Constants::C_LANGUAGE_ID);
    if (tc)
        cToolChainId = tc->id();
    sysRoot = ProjectExplorer::SysRootKitInformation::sysRoot(k);

    expander = k->macroExpander();

    configuration = bc->cmakeConfiguration();

    generator = CMakeGeneratorKitInformation::generator(k);
    extraGenerator = CMakeGeneratorKitInformation::extraGenerator(k);
    platform = CMakeGeneratorKitInformation::platform(k);
    toolset = CMakeGeneratorKitInformation::toolset(k);
    generatorArguments = CMakeGeneratorKitInformation::generatorArguments(k);
}

BuildDirReader::Parameters::Parameters(const BuildDirReader::Parameters &) = default;


BuildDirReader *BuildDirReader::createReader(const BuildDirReader::Parameters &p)
{
    if (p.cmakeHasServerMode)
        return new ServerModeReader;
    return new TeaLeafReader;
}

void BuildDirReader::setParameters(const BuildDirReader::Parameters &p)
{
    m_parameters = p;
}

} // namespace Internal
} // namespace CMakeProjectManager
