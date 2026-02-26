// Copyright (C) 2024 The Qt Company Ltd.
// Copyright (C) 2026 BogDan Vatra <bogdan@kde.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "gnproject.h"

#include "gnbuildsystem.h"
#include "gnkitaspect.h"
#include "gnpluginconstants.h"
#include "gnprojectmanagertr.h"

#include <coreplugin/icontext.h>

#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/toolchainkitaspect.h>

using namespace ProjectExplorer;

namespace GNProjectManager::Internal {

GNProject::GNProject(const Utils::FilePath &path)
    : Project{Constants::Project::MIMETYPE, path}
{
    setType(Constants::GN_PROJECT_ID);
    setProjectLanguages(Core::Context(ProjectExplorer::Constants::CXX_LANGUAGE_ID));
    setDisplayName(projectDirectory().fileName());
    setCanBuildProducts();
    setBuildSystemCreator<GNBuildSystem>();
}

ProjectExplorer::Tasks GNProject::projectIssues(const ProjectExplorer::Kit *k) const
{
    auto res = Project::projectIssues(k);
    res.append(m_issues);
    return res;
}

void GNProject::addIssue(IssueType type, const QString &text)
{
    m_issues.append(createTask(type, text));
}

void GNProject::clearIssues()
{
    m_issues.clear();
}

void setupGNProject()
{
    const auto issuesGenerator = [](const Kit *k) {
        Tasks result;
        if (!GNKitAspect::isValid(k))
            result.append(
                Project::createTask(Task::TaskType::Warning, Tr::tr("No GN tool set in kit.")));
        if (ToolchainKitAspect::toolChains(k).isEmpty())
            result.append(
                Project::createTask(Task::TaskType::Warning, Tr::tr("No compilers set in kit.")));
        return result;
    };
    ProjectManager::registerProjectType<GNProject>(Constants::Project::MIMETYPE, issuesGenerator);
}

} // namespace GNProjectManager::Internal
