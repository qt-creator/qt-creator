/****************************************************************************
**
** Copyright (C) Filippo Cucchetto <filippocucchetto@gmail.com>
** Contact: http://www.qt.io/licensing
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

#include "nimbuildconfigurationfactory.h"
#include "nimbuildconfiguration.h"
#include "nimcompilerbuildstep.h"
#include "nimcompilercleanstep.h"
#include "nimproject.h"

#include "../nimconstants.h"

#include <coreplugin/documentmanager.h>
#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/buildinfo.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/buildstep.h>
#include <projectexplorer/kit.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectmacroexpander.h>
#include <projectexplorer/target.h>
#include <utils/mimetypes/mimedatabase.h>
#include <utils/qtcassert.h>

#include <memory>

using namespace ProjectExplorer;
using namespace Utils;

namespace Nim {

NimBuildConfigurationFactory::NimBuildConfigurationFactory(QObject *parent)
    : IBuildConfigurationFactory(parent)
{}

QList<BuildInfo *> NimBuildConfigurationFactory::availableBuilds(const Target *parent) const
{
    // Retrieve the project path
    auto project = qobject_cast<NimProject *>(parent->project());
    QTC_ASSERT(project, return {});

    // Create the build info
    BuildInfo *info = createBuildInfo(parent->kit(), project->projectFilePath().toString(),
                                      BuildConfiguration::Debug);

    info->displayName.clear(); // ask for a name
    info->buildDirectory.clear(); // This depends on the displayName

    return {info};
}

QList<BuildInfo *> NimBuildConfigurationFactory::availableSetups(const Kit *k, const QString &projectPath) const
{
    BuildInfo *debug = createBuildInfo(k, projectPath, BuildConfiguration::Debug);
    BuildInfo *release = createBuildInfo(k, projectPath, BuildConfiguration::Release);
    return {debug, release};
}

BuildConfiguration *NimBuildConfigurationFactory::create(Target *parent, const BuildInfo *info) const
{
    auto project = qobject_cast<NimProject *>(parent->project());
    QTC_ASSERT(project, return nullptr);

    // Create the build configuration and initialize it from build info
    auto result = new NimBuildConfiguration(parent);
    result->setDisplayName(info->displayName);
    result->setDefaultDisplayName(info->displayName);
    result->setBuildDirectory(defaultBuildDirectory(parent->kit(),
                                                    project->projectFilePath().toString(),
                                                    info->displayName,
                                                    info->buildType));

    // Add nim compiler build step
    {
        BuildStepList *buildSteps = result->stepList(Core::Id(ProjectExplorer::Constants::BUILDSTEPS_BUILD));
        auto nimCompilerBuildStep = new NimCompilerBuildStep(buildSteps);
        NimCompilerBuildStep::DefaultBuildOptions defaultOption;
        switch (info->buildType) {
        case BuildConfiguration::Release:
            defaultOption = NimCompilerBuildStep::DefaultBuildOptions::Release;
            break;
        case BuildConfiguration::Debug:
            defaultOption = NimCompilerBuildStep::DefaultBuildOptions::Debug;
            break;
        default:
            defaultOption = NimCompilerBuildStep::DefaultBuildOptions::Empty;
            break;
        }
        nimCompilerBuildStep->setDefaultCompilerOptions(defaultOption);
        Utils::FileNameList nimFiles = project->nimFiles();
        if (!nimFiles.isEmpty())
            nimCompilerBuildStep->setTargetNimFile(nimFiles.first());
        buildSteps->appendStep(nimCompilerBuildStep);
    }

    // Add clean step
    {
        BuildStepList *cleanSteps = result->stepList(Core::Id(ProjectExplorer::Constants::BUILDSTEPS_CLEAN));
        cleanSteps->appendStep(new NimCompilerCleanStep(cleanSteps));
    }

    return result;
}

bool NimBuildConfigurationFactory::canRestore(const Target *parent, const QVariantMap &map) const
{
    Q_UNUSED(parent);
    if (!canHandle(parent))
        return false;
    return NimBuildConfiguration::canRestore(map);
}

BuildConfiguration *NimBuildConfigurationFactory::restore(Target *parent, const QVariantMap &map)
{
    QTC_ASSERT(canRestore(parent, map), return nullptr);

    // Create the build configuration
    auto result = new NimBuildConfiguration(parent);

    // Restore from map
    bool status = result->fromMap(map);
    QTC_ASSERT(status, return nullptr);

    return result;
}

bool NimBuildConfigurationFactory::canClone(const Target *parent, BuildConfiguration *product) const
{
    QTC_ASSERT(parent, return false);
    QTC_ASSERT(product, return false);
    if (!canHandle(parent))
        return false;
    return product->id() == Constants::C_NIMBUILDCONFIGURATION_ID;
}

BuildConfiguration *NimBuildConfigurationFactory::clone(Target *parent, BuildConfiguration *product)
{
    QTC_ASSERT(parent, return nullptr);
    QTC_ASSERT(product, return nullptr);
    auto buildConfiguration = qobject_cast<NimBuildConfiguration *>(product);
    QTC_ASSERT(buildConfiguration, return nullptr);
    std::unique_ptr<NimBuildConfiguration> result(new NimBuildConfiguration(parent));
    return result->fromMap(buildConfiguration->toMap()) ? result.release() : nullptr;
}

int NimBuildConfigurationFactory::priority(const Kit *k, const QString &projectPath) const
{
    if (k && Utils::mimeTypeForFile(projectPath).matchesName(Constants::C_NIM_PROJECT_MIMETYPE))
        return 0;
    return -1;
}

int NimBuildConfigurationFactory::priority(const Target *parent) const
{
    return canHandle(parent) ? 0 : -1;
}

bool NimBuildConfigurationFactory::canHandle(const Target *t) const
{
    if (!t->project()->supportsKit(t->kit()))
        return false;
    return qobject_cast<NimProject *>(t->project());
}

FileName NimBuildConfigurationFactory::defaultBuildDirectory(const Kit *k,
                                                                    const QString &projectFilePath,
                                                                    const QString &bc,
                                                                    BuildConfiguration::BuildType buildType)
{
    QFileInfo projectFileInfo(projectFilePath);

    ProjectMacroExpander expander(projectFilePath, projectFileInfo.baseName(), k, bc, buildType);
    QString buildDirectory = expander.expand(Core::DocumentManager::buildDirectory());

    if (FileUtils::isAbsolutePath(buildDirectory))
        return FileName::fromString(buildDirectory);

    auto projectDir = FileName::fromString(projectFileInfo.absoluteDir().absolutePath());
    auto result = projectDir.appendPath(buildDirectory);

    return result;
}

BuildInfo *NimBuildConfigurationFactory::createBuildInfo(const Kit *k, const QString &projectFilePath,
                                                         BuildConfiguration::BuildType buildType) const
{
    auto result = new BuildInfo(this);
    result->buildType = buildType;
    result->displayName = BuildConfiguration::buildTypeName(buildType);
    result->buildDirectory = defaultBuildDirectory(k, projectFilePath, result->displayName, buildType);
    result->kitId = k->id();
    result->typeName = tr("Build");
    return result;
}

} // namespace Nim
