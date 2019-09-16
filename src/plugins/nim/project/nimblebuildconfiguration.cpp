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

#include "nimblebuildconfiguration.h"
#include "nimconstants.h"
#include "nimblebuildstep.h"
#include "nimbleproject.h"

#include <projectexplorer/buildinfo.h>
#include <projectexplorer/buildstep.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/kit.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectmacroexpander.h>
#include <utils/fileutils.h>
#include <utils/osspecificaspects.h>

#include <QFileInfo>
#include <QDir>

using namespace Nim;
using namespace ProjectExplorer;
using namespace Utils;

NimbleBuildConfiguration::NimbleBuildConfiguration(Target *target, Core::Id id)
    : BuildConfiguration(target, id)
{
    setConfigWidgetDisplayName(tr("General"));
    setConfigWidgetHasFrame(true);
    setBuildDirectorySettingsKey("Nim.NimbleBuildConfiguration.BuildDirectory");

    m_nimbleProject = dynamic_cast<NimbleProject*>(project());
    QTC_ASSERT(m_nimbleProject, return);
    QObject::connect(m_nimbleProject, &NimbleProject::metadataChanged, this, &NimbleBuildConfiguration::updateApplicationTargets);
    updateApplicationTargets();
}

BuildConfiguration::BuildType NimbleBuildConfiguration::buildType() const
{
    return m_buildType;
}

void NimbleBuildConfiguration::initialize()
{
    BuildConfiguration::initialize();

    m_buildType = initialBuildType();

    setBuildDirectory(project()->projectDirectory());

    // Don't add a nimble build step when the package has no binaries (i.e a library package)
    if (!m_nimbleProject->metadata().bin.empty())
    {
        BuildStepList *buildSteps = stepList(ProjectExplorer::Constants::BUILDSTEPS_BUILD);
        buildSteps->appendStep(new NimbleBuildStep(buildSteps));
    }
}

void NimbleBuildConfiguration::updateApplicationTargets()
{
    QTC_ASSERT(m_nimbleProject, return);

    const NimbleMetadata &metaData = m_nimbleProject->metadata();
    const FilePath &projectDir = project()->projectDirectory();
    const FilePath binDir = projectDir.pathAppended(metaData.binDir);
    const FilePath srcDir = projectDir.pathAppended("src");

    QList<BuildTargetInfo> targets = Utils::transform(metaData.bin, [&](const QString &bin){
        BuildTargetInfo info = {};
        info.displayName = bin;
        info.targetFilePath = binDir.pathAppended(HostOsInfo::withExecutableSuffix(bin));
        info.projectFilePath = srcDir.pathAppended(bin).stringAppended(".nim");
        info.workingDirectory = binDir;
        info.buildKey = bin;
        return info;
    });

    target()->setApplicationTargets(std::move(targets));
}

bool NimbleBuildConfiguration::fromMap(const QVariantMap &map)
{
    m_buildType = static_cast<BuildType>(map[Constants::C_NIMBLEBUILDCONFIGURATION_BUILDTYPE].toInt());
    return BuildConfiguration::fromMap(map);
}

QVariantMap NimbleBuildConfiguration::toMap() const
{
    auto map = BuildConfiguration::toMap();
    map[Constants::C_NIMBLEBUILDCONFIGURATION_BUILDTYPE] = buildType();
    return map;
}


NimbleBuildConfigurationFactory::NimbleBuildConfigurationFactory()
{
    registerBuildConfiguration<NimbleBuildConfiguration>(Constants::C_NIMBLEBUILDCONFIGURATION_ID);
    setSupportedProjectType(Constants::C_NIMBLEPROJECT_ID);
    setSupportedProjectMimeTypeName(Constants::C_NIMBLE_MIMETYPE);
}

QList<BuildInfo> NimbleBuildConfigurationFactory::availableBuilds(const Kit *k, const Utils::FilePath &projectPath, bool forSetup) const
{
    static const QList<BuildConfiguration::BuildType> configurations = {BuildConfiguration::Debug, BuildConfiguration::Release};
    return Utils::transform(configurations, [&](BuildConfiguration::BuildType buildType){
        BuildInfo info(this);
        info.buildType = buildType;
        info.kitId = k->id();

        if (buildType == BuildConfiguration::Debug)
            info.typeName = tr("Debug");
        else if (buildType == BuildConfiguration::Release)
            info.typeName = tr("Release");

        if (forSetup) {
            info.displayName = info.typeName;
            info.buildDirectory = projectPath.parentDir();
        }
        return info;
    });
}
