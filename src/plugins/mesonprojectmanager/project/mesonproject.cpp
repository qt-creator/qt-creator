// Copyright (C) 2020 Alexis Jeandet.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "mesonproject.h"

#include "mesonpluginconstants.h"
#include "settings/tools/kitaspect/mesontoolkitaspect.h"
#include "settings/tools/kitaspect/ninjatoolkitaspect.h"

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

} // namespace Internal
} // namespace MesonProjectManager
