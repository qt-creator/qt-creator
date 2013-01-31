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

#include "qt4projectmanagerplugin.h"

#include "qt4projectmanager.h"
#include "qt4nodes.h"
#include "qmakestep.h"
#include "makestep.h"
#include "qt4buildconfiguration.h"
#include "wizards/consoleappwizard.h"
#include "wizards/guiappwizard.h"
#include "wizards/mobileappwizard.h"
#include "wizards/librarywizard.h"
#include "wizards/testwizard.h"
#include "wizards/emptyprojectwizard.h"
#include "wizards/subdirsprojectwizard.h"
#include "wizards/qtquickappwizard.h"
#include "wizards/html5appwizard.h"
#include "customwidgetwizard/customwidgetwizard.h"
#include "profileeditorfactory.h"
#include "profilehoverhandler.h"
#include "qt4projectmanagerconstants.h"
#include "qt4project.h"
#include "profileeditor.h"
#include "externaleditors.h"
#include "profilecompletionassist.h"
#include "qt-desktop/qt4runconfiguration.h"
#include "qt-desktop/desktopqtversionfactory.h"
#include "qt-desktop/simulatorqtversionfactory.h"
#include "winceqtversionfactory.h"
#include "unconfiguredprojectpanel.h"
#include "qmakekitinformation.h"

#include <coreplugin/id.h>
#include <coreplugin/icore.h>
#include <extensionsystem/pluginmanager.h>
#include <projectexplorer/buildmanager.h>
#include <projectexplorer/project.h>
#include <projectexplorer/session.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectnodes.h>
#include <projectexplorer/target.h>
#include <coreplugin/mimedatabase.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/editormanager/editormanager.h>
#include <texteditor/texteditoractionhandler.h>
#include <texteditor/texteditorconstants.h>
#include <texteditor/texteditorsettings.h>
#include <utils/hostosinfo.h>
#include <utils/parameteraction.h>

#ifdef WITH_TESTS
#    include <QTest>
#endif

#include <QtPlugin>
#include <QMenu>

using namespace Qt4ProjectManager::Internal;
using namespace Qt4ProjectManager;
using ProjectExplorer::Project;

Qt4ProjectManagerPlugin::Qt4ProjectManagerPlugin()
    : m_previousStartupProject(0), m_previousTarget(0)
{

}

Qt4ProjectManagerPlugin::~Qt4ProjectManagerPlugin()
{
    //removeObject(m_embeddedPropertiesPage);
    //delete m_embeddedPropertiesPage;

    removeObject(m_proFileEditorFactory);
    delete m_proFileEditorFactory;
    removeObject(m_qt4ProjectManager);
    delete m_qt4ProjectManager;
}

bool Qt4ProjectManagerPlugin::initialize(const QStringList &arguments, QString *errorMessage)
{
    Q_UNUSED(arguments)
    const Core::Context projectContext(Qt4ProjectManager::Constants::PROJECT_ID);
    Core::Context projecTreeContext(ProjectExplorer::Constants::C_PROJECT_TREE);

    if (!Core::ICore::mimeDatabase()->addMimeTypes(QLatin1String(":qt4projectmanager/Qt4ProjectManager.mimetypes.xml"), errorMessage))
        return false;

    m_projectExplorer = ProjectExplorer::ProjectExplorerPlugin::instance();

    //create and register objects
    m_qt4ProjectManager = new Qt4Manager(this);
    addObject(m_qt4ProjectManager);

    TextEditor::TextEditorActionHandler *editorHandler
            = new TextEditor::TextEditorActionHandler(Constants::C_PROFILEEDITOR,
                  TextEditor::TextEditorActionHandler::UnCommentSelection
                  | TextEditor::TextEditorActionHandler::JumpToFileUnderCursor);

    m_proFileEditorFactory = new ProFileEditorFactory(m_qt4ProjectManager, editorHandler);

    ProjectExplorer::KitManager::instance()->registerKitInformation(new QmakeKitInformation);

    addObject(m_proFileEditorFactory);

    addAutoReleasedObject(new EmptyProjectWizard);
    addAutoReleasedObject(new SubdirsProjectWizard);
    addAutoReleasedObject(new GuiAppWizard);
    addAutoReleasedObject(new ConsoleAppWizard);
    addAutoReleasedObject(new MobileAppWizard);
    QtQuickAppWizard::createInstances(this); //creates several instances with different options
    addAutoReleasedObject(new Html5AppWizard);
    addAutoReleasedObject(new LibraryWizard);
    addAutoReleasedObject(new TestWizard);
    addAutoReleasedObject(new CustomWidgetWizard);

    CustomQt4ProjectWizard::registerSelf();

    addAutoReleasedObject(new QMakeStepFactory);
    addAutoReleasedObject(new MakeStepFactory);

    addAutoReleasedObject(new Qt4BuildConfigurationFactory);
    addAutoReleasedObject(new Qt4RunConfigurationFactory);

    if (Utils::HostOsInfo::isMacHost())
        addAutoReleasedObject(new MacDesignerExternalEditor);
    else
        addAutoReleasedObject(new DesignerExternalEditor);
    addAutoReleasedObject(new LinguistExternalEditor);

    addAutoReleasedObject(new DesktopQtVersionFactory);
    addAutoReleasedObject(new SimulatorQtVersionFactory);
    addAutoReleasedObject(new WinCeQtVersionFactory);

    addAutoReleasedObject(new ProFileCompletionAssistProvider);
    addAutoReleasedObject(new ProFileHoverHandler(this));
    addAutoReleasedObject(new UnconfiguredProjectPanel);

    //menus
    Core::ActionContainer *mbuild =
            Core::ActionManager::actionContainer(ProjectExplorer::Constants::M_BUILDPROJECT);
    Core::ActionContainer *mproject =
            Core::ActionManager::actionContainer(ProjectExplorer::Constants::M_PROJECTCONTEXT);
    Core::ActionContainer *msubproject =
            Core::ActionManager::actionContainer(ProjectExplorer::Constants::M_SUBPROJECTCONTEXT);
    Core::ActionContainer *mfile =
            Core::ActionManager::actionContainer(ProjectExplorer::Constants::M_FILECONTEXT);

    //register actions
    Core::Command *command;

    m_buildSubProjectContextMenu = new Utils::ParameterAction(tr("Build"), tr("Build \"%1\""),
                                                              Utils::ParameterAction::AlwaysEnabled/*handled manually*/,
                                                              this);
    command = Core::ActionManager::registerAction(m_buildSubProjectContextMenu, Constants::BUILDSUBDIRCONTEXTMENU, projectContext);
    command->setAttribute(Core::Command::CA_Hide);
    command->setAttribute(Core::Command::CA_UpdateText);
    command->setDescription(m_buildSubProjectContextMenu->text());
    msubproject->addAction(command, ProjectExplorer::Constants::G_PROJECT_BUILD);
    connect(m_buildSubProjectContextMenu, SIGNAL(triggered()), m_qt4ProjectManager, SLOT(buildSubDirContextMenu()));

    m_runQMakeActionContextMenu = new QAction(tr("Run qmake"), this);
    command = Core::ActionManager::registerAction(m_runQMakeActionContextMenu, Constants::RUNQMAKECONTEXTMENU, projectContext);
    command->setAttribute(Core::Command::CA_Hide);
    mproject->addAction(command, ProjectExplorer::Constants::G_PROJECT_BUILD);
    msubproject->addAction(command, ProjectExplorer::Constants::G_PROJECT_BUILD);
    connect(m_runQMakeActionContextMenu, SIGNAL(triggered()), m_qt4ProjectManager, SLOT(runQMakeContextMenu()));

    command = msubproject->addSeparator(projectContext, ProjectExplorer::Constants::G_PROJECT_BUILD,
                                        &m_subProjectRebuildSeparator);
    command->setAttribute(Core::Command::CA_Hide);

    m_rebuildSubProjectContextMenu = new QAction(tr("Rebuild"), this);
    command = Core::ActionManager::registerAction(
                m_rebuildSubProjectContextMenu, Constants::REBUILDSUBDIRCONTEXTMENU, projectContext);
    command->setAttribute(Core::Command::CA_Hide);
    msubproject->addAction(command, ProjectExplorer::Constants::G_PROJECT_BUILD);
    connect(m_rebuildSubProjectContextMenu, SIGNAL(triggered()), m_qt4ProjectManager, SLOT(rebuildSubDirContextMenu()));

    m_cleanSubProjectContextMenu = new QAction(tr("Clean"), this);
    command = Core::ActionManager::registerAction(
                m_cleanSubProjectContextMenu, Constants::CLEANSUBDIRCONTEXTMENU, projectContext);
    command->setAttribute(Core::Command::CA_Hide);
    msubproject->addAction(command, ProjectExplorer::Constants::G_PROJECT_BUILD);
    connect(m_cleanSubProjectContextMenu, SIGNAL(triggered()), m_qt4ProjectManager, SLOT(cleanSubDirContextMenu()));

    m_buildFileContextMenu = new QAction(tr("Build"), this);
    command = Core::ActionManager::registerAction(m_buildFileContextMenu, Constants::BUILDFILECONTEXTMENU, projectContext);
    command->setAttribute(Core::Command::CA_Hide);
    mfile->addAction(command, ProjectExplorer::Constants::G_FILE_OTHER);
    connect(m_buildFileContextMenu, SIGNAL(triggered()), m_qt4ProjectManager, SLOT(buildFileContextMenu()));

    m_buildSubProjectAction = new Utils::ParameterAction(tr("Build Subproject"), tr("Build Subproject \"%1\""),
                                                         Utils::ParameterAction::AlwaysEnabled, this);
    command = Core::ActionManager::registerAction(m_buildSubProjectAction, Constants::BUILDSUBDIR, projectContext);
    command->setAttribute(Core::Command::CA_Hide);
    command->setAttribute(Core::Command::CA_UpdateText);
    command->setDescription(m_buildSubProjectAction->text());
    mbuild->addAction(command, ProjectExplorer::Constants::G_BUILD_BUILD);
    connect(m_buildSubProjectAction, SIGNAL(triggered()), m_qt4ProjectManager, SLOT(buildSubDirContextMenu()));

    m_runQMakeAction = new QAction(tr("Run qmake"), this);
    command = Core::ActionManager::registerAction(m_runQMakeAction, Constants::RUNQMAKE, projectContext);
    command->setAttribute(Core::Command::CA_Hide);
    mbuild->addAction(command, ProjectExplorer::Constants::G_BUILD_BUILD);
    connect(m_runQMakeAction, SIGNAL(triggered()), m_qt4ProjectManager, SLOT(runQMake()));

    m_rebuildSubProjectAction = new Utils::ParameterAction(tr("Rebuild Subproject"), tr("Rebuild Subproject \"%1\""),
                                                           Utils::ParameterAction::AlwaysEnabled, this);
    command = Core::ActionManager::registerAction(m_rebuildSubProjectAction, Constants::REBUILDSUBDIR, projectContext);
    command->setAttribute(Core::Command::CA_Hide);
    command->setAttribute(Core::Command::CA_UpdateText);
    command->setDescription(m_rebuildSubProjectAction->text());
    mbuild->addAction(command, ProjectExplorer::Constants::G_BUILD_REBUILD);
    connect(m_rebuildSubProjectAction, SIGNAL(triggered()), m_qt4ProjectManager, SLOT(rebuildSubDirContextMenu()));

    m_cleanSubProjectAction = new Utils::ParameterAction(tr("Clean Subproject"), tr("Clean Subproject \"%1\""),
                                                         Utils::ParameterAction::AlwaysEnabled, this);
    command = Core::ActionManager::registerAction(m_cleanSubProjectAction, Constants::CLEANSUBDIR, projectContext);
    command->setAttribute(Core::Command::CA_Hide);
    command->setAttribute(Core::Command::CA_UpdateText);
    command->setDescription(m_cleanSubProjectAction->text());
    mbuild->addAction(command, ProjectExplorer::Constants::G_BUILD_CLEAN);
    connect(m_cleanSubProjectAction, SIGNAL(triggered()), m_qt4ProjectManager, SLOT(cleanSubDirContextMenu()));

    m_buildFileAction = new Utils::ParameterAction(tr("Build File"), tr("Build File \"%1\""),
                                                   Utils::ParameterAction::AlwaysEnabled, this);
    command = Core::ActionManager::registerAction(m_buildFileAction, Constants::BUILDFILE, projectContext);
    command->setAttribute(Core::Command::CA_Hide);
    command->setAttribute(Core::Command::CA_UpdateText);
    command->setDescription(m_buildFileAction->text());
    command->setDefaultKeySequence(QKeySequence(tr("Ctrl+Alt+B")));
    mbuild->addAction(command, ProjectExplorer::Constants::G_BUILD_BUILD);
    connect(m_buildFileAction, SIGNAL(triggered()), m_qt4ProjectManager, SLOT(buildFileContextMenu()));

    connect(m_projectExplorer->buildManager(), SIGNAL(buildStateChanged(ProjectExplorer::Project*)),
            this, SLOT(buildStateChanged(ProjectExplorer::Project*)));
    connect(m_projectExplorer->session(), SIGNAL(startupProjectChanged(ProjectExplorer::Project*)),
            this, SLOT(startupProjectChanged()));
    connect(m_projectExplorer, SIGNAL(currentNodeChanged(ProjectExplorer::Node*,ProjectExplorer::Project*)),
            this, SLOT(updateContextActions(ProjectExplorer::Node*,ProjectExplorer::Project*)));

    Core::ActionContainer *contextMenu = Core::ActionManager::createMenu(Qt4ProjectManager::Constants::M_CONTEXT);

    Core::Context proFileEditorContext = Core::Context(Qt4ProjectManager::Constants::C_PROFILEEDITOR);

    command = Core::ActionManager::command(TextEditor::Constants::JUMP_TO_FILE_UNDER_CURSOR);
    contextMenu->addAction(command);

    m_addLibraryAction = new QAction(tr("Add Library..."), this);
    command = Core::ActionManager::registerAction(m_addLibraryAction,
        Constants::ADDLIBRARY, proFileEditorContext);
    connect(m_addLibraryAction, SIGNAL(triggered()),
            m_qt4ProjectManager, SLOT(addLibrary()));
    contextMenu->addAction(command);

    m_addLibraryActionContextMenu = new QAction(tr("Add Library..."), this);
    command = Core::ActionManager::registerAction(m_addLibraryActionContextMenu,
        Constants::ADDLIBRARY, projecTreeContext);
    connect(m_addLibraryActionContextMenu, SIGNAL(triggered()),
            m_qt4ProjectManager, SLOT(addLibraryContextMenu()));
    mproject->addAction(command, ProjectExplorer::Constants::G_PROJECT_FILES);
    msubproject->addAction(command, ProjectExplorer::Constants::G_PROJECT_FILES);

    contextMenu->addSeparator(proFileEditorContext);

    command = Core::ActionManager::command(TextEditor::Constants::UN_COMMENT_SELECTION);
    contextMenu->addAction(command);

    return true;
}

void Qt4ProjectManagerPlugin::extensionsInitialized()
{
    m_qt4ProjectManager->init();
}

void Qt4ProjectManagerPlugin::startupProjectChanged()
{
    if (m_previousStartupProject)
        disconnect(m_previousStartupProject, SIGNAL(activeTargetChanged(ProjectExplorer::Target*)),
                   this, SLOT(activeTargetChanged()));

    m_previousStartupProject = qobject_cast<Qt4Project *>(m_projectExplorer->session()->startupProject());

    if (m_previousStartupProject)
        connect(m_previousStartupProject, SIGNAL(activeTargetChanged(ProjectExplorer::Target*)),
                           this, SLOT(activeTargetChanged()));

    activeTargetChanged();
}

void Qt4ProjectManagerPlugin::activeTargetChanged()
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

void Qt4ProjectManagerPlugin::updateRunQMakeAction()
{
    bool enable = true;
    if (m_projectExplorer->buildManager()->isBuilding(m_projectExplorer->currentProject()))
        enable = false;
    Qt4Project *pro = qobject_cast<Qt4Project *>(m_projectExplorer->currentProject());
    if (!pro
            || !pro->activeTarget()
            || !pro->activeTarget()->activeBuildConfiguration())
        enable = false;

    m_runQMakeAction->setEnabled(enable);
}

void Qt4ProjectManagerPlugin::updateContextActions(ProjectExplorer::Node *node, ProjectExplorer::Project *project)
{
    m_addLibraryActionContextMenu->setEnabled(qobject_cast<Qt4ProFileNode *>(node));

    Qt4ProFileNode *proFileNode = qobject_cast<Qt4ProFileNode *>(node);
    Qt4Project *qt4Project = qobject_cast<Qt4Project *>(project);
    Qt4ProFileNode *subProjectNode = node ? qobject_cast<Qt4ProFileNode *>(node->projectNode()) : 0;
    ProjectExplorer::FileNode *fileNode = qobject_cast<ProjectExplorer::FileNode *>(node);
    bool buildFilePossible = subProjectNode && fileNode
            && (fileNode->fileType() == ProjectExplorer::SourceType);

    m_qt4ProjectManager->setContextNode(subProjectNode);
    m_qt4ProjectManager->setContextProject(qt4Project);
    m_qt4ProjectManager->setContextFile(buildFilePossible ? fileNode : 0);

    bool subProjectActionsVisible = qt4Project && subProjectNode && (subProjectNode != qt4Project->rootProjectNode());

    QString subProjectName;
    if (subProjectActionsVisible)
        subProjectName = subProjectNode->displayName();

    m_buildSubProjectAction->setParameter(subProjectName);
    m_rebuildSubProjectAction->setParameter(subProjectName);
    m_cleanSubProjectAction->setParameter(subProjectName);
    m_buildSubProjectContextMenu->setParameter(subProjectName);
    m_buildFileAction->setParameter(buildFilePossible ? QFileInfo(fileNode->path()).fileName() : QString());

    Qt4BuildConfiguration *buildConfiguration = (qt4Project && qt4Project->activeTarget()) ?
                static_cast<Qt4BuildConfiguration *>(qt4Project->activeTarget()->activeBuildConfiguration()) : 0;
    bool isProjectNode = qt4Project && proFileNode && buildConfiguration;
    bool isBuilding = m_projectExplorer->buildManager()->isBuilding(project);
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

void Qt4ProjectManagerPlugin::buildStateChanged(ProjectExplorer::Project *pro)
{
    ProjectExplorer::Project *currentProject = m_projectExplorer->currentProject();
    if (pro == currentProject) {
        updateRunQMakeAction();
        updateContextActions(m_projectExplorer->currentNode(), pro);
    }
}

Q_EXPORT_PLUGIN(Qt4ProjectManagerPlugin)
