// Copyright (C) 2020 Alexis Jeandet.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "mesonproject.h"

#include "mesonpluginconstants.h"
#include "mesonprojectimporter.h"
#include "mesonprojectmanagertr.h"
#include "toolkitaspectwidget.h"

#include <coreplugin/icontext.h>

#include <projectexplorer/kitaspects.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectmanager.h>

using namespace ProjectExplorer;

namespace MesonProjectManager::Internal {

class MesonProject final : public Project
{
public:
    explicit MesonProject(const Utils::FilePath &path)
        : Project{Constants::Project::MIMETYPE, path}
    {
        setId(Constants::Project::ID);
        setProjectLanguages(Core::Context(ProjectExplorer::Constants::CXX_LANGUAGE_ID));
        setDisplayName(projectDirectory().fileName());
        setCanBuildProducts();
        setHasMakeInstallEquivalent(true);
    }

    Tasks projectIssues(const Kit *k) const final
    {
        Tasks result = Project::projectIssues(k);

        if (!MesonToolKitAspect::isValid(k))
            result.append(
                createProjectTask(Task::TaskType::Error, Tr::tr("No Meson tool set.")));
        if (!NinjaToolKitAspect::isValid(k))
            result.append(
                createProjectTask(Task::TaskType::Error, Tr::tr("No Ninja tool set.")));
        if (ToolchainKitAspect::toolChains(k).isEmpty())
            result.append(createProjectTask(Task::TaskType::Warning,
                                            Tr::tr("No compilers set in kit.")));
        return result;
    }

    ProjectImporter *projectImporter() const final
    {
        if (m_projectImporter)
            m_projectImporter = std::make_unique<MesonProjectImporter>(projectFilePath());
        return m_projectImporter.get();
    }

private:
    DeploymentKnowledge deploymentKnowledge() const final
    {
        // TODO in next releases
        return DeploymentKnowledge::Bad;
    }

    mutable std::unique_ptr<MesonProjectImporter> m_projectImporter;
};

void setupMesonProject()
{
    ProjectManager::registerProjectType<MesonProject>(Constants::Project::MIMETYPE);
}

} // namespace MesonProjectManager::Internal
