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

Project *CMakeManager::openProject(const QString &fileName, QString *errorString)
{
    Utils::FileName file = Utils::FileName::fromString(fileName);
    if (!file.toFileInfo().isFile()) {
        if (errorString)
            *errorString = tr("Failed opening project \"%1\": Project is not a file")
                .arg(file.toUserOutput());
        return 0;
    }

    return new CMakeProject(this, file);
}

QString CMakeManager::mimeType() const
{
    return QLatin1String(Constants::CMAKEPROJECTMIMETYPE);
}

// need to refactor this out
// we probably want the process instead of this function
// cmakeproject then could even run the cmake process in the background, adding the files afterwards
// sounds like a plan
void CMakeManager::createXmlFile(Utils::QtcProcess *proc, const QString &executable,
                                 const QString &arguments, const QString &sourceDirectory,
                                 const QDir &buildDirectory, const Utils::Environment &env)
{
    QString buildDirectoryPath = buildDirectory.absolutePath();
    buildDirectory.mkpath(buildDirectoryPath);
    proc->setWorkingDirectory(buildDirectoryPath);
    proc->setEnvironment(env);

    const QString srcdir = buildDirectory.exists(QLatin1String("CMakeCache.txt")) ?
                QString(QLatin1Char('.')) : sourceDirectory;
    QString args;
    Utils::QtcProcess::addArg(&args, srcdir);
    Utils::QtcProcess::addArgs(&args, arguments);

    proc->setCommand(executable, args);
    proc->start();
}

QString CMakeManager::findCbpFile(const QDir &directory)
{
    // Find the cbp file
    //   the cbp file is named like the project() command in the CMakeList.txt file
    //   so this function below could find the wrong cbp file, if the user changes the project()
    //   2name
    QDateTime t;
    QString file;
    foreach (const QString &cbpFile , directory.entryList()) {
        if (cbpFile.endsWith(QLatin1String(".cbp"))) {
            QFileInfo fi(directory.path() + QLatin1Char('/') + cbpFile);
            if (t.isNull() || fi.lastModified() > t) {
                file = directory.path() + QLatin1Char('/') + cbpFile;
                t = fi.lastModified();
            }
        }
    }
    return file;
}
