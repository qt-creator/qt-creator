// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "cmakeproject.h"

#include "cmakekitinformation.h"
#include "cmakeprojectconstants.h"
#include "cmakeprojectimporter.h"
#include "cmaketool.h"

#include <coreplugin/icontext.h>
#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectnodes.h>
#include <projectexplorer/target.h>

using namespace ProjectExplorer;
using namespace Utils;

namespace CMakeProjectManager {

using namespace Internal;

/*!
  \class CMakeProject
*/
CMakeProject::CMakeProject(const FilePath &fileName)
    : Project(Constants::CMAKE_MIMETYPE, fileName)
{
    setId(CMakeProjectManager::Constants::CMAKE_PROJECT_ID);
    setProjectLanguages(Core::Context(ProjectExplorer::Constants::CXX_LANGUAGE_ID));
    setDisplayName(projectDirectory().fileName());
    setCanBuildProducts();
    setHasMakeInstallEquivalent(true);
}

CMakeProject::~CMakeProject()
{
    delete m_projectImporter;
}

Tasks CMakeProject::projectIssues(const Kit *k) const
{
    Tasks result = Project::projectIssues(k);

    if (!CMakeKitAspect::cmakeTool(k))
        result.append(createProjectTask(Task::TaskType::Error, tr("No cmake tool set.")));
    if (ToolChainKitAspect::toolChains(k).isEmpty())
        result.append(createProjectTask(Task::TaskType::Warning, tr("No compilers set in kit.")));

    result.append(m_issues);

    return result;
}


ProjectImporter *CMakeProject::projectImporter() const
{
    if (!m_projectImporter)
        m_projectImporter = new CMakeProjectImporter(projectFilePath());
    return m_projectImporter;
}

void CMakeProject::addIssue(IssueType type, const QString &text)
{
    m_issues.append(createProjectTask(type, text));
}

void CMakeProject::clearIssues()
{
    m_issues.clear();
}

bool CMakeProject::setupTarget(Target *t)
{
    t->updateDefaultBuildConfigurations();
    if (t->buildConfigurations().isEmpty())
        return false;
    t->updateDefaultDeployConfigurations();
    return true;
}

ProjectExplorer::DeploymentKnowledge CMakeProject::deploymentKnowledge() const
{
    return !files([](const ProjectExplorer::Node *n) {
                return n->filePath().fileName() == "QtCreatorDeployment.txt";
            })
                   .isEmpty()
               ? DeploymentKnowledge::Approximative
               : DeploymentKnowledge::Bad;
}

} // namespace CMakeProjectManager
