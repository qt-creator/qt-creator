// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "builddirparameters.h"

#include "cmakebuildconfiguration.h"
#include "cmakebuildsystem.h"
#include "cmakekitinformation.h"
#include "cmaketoolmanager.h"

#include <projectexplorer/kitinformation.h>
#include <projectexplorer/project.h>
#include <projectexplorer/target.h>
#include <projectexplorer/toolchain.h>

#include <utils/algorithm.h>
#include <utils/macroexpander.h>
#include <utils/qtcassert.h>

using namespace ProjectExplorer;

namespace CMakeProjectManager::Internal {

BuildDirParameters::BuildDirParameters() = default;

BuildDirParameters::BuildDirParameters(CMakeBuildSystem *buildSystem)
{
    QTC_ASSERT(buildSystem, return);
    auto bc = buildSystem->cmakeBuildConfiguration();
    QTC_ASSERT(bc, return);

    expander = bc->macroExpander();

    const QStringList expandedArguments = Utils::transform(bc->initialCMakeArguments.allValues(),
                                                           [this](const QString &s) {
                                                               return expander->expand(s);
                                                           });
    initialCMakeArguments = Utils::filtered(expandedArguments,
                                            [](const QString &s) { return !s.isEmpty(); });
    configurationChangesArguments = Utils::transform(buildSystem->configurationChangesArguments(),
                                                     [this](const QString &s) {
                                                         return expander->expand(s);
                                                     });
    additionalCMakeArguments = Utils::transform(bc->additionalCMakeArguments(),
                                                [this](const QString &s) {
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

    cmakeBuildType = buildSystem->cmakeBuildType();

    environment = bc->configureEnvironment();
    // Disable distributed building for configuration runs. CMake does not do those in parallel,
    // so there is no win in sending data over the network.
    // Unfortunately distcc does not have a simple environment flag to turn it off:-/
    if (Utils::HostOsInfo::isAnyUnixHost())
        environment.set("ICECC", "no");

    environment.set("QTC_RUN", "1");

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

} // CMakeProjectManager::Internal
