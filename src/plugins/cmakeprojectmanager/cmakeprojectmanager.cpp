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

#include "cmakeprojectmanager.h"
#include "builddirmanager.h"
#include "cmakebuildconfiguration.h"
#include "cmakekitinformation.h"
#include "cmakeprojectconstants.h"
#include "cmakeproject.h"
#include "cmakesettingspage.h"
#include "cmaketoolmanager.h"

#include <coreplugin/icore.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <projectexplorer/buildmanager.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projecttree.h>
#include <projectexplorer/session.h>
#include <projectexplorer/target.h>

#include <utils/qtcprocess.h>
#include <utils/synchronousprocess.h>

#include <QAction>
#include <QDateTime>
#include <QIcon>

using namespace ProjectExplorer;
using namespace CMakeProjectManager::Internal;

CMakeManager::CMakeManager() :
    m_runCMakeAction(new QAction(QIcon(), tr("Run CMake"), this)),
    m_clearCMakeCacheAction(new QAction(QIcon(), tr("Clear CMake Configuration"), this)),
    m_runCMakeActionContextMenu(new QAction(QIcon(), tr("Run CMake"), this)),
    m_rescanProjectAction(new QAction(QIcon(), tr("Rescan project"), this))
{
    Core::ActionContainer *mbuild =
            Core::ActionManager::actionContainer(ProjectExplorer::Constants::M_BUILDPROJECT);
    Core::ActionContainer *mproject =
            Core::ActionManager::actionContainer(ProjectExplorer::Constants::M_PROJECTCONTEXT);
    Core::ActionContainer *msubproject =
            Core::ActionManager::actionContainer(ProjectExplorer::Constants::M_SUBPROJECTCONTEXT);

    const Core::Context projectContext(CMakeProjectManager::Constants::PROJECTCONTEXT);
    const Core::Context globalContext(Core::Constants::C_GLOBAL);

    Core::Command *command = Core::ActionManager::registerAction(m_runCMakeAction,
                                                                 Constants::RUNCMAKE, globalContext);
    command->setAttribute(Core::Command::CA_Hide);
    mbuild->addAction(command, ProjectExplorer::Constants::G_BUILD_DEPLOY);
    connect(m_runCMakeAction, &QAction::triggered, [this]() {
        runCMake(SessionManager::startupProject());
    });

    command = Core::ActionManager::registerAction(m_clearCMakeCacheAction,
                                                  Constants::CLEARCMAKECACHE, globalContext);
    command->setAttribute(Core::Command::CA_Hide);
    mbuild->addAction(command, ProjectExplorer::Constants::G_BUILD_DEPLOY);
    connect(m_clearCMakeCacheAction, &QAction::triggered, [this]() {
        clearCMakeCache(SessionManager::startupProject());
    });

    command = Core::ActionManager::registerAction(m_runCMakeActionContextMenu,
                                                  Constants::RUNCMAKECONTEXTMENU, projectContext);
    command->setAttribute(Core::Command::CA_Hide);
    mproject->addAction(command, ProjectExplorer::Constants::G_PROJECT_BUILD);
    msubproject->addAction(command, ProjectExplorer::Constants::G_PROJECT_BUILD);
    connect(m_runCMakeActionContextMenu, &QAction::triggered, [this]() {
        runCMake(ProjectTree::currentProject());
    });

    command = Core::ActionManager::registerAction(m_rescanProjectAction,
                                                  Constants::RESCANPROJECT, globalContext);
    command->setAttribute(Core::Command::CA_Hide);
    mbuild->addAction(command, ProjectExplorer::Constants::G_BUILD_DEPLOY);
    connect(m_rescanProjectAction, &QAction::triggered, [this]() {
        rescanProject(ProjectTree::currentProject());
    });

    connect(SessionManager::instance(), &SessionManager::startupProjectChanged,
            this, &CMakeManager::updateCmakeActions);
    connect(BuildManager::instance(), &BuildManager::buildStateChanged,
            this, &CMakeManager::updateCmakeActions);

    updateCmakeActions();
}

void CMakeManager::updateCmakeActions()
{
    auto project = qobject_cast<CMakeProject *>(SessionManager::startupProject());
    const bool visible = project && !BuildManager::isBuilding(project);
    m_runCMakeAction->setVisible(visible);
    m_clearCMakeCacheAction->setVisible(visible);
    m_rescanProjectAction->setVisible(visible);
}

void CMakeManager::clearCMakeCache(Project *project)
{
    if (!project || !project->activeTarget())
        return;
    auto bc = qobject_cast<CMakeBuildConfiguration *>(project->activeTarget()->activeBuildConfiguration());
    if (!bc)
        return;

    bc->clearCache();
}

void CMakeManager::runCMake(Project *project)
{
    if (!project)
        return;
    CMakeProject *cmakeProject = qobject_cast<CMakeProject *>(project);
    if (!cmakeProject || !cmakeProject->activeTarget() || !cmakeProject->activeTarget()->activeBuildConfiguration())
        return;

    if (!ProjectExplorerPlugin::saveModifiedFiles())
        return;

    cmakeProject->runCMake();
}

void CMakeManager::rescanProject(Project *project)
{
    if (!project)
        return;
    CMakeProject *cmakeProject = qobject_cast<CMakeProject *>(project);
    if (!cmakeProject || !cmakeProject->activeTarget() || !cmakeProject->activeTarget()->activeBuildConfiguration())
        return;

    cmakeProject->scanProjectTree();
    cmakeProject->runCMake(); // by my experience: every rescan run requires cmake run too
}
