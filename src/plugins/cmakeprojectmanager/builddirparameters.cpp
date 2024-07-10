// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "builddirparameters.h"

#include "cmakebuildconfiguration.h"
#include "cmakebuildsystem.h"
#include "cmakekitaspect.h"
#include "cmaketoolmanager.h"

#include <projectexplorer/customparser.h>
#include <projectexplorer/kitaspects.h>
#include <projectexplorer/project.h>
#include <projectexplorer/target.h>
#include <projectexplorer/toolchain.h>

#include <utils/algorithm.h>
#include <utils/aspects.h>
#include <utils/macroexpander.h>
#include <utils/qtcassert.h>

using namespace ProjectExplorer;
using namespace Utils;

namespace CMakeProjectManager::Internal {

BuildDirParameters::BuildDirParameters() = default;

static void updateCMakePathsFromQMake(QStringList &initialCMakeArguments)
{
    const CMakeConfigItem qmake = CMakeConfigItem::fromString(
        Utils::findOrDefault(initialCMakeArguments, [](const QString &arg) {
            return arg.startsWith("-DQT_QMAKE_EXECUTABLE");
        }));
    const FilePath qmakeFilePath = FilePath::fromUtf8(qmake.value);
    if (qmakeFilePath.isEmpty())
        return;

    // ~Qt/6.x/platform/bin/qmake -> ~Qt/6.x/platform
    const QByteArray qmakePrefixPath = qmakeFilePath.parentDir().parentDir().path().toUtf8();
    const QByteArrayList cmakePathsVariables = {"CMAKE_PREFIX_PATH", "CMAKE_FIND_ROOT_PATH"};

    for (const QByteArray &var : cmakePathsVariables) {
        const QString varPrefix = QString("-D") + var;
        auto it = std::find_if(
            initialCMakeArguments.begin(),
            initialCMakeArguments.end(),
            [varPrefix](const QString &arg) { return arg.startsWith(varPrefix); });

        if (it == initialCMakeArguments.end())
            continue;

        CMakeConfigItem cmakeVar = CMakeConfigItem::fromString(*it);
        if (cmakeVar.isNull())
            continue;

        if (cmakeVar.value.isEmpty())
            cmakeVar.value = qmakePrefixPath;
        else
            cmakeVar.value += ";" + qmakePrefixPath;

        *it = cmakeVar.toString();
    }
}

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

    const bool noQtInstallPrefix = expander->expand(QString("%{Qt:QT_INSTALL_PREFIX}")).isEmpty();
    if (noQtInstallPrefix)
        updateCMakePathsFromQMake(initialCMakeArguments);

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

    project = t->project();
    projectName = project->displayName();

    sourceDirectory = bc->sourceDirectory();
    if (sourceDirectory.isEmpty())
        sourceDirectory = project->projectDirectory();
    buildDirectory = bc->buildDirectory();

    cmakeBuildType = buildSystem->cmakeBuildType();

    environment = bc->configureEnvironment();
    // Disable distributed building for configuration runs. CMake does not do those in parallel,
    // so there is no win in sending data over the network.
    // Unfortunately distcc does not have a simple environment flag to turn it off:-/
    if (Utils::HostOsInfo::isAnyUnixHost())
        environment.set("ICECC", "no");

    environment.set("QTC_RUN", "1");
    environment.setFallback("CMAKE_COLOR_DIAGNOSTICS", "1");
    environment.setFallback("CLICOLOR_FORCE", "1");

    cmakeToolId = CMakeKitAspect::cmakeToolId(k);

    outputParserGenerator = [k, bc]() {
        QList<OutputLineParser *> outputParsers = k->createOutputParsers();
        for (const Id id : bc->customParsers()) {
            if (auto parser = createCustomParserFromId(id))
                outputParsers << parser;
        }
        return outputParsers;
    };
}

bool BuildDirParameters::isValid() const
{
    return cmakeTool();
}

CMakeTool *BuildDirParameters::cmakeTool() const
{
    return CMakeToolManager::findById(cmakeToolId);
}

QList<OutputLineParser *> BuildDirParameters::outputParsers() const
{
    QTC_ASSERT(outputParserGenerator, return {});
    return outputParserGenerator();
}

} // CMakeProjectManager::Internal
