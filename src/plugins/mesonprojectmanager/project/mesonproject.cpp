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

#include "mesonproject.h"
#include "mesonbuildsystem.h"
#include "mesonpluginconstants.h"
#include <exewrappers/mesontools.h>
#include <settings/tools/kitaspect/mesontoolkitaspect.h>
#include <settings/tools/kitaspect/ninjatoolkitaspect.h>
#include <coreplugin/icontext.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>

namespace MesonProjectManager {
namespace Internal {
MesonProject::MesonProject(const Utils::FilePath &path)
    : ProjectExplorer::Project{Constants::Project::MIMETYPE, path}
{
    setId(Constants::Project::ID);
    setProjectLanguages(Core::Context(ProjectExplorer::Constants::CXX_LANGUAGE_ID));
    setDisplayName(projectDirectory().fileName());
    setCanBuildProducts();
    setKnowsAllBuildExecutables(true);
    setHasMakeInstallEquivalent(true);
}

ProjectExplorer::Tasks MesonProject::projectIssues(const ProjectExplorer::Kit *k) const
{
    ProjectExplorer::Tasks result = Project::projectIssues(k);

    if (!MesonToolKitAspect::isValid(k))
        result.append(
            createProjectTask(ProjectExplorer::Task::TaskType::Error, tr("No Meson tool set.")));
    if (!NinjaToolKitAspect::isValid(k))
        result.append(
            createProjectTask(ProjectExplorer::Task::TaskType::Error, tr("No Ninja tool set.")));
    if (ProjectExplorer::ToolChainKitAspect::toolChains(k).isEmpty())
        result.append(createProjectTask(ProjectExplorer::Task::TaskType::Warning,
                                        tr("No compilers set in kit.")));
    return result;
}

ProjectExplorer::ProjectImporter *MesonProject::projectImporter() const
{
    if (m_projectImporter)
        m_projectImporter = std::make_unique<MesonProjectImporter>(projectFilePath());
    return m_projectImporter.get();
}

ProjectExplorer::DeploymentKnowledge MesonProject::deploymentKnowledge() const
{
    // TODO in next releases
    return ProjectExplorer::DeploymentKnowledge::Bad;
}

ProjectExplorer::MakeInstallCommand MesonProject::makeInstallCommand(
    const ProjectExplorer::Target *target, const QString &installRoot)
{
    Q_UNUSED(target)
    Q_UNUSED(installRoot)
    // TODO in next releases
    return {};
}

} // namespace Internal
} // namespace MesonProjectManager
