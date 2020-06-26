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

NimBuildConfiguration::NimBuildConfiguration(Target *target, Utils::Id id)
    : BuildConfiguration(target, id)
{
    setConfigWidgetDisplayName(tr("General"));
    setConfigWidgetHasFrame(true);
    setBuildDirectorySettingsKey("Nim.NimBuildConfiguration.BuildDirectory");

    appendInitialBuildStep(Constants::C_NIMCOMPILERBUILDSTEP_ID);
    appendInitialCleanStep(Constants::C_NIMCOMPILERCLEANSTEP_ID);

    setInitializer([this, target](const BuildInfo &info) {
        // Create the build configuration and initialize it from build info
        setBuildDirectory(defaultBuildDirectory(target->kit(),
                                                project()->projectFilePath(),
                                                displayName(),
                                                buildType()));

        auto nimCompilerBuildStep = buildSteps()->firstOfType<NimCompilerBuildStep>();
        QTC_ASSERT(nimCompilerBuildStep, return);
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

        const Utils::FilePaths nimFiles = project()->files([](const Node *n) {
            return Project::AllFiles(n) && n->path().endsWith(".nim");
        });

        if (!nimFiles.isEmpty())
            nimCompilerBuildStep->setTargetNimFile(nimFiles.first());
    });
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
    foreach (BuildStep *step, buildSteps()->steps())
        if (step->id() == Constants::C_NIMCOMPILERBUILDSTEP_ID)
            return qobject_cast<NimCompilerBuildStep *>(step);
    return nullptr;
}


NimBuildConfigurationFactory::NimBuildConfigurationFactory()
{
    registerBuildConfiguration<NimBuildConfiguration>(Constants::C_NIMBUILDCONFIGURATION_ID);
    setSupportedProjectType(Constants::C_NIMPROJECT_ID);
    setSupportedProjectMimeTypeName(Constants::C_NIM_PROJECT_MIMETYPE);

    setBuildGenerator([](const Kit *k, const FilePath &projectPath, bool forSetup) {
        const auto oneBuild = [&](BuildConfiguration::BuildType buildType, const QString &typeName) {
            BuildInfo info;
            info.buildType = buildType;
            info.typeName = typeName;
            if (forSetup) {
                info.displayName = info.typeName;
                info.buildDirectory = defaultBuildDirectory(k, projectPath, info.typeName, buildType);
            }
            return info;
        };
        return QList<BuildInfo>{
            oneBuild(BuildConfiguration::Debug, BuildConfiguration::tr("Debug")),
            oneBuild(BuildConfiguration::Release, BuildConfiguration::tr("Release"))
        };
    });
}

} // namespace Nim

