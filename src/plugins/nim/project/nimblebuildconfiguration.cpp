// Copyright (C) Filippo Cucchetto <filippocucchetto@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "nimblebuildconfiguration.h"

#include "nimconstants.h"
#include "nimtr.h"

#include <projectexplorer/buildinfo.h>
#include <projectexplorer/buildstep.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/kit.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorer.h>

using namespace ProjectExplorer;
using namespace Utils;

namespace Nim {

NimbleBuildConfiguration::NimbleBuildConfiguration(Target *target, Id id)
    : BuildConfiguration(target, id)
{
    setConfigWidgetDisplayName(Tr::tr("General"));
    setConfigWidgetHasFrame(true);
    setBuildDirectorySettingsKey("Nim.NimbleBuildConfiguration.BuildDirectory");
    appendInitialBuildStep(Constants::C_NIMBLEBUILDSTEP_ID);

    setInitializer([this](const BuildInfo &info) {
        setBuildType(info.buildType);
        setBuildDirectory(project()->projectDirectory());
    });
}

BuildConfiguration::BuildType NimbleBuildConfiguration::buildType() const
{
    return m_buildType;
}

void NimbleBuildConfiguration::fromMap(const Store &map)
{
    m_buildType = static_cast<BuildType>(map[Constants::C_NIMBLEBUILDCONFIGURATION_BUILDTYPE].toInt());
    BuildConfiguration::fromMap(map);
}

void NimbleBuildConfiguration::toMap(Store &map) const
{
    BuildConfiguration::toMap(map);
    map[Constants::C_NIMBLEBUILDCONFIGURATION_BUILDTYPE] = buildType();
}

void NimbleBuildConfiguration::setBuildType(BuildConfiguration::BuildType buildType)
{
    if (buildType == m_buildType)
        return;
    m_buildType = buildType;
    emit buildTypeChanged();
}

NimbleBuildConfigurationFactory::NimbleBuildConfigurationFactory()
{
    registerBuildConfiguration<NimbleBuildConfiguration>(Constants::C_NIMBLEBUILDCONFIGURATION_ID);
    setSupportedProjectType(Constants::C_NIMBLEPROJECT_ID);
    setSupportedProjectMimeTypeName(Constants::C_NIMBLE_MIMETYPE);

    setBuildGenerator([](const Kit *, const FilePath &projectPath, bool forSetup) {
        const auto oneBuild = [&](BuildConfiguration::BuildType buildType, const QString &typeName) {
            BuildInfo info;
            info.buildType = buildType;
            info.typeName = typeName;
            if (forSetup) {
                info.displayName = info.typeName;
                info.buildDirectory = projectPath.parentDir();
            }
            return info;
        };
        return QList<BuildInfo>{
            oneBuild(BuildConfiguration::Debug, Tr::tr("Debug")),
            oneBuild(BuildConfiguration::Release, Tr::tr("Release"))
        };
    });
}

} // Nim
