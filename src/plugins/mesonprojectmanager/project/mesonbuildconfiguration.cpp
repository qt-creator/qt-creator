/****************************************************************************
**
** Copyright (C) 2020 Alexis Jeandet.
** Contact: https://www.qt.io/licensing/
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

#include <projectexplorer/buildinfo.h>
#include <projectexplorer/buildmanager.h>
#include <projectexplorer/buildstep.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/kit.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectmacroexpander.h>
#include <utils/fileutils.h>

#include <QDir>

#include "buildoptions/mesonbuildsettingswidget.h"
#include "mesonbuildconfiguration.h"
#include "mesonbuildsystem.h"
#include "mesonpluginconstants.h"
#include "ninjabuildstep.h"
#include <exewrappers/mesonwrapper.h>
#include <mesonpluginconstants.h>

namespace MesonProjectManager {
namespace Internal {
MesonBuildConfiguration::MesonBuildConfiguration(ProjectExplorer::Target *target, Utils::Id id)
    : ProjectExplorer::BuildConfiguration{target, id}
{
    appendInitialBuildStep(Constants::MESON_BUILD_STEP_ID);
    appendInitialCleanStep(Constants::MESON_BUILD_STEP_ID);
    setInitializer([this, target](const ProjectExplorer::BuildInfo &info) {
        m_buildType = mesonBuildType(info.typeName);
        auto k = target->kit();
        if (info.buildDirectory.isEmpty()) {
            setBuildDirectory(shadowBuildDirectory(target->project()->projectFilePath(),
                                                   k,
                                                   info.displayName,
                                                   info.buildType));
        }
        m_buildSystem = new MesonBuildSystem{this};
    });
}

MesonBuildConfiguration::~MesonBuildConfiguration()
{
    delete m_buildSystem;
}

Utils::FilePath MesonBuildConfiguration::shadowBuildDirectory(
    const Utils::FilePath &projectFilePath,
    const ProjectExplorer::Kit *k,
    const QString &bcName,
    ProjectExplorer::BuildConfiguration::BuildType buildType)
{
    if (projectFilePath.isEmpty())
        return Utils::FilePath();

    const QString projectName = projectFilePath.parentDir().fileName();
    ProjectExplorer::ProjectMacroExpander expander(projectFilePath,
                                                   projectName,
                                                   k,
                                                   bcName,
                                                   buildType);
    QDir projectDir = QDir(ProjectExplorer::Project::projectDirectory(projectFilePath).toString());
    QString buildPath = expander.expand(
        ProjectExplorer::ProjectExplorerPlugin::buildDirectoryTemplate());
    buildPath.replace(" ", "-");
    return Utils::FilePath::fromUserInput(projectDir.absoluteFilePath(buildPath));
}

ProjectExplorer::BuildSystem *MesonBuildConfiguration::buildSystem() const
{
    return m_buildSystem;
}

void MesonBuildConfiguration::build(const QString &target)
{
    auto mesonBuildStep = qobject_cast<NinjaBuildStep *>(
        Utils::findOrDefault(buildSteps()->steps(), [](const ProjectExplorer::BuildStep *bs) {
            return bs->id() == Constants::MESON_BUILD_STEP_ID;
        }));

    QString originalBuildTarget;
    if (mesonBuildStep) {
        originalBuildTarget = mesonBuildStep->targetName();
        mesonBuildStep->setBuildTarget(target);
    }

    ProjectExplorer::BuildManager::buildList(buildSteps());

    if (mesonBuildStep)
        mesonBuildStep->setBuildTarget(originalBuildTarget);
}

QVariantMap MesonBuildConfiguration::toMap() const
{
    auto data = ProjectExplorer::BuildConfiguration::toMap();
    data[Constants::BuildConfiguration::BUILD_TYPE_KEY] = mesonBuildTypeName(m_buildType);
    return data;
}

bool MesonBuildConfiguration::fromMap(const QVariantMap &map)
{
    auto res = ProjectExplorer::BuildConfiguration::fromMap(map);
    m_buildSystem = new MesonBuildSystem{this};
    m_buildType = mesonBuildType(
        map.value(Constants::BuildConfiguration::BUILD_TYPE_KEY).toString());
    return res;
}

ProjectExplorer::NamedWidget *MesonBuildConfiguration::createConfigWidget()
{
    return new MesonBuildSettingsWidget{this};
}

ProjectExplorer::BuildInfo createBuildInfo(MesonBuildType type)
{
    ProjectExplorer::BuildInfo bInfo;
    bInfo.typeName = mesonBuildTypeName(type);
    bInfo.displayName = mesonBuildTypeDisplayName(type);
    bInfo.buildType = buildType(type);
    return bInfo;
}

MesonBuildConfigurationFactory::MesonBuildConfigurationFactory()
{
    registerBuildConfiguration<MesonBuildConfiguration>(Constants::MESON_BUILD_CONFIG_ID);
    setSupportedProjectType(Constants::Project::ID);
    setSupportedProjectMimeTypeName(Constants::Project::MIMETYPE);
    setBuildGenerator(
        [](const ProjectExplorer::Kit *k, const Utils::FilePath &projectPath, bool forSetup) {
            QList<ProjectExplorer::BuildInfo> result;

            Utils::FilePath path = forSetup
                                       ? ProjectExplorer::Project::projectDirectory(projectPath)
                                       : projectPath;
            for (const auto &bType : {MesonBuildType::debug,
                                      MesonBuildType::release,
                                      MesonBuildType::debugoptimized,
                                      MesonBuildType::minsize}) {
                auto bInfo = createBuildInfo(bType);
                if (forSetup)
                    bInfo.buildDirectory
                        = MesonBuildConfiguration::shadowBuildDirectory(projectPath,
                                                                        k,
                                                                        bInfo.typeName,
                                                                        bInfo.buildType);
                result << bInfo;
            }
            return result;
        });
}

} // namespace Internal
} // namespace MesonProjectManager
