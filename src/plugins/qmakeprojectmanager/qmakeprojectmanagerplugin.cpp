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

#include "qmakeprojectmanagerplugin.h"

#include "profileeditor.h"
#include "qmakeprojectmanager.h"
#include "qmakenodes.h"
#include "qmakestep.h"
#include "makestep.h"
#include "qmakebuildconfiguration.h"
#include "desktopqmakerunconfiguration.h"
#include "wizards/guiappwizard.h"
#include "wizards/librarywizard.h"
#include "wizards/testwizard.h"
#include "wizards/simpleprojectwizard.h"
#include "wizards/subdirsprojectwizard.h"
#include "customwidgetwizard/customwidgetwizard.h"
#include "qmakeprojectmanagerconstants.h"
#include "qmakeproject.h"
#include "externaleditors.h"
#include "qmakekitinformation.h"
#include "profilehighlighter.h"

#include <coreplugin/icore.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>

#include <projectexplorer/buildmanager.h>
#include <projectexplorer/session.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/projecttree.h>
#include <projectexplorer/target.h>

#include <texteditor/texteditoractionhandler.h>
#include <texteditor/texteditorconstants.h>

#include <utils/hostosinfo.h>
#include <utils/parameteraction.h>

#ifdef WITH_TESTS
#    include <QTest>
#endif

#include <QtPlugin>

using namespace Core;
using namespace QmakeProjectManager::Internal;
using namespace QmakeProjectManager;
using namespace ProjectExplorer;

bool QmakeProjectManagerPlugin::initialize(const QStringList &arguments, QString *errorMessage)
{
    Q_UNUSED(arguments)
    Q_UNUSED(errorMessage)
    const Context projectContext(QmakeProjectManager::Constants::PROJECT_ID);
    Context projecTreeContext(ProjectExplorer::Constants::C_PROJECT_TREE);

    //create and register objects
    m_qmakeProjectManager = new QmakeManager;
    addAutoReleasedObject(m_qmakeProjectManager);

    ProjectManager::registerProjectType<QmakeProject>(QmakeProjectManager::Constants::PROFILE_MIMETYPE);

    ProjectExplorer::KitManager::registerKitInformation(new QmakeKitInformation);

    IWizardFactory::registerFactoryCreator([] {
        return QList<IWizardFactory *> {
            new SubdirsProjectWizard,
            new GuiAppWizard,
            new LibraryWizard,
            new TestWizard,
            new CustomWidgetWizard,
            new SimpleProjectWizard
        };
    });

    addAutoReleasedObject(new CustomWizardMetaFactory<CustomQmakeProjectWizard>
                          (QLatin1String("qmakeproject"), IWizardFactory::ProjectWizard));

    addAutoReleasedObject(new QMakeStepFactory);
    addAutoReleasedObject(new MakeStepFactory);

    addAutoReleasedObject(new QmakeBuildConfigurationFactory);
    addAutoReleasedObject(new DesktopQmakeRunConfigurationFactory);

    addAutoReleasedObject(ExternalQtEditor::createDesignerEditor());
    addAutoReleasedObject(ExternalQtEditor::createLinguistEditor());

    addAutoReleasedObject(new ProFileEditorFactory);

    //menus
    ActionContainer *mbuild =
            ActionManager::actionContainer(ProjectExplorer::Constants::M_BUILDPROJECT);
    ActionContainer *mproject =
            ActionManager::actionContainer(ProjectExplorer::Constants::M_PROJECTCONTEXT);
    ActionContainer *msubproject =
            ActionManager::actionContainer(ProjectExplorer::Constants::M_SUBPROJECTCONTEXT);
    ActionContainer *mfile =
            ActionManager::actionContainer(ProjectExplorer::Constants::M_FILECONTEXT);

    //register actions
    Command *command = nullptr;

    m_buildSubProjectContextMenu = new Utils::ParameterAction(tr("Build"), tr("Build \"%1\""),
                                                              Utils::ParameterAction::AlwaysEnabled/*handled manually*/,
                                                              this);
    command = ActionManager::registerAction(m_buildSubProjectContextMenu, Constants::BUILDSUBDIRCONTEXTMENU, projectContext);
    command->setAttribute(Command::CA_Hide);
    command->setAttribute(Command::CA_UpdateText);
    command->setDescription(m_buildSubProjectContextMenu->text());
    msubproject->addAction(command, ProjectExplorer::Constants::G_PROJECT_BUILD);
    connect(m_buildSubProjectContextMenu, &QAction::triggered, m_qmakeProjectManager, &QmakeManager::buildSubDirContextMenu);

    m_runQMakeActionContextMenu = new QAction(tr("Run qmake"), this);
    command = ActionManager::registerAction(m_runQMakeActionContextMenu, Constants::RUNQMAKECONTEXTMENU, projectContext);
    command->setAttribute(Command::CA_Hide);
    mproject->addAction(command, ProjectExplorer::Constants::G_PROJECT_BUILD);
    msubproject->addAction(command, ProjectExplorer::Constants::G_PROJECT_BUILD);
    connect(m_runQMakeActionContextMenu, &QAction::triggered,
            m_qmakeProjectManager, &QmakeManager::runQMakeContextMenu);

    command = msubproject->addSeparator(projectContext, ProjectExplorer::Constants::G_PROJECT_BUILD,
                                        &m_subProjectRebuildSeparator);
    command->setAttribute(Command::CA_Hide);

    m_rebuildSubProjectContextMenu = new QAction(tr("Rebuild"), this);
    command = ActionManager::registerAction(
                m_rebuildSubProjectContextMenu, Constants::REBUILDSUBDIRCONTEXTMENU, projectContext);
    command->setAttribute(Command::CA_Hide);
    msubproject->addAction(command, ProjectExplorer::Constants::G_PROJECT_BUILD);
    connect(m_rebuildSubProjectContextMenu, &QAction::triggered,
            m_qmakeProjectManager, &QmakeManager::rebuildSubDirContextMenu);

    m_cleanSubProjectContextMenu = new QAction(tr("Clean"), this);
    command = ActionManager::registerAction(
                m_cleanSubProjectContextMenu, Constants::CLEANSUBDIRCONTEXTMENU, projectContext);
    command->setAttribute(Command::CA_Hide);
    msubproject->addAction(command, ProjectExplorer::Constants::G_PROJECT_BUILD);
    connect(m_cleanSubProjectContextMenu, &QAction::triggered,
            m_qmakeProjectManager, &QmakeManager::cleanSubDirContextMenu);

    m_buildFileContextMenu = new QAction(tr("Build"), this);
    command = ActionManager::registerAction(m_buildFileContextMenu, Constants::BUILDFILECONTEXTMENU, projectContext);
    command->setAttribute(Command::CA_Hide);
    mfile->addAction(command, ProjectExplorer::Constants::G_FILE_OTHER);
    connect(m_buildFileContextMenu, &QAction::triggered,
            m_qmakeProjectManager, &QmakeManager::buildFileContextMenu);

    m_buildSubProjectAction = new Utils::ParameterAction(tr("Build Subproject"), tr("Build Subproject \"%1\""),
                                                         Utils::ParameterAction::AlwaysEnabled, this);
    command = ActionManager::registerAction(m_buildSubProjectAction, Constants::BUILDSUBDIR, projectContext);
    command->setAttribute(Command::CA_Hide);
    command->setAttribute(Command::CA_UpdateText);
    command->setDescription(m_buildSubProjectAction->text());
    mbuild->addAction(command, ProjectExplorer::Constants::G_BUILD_BUILD);
    connect(m_buildSubProjectAction, &QAction::triggered,
            m_qmakeProjectManager, &QmakeManager::buildSubDirContextMenu);

    m_runQMakeAction = new QAction(tr("Run qmake"), this);
    const Context globalcontext(Core::Constants::C_GLOBAL);
    command = ActionManager::registerAction(m_runQMakeAction, Constants::RUNQMAKE, globalcontext);
    mbuild->addAction(command, ProjectExplorer::Constants::G_BUILD_BUILD);
    connect(m_runQMakeAction, &QAction::triggered, m_qmakeProjectManager, &QmakeManager::runQMake);

    m_rebuildSubProjectAction = new Utils::ParameterAction(tr("Rebuild Subproject"), tr("Rebuild Subproject \"%1\""),
                                                           Utils::ParameterAction::AlwaysEnabled, this);
    command = ActionManager::registerAction(m_rebuildSubProjectAction, Constants::REBUILDSUBDIR, projectContext);
    command->setAttribute(Command::CA_Hide);
    command->setAttribute(Command::CA_UpdateText);
    command->setDescription(m_rebuildSubProjectAction->text());
    mbuild->addAction(command, ProjectExplorer::Constants::G_BUILD_REBUILD);
    connect(m_rebuildSubProjectAction, &QAction::triggered,
            m_qmakeProjectManager, &QmakeManager::rebuildSubDirContextMenu);

    m_cleanSubProjectAction = new Utils::ParameterAction(tr("Clean Subproject"), tr("Clean Subproject \"%1\""),
                                                         Utils::ParameterAction::AlwaysEnabled, this);
    command = ActionManager::registerAction(m_cleanSubProjectAction, Constants::CLEANSUBDIR, projectContext);
    command->setAttribute(Command::CA_Hide);
    command->setAttribute(Command::CA_UpdateText);
    command->setDescription(m_cleanSubProjectAction->text());
    mbuild->addAction(command, ProjectExplorer::Constants::G_BUILD_CLEAN);
    connect(m_cleanSubProjectAction, &QAction::triggered,
            m_qmakeProjectManager, &QmakeManager::cleanSubDirContextMenu);

    m_buildFileAction = new Utils::ParameterAction(tr("Build File"), tr("Build File \"%1\""),
                                                   Utils::ParameterAction::AlwaysEnabled, this);
    command = ActionManager::registerAction(m_buildFileAction, Constants::BUILDFILE, projectContext);
    command->setAttribute(Command::CA_Hide);
    command->setAttribute(Command::CA_UpdateText);
    command->setDescription(m_buildFileAction->text());
    command->setDefaultKeySequence(QKeySequence(tr("Ctrl+Alt+B")));
    mbuild->addAction(command, ProjectExplorer::Constants::G_BUILD_BUILD);
    connect(m_buildFileAction, &QAction::triggered, m_qmakeProjectManager, &QmakeManager::buildFile);

    connect(BuildManager::instance(), &BuildManager::buildStateChanged,
            this, &QmakeProjectManagerPlugin::buildStateChanged);
    connect(SessionManager::instance(), &SessionManager::startupProjectChanged,
            this, &QmakeProjectManagerPlugin::projectChanged);
    connect(ProjectTree::instance(), &ProjectTree::currentProjectChanged,
            this, &QmakeProjectManagerPlugin::projectChanged);

    connect(ProjectTree::instance(), &ProjectTree::currentNodeChanged,
            this, &QmakeProjectManagerPlugin::updateContextActions);

    ActionContainer *contextMenu = ActionManager::createMenu(QmakeProjectManager::Constants::M_CONTEXT);

    Context proFileEditorContext = Context(QmakeProjectManager::Constants::PROFILE_EDITOR_ID);

    command = ActionManager::command(TextEditor::Constants::JUMP_TO_FILE_UNDER_CURSOR);
    contextMenu->addAction(command);

    m_addLibraryAction = new QAction(tr("Add Library..."), this);
    command = ActionManager::registerAction(m_addLibraryAction,
        Constants::ADDLIBRARY, proFileEditorContext);
    connect(m_addLibraryAction, &QAction::triggered, m_qmakeProjectManager, &QmakeManager::addLibrary);
    contextMenu->addAction(command);

    m_addLibraryActionContextMenu = new QAction(tr("Add Library..."), this);
    command = ActionManager::registerAction(m_addLibraryActionContextMenu,
        Constants::ADDLIBRARY, projecTreeContext);
    connect(m_addLibraryActionContextMenu, &QAction::triggered,
            m_qmakeProjectManager, &QmakeManager::addLibraryContextMenu);
    mproject->addAction(command, ProjectExplorer::Constants::G_PROJECT_FILES);
    msubproject->addAction(command, ProjectExplorer::Constants::G_PROJECT_FILES);

    contextMenu->addSeparator(proFileEditorContext);

    command = ActionManager::command(TextEditor::Constants::UN_COMMENT_SELECTION);
    contextMenu->addAction(command);

    connect(EditorManager::instance(), &EditorManager::currentEditorChanged,
            this, &QmakeProjectManagerPlugin::updateBuildFileAction);

    updateRunQMakeAction();

    return true;
}

void QmakeProjectManagerPlugin::extensionsInitialized()
{ }

void QmakeProjectManagerPlugin::projectChanged()
{
    if (m_previousStartupProject)
        disconnect(m_previousStartupProject, &Project::activeTargetChanged,
                   this, &QmakeProjectManagerPlugin::activeTargetChanged);

    if (ProjectTree::currentProject())
        m_previousStartupProject = qobject_cast<QmakeProject *>(ProjectTree::currentProject());
    else
        m_previousStartupProject = qobject_cast<QmakeProject *>(SessionManager::startupProject());

    if (m_previousStartupProject)
        connect(m_previousStartupProject, &Project::activeTargetChanged,
                           this, &QmakeProjectManagerPlugin::activeTargetChanged);

    activeTargetChanged();
}

void QmakeProjectManagerPlugin::activeTargetChanged()
{
    if (m_previousTarget)
        disconnect(m_previousTarget, &Target::activeBuildConfigurationChanged,
                   this, &QmakeProjectManagerPlugin::updateRunQMakeAction);

    m_previousTarget = m_previousStartupProject ? m_previousStartupProject->activeTarget() : 0;

    if (m_previousTarget)
        connect(m_previousTarget, &Target::activeBuildConfigurationChanged,
                this, &QmakeProjectManagerPlugin::updateRunQMakeAction);

    updateRunQMakeAction();
}

void QmakeProjectManagerPlugin::updateRunQMakeAction()
{
    bool enable = true;
    if (BuildManager::isBuilding(m_previousStartupProject))
        enable = false;
    auto pro = qobject_cast<QmakeProject *>(m_previousStartupProject);
    m_runQMakeAction->setVisible(pro);
    if (!pro
            || !pro->activeTarget()
            || !pro->activeTarget()->activeBuildConfiguration())
        enable = false;

    m_runQMakeAction->setEnabled(enable);
}

void QmakeProjectManagerPlugin::updateContextActions()
{
    Node *node = ProjectTree::currentNode();
    Project *project = ProjectTree::currentProject();

    ContainerNode *containerNode = node ? node->asContainerNode() : nullptr;
    ProjectNode *proFileNode = containerNode ? containerNode->rootProjectNode() : dynamic_cast<QmakeProFileNode *>(node);

    m_addLibraryActionContextMenu->setEnabled(proFileNode);
    QmakeProject *qmakeProject = qobject_cast<QmakeProject *>(QmakeManager::contextProject());
    QmakeProFileNode *subProjectNode = nullptr;
    if (node) {
        auto subPriFileNode = dynamic_cast<QmakePriFileNode *>(node);
        if (!subPriFileNode)
            subPriFileNode = dynamic_cast<QmakePriFileNode *>(node->parentProjectNode());
        subProjectNode = subPriFileNode ? subPriFileNode->proFileNode() : nullptr;
    }
    FileNode *fileNode = node ? node->asFileNode() : nullptr;

    bool buildFilePossible = subProjectNode && fileNode && (fileNode->fileType() == FileType::Source);
    bool subProjectActionsVisible = qmakeProject && subProjectNode && (subProjectNode != qmakeProject->rootProjectNode());

    QString subProjectName;
    if (subProjectActionsVisible)
        subProjectName = subProjectNode->displayName();

    m_buildSubProjectAction->setParameter(subProjectName);
    m_rebuildSubProjectAction->setParameter(subProjectName);
    m_cleanSubProjectAction->setParameter(subProjectName);
    m_buildSubProjectContextMenu->setParameter(proFileNode ? proFileNode->displayName() : QString());
    m_buildFileAction->setParameter(buildFilePossible ? fileNode->filePath().fileName() : QString());

    auto buildConfiguration = (qmakeProject && qmakeProject->activeTarget()) ?
                static_cast<QmakeBuildConfiguration *>(qmakeProject->activeTarget()->activeBuildConfiguration()) : nullptr;
    bool isProjectNode = qmakeProject && proFileNode && buildConfiguration;
    bool isBuilding = BuildManager::isBuilding(project);
    bool enabled = subProjectActionsVisible && !isBuilding;

    m_buildSubProjectAction->setVisible(subProjectActionsVisible);
    m_rebuildSubProjectAction->setVisible(subProjectActionsVisible);
    m_cleanSubProjectAction->setVisible(subProjectActionsVisible);
    m_buildSubProjectContextMenu->setVisible(subProjectActionsVisible && isProjectNode);
    m_subProjectRebuildSeparator->setVisible(subProjectActionsVisible && isProjectNode);
    m_rebuildSubProjectContextMenu->setVisible(subProjectActionsVisible && isProjectNode);
    m_cleanSubProjectContextMenu->setVisible(subProjectActionsVisible && isProjectNode);
    m_buildFileAction->setVisible(buildFilePossible);

    m_buildSubProjectAction->setEnabled(enabled);
    m_rebuildSubProjectAction->setEnabled(enabled);
    m_cleanSubProjectAction->setEnabled(enabled);
    m_buildSubProjectContextMenu->setEnabled(enabled && isProjectNode);
    m_rebuildSubProjectContextMenu->setEnabled(enabled && isProjectNode);
    m_cleanSubProjectContextMenu->setEnabled(enabled && isProjectNode);
    m_runQMakeActionContextMenu->setEnabled(isProjectNode && !isBuilding
                                            && buildConfiguration->qmakeStep());
    m_buildFileAction->setEnabled(buildFilePossible && !isBuilding);
    m_buildFileContextMenu->setEnabled(buildFilePossible && !isBuilding);
}

void QmakeProjectManagerPlugin::buildStateChanged(ProjectExplorer::Project *pro)
{
    if (pro == ProjectTree::currentProject()) {
        updateRunQMakeAction();
        updateContextActions();
        updateBuildFileAction();
    }
}

void QmakeProjectManagerPlugin::updateBuildFileAction()
{
    bool visible = false;
    bool enabled = false;

    if (IDocument *currentDocument= EditorManager::currentDocument()) {
        Utils::FileName file = currentDocument->filePath();
        Node *node  = SessionManager::nodeForFile(file);
        Project *project = SessionManager::projectForFile(file);
        m_buildFileAction->setParameter(file.fileName());
        visible = qobject_cast<QmakeProject *>(project)
                && node
                && dynamic_cast<QmakePriFileNode *>(node->parentProjectNode());

        enabled = !BuildManager::isBuilding(project);
    }
    m_buildFileAction->setVisible(visible);
    m_buildFileAction->setEnabled(enabled);
}
