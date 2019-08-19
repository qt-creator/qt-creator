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
#include "nimbuildsystem.h"
#include "nimcompilerbuildstep.h"
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
#include <projectexplorer/projectconfigurationaspects.h>
#include <utils/mimetypes/mimedatabase.h>
#include <utils/qtcassert.h>

using namespace ProjectExplorer;
using namespace Utils;

namespace Nim {

static FilePath defaultBuildDirectory(const Kit *k,
                                      const FilePath &projectFilePath,
                                      const QString &bc,
                                      BuildConfiguration::BuildType buildType)
{
    QFileInfo projectFileInfo = projectFilePath.toFileInfo();

    ProjectMacroExpander expander(projectFilePath,
                                  projectFileInfo.baseName(), k, bc, buildType);
    QString buildDirectory = expander.expand(ProjectExplorerPlugin::buildDirectoryTemplate());

    if (FileUtils::isAbsolutePath(buildDirectory))
        return FilePath::fromString(buildDirectory);

    auto projectDir = FilePath::fromString(projectFileInfo.absoluteDir().absolutePath());
    return projectDir.pathAppended(buildDirectory);
}

NimBuildConfiguration::NimBuildConfiguration(Target *target, Core::Id id)
    : BuildConfiguration(target, id)
{
    setConfigWidgetDisplayName(tr("General"));
    setConfigWidgetHasFrame(true);
    setBuildDirectorySettingsKey("Nim.NimBuildConfiguration.BuildDirectory");
}

void NimBuildConfiguration::initialize()
{
    BuildConfiguration::initialize();

    auto bs = qobject_cast<NimBuildSystem *>(project()->buildSystem());
    QTC_ASSERT(bs, return );

    // Create the build configuration and initialize it from build info
    setBuildDirectory(defaultBuildDirectory(target()->kit(),
                                            project()->projectFilePath(),
                                            displayName(),
                                            buildType()));

    // Add nim compiler build step
    {
        BuildStepList *buildSteps = stepList(ProjectExplorer::Constants::BUILDSTEPS_BUILD);
        auto nimCompilerBuildStep = new NimCompilerBuildStep(buildSteps);
        NimCompilerBuildStep::DefaultBuildOptions defaultOption;
        switch (initialBuildType()) {
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
        Utils::FilePathList nimFiles = bs->nimFiles();
        if (!nimFiles.isEmpty())
            nimCompilerBuildStep->setTargetNimFile(nimFiles.first());
        buildSteps->appendStep(nimCompilerBuildStep);
    }

    // Add clean step
    {
        BuildStepList *cleanSteps = stepList(ProjectExplorer::Constants::BUILDSTEPS_CLEAN);
        cleanSteps->appendStep(Constants::C_NIMCOMPILERCLEANSTEP_ID);
    }
}

FilePath NimBuildConfiguration::cacheDirectory() const
{
    return buildDirectory().pathAppended("nimcache");
}

FilePath NimBuildConfiguration::outFilePath() const
{
    const NimCompilerBuildStep *step = nimCompilerBuildStep();
    QTC_ASSERT(step, return FilePath());
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

QList<BuildInfo> NimBuildConfigurationFactory::availableBuilds
    (const Kit *k, const FilePath &projectPath, bool forSetup) const
{
    QList<BuildInfo> result;
    for (auto buildType : {BuildConfiguration::Debug, BuildConfiguration::Release}) {
        BuildInfo info(this);
        info.buildType = buildType;
        info.kitId = k->id();

        if (buildType == BuildConfiguration::Debug)
            info.typeName = tr("Debug");
        else if (buildType == BuildConfiguration::Profile)
            info.typeName = tr("Profile");
        else if (buildType == BuildConfiguration::Release)
            info.typeName = tr("Release");

        if (forSetup) {
            info.displayName = info.typeName;
            info.buildDirectory = defaultBuildDirectory(k, projectPath, info.typeName, buildType);
        }
        result.push_back(info);
    }
    return result;
}

} // namespace Nim

