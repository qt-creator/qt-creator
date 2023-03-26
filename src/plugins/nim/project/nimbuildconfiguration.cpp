// Copyright (C) Filippo Cucchetto <filippocucchetto@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "nimbuildconfiguration.h"
#include "nimcompilerbuildstep.h"

#include "../nimtr.h"
#include "../nimconstants.h"

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/buildinfo.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/buildstep.h>
#include <projectexplorer/kit.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>

#include <utils/aspects.h>
#include <utils/qtcassert.h>

using namespace ProjectExplorer;
using namespace Utils;

namespace Nim {

static FilePath defaultBuildDirectory(const Kit *k,
                                      const FilePath &projectFilePath,
                                      const QString &bc,
                                      BuildConfiguration::BuildType buildType)
{
    return BuildConfiguration::buildDirectoryFromTemplate(
        projectFilePath.parentDir(), projectFilePath, projectFilePath.baseName(),
        k, bc, buildType, "nim");
}

NimBuildConfiguration::NimBuildConfiguration(Target *target, Utils::Id id)
    : BuildConfiguration(target, id)
{
    setConfigWidgetDisplayName(Tr::tr("General"));
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
        nimCompilerBuildStep->setBuildType(info.buildType);
    });
}


FilePath NimBuildConfiguration::cacheDirectory() const
{
    return buildDirectory().pathAppended("nimcache");
}

FilePath NimBuildConfiguration::outFilePath() const
{
    auto nimCompilerBuildStep = buildSteps()->firstOfType<NimCompilerBuildStep>();
    QTC_ASSERT(nimCompilerBuildStep, return {});
    return nimCompilerBuildStep->outFilePath();
}

// NimBuildConfigurationFactory

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
            oneBuild(BuildConfiguration::Debug, Tr::tr("Debug")),
            oneBuild(BuildConfiguration::Release, Tr::tr("Release"))
        };
    });
}

} // namespace Nim

