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

#include "nimbuildconfiguration.h"
#include "nimbuildconfigurationwidget.h"
#include "nimcompilerbuildstep.h"
#include "nimproject.h"
#include "nimbuildconfiguration.h"
#include "nimcompilerbuildstep.h"
#include "nimcompilercleanstep.h"
#include "nimproject.h"

#include "../nimconstants.h"

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/buildinfo.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/buildstep.h>
#include <projectexplorer/kit.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectmacroexpander.h>
#include <projectexplorer/target.h>
#include <utils/mimetypes/mimedatabase.h>
#include <utils/qtcassert.h>

using namespace ProjectExplorer;
using namespace Utils;

namespace Nim {

static FileName defaultBuildDirectory(const Kit *k,
                                      const QString &projectFilePath,
                                      const QString &bc,
                                      BuildConfiguration::BuildType buildType)
{
    QFileInfo projectFileInfo(projectFilePath);

    ProjectMacroExpander expander(projectFilePath, projectFileInfo.baseName(), k, bc, buildType);
    QString buildDirectory = expander.expand(ProjectExplorerPlugin::buildDirectoryTemplate());

    if (FileUtils::isAbsolutePath(buildDirectory))
        return FileName::fromString(buildDirectory);

    auto projectDir = FileName::fromString(projectFileInfo.absoluteDir().absolutePath());
    auto result = projectDir.appendPath(buildDirectory);

    return result;
}

NimBuildConfiguration::NimBuildConfiguration(Target *target, Core::Id id)
    : BuildConfiguration(target, id)
{
}

void NimBuildConfiguration::initialize(const BuildInfo &info)
{
    BuildConfiguration::initialize(info);

    auto project = qobject_cast<NimProject *>(target()->project());
    QTC_ASSERT(project, return);

    // Create the build configuration and initialize it from build info
    setBuildDirectory(defaultBuildDirectory(target()->kit(),
                                            project->projectFilePath().toString(),
                                            info.displayName,
                                            info.buildType));

    // Add nim compiler build step
    {
        BuildStepList *buildSteps = stepList(ProjectExplorer::Constants::BUILDSTEPS_BUILD);
        auto nimCompilerBuildStep = new NimCompilerBuildStep(buildSteps);
        NimCompilerBuildStep::DefaultBuildOptions defaultOption;
        switch (info.buildType) {
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
        BuildStepList *cleanSteps = stepList(ProjectExplorer::Constants::BUILDSTEPS_CLEAN);
        cleanSteps->appendStep(new NimCompilerCleanStep(cleanSteps));
    }
}


NamedWidget *NimBuildConfiguration::createConfigWidget()
{
    return new NimBuildConfigurationWidget(this);
}

BuildConfiguration::BuildType NimBuildConfiguration::buildType() const
{
    return BuildConfiguration::Unknown;
}

bool NimBuildConfiguration::fromMap(const QVariantMap &map)
{
    if (!BuildConfiguration::fromMap(map))
        return false;

    const QString displayName = map[Constants::C_NIMBUILDCONFIGURATION_DISPLAY_KEY].toString();
    const QString buildDirectory = map[Constants::C_NIMBUILDCONFIGURATION_BUILDDIRECTORY_KEY].toString();

    setDisplayName(displayName);
    setBuildDirectory(FileName::fromString(buildDirectory));

    return true;
}

QVariantMap NimBuildConfiguration::toMap() const
{
    QVariantMap result = BuildConfiguration::toMap();
    result[Constants::C_NIMBUILDCONFIGURATION_DISPLAY_KEY] = displayName();
    result[Constants::C_NIMBUILDCONFIGURATION_BUILDDIRECTORY_KEY] = buildDirectory().toString();
    return result;
}

FileName NimBuildConfiguration::cacheDirectory() const
{
    return buildDirectory().appendPath(QStringLiteral("nimcache"));
}

FileName NimBuildConfiguration::outFilePath() const
{
    const NimCompilerBuildStep *step = nimCompilerBuildStep();
    QTC_ASSERT(step, return FileName());
    return step->outFilePath();
}

const NimCompilerBuildStep *NimBuildConfiguration::nimCompilerBuildStep() const
{
    BuildStepList *steps = stepList(ProjectExplorer::Constants::BUILDSTEPS_BUILD);
    QTC_ASSERT(steps, return nullptr);
    foreach (BuildStep *step, steps->steps())
        if (step->id() == Constants::C_NIMCOMPILERBUILDSTEP_ID)
            return qobject_cast<NimCompilerBuildStep *>(step);
    return nullptr;
}


NimBuildConfigurationFactory::NimBuildConfigurationFactory()
{
    registerBuildConfiguration<NimBuildConfiguration>(Constants::C_NIMBUILDCONFIGURATION_ID);
    setSupportedProjectType(Constants::C_NIMPROJECT_ID);
    setSupportedProjectMimeTypeName(Constants::C_NIM_PROJECT_MIMETYPE);
}

QList<BuildInfo> NimBuildConfigurationFactory::availableBuilds(const Target *parent) const
{
    QList<BuildInfo> result;
    for (auto buildType : {BuildConfiguration::Debug, BuildConfiguration::Release})
        result.push_back(createBuildInfo(parent->kit(), buildType));
    return result;
}

QList<BuildInfo> NimBuildConfigurationFactory::availableSetups(const Kit *k, const QString &projectPath) const
{
    QList<BuildInfo> result;
    for (auto buildType : {BuildConfiguration::Debug, BuildConfiguration::Release}) {
        BuildInfo info = createBuildInfo(k, buildType);
        info.displayName = info.typeName;
        info.buildDirectory = defaultBuildDirectory(k, projectPath, info.typeName, buildType);
        result.push_back(info);
    }
    return result;
}

BuildInfo NimBuildConfigurationFactory::createBuildInfo(const Kit *k, BuildConfiguration::BuildType buildType) const
{
    BuildInfo info(this);
    info.buildType = buildType;
    info.kitId = k->id();
    info.typeName = displayName(buildType);
    return info;
}

QString NimBuildConfigurationFactory::displayName(BuildConfiguration::BuildType buildType) const
{
    switch (buildType) {
    case ProjectExplorer::BuildConfiguration::Debug:
        return tr("Debug");
    case ProjectExplorer::BuildConfiguration::Profile:
        return tr("Profile");
    case ProjectExplorer::BuildConfiguration::Release:
        return tr("Release");
    default:
        return QString();
    }
}

} // namespace Nim

