// Copyright (C) 2020 Alexis Jeandet.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "mesonproject.h"

#include "mesonpluginconstants.h"
#include "mesonprojectmanagertr.h"
#include "mesontoolkitaspect.h"
#include "ninjatoolkitaspect.h"

#include <coreplugin/icontext.h>

#include <projectexplorer/kitaspects.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>

using namespace ProjectExplorer;

namespace MesonProjectManager {
namespace Internal {

MesonProject::MesonProject(const Utils::FilePath &path)
    : Project{Constants::Project::MIMETYPE, path}
{
    setId(Constants::Project::ID);
    setProjectLanguages(Core::Context(ProjectExplorer::Constants::CXX_LANGUAGE_ID));
    setDisplayName(projectDirectory().fileName());
    setCanBuildProducts();
    setHasMakeInstallEquivalent(true);
}

Tasks MesonProject::projectIssues(const Kit *k) const
{
    Tasks result = Project::projectIssues(k);

    if (!MesonToolKitAspect::isValid(k))
        result.append(
            createProjectTask(Task::TaskType::Error, Tr::tr("No Meson tool set.")));
    if (!NinjaToolKitAspect::isValid(k))
        result.append(
            createProjectTask(Task::TaskType::Error, Tr::tr("No Ninja tool set.")));
    if (ToolChainKitAspect::toolChains(k).isEmpty())
        result.append(createProjectTask(Task::TaskType::Warning,
                                        Tr::tr("No compilers set in kit.")));
    return result;
}

ProjectImporter *MesonProject::projectImporter() const
{
    if (m_projectImporter)
        m_projectImporter = std::make_unique<MesonProjectImporter>(projectFilePath());
    return m_projectImporter.get();
}

DeploymentKnowledge MesonProject::deploymentKnowledge() const
{
    // TODO in next releases
    return DeploymentKnowledge::Bad;
}

} // namespace Internal
} // namespace MesonProjectManager
