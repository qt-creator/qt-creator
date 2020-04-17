/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "cmakeproject.h"

#include "cmakebuildstep.h"
#include "cmakekitinformation.h"
#include "cmakeprojectconstants.h"
#include "cmakeprojectimporter.h"
#include "cmakeprojectnodes.h"
#include "cmaketool.h"

#include <coreplugin/icontext.h>
#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>

using namespace ProjectExplorer;
using namespace Utils;

namespace CMakeProjectManager {

using namespace Internal;

// QtCreator CMake Generator wishlist:
// Which make targets we need to build to get all executables
// What is the actual compiler executable
// DEFINES

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
    setKnowsAllBuildExecutables(false);
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

    return result;
}


ProjectImporter *CMakeProject::projectImporter() const
{
    if (!m_projectImporter)
        m_projectImporter = new CMakeProjectImporter(projectFilePath());
    return m_projectImporter;
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

MakeInstallCommand CMakeProject::makeInstallCommand(const Target *target,
                                                    const QString &installRoot)
{
    MakeInstallCommand cmd;
    if (const BuildConfiguration * const bc = target->activeBuildConfiguration()) {
        if (const auto cmakeStep = bc->buildSteps()->firstOfType<CMakeBuildStep>()) {
            if (CMakeTool *tool = CMakeKitAspect::cmakeTool(target->kit()))
                cmd.command = tool->cmakeExecutable();
        }
    }
    cmd.arguments << "--build" << "." << "--target" << "install";
    cmd.environment.set("DESTDIR", QDir::toNativeSeparators(installRoot));
    return cmd;
}

} // namespace CMakeProjectManager
