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
        k, bc, buildType);
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
            oneBuild(BuildConfiguration::Debug, BuildConfiguration::tr("Debug")),
            oneBuild(BuildConfiguration::Release, BuildConfiguration::tr("Release"))
        };
    });
}

} // namespace Nim

