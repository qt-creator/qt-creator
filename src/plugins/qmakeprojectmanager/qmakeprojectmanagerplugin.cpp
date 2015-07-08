/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
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
#include "wizards/subdirsprojectwizard.h"
#include "customwidgetwizard/customwidgetwizard.h"
#include "qmakeprojectmanagerconstants.h"
#include "qmakeproject.h"
#include "externaleditors.h"
#include "qmakekitinformation.h"
#include "profilehighlighter.h"

#include <coreplugin/icore.h>
#include <projectexplorer/buildmanager.h>
#include <projectexplorer/session.h>
#include <projectexplorer/projecttree.h>
#include <projectexplorer/target.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>
#include <texteditor/texteditoractionhandler.h>
#include <texteditor/texteditorconstants.h>
#include <utils/hostosinfo.h>
#include <utils/mimetypes/mimedatabase.h>
#include <utils/parameteraction.h>

#ifdef WITH_TESTS
#    include <QTest>
#endif

#include <QtPlugin>

using namespace Core;
using namespace QmakeProjectManager::Internal;
using namespace QmakeProjectManager;
using namespace ProjectExplorer;

QmakeProjectManagerPlugin::~QmakeProjectManagerPlugin()
{
}

bool QmakeProjectManagerPlugin::initialize(const QStringList &arguments, QString *errorMessage)
{
    Q_UNUSED(arguments)
    Q_UNUSED(errorMessage)
    const Context projectContext(QmakeProjectManager::Constants::PROJECT_ID);
    Context projecTreeContext(ProjectExplorer::Constants::C_PROJECT_TREE);

    Utils::MimeDatabase::addMimeTypes(QLatin1String(":qmakeprojectmanager/QmakeProjectManager.mimetypes.xml"));

    //create and register objects
    m_qmakeProjectManager = new QmakeManager;
    addAutoReleasedObject(m_qmakeProjectManager);

    ProjectExplorer::KitManager::registerKitInformation(new QmakeKitInformation);

    IWizardFactory::registerFactoryCreator([]() {
        QList<IWizardFactory *> result;
        result << new SubdirsProjectWizard << new GuiAppWizard << new LibraryWizard
               << new TestWizard << new CustomWidgetWizard;
        return result;
    });

    addAutoReleasedObject(new CustomWizardMetaFactory<CustomQmakeProjectWizard>
                          (QLatin1String("qmakeproject"), IWizardFactory::ProjectWizard));

    addAutoReleasedObject(new QMakeStepFactory);
    addAutoReleasedObject(new MakeStepFactory);

    addAutoReleasedObject(new QmakeBuildConfigurationFactory);
    addAutoReleasedObject(new DesktopQmakeRunConfigurationFactory);

    if (Utils::HostOsInfo::isMacHost())
        addAutoReleasedObject(new MacDesignerExternalEditor);
    else
        addAutoReleasedObject(new DesignerExternalEditor);
    addAutoReleasedObject(new LinguistExternalEditor);

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
    Command *command;

    m_buildSubProjectContextMenu = new Utils::ParameterAction(tr("Build"), tr("Build \"%1\""),
                                                              Utils::ParameterAction::AlwaysEnabled/*handled manually*/,
                                                              this);
    command = ActionManager::registerAction(m_buildSubProjectContextMenu, Constants::BUILDSUBDIRCONTEXTMENU, projectContext);
    command->setAttribute(Command::CA_Hide);
    command->setAttribute(Command::CA_UpdateText);
    command->setDescription(m_buildSubProjectContextMenu->text());
    msubproject->addAction(command, ProjectExplorer::Constants::G_PROJECT_BUILD);
    connect(m_buildSubProjectContextMenu, SIGNAL(triggered()), m_qmakeProjectManager, SLOT(buildSubDirContextMenu()));

    m_runQMakeActionContextMenu = new QAction(tr("Run qmake"), this);
    command = ActionManager::registerAction(m_runQMakeActionContextMenu, Constants::RUNQMAKECONTEXTMENU, projectContext);
    command->setAttribute(Command::CA_Hide);
    mproject->addAction(command, ProjectExplorer::Constants::G_PROJECT_BUILD);
    msubproject->addAction(command, ProjectExplorer::Constants::G_PROJECT_BUILD);
    connect(m_runQMakeActionContextMenu, SIGNAL(triggered()), m_qmakeProjectManager, SLOT(runQMakeContextMenu()));

    command = msubproject->addSeparator(projectContext, ProjectExplorer::Constants::G_PROJECT_BUILD,
                                        &m_subProjectRebuildSeparator);
    command->setAttribute(Command::CA_Hide);

    m_rebuildSubProjectContextMenu = new QAction(tr("Rebuild"), this);
    command = ActionManager::registerAction(
                m_rebuildSubProjectContextMenu, Constants::REBUILDSUBDIRCONTEXTMENU, projectContext);
    command->setAttribute(Command::CA_Hide);
    msubproject->addAction(command, ProjectExplorer::Constants::G_PROJECT_BUILD);
    connect(m_rebuildSubProjectContextMenu, SIGNAL(triggered()), m_qmakeProjectManager, SLOT(rebuildSubDirContextMenu()));

    m_cleanSubProjectContextMenu = new QAction(tr("Clean"), this);
    command = ActionManager::registerAction(
                m_cleanSubProjectContextMenu, Constants::CLEANSUBDIRCONTEXTMENU, projectContext);
    command->setAttribute(Command::CA_Hide);
    msubproject->addAction(command, ProjectExplorer::Constants::G_PROJECT_BUILD);
    connect(m_cleanSubProjectContextMenu, SIGNAL(triggered()), m_qmakeProjectManager, SLOT(cleanSubDirContextMenu()));

    m_buildFileContextMenu = new QAction(tr("Build"), this);
    command = ActionManager::registerAction(m_buildFileContextMenu, Constants::BUILDFILECONTEXTMENU, projectContext);
    command->setAttribute(Command::CA_Hide);
    mfile->addAction(command, ProjectExplorer::Constants::G_FILE_OTHER);
    connect(m_buildFileContextMenu, SIGNAL(triggered()), m_qmakeProjectManager, SLOT(buildFileContextMenu()));

    m_buildSubProjectAction = new Utils::ParameterAction(tr("Build Subproject"), tr("Build Subproject \"%1\""),
                                                         Utils::ParameterAction::AlwaysEnabled, this);
    command = ActionManager::registerAction(m_buildSubProjectAction, Constants::BUILDSUBDIR, projectContext);
    command->setAttribute(Command::CA_Hide);
    command->setAttribute(Command::CA_UpdateText);
    command->setDescription(m_buildSubProjectAction->text());
    mbuild->addAction(command, ProjectExplorer::Constants::G_BUILD_BUILD);
    connect(m_buildSubProjectAction, SIGNAL(triggered()), m_qmakeProjectManager, SLOT(buildSubDirContextMenu()));

    m_runQMakeAction = new QAction(tr("Run qmake"), this);
    const Context globalcontext(Core::Constants::C_GLOBAL);
    command = ActionManager::registerAction(m_runQMakeAction, Constants::RUNQMAKE, globalcontext);
    mbuild->addAction(command, ProjectExplorer::Constants::G_BUILD_BUILD);
    connect(m_runQMakeAction, SIGNAL(triggered()), m_qmakeProjectManager, SLOT(runQMake()));

    m_rebuildSubProjectAction = new Utils::ParameterAction(tr("Rebuild Subproject"), tr("Rebuild Subproject \"%1\""),
                                                           Utils::ParameterAction::AlwaysEnabled, this);
    command = ActionManager::registerAction(m_rebuildSubProjectAction, Constants::REBUILDSUBDIR, projectContext);
    command->setAttribute(Command::CA_Hide);
    command->setAttribute(Command::CA_UpdateText);
    command->setDescription(m_rebuildSubProjectAction->text());
    mbuild->addAction(command, ProjectExplorer::Constants::G_BUILD_REBUILD);
    connect(m_rebuildSubProjectAction, SIGNAL(triggered()), m_qmakeProjectManager, SLOT(rebuildSubDirContextMenu()));

    m_cleanSubProjectAction = new Utils::ParameterAction(tr("Clean Subproject"), tr("Clean Subproject \"%1\""),
                                                         Utils::ParameterAction::AlwaysEnabled, this);
    command = ActionManager::registerAction(m_cleanSubProjectAction, Constants::CLEANSUBDIR, projectContext);
    command->setAttribute(Command::CA_Hide);
    command->setAttribute(Command::CA_UpdateText);
    command->setDescription(m_cleanSubProjectAction->text());
    mbuild->addAction(command, ProjectExplorer::Constants::G_BUILD_CLEAN);
    connect(m_cleanSubProjectAction, SIGNAL(triggered()), m_qmakeProjectManager, SLOT(cleanSubDirContextMenu()));

    m_buildFileAction = new Utils::ParameterAction(tr("Build File"), tr("Build File \"%1\""),
                                                   Utils::ParameterAction::AlwaysEnabled, this);
    command = ActionManager::registerAction(m_buildFileAction, Constants::BUILDFILE);
    command->setAttribute(Command::CA_Hide);
    command->setAttribute(Command::CA_UpdateText);
    command->setDescription(m_buildFileAction->text());
    command->setDefaultKeySequence(QKeySequence(tr("Ctrl+Alt+B")));
    mbuild->addAction(command, ProjectExplorer::Constants::G_BUILD_BUILD);
    connect(m_buildFileAction, SIGNAL(triggered()), m_qmakeProjectManager, SLOT(buildFile()));

    connect(BuildManager::instance(), SIGNAL(buildStateChanged(ProjectExplorer::Project*)),
            this, SLOT(buildStateChanged(ProjectExplorer::Project*)));
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
    connect(m_addLibraryAction, SIGNAL(triggered()),
            m_qmakeProjectManager, SLOT(addLibrary()));
    contextMenu->addAction(command);

    m_addLibraryActionContextMenu = new QAction(tr("Add Library..."), this);
    command = ActionManager::registerAction(m_addLibraryActionContextMenu,
        Constants::ADDLIBRARY, projecTreeContext);
    connect(m_addLibraryActionContextMenu, SIGNAL(triggered()),
            m_qmakeProjectManager, SLOT(addLibraryContextMenu()));
    mproject->addAction(command, ProjectExplorer::Constants::G_PROJECT_FILES);
    msubproject->addAction(command, ProjectExplorer::Constants::G_PROJECT_FILES);

    contextMenu->addSeparator(proFileEditorContext);

    command = ActionManager::command(TextEditor::Constants::UN_COMMENT_SELECTION);
    contextMenu->addAction(command);

    connect(EditorManager::instance(), &EditorManager::currentEditorChanged,
            this, &QmakeProjectManagerPlugin::updateBuildFileAction);

    return true;
}

void QmakeProjectManagerPlugin::extensionsInitialized()
{ }

void QmakeProjectManagerPlugin::projectChanged()
{
    if (m_previousStartupProject)
        disconnect(m_previousStartupProject, SIGNAL(activeTargetChanged(ProjectExplorer::Target*)),
                   this, SLOT(activeTargetChanged()));

    if (ProjectTree::currentProject())
        m_previousStartupProject = qobject_cast<QmakeProject *>(ProjectTree::currentProject());
    else
        m_previousStartupProject = qobject_cast<QmakeProject *>(SessionManager::startupProject());

    if (m_previousStartupProject)
        connect(m_previousStartupProject, SIGNAL(activeTargetChanged(ProjectExplorer::Target*)),
                           this, SLOT(activeTargetChanged()));

    activeTargetChanged();
}

void QmakeProjectManagerPlugin::activeTargetChanged()
{
    if (m_previousTarget)
        disconnect(m_previousTarget, SIGNAL(activeBuildConfigurationChanged(ProjectExplorer::BuildConfiguration*)),
                   this, SLOT(updateRunQMakeAction()));

    m_previousTarget = m_previousStartupProject ? m_previousStartupProject->activeTarget() : 0;

    if (m_previousTarget)
        connect(m_previousTarget, SIGNAL(activeBuildConfigurationChanged(ProjectExplorer::BuildConfiguration*)),
                this, SLOT(updateRunQMakeAction()));

    updateRunQMakeAction();
}

void QmakeProjectManagerPlugin::updateRunQMakeAction()
{
    bool enable = true;
    if (BuildManager::isBuilding(m_previousStartupProject))
        enable = false;
    QmakeProject *pro = qobject_cast<QmakeProject *>(m_previousStartupProject);
    m_runQMakeAction->setVisible(pro);
    if (!pro
            || !pro->activeTarget()
            || !pro->activeTarget()->activeBuildConfiguration())
        enable = false;

    m_runQMakeAction->setEnabled(enable);
}

void QmakeProjectManagerPlugin::updateContextActions(ProjectExplorer::Node *node, ProjectExplorer::Project *project)
{
    m_addLibraryActionContextMenu->setEnabled(dynamic_cast<QmakeProFileNode *>(node));

    QmakeProFileNode *proFileNode = dynamic_cast<QmakeProFileNode *>(node);
    QmakeProject *qmakeProject = qobject_cast<QmakeProject *>(project);
    QmakeProFileNode *subProjectNode = 0;
    if (node) {
        if (QmakePriFileNode *subPriFileNode = dynamic_cast<QmakePriFileNode *>(node->projectNode()))
            subProjectNode = subPriFileNode->proFileNode();
    }
    ProjectExplorer::FileNode *fileNode = node ? node->asFileNode() : 0;
    bool buildFilePossible = subProjectNode && fileNode
            && (fileNode->fileType() == ProjectExplorer::SourceType);

    m_qmakeProjectManager->setContextNode(subProjectNode);
    m_qmakeProjectManager->setContextProject(qmakeProject);
    m_qmakeProjectManager->setContextFile(buildFilePossible ? fileNode : 0);

    bool subProjectActionsVisible = qmakeProject && subProjectNode && (subProjectNode != qmakeProject->rootProjectNode());

    QString subProjectName;
    if (subProjectActionsVisible)
        subProjectName = subProjectNode->displayName();

    m_buildSubProjectAction->setParameter(subProjectName);
    m_rebuildSubProjectAction->setParameter(subProjectName);
    m_cleanSubProjectAction->setParameter(subProjectName);
    m_buildSubProjectContextMenu->setParameter(subProjectName);
    m_buildFileAction->setParameter(buildFilePossible ? fileNode->path().fileName() : QString());

    QmakeBuildConfiguration *buildConfiguration = (qmakeProject && qmakeProject->activeTarget()) ?
                static_cast<QmakeBuildConfiguration *>(qmakeProject->activeTarget()->activeBuildConfiguration()) : 0;
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
    m_runQMakeActionContextMenu->setVisible(isProjectNode && buildConfiguration->qmakeStep());
    m_buildFileAction->setVisible(buildFilePossible);
    m_buildFileContextMenu->setVisible(buildFilePossible);

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
    ProjectExplorer::Project *currentProject = ProjectTree::currentProject();
    if (pro == currentProject) {
        updateRunQMakeAction();
        updateContextActions(ProjectTree::currentNode(), pro);
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
                && dynamic_cast<QmakePriFileNode *>(node->projectNode());

        enabled = !BuildManager::isBuilding(project);
    }
    m_buildFileAction->setVisible(visible);
    m_buildFileAction->setEnabled(enabled);
}
