/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "qbsprojectmanagerplugin.h"

#include "qbsbuildconfiguration.h"
#include "qbsbuildstep.h"
#include "qbscleanstep.h"
#include "qbsproject.h"
#include "qbsprojectmanager.h"
#include "qbsprojectmanagerconstants.h"

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/mimedatabase.h>
#include <projectexplorer/buildmanager.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>
#include <utils/qtcassert.h>

#include <QAction>
#include <QtPlugin>

namespace QbsProjectManager {
namespace Internal {

QbsProjectManagerPlugin::QbsProjectManagerPlugin() :
    m_manager(0),
    m_projectExplorer(0),
    m_currentProject(0),
    m_currentTarget(0),
    m_currentNode(0)
{ }

bool QbsProjectManagerPlugin::initialize(const QStringList &arguments, QString *errorMessage)
{
    m_manager = new QbsManager(this);
    m_projectExplorer = ProjectExplorer::ProjectExplorerPlugin::instance();
    const Core::Context projectContext(::QbsProjectManager::Constants::PROJECT_ID);

    Q_UNUSED(arguments);

    //create and register objects
    addAutoReleasedObject(m_manager);
    addAutoReleasedObject(new QbsBuildConfigurationFactory);
    addAutoReleasedObject(new QbsBuildStepFactory);
    addAutoReleasedObject(new QbsCleanStepFactory);

    //menus
    // Build Menu:
    Core::ActionContainer *mbuild =
            Core::ActionManager::actionContainer(ProjectExplorer::Constants::M_BUILDPROJECT);
    // PE Context menu for projects
    Core::ActionContainer *mproject =
            Core::ActionManager::actionContainer(ProjectExplorer::Constants::M_PROJECTCONTEXT);
    // <debug>
    // Core::ActionContainer *msubproject =
    //         Core::ActionManager::actionContainer(ProjectExplorer::Constants::M_SUBPROJECTCONTEXT);
    // </debug>
    Core::ActionContainer *mfile =
            Core::ActionManager::actionContainer(ProjectExplorer::Constants::M_FILECONTEXT);


    //register actions
    Core::Command *command;

    m_reparseQbs = new QAction(tr("Reparse Qbs"), this);
    command = Core::ActionManager::registerAction(m_reparseQbs, Constants::ACTION_REPARSE_QBS, projectContext);
    command->setAttribute(Core::Command::CA_Hide);
    mbuild->addAction(command, ProjectExplorer::Constants::G_BUILD_BUILD);
    connect(m_reparseQbs, SIGNAL(triggered()), this, SLOT(reparseCurrentProject()));

    m_reparseQbsCtx = new QAction(tr("Reparse Qbs"), this);
    command = Core::ActionManager::registerAction(m_reparseQbsCtx, Constants::ACTION_REPARSE_QBS_CONTEXT, projectContext);
    command->setAttribute(Core::Command::CA_Hide);
    mproject->addAction(command, ProjectExplorer::Constants::G_PROJECT_BUILD);
    connect(m_reparseQbsCtx, SIGNAL(triggered()), this, SLOT(reparseCurrentProject()));

    m_buildFileContextMenu = new QAction(tr("Build"), this);
    command = Core::ActionManager::registerAction(m_buildFileContextMenu, Constants::ACTION_BUILD_FILE_QBS_CONTEXT, projectContext);
    command->setAttribute(Core::Command::CA_Hide);
    mfile->addAction(command, ProjectExplorer::Constants::G_FILE_OTHER);
    connect(m_buildFileContextMenu, SIGNAL(triggered()), this, SLOT(buildFileContextMenu()));

    // Connect
    connect(m_projectExplorer, SIGNAL(currentNodeChanged(ProjectExplorer::Node*,ProjectExplorer::Project*)),
            this, SLOT(updateContextActions(ProjectExplorer::Node*,ProjectExplorer::Project*)));

    connect(m_projectExplorer->buildManager(), SIGNAL(buildStateChanged(ProjectExplorer::Project*)),
            this, SLOT(buildStateChanged(ProjectExplorer::Project*)));

    // Run initial setup routines
    updateContextActions(0, 0);
    updateReparseQbsAction();

    return true;
}

void QbsProjectManagerPlugin::extensionsInitialized()
{ }

void QbsProjectManagerPlugin::updateContextActions(ProjectExplorer::Node *node, ProjectExplorer::Project *project)
{
    if (m_currentProject) {
        disconnect(m_currentProject, SIGNAL(activeTargetChanged(ProjectExplorer::Target*)),
                   this, SLOT(activeTargetChanged()));
        disconnect(m_currentProject, SIGNAL(projectParsingStarted()),
                   this, SLOT(parsingStateChanged()));
        disconnect(m_currentProject, SIGNAL(projectParsingDone(bool)),
                   this, SLOT(parsingStateChanged()));
    }

    m_currentNode = node;
    m_currentProject = qobject_cast<Internal::QbsProject *>(project);
    if (m_currentProject) {
        connect(m_currentProject, SIGNAL(activeTargetChanged(ProjectExplorer::Target*)),
                this, SLOT(activeTargetChanged()));
        connect(m_currentProject, SIGNAL(projectParsingStarted()),
                this, SLOT(parsingStateChanged()));
        connect(m_currentProject, SIGNAL(projectParsingDone(bool)),
                this, SLOT(parsingStateChanged()));
    }

    activeTargetChanged();

    bool isBuilding = m_projectExplorer->buildManager()->isBuilding(project);
    bool isFile = m_currentProject && node && (node->nodeType() == ProjectExplorer::FileNodeType);
    bool isFileEnabled = isFile && node->isEnabled();

    m_reparseQbsCtx->setEnabled(!isBuilding && m_currentProject && !m_currentProject->isParsing());
    m_buildFileContextMenu->setEnabled(isFileEnabled);
}

void QbsProjectManagerPlugin::updateReparseQbsAction()
{
    m_reparseQbs->setEnabled(m_currentProject
                             && !m_projectExplorer->buildManager()->isBuilding(m_currentProject)
                             && !m_currentProject->isParsing());
}

void QbsProjectManagerPlugin::activeTargetChanged()
{
    if (m_currentTarget)
        disconnect(m_currentTarget, SIGNAL(activeBuildConfigurationChanged(ProjectExplorer::BuildConfiguration*)),
                   this, SLOT(updateReparseQbsAction()));

    m_currentTarget = m_currentProject ? m_currentProject->activeTarget() : 0;

    if (m_currentTarget)
        connect(m_currentTarget, SIGNAL(activeBuildConfigurationChanged(ProjectExplorer::BuildConfiguration*)),
                this, SLOT(updateReparseQbsAction()));

    updateReparseQbsAction();
}

void QbsProjectManagerPlugin::buildStateChanged(ProjectExplorer::Project *project)
{
    if (project == m_currentProject) {
        updateReparseQbsAction();
        updateContextActions(m_currentNode, m_currentProject);
    }
}

void QbsProjectManagerPlugin::parsingStateChanged()
{
    if (m_currentProject) {
        updateReparseQbsAction();
        updateContextActions(m_currentNode, m_currentProject);
    }
}

void QbsProjectManagerPlugin::buildFileContextMenu()
{
    // <debug>
    qDebug() << "Build file...";
    // </debug>
    QTC_ASSERT(m_currentNode, return);
    QTC_ASSERT(m_currentProject, return);

    ProjectExplorer::Target *t = m_currentProject->activeTarget();
    if (!t)
        return;
    QbsBuildConfiguration *bc = qobject_cast<QbsBuildConfiguration *>(t->activeBuildConfiguration());
    if (!bc)
        return;

    ProjectExplorer::ProjectExplorerPlugin *pe = ProjectExplorer::ProjectExplorerPlugin::instance();

    if (!pe->saveModifiedFiles())
        return;

    bc->setChangedFiles(QStringList(m_currentNode->path()));

    const Core::Id buildStep = Core::Id(ProjectExplorer::Constants::BUILDSTEPS_BUILD);

    const QString name = ProjectExplorer::ProjectExplorerPlugin::displayNameForStepId(buildStep);
    pe->buildManager()->buildList(bc->stepList(buildStep), name);

    bc->setChangedFiles(QStringList());
}

void QbsProjectManagerPlugin::reparseCurrentProject()
{
    if (m_currentProject)
        m_currentProject->parseCurrentBuildConfiguration();
}

} // namespace Internal
} // namespace QbsProjectManager

Q_EXPORT_PLUGIN(QbsProjectManager::Internal::QbsProjectManagerPlugin)
