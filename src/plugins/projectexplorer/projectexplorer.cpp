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

#include "projectexplorer.h"

#include "buildsteplist.h"
#include "deployablefile.h"
#include "deployconfiguration.h"
#include "gcctoolchainfactories.h"
#include "project.h"
#include "projectexplorersettings.h"
#include "removetaskhandler.h"
#include "kitmanager.h"
#include "kitoptionspage.h"
#include "target.h"
#include "targetsettingspanel.h"
#include "toolchainmanager.h"
#include "toolchainoptionspage.h"
#include "copytaskhandler.h"
#include "showineditortaskhandler.h"
#include "vcsannotatetaskhandler.h"
#include "localapplicationruncontrol.h"
#include "allprojectsfilter.h"
#include "allprojectsfind.h"
#include "buildmanager.h"
#include "buildsettingspropertiespage.h"
#include "currentprojectfind.h"
#include "currentprojectfilter.h"
#include "editorsettingspropertiespage.h"
#include "codestylesettingspropertiespage.h"
#include "dependenciespanel.h"
#include "foldernavigationwidget.h"
#include "iprojectmanager.h"
#include "metatypedeclarations.h"
#include "nodesvisitor.h"
#include "appoutputpane.h"
#include "pluginfilefactory.h"
#include "processstep.h"
#include "projectexplorerconstants.h"
#include "customwizard.h"
#include "kitinformation.h"
#include "projectfilewizardextension.h"
#include "projecttreewidget.h"
#include "projectwindow.h"
#include "runsettingspropertiespage.h"
#include "session.h"
#include "projectnodes.h"
#include "sessiondialog.h"
#include "target.h"
#include "projectexplorersettingspage.h"
#include "projectwelcomepage.h"
#include "corelistenercheckingforrunningbuild.h"
#include "buildconfiguration.h"
#include "miniprojecttargetselector.h"
#include "taskhub.h"
#include "customtoolchain.h"
#include "devicesupport/desktopdevice.h"
#include "devicesupport/desktopdevicefactory.h"
#include "devicesupport/devicemanager.h"
#include "devicesupport/devicesettingspage.h"
#include "publishing/ipublishingwizardfactory.h"
#include "publishing/publishingwizardselectiondialog.h"

#ifdef Q_OS_WIN
#    include "windebuginterface.h"
#    include "msvctoolchain.h"
#    include "wincetoolchain.h"
#endif

#include <extensionsystem/pluginspec.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/documentmanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/imode.h>
#include <coreplugin/mimedatabase.h>
#include <coreplugin/modemanager.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/id.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/findplaceholder.h>
#include <coreplugin/basefilewizard.h>
#include <coreplugin/vcsmanager.h>
#include <coreplugin/iversioncontrol.h>
#include <coreplugin/variablemanager.h>
#include <coreplugin/fileutils.h>
#include <coreplugin/removefiledialog.h>
#include <extensionsystem/pluginmanager.h>
#include <find/searchresultwindow.h>
#include <utils/consoleprocess.h>
#include <utils/qtcassert.h>
#include <utils/parameteraction.h>
#include <utils/stringutils.h>
#include <utils/persistentsettings.h>

#include <QtPlugin>
#include <QDateTime>
#include <QDebug>
#include <QFileInfo>
#include <QSettings>

#include <QAction>
#include <QApplication>
#include <QFileDialog>
#include <QMenu>
#include <QMessageBox>
#include <QWizard>

/*!
    \namespace ProjectExplorer
    ProjectExplorer plugin namespace
*/

/*!
    \namespace ProjectExplorer::Internal
    Internal namespace of the ProjectExplorer plugin
    \internal
*/

/*!
    \class ProjectExplorer::ProjectExplorerPlugin

    \brief ProjectExplorerPlugin with static accessor and utility functions to obtain
    current project, open projects, etc.
*/

namespace {
bool debug = false;
}

namespace ProjectExplorer {

struct ProjectExplorerPluginPrivate {
    ProjectExplorerPluginPrivate();

    QMenu *m_sessionContextMenu;
    QMenu *m_sessionMenu;
    QMenu *m_projectMenu;
    QMenu *m_subProjectMenu;
    QMenu *m_folderMenu;
    QMenu *m_fileMenu;
    QMenu *m_openWithMenu;

    QMultiMap<int, QObject*> m_actionMap;
    QAction *m_sessionManagerAction;
    QAction *m_newAction;
    QAction *m_loadAction;
    Utils::ParameterAction *m_unloadAction;
    QAction *m_closeAllProjects;
    QAction *m_buildProjectOnlyAction;
    Utils::ParameterAction *m_buildAction;
    QAction *m_buildActionContextMenu;
    QAction *m_buildSessionAction;
    QAction *m_rebuildProjectOnlyAction;
    Utils::ParameterAction *m_rebuildAction;
    QAction *m_rebuildActionContextMenu;
    QAction *m_rebuildSessionAction;
    QAction *m_cleanProjectOnlyAction;
    QAction *m_deployProjectOnlyAction;
    Utils::ParameterAction *m_deployAction;
    QAction *m_deployActionContextMenu;
    QAction *m_deploySessionAction;
    Utils::ParameterAction *m_publishAction;
    Utils::ParameterAction *m_cleanAction;
    QAction *m_cleanActionContextMenu;
    QAction *m_cleanSessionAction;
    QAction *m_runAction;
    QAction *m_runActionContextMenu;
    QAction *m_runWithoutDeployAction;
    QAction *m_cancelBuildAction;
    QAction *m_addNewFileAction;
    QAction *m_addExistingFilesAction;
    QAction *m_addNewSubprojectAction;
    QAction *m_removeFileAction;
    QAction *m_removeProjectAction;
    QAction *m_deleteFileAction;
    QAction *m_renameFileAction;
    QAction *m_openFileAction;
    QAction *m_projectTreeCollapseAllAction;
    QAction *m_searchOnFileSystem;
    QAction *m_showInGraphicalShell;
    QAction *m_openTerminalHere;
    Utils::ParameterAction *m_setStartupProjectAction;
    QAction *m_projectSelectorAction;
    QAction *m_projectSelectorActionMenu;
    QAction *m_projectSelectorActionQuick;
    QAction *m_runSubProject;

    Internal::ProjectWindow *m_proWindow;
    SessionManager *m_session;
    QString m_sessionToRestoreAtStartup;

    Project *m_currentProject;
    Node *m_currentNode;

    BuildManager *m_buildManager;

    QList<Internal::ProjectFileFactory*> m_fileFactories;
    QStringList m_profileMimeTypes;
    Internal::AppOutputPane *m_outputPane;

    QList<QPair<QString, QString> > m_recentProjects; // pair of filename, displayname
    static const int m_maxRecentProjects = 7;

    QString m_lastOpenDirectory;
    RunConfiguration *m_delayedRunConfiguration;
    RunMode m_runMode;
    QString m_projectFilterString;
    Internal::MiniProjectTargetSelector * m_targetSelector;
    Internal::ProjectExplorerSettings m_projectExplorerSettings;
    Internal::ProjectWelcomePage *m_welcomePage;

    Core::IMode *m_projectsMode;

    TaskHub *m_taskHub;
    KitManager *m_kitManager;
    ToolChainManager *m_toolChainManager;
    bool m_shuttingDown;
};

ProjectExplorerPluginPrivate::ProjectExplorerPluginPrivate() :
    m_currentProject(0),
    m_currentNode(0),
    m_delayedRunConfiguration(0),
    m_runMode(NoRunMode),
    m_projectsMode(0),
    m_kitManager(0),
    m_toolChainManager(0),
    m_shuttingDown(false)
{
}

class ProjectsMode : public Core::IMode
{
public:
    ProjectsMode(QWidget *proWindow)
    {
        setWidget(proWindow);
        setContext(Core::Context(Constants::C_PROJECTEXPLORER));
        setDisplayName(QCoreApplication::translate("ProjectExplorer::ProjectsMode", "Projects"));
        setIcon(QIcon(QLatin1String(":/fancyactionbar/images/mode_Project.png")));
        setPriority(Constants::P_MODE_SESSION);
        setId(Constants::MODE_SESSION);
        setType(Core::Id());
        setContextHelpId(QLatin1String("Managing Projects"));
    }
};

}  // namespace ProjectExplorer

using namespace ProjectExplorer;
using namespace ProjectExplorer::Internal;


ProjectExplorerPlugin *ProjectExplorerPlugin::m_instance = 0;

ProjectExplorerPlugin::ProjectExplorerPlugin()
    : d(new ProjectExplorerPluginPrivate)
{
    m_instance = this;
}

ProjectExplorerPlugin::~ProjectExplorerPlugin()
{
    removeObject(d->m_welcomePage);
    delete d->m_welcomePage;
    removeObject(this);
    // Force sequence of deletion:
    delete d->m_kitManager; // remove all the profile informations
    delete d->m_toolChainManager;

    delete d;
}

ProjectExplorerPlugin *ProjectExplorerPlugin::instance()
{
    return m_instance;
}

bool ProjectExplorerPlugin::parseArguments(const QStringList &arguments, QString * /* error */)
{
    CustomWizard::setVerbose(arguments.count(QLatin1String("-customwizard-verbose")));
    return true;
}

bool ProjectExplorerPlugin::initialize(const QStringList &arguments, QString *error)
{
    qRegisterMetaType<ProjectExplorer::RunControl *>();
    qRegisterMetaType<ProjectExplorer::DeployableFile>("ProjectExplorer::DeployableFile");

    if (!parseArguments(arguments, error))
        return false;
    addObject(this);

    // Add ToolChainFactories:
#ifdef Q_OS_WIN
    addAutoReleasedObject(new WinDebugInterface);

    addAutoReleasedObject(new Internal::MsvcToolChainFactory);
    addAutoReleasedObject(new Internal::WinCEToolChainFactory);
#else
    addAutoReleasedObject(new Internal::LinuxIccToolChainFactory);
#endif
#ifndef Q_OS_MAC
    addAutoReleasedObject(new Internal::MingwToolChainFactory); // Mingw offers cross-compiling to windows
#endif
    addAutoReleasedObject(new Internal::GccToolChainFactory);
    addAutoReleasedObject(new Internal::ClangToolChainFactory);
    addAutoReleasedObject(new Internal::CustomToolChainFactory);

    addAutoReleasedObject(new Internal::DesktopDeviceFactory);

    d->m_kitManager = new KitManager; // register before ToolChainManager
    d->m_toolChainManager = new ToolChainManager;
    addAutoReleasedObject(new Internal::ToolChainOptionsPage);
    addAutoReleasedObject(new KitOptionsPage);

    d->m_taskHub = new TaskHub;
    addAutoReleasedObject(d->m_taskHub);

    connect(Core::ICore::instance(), SIGNAL(newItemsDialogRequested()), this, SLOT(loadCustomWizards()));

    d->m_welcomePage = new ProjectWelcomePage;
    connect(d->m_welcomePage, SIGNAL(manageSessions()), this, SLOT(showSessionManager()));
    addObject(d->m_welcomePage);

    connect(Core::DocumentManager::instance(), SIGNAL(currentFileChanged(QString)),
            this, SLOT(setCurrentFile(QString)));

    d->m_session = new SessionManager(this);

    connect(d->m_session, SIGNAL(projectAdded(ProjectExplorer::Project*)),
            this, SIGNAL(fileListChanged()));
    connect(d->m_session, SIGNAL(aboutToRemoveProject(ProjectExplorer::Project*)),
            this, SLOT(invalidateProject(ProjectExplorer::Project*)));
    connect(d->m_session, SIGNAL(projectRemoved(ProjectExplorer::Project*)),
            this, SIGNAL(fileListChanged()));
    connect(d->m_session, SIGNAL(projectAdded(ProjectExplorer::Project*)),
            this, SLOT(projectAdded(ProjectExplorer::Project*)));
    connect(d->m_session, SIGNAL(projectRemoved(ProjectExplorer::Project*)),
            this, SLOT(projectRemoved(ProjectExplorer::Project*)));
    connect(d->m_session, SIGNAL(startupProjectChanged(ProjectExplorer::Project*)),
            this, SLOT(startupProjectChanged()));
    connect(d->m_session, SIGNAL(projectDisplayNameChanged(ProjectExplorer::Project*)),
            this, SLOT(projectDisplayNameChanged(ProjectExplorer::Project*)));
    connect(d->m_session, SIGNAL(dependencyChanged(ProjectExplorer::Project*,ProjectExplorer::Project*)),
            this, SLOT(updateActions()));
    connect(d->m_session, SIGNAL(sessionLoaded(QString)),
            this, SLOT(updateActions()));
    connect(d->m_session, SIGNAL(sessionLoaded(QString)),
            this, SLOT(updateWelcomePage()));

    d->m_proWindow = new ProjectWindow;
    addAutoReleasedObject(d->m_proWindow);

    Core::Context globalcontext(Core::Constants::C_GLOBAL);
    Core::Context projecTreeContext(Constants::C_PROJECT_TREE);

    d->m_projectsMode = new ProjectsMode(d->m_proWindow);
    d->m_projectsMode->setEnabled(false);
    addAutoReleasedObject(d->m_projectsMode);
    d->m_proWindow->layout()->addWidget(new Core::FindToolBarPlaceHolder(d->m_proWindow));

    addAutoReleasedObject(new CopyTaskHandler);
    addAutoReleasedObject(new ShowInEditorTaskHandler);
    addAutoReleasedObject(new VcsAnnotateTaskHandler);
    addAutoReleasedObject(new RemoveTaskHandler);
    addAutoReleasedObject(new CoreListener);

    d->m_outputPane = new AppOutputPane;
    addAutoReleasedObject(d->m_outputPane);
    connect(d->m_session, SIGNAL(projectRemoved(ProjectExplorer::Project*)),
            d->m_outputPane, SLOT(projectRemoved()));

    connect(d->m_outputPane, SIGNAL(runControlStarted(ProjectExplorer::RunControl*)),
            this, SIGNAL(runControlStarted(ProjectExplorer::RunControl*)));
    connect(d->m_outputPane, SIGNAL(runControlFinished(ProjectExplorer::RunControl*)),
            this, SIGNAL(runControlFinished(ProjectExplorer::RunControl*)));

    AllProjectsFilter *allProjectsFilter = new AllProjectsFilter(this);
    addAutoReleasedObject(allProjectsFilter);

    CurrentProjectFilter *currentProjectFilter = new CurrentProjectFilter(this);
    addAutoReleasedObject(currentProjectFilter);

    addAutoReleasedObject(new BuildSettingsPanelFactory);
    addAutoReleasedObject(new RunSettingsPanelFactory);
    addAutoReleasedObject(new EditorSettingsPanelFactory);
    addAutoReleasedObject(new CodeStyleSettingsPanelFactory);
    addAutoReleasedObject(new DependenciesPanelFactory(d->m_session));

    ProcessStepFactory *processStepFactory = new ProcessStepFactory;
    addAutoReleasedObject(processStepFactory);

    AllProjectsFind *allProjectsFind = new AllProjectsFind(this);
    addAutoReleasedObject(allProjectsFind);

    CurrentProjectFind *currentProjectFind = new CurrentProjectFind(this);
    addAutoReleasedObject(currentProjectFind);

    addAutoReleasedObject(new LocalApplicationRunControlFactory);

    addAutoReleasedObject(new ProjectFileWizardExtension);

    // Settings pages
    addAutoReleasedObject(new ProjectExplorerSettingsPage);
    addAutoReleasedObject(new DeviceSettingsPage);

    // context menus
    Core::ActionContainer *msessionContextMenu =
        Core::ActionManager::createMenu(Constants::M_SESSIONCONTEXT);
    Core::ActionContainer *mprojectContextMenu =
        Core::ActionManager::createMenu(Constants::M_PROJECTCONTEXT);
    Core::ActionContainer *msubProjectContextMenu =
        Core::ActionManager::createMenu(Constants::M_SUBPROJECTCONTEXT);
    Core::ActionContainer *mfolderContextMenu =
        Core::ActionManager::createMenu(Constants::M_FOLDERCONTEXT);
    Core::ActionContainer *mfileContextMenu =
        Core::ActionManager::createMenu(Constants::M_FILECONTEXT);

    d->m_sessionContextMenu = msessionContextMenu->menu();
    d->m_projectMenu = mprojectContextMenu->menu();
    d->m_subProjectMenu = msubProjectContextMenu->menu();
    d->m_folderMenu = mfolderContextMenu->menu();
    d->m_fileMenu = mfileContextMenu->menu();

    Core::ActionContainer *mfile =
        Core::ActionManager::actionContainer(Core::Constants::M_FILE);
    Core::ActionContainer *menubar =
        Core::ActionManager::actionContainer(Core::Constants::MENU_BAR);

    // build menu
    Core::ActionContainer *mbuild =
        Core::ActionManager::createMenu(Constants::M_BUILDPROJECT);
    mbuild->menu()->setTitle(tr("&Build"));
    menubar->addMenu(mbuild, Core::Constants::G_VIEW);

    // debug menu
    Core::ActionContainer *mdebug =
        Core::ActionManager::createMenu(Constants::M_DEBUG);
    mdebug->menu()->setTitle(tr("&Debug"));
    menubar->addMenu(mdebug, Core::Constants::G_VIEW);

    Core::ActionContainer *mstartdebugging =
        Core::ActionManager::createMenu(Constants::M_DEBUG_STARTDEBUGGING);
    mstartdebugging->menu()->setTitle(tr("&Start Debugging"));
    mdebug->addMenu(mstartdebugging, Core::Constants::G_DEFAULT_ONE);

    //
    // Groups
    //

    mbuild->appendGroup(Constants::G_BUILD_BUILD);
    mbuild->appendGroup(Constants::G_BUILD_DEPLOY);
    mbuild->appendGroup(Constants::G_BUILD_REBUILD);
    mbuild->appendGroup(Constants::G_BUILD_CLEAN);
    mbuild->appendGroup(Constants::G_BUILD_CANCEL);
    mbuild->appendGroup(Constants::G_BUILD_RUN);

    msessionContextMenu->appendGroup(Constants::G_SESSION_BUILD);
    msessionContextMenu->appendGroup(Constants::G_SESSION_REBUILD);
    msessionContextMenu->appendGroup(Constants::G_SESSION_FILES);
    msessionContextMenu->appendGroup(Constants::G_SESSION_OTHER);
    msessionContextMenu->appendGroup(Constants::G_PROJECT_TREE);

    mprojectContextMenu->appendGroup(Constants::G_PROJECT_FIRST);
    mprojectContextMenu->appendGroup(Constants::G_PROJECT_BUILD);
    mprojectContextMenu->appendGroup(Constants::G_PROJECT_RUN);
    mprojectContextMenu->appendGroup(Constants::G_PROJECT_REBUILD);
    mprojectContextMenu->appendGroup(Constants::G_PROJECT_FILES);
    mprojectContextMenu->appendGroup(Constants::G_PROJECT_LAST);
    mprojectContextMenu->appendGroup(Constants::G_PROJECT_TREE);

    msubProjectContextMenu->appendGroup(Constants::G_PROJECT_FIRST);
    msubProjectContextMenu->appendGroup(Constants::G_PROJECT_BUILD);
    msubProjectContextMenu->appendGroup(Constants::G_PROJECT_RUN);
    msubProjectContextMenu->appendGroup(Constants::G_PROJECT_FILES);
    msubProjectContextMenu->appendGroup(Constants::G_PROJECT_LAST);
    msubProjectContextMenu->appendGroup(Constants::G_PROJECT_TREE);

    Core::ActionContainer *runMenu = Core::ActionManager::createMenu(Constants::RUNMENUCONTEXTMENU);
    runMenu->setOnAllDisabledBehavior(Core::ActionContainer::Hide);
    QIcon runIcon = QIcon(QLatin1String(Constants::ICON_RUN));
    runIcon.addFile(QLatin1String(Constants::ICON_RUN_SMALL));
    runMenu->menu()->setIcon(runIcon);
    runMenu->menu()->setTitle(tr("Run"));
    msubProjectContextMenu->addMenu(runMenu, ProjectExplorer::Constants::G_PROJECT_RUN);

    mfolderContextMenu->appendGroup(Constants::G_FOLDER_FILES);
    mfolderContextMenu->appendGroup(Constants::G_FOLDER_OTHER);
    mfolderContextMenu->appendGroup(Constants::G_FOLDER_CONFIG);
    mfolderContextMenu->appendGroup(Constants::G_PROJECT_TREE);

    mfileContextMenu->appendGroup(Constants::G_FILE_OPEN);
    mfileContextMenu->appendGroup(Constants::G_FILE_OTHER);
    mfileContextMenu->appendGroup(Constants::G_FILE_CONFIG);
    mfileContextMenu->appendGroup(Constants::G_PROJECT_TREE);
    // "open with" submenu
    Core::ActionContainer * const openWith =
            Core::ActionManager::createMenu(ProjectExplorer::Constants::M_OPENFILEWITHCONTEXT);
    openWith->setOnAllDisabledBehavior(Core::ActionContainer::Show);
    d->m_openWithMenu = openWith->menu();
    d->m_openWithMenu->setTitle(tr("Open With"));

    connect(d->m_openWithMenu, SIGNAL(triggered(QAction*)),
            Core::DocumentManager::instance(), SLOT(slotExecuteOpenWithMenuAction(QAction*)));

    //
    // Separators
    //

    Core::Command *cmd;

    msessionContextMenu->addSeparator(projecTreeContext, Constants::G_SESSION_REBUILD);

    msessionContextMenu->addSeparator(projecTreeContext, Constants::G_SESSION_FILES);
    mprojectContextMenu->addSeparator(projecTreeContext, Constants::G_PROJECT_FILES);
    msubProjectContextMenu->addSeparator(projecTreeContext, Constants::G_PROJECT_FILES);
    mfile->addSeparator(globalcontext, Core::Constants::G_FILE_PROJECT);
    mbuild->addSeparator(globalcontext, Constants::G_BUILD_REBUILD);
    msessionContextMenu->addSeparator(globalcontext, Constants::G_SESSION_OTHER);
    mbuild->addSeparator(globalcontext, Constants::G_BUILD_CANCEL);
    mbuild->addSeparator(globalcontext, Constants::G_BUILD_RUN);
    mprojectContextMenu->addSeparator(globalcontext, Constants::G_PROJECT_REBUILD);

    //
    // Actions
    //

    // new action
    d->m_newAction = new QAction(tr("New Project..."), this);
    cmd = Core::ActionManager::registerAction(d->m_newAction, Constants::NEWPROJECT, globalcontext);
    cmd->setDefaultKeySequence(QKeySequence(tr("Ctrl+Shift+N")));
    msessionContextMenu->addAction(cmd, Constants::G_SESSION_FILES);

    // open action
    d->m_loadAction = new QAction(tr("Load Project..."), this);
    cmd = Core::ActionManager::registerAction(d->m_loadAction, Constants::LOAD, globalcontext);
#ifndef Q_OS_MAC
    cmd->setDefaultKeySequence(QKeySequence(tr("Ctrl+Shift+O")));
#endif
    msessionContextMenu->addAction(cmd, Constants::G_SESSION_FILES);

    // Default open action
    d->m_openFileAction = new QAction(tr("Open File"), this);
    cmd = Core::ActionManager::registerAction(d->m_openFileAction, ProjectExplorer::Constants::OPENFILE,
                       projecTreeContext);
    mfileContextMenu->addAction(cmd, Constants::G_FILE_OPEN);

    d->m_searchOnFileSystem = new QAction(FolderNavigationWidget::msgFindOnFileSystem(), this);
    cmd = Core::ActionManager::registerAction(d->m_searchOnFileSystem, ProjectExplorer::Constants::SEARCHONFILESYSTEM, projecTreeContext);
    mfolderContextMenu->addAction(cmd, Constants::G_FOLDER_CONFIG);
    msubProjectContextMenu->addAction(cmd, Constants::G_PROJECT_LAST);
    mprojectContextMenu->addAction(cmd, Constants::G_PROJECT_LAST);

    d->m_showInGraphicalShell = new QAction(Core::FileUtils::msgGraphicalShellAction(), this);
    cmd = Core::ActionManager::registerAction(d->m_showInGraphicalShell, ProjectExplorer::Constants::SHOWINGRAPHICALSHELL,
                       projecTreeContext);
    mfileContextMenu->addAction(cmd, Constants::G_FILE_OPEN);
    mfolderContextMenu->addAction(cmd, Constants::G_FOLDER_FILES);

    d->m_openTerminalHere = new QAction(Core::FileUtils::msgTerminalAction(), this);
    cmd = Core::ActionManager::registerAction(d->m_openTerminalHere, ProjectExplorer::Constants::OPENTERMIANLHERE,
                       projecTreeContext);
    mfileContextMenu->addAction(cmd, Constants::G_FILE_OPEN);
    mfolderContextMenu->addAction(cmd, Constants::G_FOLDER_FILES);

    // Open With menu
    mfileContextMenu->addMenu(openWith, ProjectExplorer::Constants::G_FILE_OPEN);

    // recent projects menu
    Core::ActionContainer *mrecent =
        Core::ActionManager::createMenu(Constants::M_RECENTPROJECTS);
    mrecent->menu()->setTitle(tr("Recent P&rojects"));
    mrecent->setOnAllDisabledBehavior(Core::ActionContainer::Show);
    mfile->addMenu(mrecent, Core::Constants::G_FILE_OPEN);
    connect(mfile->menu(), SIGNAL(aboutToShow()),
        this, SLOT(updateRecentProjectMenu()));

    // session menu
    Core::ActionContainer *msession = Core::ActionManager::createMenu(Constants::M_SESSION);
    msession->menu()->setTitle(tr("Sessions"));
    msession->setOnAllDisabledBehavior(Core::ActionContainer::Show);
    mfile->addMenu(msession, Core::Constants::G_FILE_OPEN);
    d->m_sessionMenu = msession->menu();
    connect(mfile->menu(), SIGNAL(aboutToShow()),
            this, SLOT(updateSessionMenu()));

    // session manager action
    d->m_sessionManagerAction = new QAction(tr("Session Manager..."), this);
    cmd = Core::ActionManager::registerAction(d->m_sessionManagerAction, Constants::NEWSESSION, globalcontext);
    mfile->addAction(cmd, Core::Constants::G_FILE_OPEN);
    cmd->setDefaultKeySequence(QKeySequence());


    // XXX same action?
    // unload action
    d->m_unloadAction = new Utils::ParameterAction(tr("Close Project"), tr("Close Project \"%1\""),
                                                      Utils::ParameterAction::EnabledWithParameter, this);
    cmd = Core::ActionManager::registerAction(d->m_unloadAction, Constants::UNLOAD, globalcontext);
    cmd->setAttribute(Core::Command::CA_UpdateText);
    cmd->setDescription(d->m_unloadAction->text());
    mfile->addAction(cmd, Core::Constants::G_FILE_PROJECT);

    // unload session action
    d->m_closeAllProjects = new QAction(tr("Close All Projects and Editors"), this);
    cmd = Core::ActionManager::registerAction(d->m_closeAllProjects, Constants::CLEARSESSION, globalcontext);
    mfile->addAction(cmd, Core::Constants::G_FILE_PROJECT);
    msessionContextMenu->addAction(cmd, Constants::G_SESSION_FILES);

    // build session action
    QIcon buildIcon = QIcon(QLatin1String(Constants::ICON_BUILD));
    buildIcon.addFile(QLatin1String(Constants::ICON_BUILD_SMALL));
    d->m_buildSessionAction = new QAction(buildIcon, tr("Build All"), this);
    cmd = Core::ActionManager::registerAction(d->m_buildSessionAction, Constants::BUILDSESSION, globalcontext);
    cmd->setDefaultKeySequence(QKeySequence(tr("Ctrl+Shift+B")));
    mbuild->addAction(cmd, Constants::G_BUILD_BUILD);
    msessionContextMenu->addAction(cmd, Constants::G_SESSION_BUILD);

    // deploy session
    d->m_deploySessionAction = new QAction(tr("Deploy All"), this);
    cmd = Core::ActionManager::registerAction(d->m_deploySessionAction, Constants::DEPLOYSESSION, globalcontext);
    mbuild->addAction(cmd, Constants::G_BUILD_DEPLOY);
    msessionContextMenu->addAction(cmd, Constants::G_SESSION_BUILD);

    // rebuild session action
    QIcon rebuildIcon = QIcon(QLatin1String(Constants::ICON_REBUILD));
    rebuildIcon.addFile(QLatin1String(Constants::ICON_REBUILD_SMALL));
    d->m_rebuildSessionAction = new QAction(rebuildIcon, tr("Rebuild All"), this);
    cmd = Core::ActionManager::registerAction(d->m_rebuildSessionAction, Constants::REBUILDSESSION, globalcontext);
    mbuild->addAction(cmd, Constants::G_BUILD_REBUILD);
    msessionContextMenu->addAction(cmd, Constants::G_SESSION_REBUILD);

    // clean session
    QIcon cleanIcon = QIcon(QLatin1String(Constants::ICON_CLEAN));
    cleanIcon.addFile(QLatin1String(Constants::ICON_CLEAN_SMALL));
    d->m_cleanSessionAction = new QAction(cleanIcon, tr("Clean All"), this);
    cmd = Core::ActionManager::registerAction(d->m_cleanSessionAction, Constants::CLEANSESSION, globalcontext);
    mbuild->addAction(cmd, Constants::G_BUILD_CLEAN);
    msessionContextMenu->addAction(cmd, Constants::G_SESSION_REBUILD);

    // build action
    d->m_buildAction = new Utils::ParameterAction(tr("Build Project"), tr("Build Project \"%1\""),
                                                     Utils::ParameterAction::AlwaysEnabled, this);
    d->m_buildAction->setIcon(buildIcon);
    cmd = Core::ActionManager::registerAction(d->m_buildAction, Constants::BUILD, globalcontext);
    cmd->setAttribute(Core::Command::CA_UpdateText);
    cmd->setDescription(d->m_buildAction->text());
    cmd->setDefaultKeySequence(QKeySequence(tr("Ctrl+B")));
    mbuild->addAction(cmd, Constants::G_BUILD_BUILD);

    // Add to mode bar
    Core::ModeManager::addAction(cmd->action(), Constants::P_ACTION_BUILDPROJECT);

    // deploy action
    d->m_deployAction = new Utils::ParameterAction(tr("Deploy Project"), tr("Deploy Project \"%1\""),
                                                     Utils::ParameterAction::AlwaysEnabled, this);
    cmd = Core::ActionManager::registerAction(d->m_deployAction, Constants::DEPLOY, globalcontext);
    cmd->setAttribute(Core::Command::CA_UpdateText);
    cmd->setDescription(d->m_deployAction->text());
    mbuild->addAction(cmd, Constants::G_BUILD_DEPLOY);

    // rebuild action
    d->m_rebuildAction = new Utils::ParameterAction(tr("Rebuild Project"), tr("Rebuild Project \"%1\""),
                                                       Utils::ParameterAction::AlwaysEnabled, this);
    cmd = Core::ActionManager::registerAction(d->m_rebuildAction, Constants::REBUILD, globalcontext);
    cmd->setAttribute(Core::Command::CA_UpdateText);
    cmd->setDescription(d->m_rebuildAction->text());
    mbuild->addAction(cmd, Constants::G_BUILD_REBUILD);

    // clean action
    d->m_cleanAction = new Utils::ParameterAction(tr("Clean Project"), tr("Clean Project \"%1\""),
                                                     Utils::ParameterAction::AlwaysEnabled, this);
    cmd = Core::ActionManager::registerAction(d->m_cleanAction, Constants::CLEAN, globalcontext);
    cmd->setAttribute(Core::Command::CA_UpdateText);
    cmd->setDescription(d->m_cleanAction->text());
    mbuild->addAction(cmd, Constants::G_BUILD_CLEAN);

    // cancel build action
    QIcon stopIcon = QIcon(QLatin1String(Constants::ICON_STOP));
    stopIcon.addFile(QLatin1String(Constants::ICON_STOP_SMALL));
    d->m_cancelBuildAction = new QAction(stopIcon, tr("Cancel Build"), this);
    cmd = Core::ActionManager::registerAction(d->m_cancelBuildAction, Constants::CANCELBUILD, globalcontext);
    mbuild->addAction(cmd, Constants::G_BUILD_CANCEL);

    // run action
    d->m_runAction = new QAction(runIcon, tr("Run"), this);
    cmd = Core::ActionManager::registerAction(d->m_runAction, Constants::RUN, globalcontext);
    cmd->setAttribute(Core::Command::CA_UpdateText);

    cmd->setDefaultKeySequence(QKeySequence(tr("Ctrl+R")));
    mbuild->addAction(cmd, Constants::G_BUILD_RUN);

    Core::ModeManager::addAction(cmd->action(), Constants::P_ACTION_RUN);

    // Run without deployment action
    d->m_runWithoutDeployAction = new QAction(tr("Run Without Deployment"), this);
    cmd = Core::ActionManager::registerAction(d->m_runWithoutDeployAction, Constants::RUNWITHOUTDEPLOY, globalcontext);
    mbuild->addAction(cmd, Constants::G_BUILD_RUN);

    // Publish action
    d->m_publishAction = new Utils::ParameterAction(tr("Publish Project..."), tr("Publish Project \"%1\"..."),
                                                    Utils::ParameterAction::AlwaysEnabled, this);
    cmd = Core::ActionManager::registerAction(d->m_publishAction, Constants::PUBLISH, globalcontext);
    cmd->setAttribute(Core::Command::CA_UpdateText);
    cmd->setDescription(d->m_publishAction->text());
    mbuild->addAction(cmd, Constants::G_BUILD_RUN);

    // build action (context menu)
    d->m_buildActionContextMenu = new QAction(tr("Build"), this);
    cmd = Core::ActionManager::registerAction(d->m_buildActionContextMenu, Constants::BUILDCM, projecTreeContext);
    cmd->setAttribute(Core::Command::CA_UpdateText);
    mprojectContextMenu->addAction(cmd, Constants::G_PROJECT_BUILD);

    // rebuild action (context menu)
    d->m_rebuildActionContextMenu = new QAction(tr("Rebuild"), this);
    cmd = Core::ActionManager::registerAction(d->m_rebuildActionContextMenu, Constants::REBUILDCM, projecTreeContext);
    cmd->setAttribute(Core::Command::CA_UpdateText);
    mprojectContextMenu->addAction(cmd, Constants::G_PROJECT_REBUILD);

    // clean action (context menu)
    d->m_cleanActionContextMenu = new QAction(tr("Clean"), this);
    cmd = Core::ActionManager::registerAction(d->m_cleanActionContextMenu, Constants::CLEANCM, projecTreeContext);
    cmd->setAttribute(Core::Command::CA_UpdateText);
    mprojectContextMenu->addAction(cmd, Constants::G_PROJECT_REBUILD);

    // build without dependencies action
    d->m_buildProjectOnlyAction = new QAction(tr("Build Without Dependencies"), this);
    cmd = Core::ActionManager::registerAction(d->m_buildProjectOnlyAction, Constants::BUILDPROJECTONLY, globalcontext);

    // rebuild without dependencies action
    d->m_rebuildProjectOnlyAction = new QAction(tr("Rebuild Without Dependencies"), this);
    cmd = Core::ActionManager::registerAction(d->m_rebuildProjectOnlyAction, Constants::REBUILDPROJECTONLY, globalcontext);

    // deploy without dependencies action
    d->m_deployProjectOnlyAction = new QAction(tr("Deploy Without Dependencies"), this);
    cmd = Core::ActionManager::registerAction(d->m_deployProjectOnlyAction, Constants::DEPLOYPROJECTONLY, globalcontext);

    // clean without dependencies action
    d->m_cleanProjectOnlyAction = new QAction(tr("Clean Without Dependencies"), this);
    cmd = Core::ActionManager::registerAction(d->m_cleanProjectOnlyAction, Constants::CLEANPROJECTONLY, globalcontext);

    // deploy action (context menu)
    d->m_deployActionContextMenu = new QAction(tr("Deploy"), this);
    cmd = Core::ActionManager::registerAction(d->m_deployActionContextMenu, Constants::DEPLOYCM, projecTreeContext);
    mprojectContextMenu->addAction(cmd, Constants::G_PROJECT_RUN);

    d->m_runActionContextMenu = new QAction(runIcon, tr("Run"), this);
    cmd = Core::ActionManager::registerAction(d->m_runActionContextMenu, Constants::RUNCONTEXTMENU, projecTreeContext);
    mprojectContextMenu->addAction(cmd, Constants::G_PROJECT_RUN);
    msubProjectContextMenu->addAction(cmd, Constants::G_PROJECT_RUN);

    // add new file action
    d->m_addNewFileAction = new QAction(tr("Add New..."), this);
    cmd = Core::ActionManager::registerAction(d->m_addNewFileAction, ProjectExplorer::Constants::ADDNEWFILE,
                       projecTreeContext);
    mprojectContextMenu->addAction(cmd, Constants::G_PROJECT_FILES);
    msubProjectContextMenu->addAction(cmd, Constants::G_PROJECT_FILES);
    mfolderContextMenu->addAction(cmd, Constants::G_FOLDER_FILES);

    // add existing file action
    d->m_addExistingFilesAction = new QAction(tr("Add Existing Files..."), this);
    cmd = Core::ActionManager::registerAction(d->m_addExistingFilesAction, ProjectExplorer::Constants::ADDEXISTINGFILES,
                       projecTreeContext);
    mprojectContextMenu->addAction(cmd, Constants::G_PROJECT_FILES);
    msubProjectContextMenu->addAction(cmd, Constants::G_PROJECT_FILES);
    mfolderContextMenu->addAction(cmd, Constants::G_FOLDER_FILES);

    // new subproject action
    d->m_addNewSubprojectAction = new QAction(tr("New Subproject..."), this);
    cmd = Core::ActionManager::registerAction(d->m_addNewSubprojectAction, ProjectExplorer::Constants::ADDNEWSUBPROJECT,
                       projecTreeContext);
    mprojectContextMenu->addAction(cmd, Constants::G_PROJECT_FILES);
    msubProjectContextMenu->addAction(cmd, Constants::G_PROJECT_FILES);

    // unload project again, in right position
    mprojectContextMenu->addAction(Core::ActionManager::command(Constants::UNLOAD), Constants::G_PROJECT_LAST);

    // remove file action
    d->m_removeFileAction = new QAction(tr("Remove File..."), this);
    cmd = Core::ActionManager::registerAction(d->m_removeFileAction, ProjectExplorer::Constants::REMOVEFILE,
                       projecTreeContext);
    cmd->setDefaultKeySequence(QKeySequence::Delete);
    mfileContextMenu->addAction(cmd, Constants::G_FILE_OTHER);

    //: Remove project from parent profile (Project explorer view); will not physically delete any files.
    d->m_removeProjectAction = new QAction(tr("Remove Project..."), this);
    cmd = Core::ActionManager::registerAction(d->m_removeProjectAction, ProjectExplorer::Constants::REMOVEPROJECT,
                       projecTreeContext);
    msubProjectContextMenu->addAction(cmd, Constants::G_PROJECT_FILES);

    // delete file action
    d->m_deleteFileAction = new QAction(tr("Delete File..."), this);
    cmd = Core::ActionManager::registerAction(d->m_deleteFileAction, ProjectExplorer::Constants::DELETEFILE,
                             projecTreeContext);
    cmd->setDefaultKeySequence(QKeySequence::Delete);
    mfileContextMenu->addAction(cmd, Constants::G_FILE_OTHER);

    // renamefile action
    d->m_renameFileAction = new QAction(tr("Rename..."), this);
    cmd = Core::ActionManager::registerAction(d->m_renameFileAction, ProjectExplorer::Constants::RENAMEFILE,
                       projecTreeContext);
    mfileContextMenu->addAction(cmd, Constants::G_FILE_OTHER);
    // Not yet used by anyone, so hide for now
//    mfolder->addAction(cmd, Constants::G_FOLDER_FILES);
//    msubProject->addAction(cmd, Constants::G_FOLDER_FILES);
//    mproject->addAction(cmd, Constants::G_FOLDER_FILES);

    // set startup project action
    d->m_setStartupProjectAction = new Utils::ParameterAction(tr("Set as Active Project"),
                                                              tr("Set \"%1\" as Active Project"),
                                                              Utils::ParameterAction::AlwaysEnabled, this);
    cmd = Core::ActionManager::registerAction(d->m_setStartupProjectAction, ProjectExplorer::Constants::SETSTARTUP,
                             projecTreeContext);
    cmd->setAttribute(Core::Command::CA_UpdateText);
    cmd->setDescription(d->m_setStartupProjectAction->text());
    mprojectContextMenu->addAction(cmd, Constants::G_PROJECT_FIRST);

    // Collapse All.
    d->m_projectTreeCollapseAllAction = new QAction(tr("Collapse All"), this);
    cmd = Core::ActionManager::registerAction(d->m_projectTreeCollapseAllAction, Constants::PROJECTTREE_COLLAPSE_ALL,
                             projecTreeContext);
    const Core::Id treeGroup = Constants::G_PROJECT_TREE;
    mfileContextMenu->addSeparator(globalcontext, treeGroup);
    mfileContextMenu->addAction(cmd, treeGroup);
    msubProjectContextMenu->addSeparator(globalcontext, treeGroup);
    msubProjectContextMenu->addAction(cmd, treeGroup);
    mfolderContextMenu->addSeparator(globalcontext, treeGroup);
    mfolderContextMenu->addAction(cmd, treeGroup);
    mprojectContextMenu->addSeparator(globalcontext, treeGroup);
    mprojectContextMenu->addAction(cmd, treeGroup);
    msessionContextMenu->addSeparator(globalcontext, treeGroup);
    msessionContextMenu->addAction(cmd, treeGroup);

    // target selector
    d->m_projectSelectorAction = new QAction(this);
    d->m_projectSelectorAction->setCheckable(true);
    d->m_projectSelectorAction->setEnabled(false);
    QWidget *mainWindow = Core::ICore::mainWindow();
    d->m_targetSelector = new Internal::MiniProjectTargetSelector(d->m_projectSelectorAction, d->m_session, mainWindow);
    connect(d->m_projectSelectorAction, SIGNAL(triggered()), d->m_targetSelector, SLOT(show()));
    Core::ModeManager::addProjectSelector(d->m_projectSelectorAction);

    d->m_projectSelectorActionMenu = new QAction(this);
    d->m_projectSelectorActionMenu->setEnabled(false);
    d->m_projectSelectorActionMenu->setText(tr("Open Build and Run Kit Selector..."));
    connect(d->m_projectSelectorActionMenu, SIGNAL(triggered()), d->m_targetSelector, SLOT(toggleVisible()));
    cmd = Core::ActionManager::registerAction(d->m_projectSelectorActionMenu, ProjectExplorer::Constants::SELECTTARGET,
                       globalcontext);
    mbuild->addAction(cmd, Constants::G_BUILD_RUN);

    d->m_projectSelectorActionQuick = new QAction(this);
    d->m_projectSelectorActionQuick->setEnabled(false);
    d->m_projectSelectorActionQuick->setText(tr("Quick Switch Kit Selector"));
    connect(d->m_projectSelectorActionQuick, SIGNAL(triggered()), d->m_targetSelector, SLOT(nextOrShow()));
    cmd = Core::ActionManager::registerAction(d->m_projectSelectorActionQuick, ProjectExplorer::Constants::SELECTTARGETQUICK, globalcontext);
    cmd->setDefaultKeySequence(QKeySequence(tr("Ctrl+T")));

    connect(Core::ICore::instance(), SIGNAL(saveSettingsRequested()),
        this, SLOT(savePersistentSettings()));

    addAutoReleasedObject(new ProjectTreeWidgetFactory);
    addAutoReleasedObject(new FolderNavigationWidgetFactory);
    addAutoReleasedObject(new DeployConfigurationFactory);

    QSettings *s = Core::ICore::settings();
    const QStringList fileNames =
            s->value(QLatin1String("ProjectExplorer/RecentProjects/FileNames")).toStringList();
    const QStringList displayNames =
            s->value(QLatin1String("ProjectExplorer/RecentProjects/DisplayNames")).toStringList();
    if (fileNames.size() == displayNames.size()) {
        for (int i = 0; i < fileNames.size(); ++i) {
            if (QFileInfo(fileNames.at(i)).isFile())
                d->m_recentProjects.append(qMakePair(fileNames.at(i), displayNames.at(i)));
        }
    }

    d->m_projectExplorerSettings.buildBeforeDeploy =
            s->value(QLatin1String("ProjectExplorer/Settings/BuildBeforeDeploy"), true).toBool();
    d->m_projectExplorerSettings.deployBeforeRun =
            s->value(QLatin1String("ProjectExplorer/Settings/DeployBeforeRun"), true).toBool();
    d->m_projectExplorerSettings.saveBeforeBuild =
            s->value(QLatin1String("ProjectExplorer/Settings/SaveBeforeBuild"), false).toBool();
    d->m_projectExplorerSettings.showCompilerOutput =
            s->value(QLatin1String("ProjectExplorer/Settings/ShowCompilerOutput"), false).toBool();
    d->m_projectExplorerSettings.showRunOutput =
            s->value(QLatin1String("ProjectExplorer/Settings/ShowRunOutput"), true).toBool();
    d->m_projectExplorerSettings.showDebugOutput =
            s->value(QLatin1String("ProjectExplorer/Settings/ShowDebugOutput"), false).toBool();
    d->m_projectExplorerSettings.cleanOldAppOutput =
            s->value(QLatin1String("ProjectExplorer/Settings/CleanOldAppOutput"), false).toBool();
    d->m_projectExplorerSettings.mergeStdErrAndStdOut =
            s->value(QLatin1String("ProjectExplorer/Settings/MergeStdErrAndStdOut"), false).toBool();
    d->m_projectExplorerSettings.wrapAppOutput =
            s->value(QLatin1String("ProjectExplorer/Settings/WrapAppOutput"), true).toBool();
    d->m_projectExplorerSettings.useJom =
            s->value(QLatin1String("ProjectExplorer/Settings/UseJom"), true).toBool();
    d->m_projectExplorerSettings.autorestoreLastSession =
            s->value(QLatin1String("ProjectExplorer/Settings/AutoRestoreLastSession"), false).toBool();
    d->m_projectExplorerSettings.prompToStopRunControl =
            s->value(QLatin1String("ProjectExplorer/Settings/PromptToStopRunControl"), false).toBool();
    d->m_projectExplorerSettings.maxAppOutputLines =
            s->value(QLatin1String("ProjectExplorer/Settings/MaxAppOutputLines"), 100000).toInt();
    d->m_projectExplorerSettings.environmentId =
            QUuid(s->value(QLatin1String("ProjectExplorer/Settings/EnvironmentId")).toString());
    if (d->m_projectExplorerSettings.environmentId.isNull())
        d->m_projectExplorerSettings.environmentId = QUuid::createUuid();

    connect(d->m_sessionManagerAction, SIGNAL(triggered()), this, SLOT(showSessionManager()));
    connect(d->m_newAction, SIGNAL(triggered()), this, SLOT(newProject()));
    connect(d->m_loadAction, SIGNAL(triggered()), this, SLOT(loadAction()));
    connect(d->m_buildProjectOnlyAction, SIGNAL(triggered()), this, SLOT(buildProjectOnly()));
    connect(d->m_buildAction, SIGNAL(triggered()), this, SLOT(buildProject()));
    connect(d->m_buildActionContextMenu, SIGNAL(triggered()), this, SLOT(buildProjectContextMenu()));
    connect(d->m_buildSessionAction, SIGNAL(triggered()), this, SLOT(buildSession()));
    connect(d->m_rebuildProjectOnlyAction, SIGNAL(triggered()), this, SLOT(rebuildProjectOnly()));
    connect(d->m_rebuildAction, SIGNAL(triggered()), this, SLOT(rebuildProject()));
    connect(d->m_rebuildActionContextMenu, SIGNAL(triggered()), this, SLOT(rebuildProjectContextMenu()));
    connect(d->m_rebuildSessionAction, SIGNAL(triggered()), this, SLOT(rebuildSession()));
    connect(d->m_deployProjectOnlyAction, SIGNAL(triggered()), this, SLOT(deployProjectOnly()));
    connect(d->m_deployAction, SIGNAL(triggered()), this, SLOT(deployProject()));
    connect(d->m_deployActionContextMenu, SIGNAL(triggered()), this, SLOT(deployProjectContextMenu()));
    connect(d->m_deploySessionAction, SIGNAL(triggered()), this, SLOT(deploySession()));
    connect(d->m_publishAction, SIGNAL(triggered()), this, SLOT(publishProject()));
    connect(d->m_cleanProjectOnlyAction, SIGNAL(triggered()), this, SLOT(cleanProjectOnly()));
    connect(d->m_cleanAction, SIGNAL(triggered()), this, SLOT(cleanProject()));
    connect(d->m_cleanActionContextMenu, SIGNAL(triggered()), this, SLOT(cleanProjectContextMenu()));
    connect(d->m_cleanSessionAction, SIGNAL(triggered()), this, SLOT(cleanSession()));
    connect(d->m_runAction, SIGNAL(triggered()), this, SLOT(runProject()));
    connect(d->m_runActionContextMenu, SIGNAL(triggered()), this, SLOT(runProjectContextMenu()));
    connect(d->m_runWithoutDeployAction, SIGNAL(triggered()), this, SLOT(runProjectWithoutDeploy()));
    connect(d->m_cancelBuildAction, SIGNAL(triggered()), this, SLOT(cancelBuild()));
    connect(d->m_unloadAction, SIGNAL(triggered()), this, SLOT(unloadProject()));
    connect(d->m_closeAllProjects, SIGNAL(triggered()), this, SLOT(closeAllProjects()));
    connect(d->m_addNewFileAction, SIGNAL(triggered()), this, SLOT(addNewFile()));
    connect(d->m_addExistingFilesAction, SIGNAL(triggered()), this, SLOT(addExistingFiles()));
    connect(d->m_addNewSubprojectAction, SIGNAL(triggered()), this, SLOT(addNewSubproject()));
    connect(d->m_removeProjectAction, SIGNAL(triggered()), this, SLOT(removeProject()));
    connect(d->m_openFileAction, SIGNAL(triggered()), this, SLOT(openFile()));
    connect(d->m_searchOnFileSystem, SIGNAL(triggered()), this, SLOT(searchOnFileSystem()));
    connect(d->m_showInGraphicalShell, SIGNAL(triggered()), this, SLOT(showInGraphicalShell()));
    connect(d->m_openTerminalHere, SIGNAL(triggered()), this, SLOT(openTerminalHere()));
    connect(d->m_removeFileAction, SIGNAL(triggered()), this, SLOT(removeFile()));
    connect(d->m_deleteFileAction, SIGNAL(triggered()), this, SLOT(deleteFile()));
    connect(d->m_renameFileAction, SIGNAL(triggered()), this, SLOT(renameFile()));
    connect(d->m_setStartupProjectAction, SIGNAL(triggered()), this, SLOT(setStartupProject()));

    connect(this, SIGNAL(updateRunActions()), this, SLOT(slotUpdateRunActions()));
    connect(this, SIGNAL(settingsChanged()), this, SLOT(updateRunWithoutDeployMenu()));

    d->m_buildManager = new BuildManager(this, d->m_cancelBuildAction);
    connect(d->m_buildManager, SIGNAL(buildStateChanged(ProjectExplorer::Project*)),
            this, SLOT(buildStateChanged(ProjectExplorer::Project*)));
    connect(d->m_buildManager, SIGNAL(buildQueueFinished(bool)),
            this, SLOT(buildQueueFinished(bool)));

    updateActions();

    connect(Core::ICore::instance(), SIGNAL(coreAboutToOpen()),
            this, SLOT(determineSessionToRestoreAtStartup()));
    connect(Core::ICore::instance(), SIGNAL(coreOpened()), this, SLOT(restoreSession()));

    updateWelcomePage();

    Core::VariableManager *vm = Core::VariableManager::instance();
    vm->registerVariable(Constants::VAR_CURRENTPROJECT_FILEPATH,
        tr("Full path of the current project's main file, including file name."));
    vm->registerVariable(Constants::VAR_CURRENTPROJECT_PATH,
        tr("Full path of the current project's main file, excluding file name."));
    vm->registerVariable(Constants::VAR_CURRENTPROJECT_BUILDPATH,
        tr("Full build path of the current project's active build configuration."));
    vm->registerVariable(Constants::VAR_CURRENTPROJECT_NAME, tr("The current project's name."));
    vm->registerVariable(Constants::VAR_CURRENTKIT_NAME, tr("The currently active kit's name."));
    vm->registerVariable(Constants::VAR_CURRENTKIT_FILESYSTEMNAME,
                         tr("The currently active kit's name in a filesystem friendly version."));
    vm->registerVariable(Constants::VAR_CURRENTKIT_ID, tr("The currently active kit's id."));
    vm->registerVariable(Constants::VAR_CURRENTBUILD_NAME, tr("The currently active build configuration's name."));
    vm->registerVariable(Constants::VAR_CURRENTBUILD_TYPE, tr("The currently active build configuration's type."));

    connect(vm, SIGNAL(variableUpdateRequested(QByteArray)),
            this, SLOT(updateVariable(QByteArray)));

    return true;
}

void ProjectExplorerPlugin::loadAction()
{
    if (debug)
        qDebug() << "ProjectExplorerPlugin::loadAction";


    QString dir = d->m_lastOpenDirectory;

    // for your special convenience, we preselect a pro file if it is
    // the current file
    if (Core::IEditor *editor = Core::EditorManager::currentEditor()) {
        if (const Core::IDocument *document= editor->document()) {
            const QString fn = document->fileName();
            const bool isProject = d->m_profileMimeTypes.contains(document->mimeType());
            dir = isProject ? fn : QFileInfo(fn).absolutePath();
        }
    }

    QString filename = QFileDialog::getOpenFileName(0, tr("Load Project"),
                                                    dir,
                                                    d->m_projectFilterString);
    if (filename.isEmpty())
        return;
    QString errorMessage;
    openProject(filename, &errorMessage);

    if (!errorMessage.isEmpty())
        QMessageBox::critical(Core::ICore::mainWindow(), tr("Failed to open project"), errorMessage);
    updateActions();
}

void ProjectExplorerPlugin::unloadProject()
{
    if (debug)
        qDebug() << "ProjectExplorerPlugin::unloadProject";

    if (buildManager()->isBuilding(d->m_currentProject)) {
        QMessageBox box;
        QPushButton *closeAnyway = box.addButton(tr("Cancel Build && Unload"), QMessageBox::AcceptRole);
        QPushButton *cancelClose = box.addButton(tr("Do Not Unload"), QMessageBox::RejectRole);
        box.setDefaultButton(cancelClose);
        box.setWindowTitle(tr("Unload Project %1?").arg(d->m_currentProject->displayName()));
        box.setText(tr("The project %1 is currently being built.").arg(d->m_currentProject->displayName()));
        box.setInformativeText(tr("Do you want to cancel the build process and unload the project anyway?"));
        box.exec();
        if (box.clickedButton() != closeAnyway)
            return;
        buildManager()->cancel();
    }

    Core::IDocument *document = d->m_currentProject->document();

    if (!document || document->fileName().isEmpty()) //nothing to save?
        return;

    QList<Core::IDocument*> documentsToSave;
    documentsToSave << document;
    bool success = false;
    if (document->isFileReadOnly())
        success = Core::DocumentManager::saveModifiedDocuments(documentsToSave).isEmpty();
    else
        success = Core::DocumentManager::saveModifiedDocumentsSilently(documentsToSave).isEmpty();

    if (!success)
        return;

    addToRecentProjects(document->fileName(), d->m_currentProject->displayName());
    unloadProject(d->m_currentProject);
}

void ProjectExplorerPlugin::unloadProject(Project *project)
{
    d->m_session->removeProject(project);
    updateActions();
}

void ProjectExplorerPlugin::closeAllProjects()
{
    if (debug)
        qDebug() << "ProjectExplorerPlugin::closeAllProject";

    if (!Core::ICore::editorManager()->closeAllEditors())
        return; // Action has been cancelled

    d->m_session->closeAllProjects();
    updateActions();

    Core::ModeManager::activateMode(Core::Id(Core::Constants::MODE_WELCOME));
}

void ProjectExplorerPlugin::extensionsInitialized()
{
    d->m_proWindow->extensionsInitialized();
    d->m_fileFactories = ProjectFileFactory::createFactories(&d->m_projectFilterString);
    foreach (ProjectFileFactory *pf, d->m_fileFactories) {
        d->m_profileMimeTypes += pf->mimeTypes();
        addAutoReleasedObject(pf);
    }
    d->m_buildManager->extensionsInitialized();

    // Register KitInformation:
    // Only do this now to make sure all device factories were properly initialized.
    KitManager::instance()->registerKitInformation(new SysRootKitInformation);
    KitManager::instance()->registerKitInformation(new DeviceKitInformation);
    KitManager::instance()->registerKitInformation(new DeviceTypeKitInformation);
    KitManager::instance()->registerKitInformation(new ToolChainKitInformation);

    DeviceManager *dm = DeviceManager::instance();
    if (dm->find(Core::Id(Constants::DESKTOP_DEVICE_ID)).isNull())
        DeviceManager::instance()->addDevice(IDevice::Ptr(new DesktopDevice));
}

void ProjectExplorerPlugin::loadCustomWizards()
{
    // Add custom wizards, for which other plugins might have registered
    // class factories
    static bool firstTime = true;
    if (firstTime) {
        firstTime = false;
        foreach (Core::IWizard *cpw, ProjectExplorer::CustomWizard::createWizards())
            addAutoReleasedObject(cpw);
    }
}

void ProjectExplorerPlugin::updateVariable(const QByteArray &variable)
{
    if (variable == Constants::VAR_CURRENTPROJECT_FILEPATH) {
        if (currentProject() && currentProject()->document()) {
            Core::VariableManager::instance()->insert(variable,
                                                      currentProject()->document()->fileName());
        } else {
            Core::VariableManager::instance()->remove(variable);
        }
    } else if (variable == Constants::VAR_CURRENTPROJECT_PATH) {
        if (currentProject() && currentProject()->document()) {
            Core::VariableManager::instance()->insert(variable,
                                                      QFileInfo(currentProject()->document()->fileName()).path());
        } else {
            Core::VariableManager::instance()->remove(variable);
        }
    } else if (variable == Constants::VAR_CURRENTPROJECT_BUILDPATH) {
        if (currentProject() && currentProject()->activeTarget() && currentProject()->activeTarget()->activeBuildConfiguration()) {
            Core::VariableManager::instance()->insert(variable,
                                                      currentProject()->activeTarget()->activeBuildConfiguration()->buildDirectory());
        } else {
            Core::VariableManager::instance()->remove(variable);
        }
    } else if (variable == Constants::VAR_CURRENTPROJECT_NAME) {
        if (currentProject())
            Core::VariableManager::instance()->insert(variable, currentProject()->displayName());
        else
            Core::VariableManager::instance()->remove(variable);
    } else if (variable == Constants::VAR_CURRENTKIT_NAME) {
        if (currentProject() && currentProject()->activeTarget() && currentProject()->activeTarget()->kit())
            Core::VariableManager::instance()->insert(variable, currentProject()->activeTarget()->kit()->displayName());
        else
            Core::VariableManager::instance()->remove(variable);
    } else if (variable == Constants::VAR_CURRENTKIT_FILESYSTEMNAME) {
        if (currentProject() && currentProject()->activeTarget() && currentProject()->activeTarget()->kit())
            Core::VariableManager::instance()->insert(variable, currentProject()->activeTarget()->kit()->fileSystemFriendlyName());
        else
            Core::VariableManager::instance()->remove(variable);
    } else if (variable == Constants::VAR_CURRENTKIT_ID) {
        if (currentProject() && currentProject()->activeTarget() && currentProject()->activeTarget()->kit())
            Core::VariableManager::instance()->insert(variable, currentProject()->activeTarget()->kit()->id().toString());
        else
            Core::VariableManager::instance()->remove(variable);
    } else if (variable == Constants::VAR_CURRENTBUILD_NAME) {
        if (currentProject() && currentProject()->activeTarget() && currentProject()->activeTarget()->activeBuildConfiguration())
            Core::VariableManager::instance()->insert(variable, currentProject()->activeTarget()->activeBuildConfiguration()->displayName());
        else
            Core::VariableManager::instance()->remove(variable);
    } else if (variable == Constants::VAR_CURRENTBUILD_TYPE) {
        if (currentProject() && currentProject()->activeTarget() && currentProject()->activeTarget()->activeBuildConfiguration()) {
            BuildConfiguration::BuildType type = currentProject()->activeTarget()->activeBuildConfiguration()->buildType();
            QString typeString;
            if (type == BuildConfiguration::Debug)
                typeString = tr("debug");
            else if (type == BuildConfiguration::Release)
                typeString = tr("release");
            else
                typeString = tr("unknown");
            Core::VariableManager::instance()->insert(variable, typeString);
        } else {
            Core::VariableManager::instance()->remove(variable);
        }
    }
}

void ProjectExplorerPlugin::updateRunWithoutDeployMenu()
{
    d->m_runWithoutDeployAction->setVisible(d->m_projectExplorerSettings.deployBeforeRun);
}

ExtensionSystem::IPlugin::ShutdownFlag ProjectExplorerPlugin::aboutToShutdown()
{
    d->m_proWindow->aboutToShutdown(); // disconnect from session
    d->m_session->closeAllProjects();
    d->m_projectsMode = 0;
    d->m_shuttingDown = true;
    // Attempt to synchronously shutdown all run controls.
    // If that fails, fall back to asynchronous shutdown (Debugger run controls
    // might shutdown asynchronously).
    if (d->m_outputPane->closeTabs(AppOutputPane::CloseTabNoPrompt /* No prompt any more */))
        return SynchronousShutdown;
    connect(d->m_outputPane, SIGNAL(allRunControlsFinished()),
            this, SIGNAL(asynchronousShutdownFinished()));
    return AsynchronousShutdown;
}

void ProjectExplorerPlugin::newProject()
{
    if (debug)
        qDebug() << "ProjectExplorerPlugin::newProject";

    Core::ICore::showNewItemDialog(tr("New Project", "Title of dialog"),
                              Core::IWizard::wizardsOfKind(Core::IWizard::ProjectWizard));
    updateActions();
}

void ProjectExplorerPlugin::showSessionManager()
{
    if (debug)
        qDebug() << "ProjectExplorerPlugin::showSessionManager";

    if (d->m_session->isDefaultVirgin()) {
        // do not save new virgin default sessions
    } else {
        d->m_session->save();
    }
    SessionDialog sessionDialog(d->m_session, Core::ICore::mainWindow());
    sessionDialog.setAutoLoadSession(d->m_projectExplorerSettings.autorestoreLastSession);
    sessionDialog.exec();
    d->m_projectExplorerSettings.autorestoreLastSession = sessionDialog.autoLoadSession();

    updateActions();

    Core::IMode *welcomeMode = Core::ModeManager::mode(Core::Constants::MODE_WELCOME);
    if (Core::ModeManager::currentMode() == welcomeMode)
        updateWelcomePage();
}

void ProjectExplorerPlugin::setStartupProject(Project *project)
{
    if (debug)
        qDebug() << "ProjectExplorerPlugin::setStartupProject";

    if (!project)
        return;
    d->m_session->setStartupProject(project);
    updateActions();
}

void ProjectExplorerPlugin::publishProject()
{
    const Project * const project = d->m_session->startupProject();
    QTC_ASSERT(project, return);
    PublishingWizardSelectionDialog selectionDialog(project);
    if (selectionDialog.exec() == QDialog::Accepted) {
        QWizard * const publishingWizard
            = selectionDialog.createSelectedWizard();
        publishingWizard->exec();
        delete publishingWizard;
    }
}

void ProjectExplorerPlugin::savePersistentSettings()
{
    if (debug)
        qDebug()<<"ProjectExplorerPlugin::savePersistentSettings()";

    if (d->m_shuttingDown)
        return;

    if (!d->m_session->loadingSession())  {
        foreach (Project *pro, d->m_session->projects())
            pro->saveSettings();

        if (d->m_session->isDefaultVirgin()) {
            // do not save new virgin default sessions
        } else {
            d->m_session->save();
        }
    }

    QSettings *s = Core::ICore::settings();
    if (s) {
        s->setValue(QLatin1String("ProjectExplorer/StartupSession"), d->m_session->activeSession());
        s->remove(QLatin1String("ProjectExplorer/RecentProjects/Files"));

        QStringList fileNames;
        QStringList displayNames;
        QList<QPair<QString, QString> >::const_iterator it, end;
        end = d->m_recentProjects.constEnd();
        for (it = d->m_recentProjects.constBegin(); it != end; ++it) {
            fileNames << (*it).first;
            displayNames << (*it).second;
        }

        s->setValue(QLatin1String("ProjectExplorer/RecentProjects/FileNames"), fileNames);
        s->setValue(QLatin1String("ProjectExplorer/RecentProjects/DisplayNames"), displayNames);

        s->setValue(QLatin1String("ProjectExplorer/Settings/BuildBeforeDeploy"), d->m_projectExplorerSettings.buildBeforeDeploy);
        s->setValue(QLatin1String("ProjectExplorer/Settings/DeployBeforeRun"), d->m_projectExplorerSettings.deployBeforeRun);
        s->setValue(QLatin1String("ProjectExplorer/Settings/SaveBeforeBuild"), d->m_projectExplorerSettings.saveBeforeBuild);
        s->setValue(QLatin1String("ProjectExplorer/Settings/ShowCompilerOutput"), d->m_projectExplorerSettings.showCompilerOutput);
        s->setValue(QLatin1String("ProjectExplorer/Settings/ShowRunOutput"), d->m_projectExplorerSettings.showRunOutput);
        s->setValue(QLatin1String("ProjectExplorer/Settings/ShowDebugOutput"), d->m_projectExplorerSettings.showDebugOutput);
        s->setValue(QLatin1String("ProjectExplorer/Settings/CleanOldAppOutput"), d->m_projectExplorerSettings.cleanOldAppOutput);
        s->setValue(QLatin1String("ProjectExplorer/Settings/MergeStdErrAndStdOut"), d->m_projectExplorerSettings.mergeStdErrAndStdOut);
        s->setValue(QLatin1String("ProjectExplorer/Settings/WrapAppOutput"), d->m_projectExplorerSettings.wrapAppOutput);
        s->setValue(QLatin1String("ProjectExplorer/Settings/UseJom"), d->m_projectExplorerSettings.useJom);
        s->setValue(QLatin1String("ProjectExplorer/Settings/AutoRestoreLastSession"), d->m_projectExplorerSettings.autorestoreLastSession);
        s->setValue(QLatin1String("ProjectExplorer/Settings/PromptToStopRunControl"), d->m_projectExplorerSettings.prompToStopRunControl);
        s->setValue(QLatin1String("ProjectExplorer/Settings/MaxAppOutputLines"), d->m_projectExplorerSettings.maxAppOutputLines);
        s->setValue(QLatin1String("ProjectExplorer/Settings/EnvironmentId"), d->m_projectExplorerSettings.environmentId.toString());
    }
}

void ProjectExplorerPlugin::openProjectWelcomePage(const QString &fileName)
{
    QString errorMessage;
    openProject(fileName, &errorMessage);
    if (!errorMessage.isEmpty())
        QMessageBox::critical(Core::ICore::mainWindow(), tr("Failed to Open Project"), errorMessage);
}

Project *ProjectExplorerPlugin::openProject(const QString &fileName, QString *errorString)
{
    if (debug)
        qDebug() << "ProjectExplorerPlugin::openProject";

    QList<Project *> list = openProjects(QStringList() << fileName, errorString);
    if (!list.isEmpty()) {
        addToRecentProjects(fileName, list.first()->displayName());
        d->m_session->setStartupProject(list.first());
        return list.first();
    }
    return 0;
}

static inline QList<IProjectManager*> allProjectManagers()
{
    return ExtensionSystem::PluginManager::getObjects<IProjectManager>();
}

QList<Project *> ProjectExplorerPlugin::openProjects(const QStringList &fileNames, QString *errorString)
{
    if (debug)
        qDebug() << "ProjectExplorerPlugin - opening projects " << fileNames;

    const QList<IProjectManager*> projectManagers = allProjectManagers();

    QList<Project*> openedPro;
    foreach (const QString &fileName, fileNames) {
        if (const Core::MimeType mt = Core::ICore::mimeDatabase()->findByFile(QFileInfo(fileName))) {
            foreach (IProjectManager *manager, projectManagers) {
                if (manager->mimeType() == mt.type()) {
                    QString tmp;
                    if (Project *pro = manager->openProject(fileName, &tmp)) {
                        if (pro->restoreSettings()) {
                            connect(pro, SIGNAL(fileListChanged()), this, SIGNAL(fileListChanged()));
                            d->m_session->addProject(pro);
                            // Make sure we always have a current project / node
                            if (!d->m_currentProject && !openedPro.isEmpty())
                                setCurrentNode(pro->rootProjectNode());
                            openedPro += pro;
                        } else {
                            delete pro;
                        }
                    }
                    if (errorString) {
                        if (!errorString->isEmpty() && !tmp.isEmpty())
                            errorString->append(QLatin1Char('\n'));
                        errorString->append(tmp);
                    }
                    d->m_session->reportProjectLoadingProgress();
                    break;
                }
            }
        }
    }
    updateActions();

    bool switchToProjectsMode = false;
    foreach (Project *p, openedPro) {
        if (p->needsConfiguration()) {
            switchToProjectsMode = true;
            break;
        }
    }

    if (!openedPro.isEmpty()) {
        if (switchToProjectsMode)
            Core::ModeManager::activateMode(ProjectExplorer::Constants::MODE_SESSION);
        else
            Core::ModeManager::activateMode(Core::Constants::MODE_EDIT);
        Core::ModeManager::setFocusToCurrentMode();
    }

    return openedPro;
}

Project *ProjectExplorerPlugin::currentProject()
{
    Project *project = m_instance->d->m_currentProject;
    if (debug) {
        if (project)
            qDebug() << "ProjectExplorerPlugin::currentProject returns " << project->displayName();
        else
            qDebug() << "ProjectExplorerPlugin::currentProject returns 0";
    }
    return project;
}

Node *ProjectExplorerPlugin::currentNode() const
{
    return d->m_currentNode;
}

void ProjectExplorerPlugin::setCurrentFile(Project *project, const QString &filePath)
{
    setCurrent(project, filePath, 0);
}

void ProjectExplorerPlugin::setCurrentFile(const QString &filePath)
{
    Project *project = d->m_session->projectForFile(filePath);
    // If the file is not in any project, stay with the current project
    // e.g. on opening a git diff buffer, git log buffer, we don't change the project
    // I'm not 100% sure this is correct
    if (!project)
        project = d->m_currentProject;
    setCurrent(project, filePath, 0);
}

void ProjectExplorerPlugin::setCurrentNode(Node *node)
{
    setCurrent(d->m_session->projectForNode(node), QString(), node);
}

SessionManager *ProjectExplorerPlugin::session() const
{
    return d->m_session;
}

Project *ProjectExplorerPlugin::startupProject() const
{
    if (debug)
        qDebug() << "ProjectExplorerPlugin::startupProject";

    return d->m_session->startupProject();
}

void ProjectExplorerPlugin::updateWelcomePage()
{
    d->m_welcomePage->reloadWelcomeScreenData();
}

void ProjectExplorerPlugin::currentModeChanged(Core::IMode *mode, Core::IMode *oldMode)
{
    Q_UNUSED(oldMode);
    if (mode && mode->id() == Core::Constants::MODE_WELCOME)
        updateWelcomePage();
}

void ProjectExplorerPlugin::determineSessionToRestoreAtStartup()
{
    // Process command line arguments first:
    if (pluginSpec()->arguments().contains(QLatin1String("-lastsession")))
        d->m_sessionToRestoreAtStartup = d->m_session->lastSession();
    QStringList arguments = ExtensionSystem::PluginManager::arguments();
    if (d->m_sessionToRestoreAtStartup.isNull()) {
        QStringList sessions = d->m_session->sessions();
        // We have command line arguments, try to find a session in them
        // Default to no session loading
        foreach (const QString &arg, arguments) {
            if (sessions.contains(arg)) {
                // Session argument
                d->m_sessionToRestoreAtStartup = arg;
                break;
            }
        }
    }
    // Handle settings only after command line arguments:
    if (d->m_sessionToRestoreAtStartup.isNull()
        && d->m_projectExplorerSettings.autorestoreLastSession)
        d->m_sessionToRestoreAtStartup = d->m_session->lastSession();

    if (!d->m_sessionToRestoreAtStartup.isNull())
        Core::ModeManager::activateMode(Core::Constants::MODE_EDIT);
}

// Return a list of glob patterns for project files ("*.pro", etc), use first, main pattern only.
static inline QStringList projectFileGlobs()
{
    QStringList result;
    const Core::MimeDatabase *mimeDatabase = Core::ICore::instance()->mimeDatabase();
    foreach (const IProjectManager *ipm, ExtensionSystem::PluginManager::getObjects<IProjectManager>()) {
        if (const Core::MimeType mimeType = mimeDatabase->findByType(ipm->mimeType())) {
            const QList<Core::MimeGlobPattern> patterns = mimeType.globPatterns();
            if (!patterns.isEmpty())
                result.push_back(patterns.front().regExp().pattern());
        }
    }
    return result;
}

/*!
    \fn void ProjectExplorerPlugin::restoreSession()

    This method is connected to the ICore::coreOpened signal.  If
    there was no session explicitly loaded, it creates an empty new
    default session and puts the list of recent projects and sessions
    onto the welcome page.
*/
void ProjectExplorerPlugin::restoreSession()
{
    if (debug)
        qDebug() << "ProjectExplorerPlugin::restoreSession";

    // We have command line arguments, try to find a session in them
    QStringList arguments = ExtensionSystem::PluginManager::arguments();
    if (!d->m_sessionToRestoreAtStartup.isEmpty() && !arguments.isEmpty())
        arguments.removeOne(d->m_sessionToRestoreAtStartup);

    // Massage the argument list.
    // Be smart about directories: If there is a session of that name, load it.
    //   Other than that, look for project files in it. The idea is to achieve
    //   'Do what I mean' functionality when starting Creator in a directory with
    //   the single command line argument '.' and avoid editor warnings about not
    //   being able to open directories.
    // In addition, convert "filename" "+45" or "filename" ":23" into
    //   "filename+45"   and "filename:23".
    if (!arguments.isEmpty()) {
        const QStringList sessions = d->m_session->sessions();
        QStringList projectGlobs = projectFileGlobs();
        for (int a = 0; a < arguments.size(); ) {
            const QString &arg = arguments.at(a);
            const QFileInfo fi(arg);
            if (fi.isDir()) {
                const QDir dir(fi.absoluteFilePath());
                // Does the directory name match a session?
                if (d->m_sessionToRestoreAtStartup.isEmpty()
                    && sessions.contains(dir.dirName())) {
                    d->m_sessionToRestoreAtStartup = dir.dirName();
                    arguments.removeAt(a);
                    continue;
                } else {
                    // Are there project files in that directory?
                    const QFileInfoList proFiles
                        = dir.entryInfoList(projectGlobs, QDir::Files);
                    if (!proFiles.isEmpty()) {
                        arguments[a] = proFiles.front().absoluteFilePath();
                        ++a;
                        continue;
                    }
                }
                // Cannot handle: Avoid mime type warning for directory.
                qWarning("Skipping directory '%s' passed on to command line.",
                         qPrintable(QDir::toNativeSeparators(arg)));
                arguments.removeAt(a);
                continue;
            } // Done directories.
            // Converts "filename" "+45" or "filename" ":23" into "filename+45" and "filename:23"
            if (a && (arg.startsWith(QLatin1Char('+')) || arg.startsWith(QLatin1Char(':')))) {
                arguments[a - 1].append(arguments.takeAt(a));
                continue;
            }
            ++a;
        } // for arguments
    } // !arguments.isEmpty()
    // Restore latest session or what was passed on the command line
    if (!d->m_sessionToRestoreAtStartup.isEmpty())
        d->m_session->loadSession(d->m_sessionToRestoreAtStartup);

    // update welcome page
    connect(Core::ModeManager::instance(),
            SIGNAL(currentModeChanged(Core::IMode*,Core::IMode*)),
            SLOT(currentModeChanged(Core::IMode*,Core::IMode*)));
    connect(d->m_welcomePage, SIGNAL(requestSession(QString)), this, SLOT(loadSession(QString)));
    connect(d->m_welcomePage, SIGNAL(requestProject(QString)), this, SLOT(openProjectWelcomePage(QString)));

    Core::ICore::openFiles(arguments, Core::ICore::OpenFilesFlags(Core::ICore::CanContainLineNumbers | Core::ICore::SwitchMode));
    updateActions();
}

void ProjectExplorerPlugin::loadSession(const QString &session)
{
    if (debug)
        qDebug() << "ProjectExplorerPlugin::loadSession" << session;
    d->m_session->loadSession(session);
}


void ProjectExplorerPlugin::showContextMenu(QWidget *view, const QPoint &globalPos, Node *node)
{
    QMenu *contextMenu = 0;

    if (!node)
        node = d->m_session->sessionNode();

    if (node->nodeType() != SessionNodeType) {
        Project *project = d->m_session->projectForNode(node);
        setCurrentNode(node);

        emit aboutToShowContextMenu(project, node);
        switch (node->nodeType()) {
        case ProjectNodeType:
            if (node->parentFolderNode() == d->m_session->sessionNode())
                contextMenu = d->m_projectMenu;
            else
                contextMenu = d->m_subProjectMenu;
            break;
        case FolderNodeType:
            contextMenu = d->m_folderMenu;
            break;
        case FileNodeType:
            populateOpenWithMenu();
            contextMenu = d->m_fileMenu;
            break;
        default:
            qWarning("ProjectExplorerPlugin::showContextMenu - Missing handler for node type");
        }
    } else { // session item
        emit aboutToShowContextMenu(0, node);

        contextMenu = d->m_sessionContextMenu;
    }

    updateContextMenuActions();
    d->m_projectTreeCollapseAllAction->disconnect(SIGNAL(triggered()));
    connect(d->m_projectTreeCollapseAllAction, SIGNAL(triggered()), view, SLOT(collapseAll()));
    if (contextMenu && contextMenu->actions().count() > 0)
        contextMenu->popup(globalPos);
}

BuildManager *ProjectExplorerPlugin::buildManager() const
{
    return d->m_buildManager;
}

TaskHub *ProjectExplorerPlugin::taskHub() const
{
    return d->m_taskHub;
}

void ProjectExplorerPlugin::buildStateChanged(Project * pro)
{
    if (debug) {
        qDebug() << "buildStateChanged";
        qDebug() << pro->document()->fileName() << "isBuilding()" << d->m_buildManager->isBuilding(pro);
    }
    Q_UNUSED(pro)
    updateActions();
}

void ProjectExplorerPlugin::executeRunConfiguration(RunConfiguration *runConfiguration, RunMode runMode)
{
    QString errorMessage;
    if (!runConfiguration->ensureConfigured(&errorMessage)) {
        showRunErrorMessage(errorMessage);
        return;
    }
    if (IRunControlFactory *runControlFactory = findRunControlFactory(runConfiguration, runMode)) {
        emit aboutToExecuteProject(runConfiguration->target()->project(), runMode);

        RunControl *control = runControlFactory->create(runConfiguration, runMode, &errorMessage);
        if (!control) {
            showRunErrorMessage(errorMessage);
            return;
        }
        startRunControl(control, runMode);
    }
}

void ProjectExplorerPlugin::showRunErrorMessage(const QString &errorMessage)
{
    if (errorMessage.isNull()) {
        // a error occured, but message was not set
        QMessageBox::critical(Core::ICore::mainWindow(), tr("Unknown error"), errorMessage);
    } else if (errorMessage.isEmpty()) {
        // a error, but the message was set to empty
        // hack for qml observer warning, show nothing at all
    } else {
        QMessageBox::critical(Core::ICore::mainWindow(), tr("Could Not Run"), errorMessage);
    }
}

void ProjectExplorerPlugin::startRunControl(RunControl *runControl, RunMode runMode)
{
    d->m_outputPane->createNewOutputWindow(runControl);
    if (runMode == NormalRunMode && d->m_projectExplorerSettings.showRunOutput)
        d->m_outputPane->popup(Core::IOutputPane::NoModeSwitch);
    if ((runMode == DebugRunMode || runMode == DebugRunModeWithBreakOnMain)
            && d->m_projectExplorerSettings.showDebugOutput)
        d->m_outputPane->popup(Core::IOutputPane::NoModeSwitch);
    d->m_outputPane->showTabFor(runControl);
    connect(runControl, SIGNAL(finished()), this, SLOT(runControlFinished()));
    runControl->start();
    emit updateRunActions();
}

QList<RunControl *> ProjectExplorerPlugin::runControls() const
{
    return d->m_outputPane->runControls();
}

void ProjectExplorerPlugin::buildQueueFinished(bool success)
{
    if (debug)
        qDebug() << "buildQueueFinished()" << success;

    updateActions();

    bool ignoreErrors = true;
    if (d->m_delayedRunConfiguration && success && d->m_buildManager->getErrorTaskCount() > 0) {
        ignoreErrors = QMessageBox::question(Core::ICore::mainWindow(),
                                             tr("Ignore all errors?"),
                                             tr("Found some build errors in current task.\n"
                                                "Do you want to ignore them?"),
                                             QMessageBox::Yes | QMessageBox::No,
                                             QMessageBox::No) == QMessageBox::Yes;
    }

    if (success && ignoreErrors && d->m_delayedRunConfiguration) {
        executeRunConfiguration(d->m_delayedRunConfiguration, d->m_runMode);
    } else {
        if (d->m_buildManager->tasksAvailable())
            d->m_buildManager->showTaskWindow();
    }
    d->m_delayedRunConfiguration = 0;
    d->m_runMode = NoRunMode;
}

void ProjectExplorerPlugin::setCurrent(Project *project, QString filePath, Node *node)
{
    if (debug)
        qDebug() << "ProjectExplorer - setting path to " << (node ? node->path() : filePath)
                << " and project to " << (project ? project->displayName() : QLatin1String("0"));

    if (node)
        filePath = node->path();
    else
        node = d->m_session->nodeForFile(filePath, project);

    bool projectChanged = false;
    if (d->m_currentProject != project) {
        Core::Context oldContext;
        Core::Context newContext;

        if (d->m_currentProject) {
            oldContext.add(d->m_currentProject->projectContext());
            oldContext.add(d->m_currentProject->projectLanguage());
        }
        if (project) {
            newContext.add(project->projectContext());
            newContext.add(project->projectLanguage());
        }

        Core::ICore::updateAdditionalContexts(oldContext, newContext);

        d->m_currentProject = project;

        projectChanged = true;
    }

    if (projectChanged || d->m_currentNode != node) {
        d->m_currentNode = node;
        if (debug)
            qDebug() << "ProjectExplorer - currentNodeChanged(" << (node ? node->path() : QLatin1String("0")) << ", " << (project ? project->displayName() : QLatin1String("0")) << ')';
        emit currentNodeChanged(d->m_currentNode, project);
        updateContextMenuActions();
    }
    if (projectChanged) {
        if (debug)
            qDebug() << "ProjectExplorer - currentProjectChanged(" << (project ? project->displayName() : QLatin1String("0")) << ')';
        emit currentProjectChanged(project);
        updateActions();
    }

    Core::DocumentManager::setCurrentFile(filePath);
}

void ProjectExplorerPlugin::updateActions()
{
    if (debug)
        qDebug() << "ProjectExplorerPlugin::updateActions";

    QPair<bool, QString> buildActionState = buildSettingsEnabled(startupProject());
    QPair<bool, QString> buildActionContextState = buildSettingsEnabled(d->m_currentProject);
    QPair<bool, QString> buildSessionState = buildSettingsEnabledForSession();

    Project *project = startupProject();
    QString projectName = project ? project->displayName() : QString();
    QString projectNameContextMenu = d->m_currentProject ? d->m_currentProject->displayName() : QString();

    d->m_unloadAction->setParameter(projectNameContextMenu);

    // Normal actions
    d->m_buildAction->setParameter(projectName);
    d->m_rebuildAction->setParameter(projectName);
    d->m_cleanAction->setParameter(projectName);
    d->m_publishAction->setParameter(projectName);

    d->m_buildAction->setEnabled(buildActionState.first);
    d->m_rebuildAction->setEnabled(buildActionState.first);
    d->m_cleanAction->setEnabled(buildActionState.first);

    d->m_buildAction->setToolTip(buildActionState.second);
    d->m_rebuildAction->setToolTip(buildActionState.second);
    d->m_cleanAction->setToolTip(buildActionState.second);

    // Context menu actions
    d->m_setStartupProjectAction->setParameter(projectNameContextMenu);

    bool hasDependencies = session()->projectOrder(d->m_currentProject).size() > 1;
    if (hasDependencies) {
        d->m_buildActionContextMenu->setText(tr("Build Without Dependencies"));
        d->m_rebuildActionContextMenu->setText(tr("Rebuild Without Dependencies"));
        d->m_cleanActionContextMenu->setText(tr("Clean Without Dependencies"));
    } else {
        d->m_buildActionContextMenu->setText(tr("Build"));
        d->m_rebuildActionContextMenu->setText(tr("Rebuild"));
        d->m_cleanActionContextMenu->setText(tr("Clean"));
    }

    d->m_buildActionContextMenu->setEnabled(buildActionContextState.first);
    d->m_rebuildActionContextMenu->setEnabled(buildActionContextState.first);
    d->m_cleanActionContextMenu->setEnabled(buildActionContextState.first);

    d->m_buildActionContextMenu->setToolTip(buildActionState.second);
    d->m_rebuildActionContextMenu->setToolTip(buildActionState.second);
    d->m_cleanActionContextMenu->setToolTip(buildActionState.second);

    // build project only
    d->m_buildProjectOnlyAction->setEnabled(buildActionState.first);
    d->m_rebuildProjectOnlyAction->setEnabled(buildActionState.first);
    d->m_cleanProjectOnlyAction->setEnabled(buildActionState.first);

    d->m_buildProjectOnlyAction->setToolTip(buildActionState.second);
    d->m_rebuildProjectOnlyAction->setToolTip(buildActionState.second);
    d->m_cleanProjectOnlyAction->setToolTip(buildActionState.second);

    // Session actions
    d->m_closeAllProjects->setEnabled(!d->m_session->projects().isEmpty());

    d->m_buildSessionAction->setEnabled(buildSessionState.first);
    d->m_rebuildSessionAction->setEnabled(buildSessionState.first);
    d->m_cleanSessionAction->setEnabled(buildSessionState.first);

    d->m_buildSessionAction->setToolTip(buildSessionState.second);
    d->m_rebuildSessionAction->setToolTip(buildSessionState.second);
    d->m_cleanSessionAction->setToolTip(buildSessionState.second);

    d->m_cancelBuildAction->setEnabled(d->m_buildManager->isBuilding());

    d->m_publishAction->setEnabled(!d->m_session->projects().isEmpty());

    d->m_projectSelectorAction->setEnabled(!session()->projects().isEmpty());
    d->m_projectSelectorActionMenu->setEnabled(!session()->projects().isEmpty());
    d->m_projectSelectorActionQuick->setEnabled(!session()->projects().isEmpty());

    updateDeployActions();
    updateRunWithoutDeployMenu();
}

// NBS TODO check projectOrder()
// what we want here is all the projects pro depends on
QStringList ProjectExplorerPlugin::allFilesWithDependencies(Project *pro)
{
    if (debug)
        qDebug() << "ProjectExplorerPlugin::allFilesWithDependencies(" << pro->document()->fileName() << ")";

    QStringList filesToSave;
    foreach (Project *p, d->m_session->projectOrder(pro)) {
        FindAllFilesVisitor filesVisitor;
        p->rootProjectNode()->accept(&filesVisitor);
        filesToSave << filesVisitor.filePaths();
    }
    qSort(filesToSave);
    return filesToSave;
}

bool ProjectExplorerPlugin::saveModifiedFiles()
{
    if (debug)
        qDebug() << "ProjectExplorerPlugin::saveModifiedFiles";

    QList<Core::IDocument *> documentsToSave = Core::DocumentManager::modifiedDocuments();
    if (!documentsToSave.isEmpty()) {
        if (d->m_projectExplorerSettings.saveBeforeBuild) {
            bool cancelled = false;
            Core::DocumentManager::saveModifiedDocumentsSilently(documentsToSave, &cancelled);
            if (cancelled)
                return false;
        } else {
            bool cancelled = false;
            bool alwaysSave = false;
            Core::DocumentManager::saveModifiedDocuments(documentsToSave, &cancelled, QString(),
                                  tr("Always save files before build"), &alwaysSave);

            if (cancelled)
                return false;
            if (alwaysSave)
                d->m_projectExplorerSettings.saveBeforeBuild = true;
        }
    }
    return true;
}

//NBS handle case where there is no activeBuildConfiguration
// because someone delete all build configurations

void ProjectExplorerPlugin::deploy(QList<Project *> projects)
{
    QList<Core::Id> steps;
    if (d->m_projectExplorerSettings.buildBeforeDeploy)
        steps << Core::Id(Constants::BUILDSTEPS_BUILD);
    steps << Core::Id(Constants::BUILDSTEPS_DEPLOY);
    queue(projects, steps);
}

QString ProjectExplorerPlugin::displayNameForStepId(Core::Id stepId)
{
    if (stepId == Constants::BUILDSTEPS_CLEAN)
        return tr("Clean");
    if (stepId == Constants::BUILDSTEPS_BUILD)
        return tr("Build", "Build step");
    if (stepId == Constants::BUILDSTEPS_DEPLOY)
        return tr("Deploy");
    return tr("Build", "Build step");
}

int ProjectExplorerPlugin::queue(QList<Project *> projects, QList<Core::Id> stepIds)
{
    if (debug) {
        QStringList projectNames, stepNames;
        foreach (const Project *p, projects)
            projectNames << p->displayName();
        foreach (const Core::Id id, stepIds)
            stepNames << id.toString();
        qDebug() << "Building" << stepNames << "for projects" << projectNames;
    }

    if (!saveModifiedFiles())
        return -1;

    QList<BuildStepList *> stepLists;
    QStringList names;
    QStringList preambleMessage;

    foreach (Project *pro, projects)
        if (pro && pro->needsConfiguration())
            preambleMessage.append(tr("The project %1 is not configured, skipping it.\n")
                                   .arg(pro->displayName()));
    foreach (Core::Id id, stepIds) {
        foreach (Project *pro, projects) {
            if (!pro || !pro->activeTarget())
                continue;
            BuildStepList *bsl = 0;
            if (id == Constants::BUILDSTEPS_DEPLOY
                && pro->activeTarget()->activeDeployConfiguration())
                bsl = pro->activeTarget()->activeDeployConfiguration()->stepList();
            else if (pro->activeTarget()->activeBuildConfiguration())
                bsl = pro->activeTarget()->activeBuildConfiguration()->stepList(id);

            if (!bsl || bsl->isEmpty())
                continue;
            stepLists << bsl;
            names << displayNameForStepId(id);
        }
    }

    if (stepLists.isEmpty())
        return 0;

    if (!d->m_buildManager->buildLists(stepLists, names, preambleMessage))
        return -1;
    return stepLists.count();
}

void ProjectExplorerPlugin::buildProjectOnly()
{
    queue(QList<Project *>() << session()->startupProject(), QList<Core::Id>() << Core::Id(Constants::BUILDSTEPS_BUILD));
}

void ProjectExplorerPlugin::buildProject(ProjectExplorer::Project *p)
{
    queue(d->m_session->projectOrder(p),
          QList<Core::Id>() << Core::Id(Constants::BUILDSTEPS_BUILD));
}

void ProjectExplorerPlugin::requestProjectModeUpdate(Project *p)
{
    d->m_proWindow->projectUpdated(p);
}

void ProjectExplorerPlugin::buildProject()
{
    queue(d->m_session->projectOrder(session()->startupProject()),
          QList<Core::Id>() << Core::Id(Constants::BUILDSTEPS_BUILD));
}

void ProjectExplorerPlugin::buildProjectContextMenu()
{
    queue(QList<Project *>() <<  d->m_currentProject,
          QList<Core::Id>() << Core::Id(Constants::BUILDSTEPS_BUILD));
}

void ProjectExplorerPlugin::buildSession()
{
    queue(d->m_session->projectOrder(),
          QList<Core::Id>() << Core::Id(Constants::BUILDSTEPS_BUILD));
}

void ProjectExplorerPlugin::rebuildProjectOnly()
{
    queue(QList<Project *>() << session()->startupProject(),
          QList<Core::Id>() << Core::Id(Constants::BUILDSTEPS_CLEAN) << Core::Id(Constants::BUILDSTEPS_BUILD));
}

void ProjectExplorerPlugin::rebuildProject()
{
    queue(d->m_session->projectOrder(session()->startupProject()),
          QList<Core::Id>() << Core::Id(Constants::BUILDSTEPS_CLEAN) << Core::Id(Constants::BUILDSTEPS_BUILD));
}

void ProjectExplorerPlugin::rebuildProjectContextMenu()
{
    queue(QList<Project *>() <<  d->m_currentProject,
          QList<Core::Id>() << Core::Id(Constants::BUILDSTEPS_CLEAN) << Core::Id(Constants::BUILDSTEPS_BUILD));
}

void ProjectExplorerPlugin::rebuildSession()
{
    queue(d->m_session->projectOrder(),
          QList<Core::Id>() << Core::Id(Constants::BUILDSTEPS_CLEAN) << Core::Id(Constants::BUILDSTEPS_BUILD));
}

void ProjectExplorerPlugin::deployProjectOnly()
{
    deploy(QList<Project *>() << session()->startupProject());
}

void ProjectExplorerPlugin::deployProject()
{
    deploy(d->m_session->projectOrder(session()->startupProject()));
}

void ProjectExplorerPlugin::deployProjectContextMenu()
{
    deploy(QList<Project *>() << d->m_currentProject);
}

void ProjectExplorerPlugin::deploySession()
{
    deploy(d->m_session->projectOrder());
}

void ProjectExplorerPlugin::cleanProjectOnly()
{
    queue(QList<Project *>() << session()->startupProject(),
          QList<Core::Id>() << Core::Id(Constants::BUILDSTEPS_CLEAN));
}

void ProjectExplorerPlugin::cleanProject()
{
    queue(d->m_session->projectOrder(session()->startupProject()),
          QList<Core::Id>() << Core::Id(Constants::BUILDSTEPS_CLEAN));
}

void ProjectExplorerPlugin::cleanProjectContextMenu()
{
    queue(QList<Project *>() <<  d->m_currentProject,
          QList<Core::Id>() << Core::Id(Constants::BUILDSTEPS_CLEAN));
}

void ProjectExplorerPlugin::cleanSession()
{
    queue(d->m_session->projectOrder(),
          QList<Core::Id>() << Core::Id(Constants::BUILDSTEPS_CLEAN));
}

void ProjectExplorerPlugin::runProject()
{
    runProject(startupProject(), NormalRunMode);
}

void ProjectExplorerPlugin::runProjectWithoutDeploy()
{
    runProject(startupProject(), NormalRunMode, true);
}

void ProjectExplorerPlugin::runProjectContextMenu()
{
    ProjectNode *projectNode = qobject_cast<ProjectNode*>(d->m_currentNode);
    if (projectNode == d->m_currentProject->rootProjectNode() || !projectNode) {
        runProject(d->m_currentProject, NormalRunMode);
    } else {
        QAction *act = qobject_cast<QAction *>(sender());
        if (!act)
            return;
        RunConfiguration *rc = act->data().value<RunConfiguration *>();
        if (!rc)
            return;
        runRunConfiguration(rc, NormalRunMode);
    }
}

bool ProjectExplorerPlugin::hasBuildSettings(Project *pro)
{
    const QList<Project *> & projects = d->m_session->projectOrder(pro);
    foreach (Project *project, projects)
        if (project
                && project->activeTarget()
                && project->activeTarget()->activeBuildConfiguration())
            return true;
    return false;
}

QPair<bool, QString> ProjectExplorerPlugin::buildSettingsEnabled(Project *pro)
{
    QPair<bool, QString> result;
    result.first = true;
    if (!pro) {
        result.first = false;
        result.second = tr("No project loaded.");
    } else if (d->m_buildManager->isBuilding(pro)) {
        result.first = false;
        result.second = tr("Currently building the active project.");
    } else if (pro->needsConfiguration()) {
        result.first = false;
        result.second = tr("The project %1 is not configured.").arg(pro->displayName());
    } else if (!hasBuildSettings(pro)) {
        result.first = false;
        result.second = tr("Project has no build settings.");
    } else {
        const QList<Project *> & projects = d->m_session->projectOrder(pro);
        foreach (Project *project, projects) {
            if (project
                    && project->activeTarget()
                    && project->activeTarget()->activeBuildConfiguration()
                    && !project->activeTarget()->activeBuildConfiguration()->isEnabled()) {
                result.first = false;
                result.second += tr("Building '%1' is disabled: %2<br>")
                        .arg(project->displayName(),
                             project->activeTarget()->activeBuildConfiguration()->disabledReason());
            }
        }
    }
    return result;
}

QPair<bool, QString> ProjectExplorerPlugin::buildSettingsEnabledForSession()
{
    QPair<bool, QString> result;
    result.first = true;
    if (d->m_session->projects().isEmpty()) {
        result.first = false;
        result.second = tr("No project loaded");
    } else if (d->m_buildManager->isBuilding()) {
        result.first = false;
        result.second = tr("A build is in progress");
    } else if (!hasBuildSettings(0)) {
        result.first = false;
        result.second = tr("Project has no build settings");
    } else {
        const QList<Project *> & projects = d->m_session->projectOrder(0);
        foreach (Project *project, projects) {
            if (project
                    && project->activeTarget()
                    && project->activeTarget()->activeBuildConfiguration()
                    && !project->activeTarget()->activeBuildConfiguration()->isEnabled()) {
                result.first = false;
                result.second += tr("Building '%1' is disabled: %2\n")
                        .arg(project->displayName(),
                             project->activeTarget()->activeBuildConfiguration()->disabledReason());
            }
        }
    }
    return result;
}

bool ProjectExplorerPlugin::coreAboutToClose()
{
    if (d->m_buildManager->isBuilding()) {
        QMessageBox box;
        QPushButton *closeAnyway = box.addButton(tr("Cancel Build && Close"), QMessageBox::AcceptRole);
        QPushButton *cancelClose = box.addButton(tr("Do Not Close"), QMessageBox::RejectRole);
        box.setDefaultButton(cancelClose);
        box.setWindowTitle(tr("Close Qt Creator?"));
        box.setText(tr("A project is currently being built."));
        box.setInformativeText(tr("Do you want to cancel the build process and close Qt Creator anyway?"));
        box.exec();
        if (box.clickedButton() != closeAnyway)
            return false;
    }
    if (!d->m_outputPane->aboutToClose())
        return false;
    return true;
}

bool ProjectExplorerPlugin::hasDeploySettings(Project *pro)
{
    const QList<Project *> & projects = d->m_session->projectOrder(pro);
    foreach (Project *project, projects)
        if (project->activeTarget()
                && project->activeTarget()->activeDeployConfiguration()
                && !project->activeTarget()->activeDeployConfiguration()->stepList()->isEmpty())
            return true;
    return false;
}

void ProjectExplorerPlugin::runProject(Project *pro, RunMode mode, const bool forceSkipDeploy)
{
    if (!pro)
        return;

    if (Target *target = pro->activeTarget())
        if (RunConfiguration *rc = target->activeRunConfiguration())
            runRunConfiguration(rc, mode, forceSkipDeploy);
}

void ProjectExplorerPlugin::runRunConfiguration(ProjectExplorer::RunConfiguration *rc,
                                                RunMode runMode,
                                                const bool forceSkipDeploy)
{
    if (!rc->isEnabled())
        return;

    QList<Core::Id> stepIds;
    if (!forceSkipDeploy && d->m_projectExplorerSettings.deployBeforeRun) {
        if (d->m_projectExplorerSettings.buildBeforeDeploy)
            stepIds << Core::Id(Constants::BUILDSTEPS_BUILD);
        stepIds << Core::Id(Constants::BUILDSTEPS_DEPLOY);
    }

    Project *pro = rc->target()->project();
    const QList<Project *> &projects = d->m_session->projectOrder(pro);
    int queueCount = queue(projects, stepIds);

    if (queueCount < 0) // something went wrong
        return;

    if (queueCount > 0) {
        // delay running till after our queued steps were processed
        d->m_runMode = runMode;
        d->m_delayedRunConfiguration = rc;
    } else {
        executeRunConfiguration(rc, runMode);
    }
    emit updateRunActions();
}

void ProjectExplorerPlugin::runControlFinished()
{
    emit updateRunActions();
}

void ProjectExplorerPlugin::projectAdded(ProjectExplorer::Project *pro)
{
    if (d->m_projectsMode)
        d->m_projectsMode->setEnabled(true);
    // more specific action en and disabling ?
    connect(pro, SIGNAL(buildConfigurationEnabledChanged()),
            this, SLOT(updateActions()));
}

void ProjectExplorerPlugin::projectRemoved(ProjectExplorer::Project * pro)
{
    if (d->m_projectsMode)
        d->m_projectsMode->setEnabled(!session()->projects().isEmpty());
    // more specific action en and disabling ?
    disconnect(pro, SIGNAL(buildConfigurationEnabledChanged()),
               this, SLOT(updateActions()));
}

void ProjectExplorerPlugin::projectDisplayNameChanged(Project *pro)
{
    addToRecentProjects(pro->document()->fileName(), pro->displayName());
    updateActions();
}

void ProjectExplorerPlugin::startupProjectChanged()
{
    static QPointer<Project> previousStartupProject = 0;
    Project *project = startupProject();
    if (project == previousStartupProject)
        return;

    if (previousStartupProject) {
        disconnect(previousStartupProject, SIGNAL(activeTargetChanged(ProjectExplorer::Target*)),
                   this, SLOT(activeTargetChanged()));
    }

    previousStartupProject = project;

    if (project) {
        connect(project, SIGNAL(activeTargetChanged(ProjectExplorer::Target*)),
                this, SLOT(activeTargetChanged()));
    }

    activeTargetChanged();
    updateActions();
}

void ProjectExplorerPlugin::activeTargetChanged()
{
    static QPointer<Target> previousTarget = 0;
    Target *target = 0;
    if (startupProject())
        target = startupProject()->activeTarget();
    if (target == previousTarget)
        return;

    if (previousTarget) {
        disconnect(previousTarget, SIGNAL(activeRunConfigurationChanged(ProjectExplorer::RunConfiguration*)),
                   this, SLOT(activeRunConfigurationChanged()));
    }
    previousTarget = target;
    if (target) {
        connect(target, SIGNAL(activeRunConfigurationChanged(ProjectExplorer::RunConfiguration*)),
                this, SLOT(activeRunConfigurationChanged()));
    }

    activeRunConfigurationChanged();
    updateDeployActions();
}

void ProjectExplorerPlugin::activeRunConfigurationChanged()
{
    static QPointer<RunConfiguration> previousRunConfiguration = 0;
    RunConfiguration *rc = 0;
    if (startupProject() && startupProject()->activeTarget())
        rc = startupProject()->activeTarget()->activeRunConfiguration();
    if (rc == previousRunConfiguration)
        return;
    if (previousRunConfiguration) {
        disconnect(previousRunConfiguration, SIGNAL(enabledChanged()),
                   this, SIGNAL(updateRunActions()));
        disconnect(previousRunConfiguration->debuggerAspect(), SIGNAL(debuggersChanged()),
                   this, SIGNAL(updateRunActions()));
    }
    previousRunConfiguration = rc;
    if (rc) {
        connect(rc, SIGNAL(enabledChanged()),
                this, SIGNAL(updateRunActions()));
        connect(rc->debuggerAspect(), SIGNAL(debuggersChanged()),
                this, SIGNAL(updateRunActions()));
    }
    emit updateRunActions();
}

// NBS TODO implement more than one runner
IRunControlFactory *ProjectExplorerPlugin::findRunControlFactory(RunConfiguration *config, RunMode mode)
{
    const QList<IRunControlFactory *> factories = ExtensionSystem::PluginManager::getObjects<IRunControlFactory>();
    foreach (IRunControlFactory *f, factories)
        if (f->canRun(config, mode))
            return f;
    return 0;
}

void ProjectExplorerPlugin::updateDeployActions()
{
    Project *project = startupProject();

    bool enableDeployActions = project
            && ! (d->m_buildManager->isBuilding(project))
            && hasDeploySettings(project);
    bool enableDeployActionsContextMenu = d->m_currentProject
                              && ! (d->m_buildManager->isBuilding(d->m_currentProject))
                              && hasDeploySettings(d->m_currentProject);

    if (d->m_projectExplorerSettings.buildBeforeDeploy) {
        if (hasBuildSettings(project)
                && !buildSettingsEnabled(project).first)
            enableDeployActions = false;
        if (hasBuildSettings(d->m_currentProject)
                && !buildSettingsEnabled(d->m_currentProject).first)
            enableDeployActionsContextMenu = false;
    }

    const QString projectName = project ? project->displayName() : QString();
    const QString projectNameContextMenu = d->m_currentProject ? d->m_currentProject->displayName() : QString();
    bool hasProjects = !d->m_session->projects().isEmpty();

    d->m_deployAction->setParameter(projectName);
    d->m_deployAction->setEnabled(enableDeployActions);

    d->m_deployActionContextMenu->setEnabled(enableDeployActionsContextMenu);

    d->m_deployProjectOnlyAction->setEnabled(enableDeployActions);

    bool enableDeploySessionAction = true;
    if (d->m_projectExplorerSettings.buildBeforeDeploy) {
        const QList<Project *> & projects = d->m_session->projectOrder(0);
        foreach (Project *project, projects) {
            if (project
                    && project->activeTarget()
                    && project->activeTarget()->activeBuildConfiguration()
                    && !project->activeTarget()->activeBuildConfiguration()->isEnabled()) {
                enableDeploySessionAction = false;
                break;
            }
        }
    }
    if (!hasProjects
            || !hasDeploySettings(0)
            || d->m_buildManager->isBuilding())
        enableDeploySessionAction = false;
    d->m_deploySessionAction->setEnabled(enableDeploySessionAction);

    emit updateRunActions();
}

bool ProjectExplorerPlugin::canRun(Project *project, RunMode runMode)
{
    if (!project ||
        !project->activeTarget() ||
        !project->activeTarget()->activeRunConfiguration()) {
        return false;
    }

    if (d->m_projectExplorerSettings.buildBeforeDeploy
            && d->m_projectExplorerSettings.deployBeforeRun
            && hasBuildSettings(project)
            && !buildSettingsEnabled(project).first)
        return false;


    RunConfiguration *activeRC = project->activeTarget()->activeRunConfiguration();

    bool canRun = findRunControlFactory(activeRC, runMode)
                  && activeRC->isEnabled();
    const bool building = d->m_buildManager->isBuilding();
    return (canRun && !building);
}

QString ProjectExplorerPlugin::cannotRunReason(Project *project, RunMode runMode)
{
    if (!project)
        return tr("No active project.");

    if (project->needsConfiguration())
        return tr("The project %1 is not configured.").arg(project->displayName());

    if (!project->activeTarget())
        return tr("The project '%1' has no active kit.").arg(project->displayName());

    if (!project->activeTarget()->activeRunConfiguration())
        return tr("The kit '%1' for the project '%2' has no active run configuration.")
                .arg(project->activeTarget()->displayName(), project->displayName());


    if (d->m_projectExplorerSettings.buildBeforeDeploy
            && d->m_projectExplorerSettings.deployBeforeRun
            && hasBuildSettings(project)) {
        QPair<bool, QString> buildState = buildSettingsEnabled(project);
        if (!buildState.first)
            return buildState.second;
    }


    RunConfiguration *activeRC = project->activeTarget()->activeRunConfiguration();
    if (!activeRC->isEnabled())
        return activeRC->disabledReason();

    // shouldn't actually be shown to the user...
    if (!findRunControlFactory(activeRC, runMode))
        return tr("Cannot run '%1'.").arg(activeRC->displayName());


    if (d->m_buildManager->isBuilding())
        return tr("A build is still in progress.");
    return QString();
}

void ProjectExplorerPlugin::slotUpdateRunActions()
{
    Project *project = startupProject();
    const bool state = canRun(project, NormalRunMode);
    d->m_runAction->setEnabled(state);
    d->m_runAction->setToolTip(cannotRunReason(project, NormalRunMode));
    d->m_runWithoutDeployAction->setEnabled(state);
}

void ProjectExplorerPlugin::cancelBuild()
{
    if (debug)
        qDebug() << "ProjectExplorerPlugin::cancelBuild";

    if (d->m_buildManager->isBuilding())
        d->m_buildManager->cancel();
}

void ProjectExplorerPlugin::addToRecentProjects(const QString &fileName, const QString &displayName)
{
    if (debug)
        qDebug() << "ProjectExplorerPlugin::addToRecentProjects(" << fileName << ")";

    if (fileName.isEmpty())
        return;
    QString prettyFileName(QDir::toNativeSeparators(fileName));

    QList<QPair<QString, QString> >::iterator it;
    for (it = d->m_recentProjects.begin(); it != d->m_recentProjects.end();)
        if ((*it).first == prettyFileName)
            it = d->m_recentProjects.erase(it);
        else
            ++it;

    if (d->m_recentProjects.count() > d->m_maxRecentProjects)
        d->m_recentProjects.removeLast();
    d->m_recentProjects.prepend(qMakePair(prettyFileName, displayName));
    QFileInfo fi(prettyFileName);
    d->m_lastOpenDirectory = fi.absolutePath();
    emit recentProjectsChanged();
}

void ProjectExplorerPlugin::updateRecentProjectMenu()
{
    typedef QList<QPair<QString, QString> >::const_iterator StringPairListConstIterator;
    if (debug)
        qDebug() << "ProjectExplorerPlugin::updateRecentProjectMenu";

    Core::ActionContainer *aci =
        Core::ActionManager::actionContainer(Constants::M_RECENTPROJECTS);
    QMenu *menu = aci->menu();
    menu->clear();

    bool hasRecentProjects = false;
    //projects (ignore sessions, they used to be in this list)
    const StringPairListConstIterator end = d->m_recentProjects.constEnd();
    for (StringPairListConstIterator it = d->m_recentProjects.constBegin(); it != end; ++it) {
        const QPair<QString, QString> &s = *it;
        if (s.first.endsWith(QLatin1String(".qws")))
            continue;
        QAction *action = menu->addAction(Utils::withTildeHomePath(s.first));
        action->setData(s.first);
        connect(action, SIGNAL(triggered()), this, SLOT(openRecentProject()));
        hasRecentProjects = true;
    }
    menu->setEnabled(hasRecentProjects);

    // add the Clear Menu item
    if (hasRecentProjects) {
        menu->addSeparator();
        QAction *action = menu->addAction(QCoreApplication::translate(
                                          "Core", Core::Constants::TR_CLEAR_MENU));
        connect(action, SIGNAL(triggered()), this, SLOT(clearRecentProjects()));
    }
    emit recentProjectsChanged();
}

void ProjectExplorerPlugin::clearRecentProjects()
{
    d->m_recentProjects.clear();
    updateWelcomePage();
}

void ProjectExplorerPlugin::openRecentProject()
{
    if (debug)
        qDebug() << "ProjectExplorerPlugin::openRecentProject()";

    QAction *a = qobject_cast<QAction*>(sender());
    if (!a)
        return;
    QString fileName = a->data().toString();
    if (!fileName.isEmpty()) {
        QString errorMessage;
        openProject(fileName, &errorMessage);
        if (!errorMessage.isEmpty())
            QMessageBox::critical(Core::ICore::mainWindow(), tr("Failed to open project"), errorMessage);
    }
}

void ProjectExplorerPlugin::invalidateProject(Project *project)
{
    if (debug)
        qDebug() << "ProjectExplorerPlugin::invalidateProject" << project->displayName();
    if (d->m_currentProject == project) {
        //
        // Workaround for a bug in QItemSelectionModel
        // - currentChanged etc are not emitted if the
        // item is removed from the underlying data model
        //
        setCurrent(0, QString(), 0);
    }

    disconnect(project, SIGNAL(fileListChanged()), this, SIGNAL(fileListChanged()));
    updateActions();
}

void ProjectExplorerPlugin::updateContextMenuActions()
{
    d->m_addExistingFilesAction->setEnabled(false);
    d->m_addNewFileAction->setEnabled(false);
    d->m_addNewSubprojectAction->setEnabled(false);
    d->m_removeFileAction->setEnabled(false);
    d->m_deleteFileAction->setEnabled(false);
    d->m_renameFileAction->setEnabled(false);

    d->m_addExistingFilesAction->setVisible(true);
    d->m_removeFileAction->setVisible(true);
    d->m_deleteFileAction->setVisible(true);
    d->m_runActionContextMenu->setVisible(false);

    Core::ActionContainer *runMenu = Core::ActionManager::actionContainer(Constants::RUNMENUCONTEXTMENU);
    runMenu->menu()->clear();

    if (d->m_currentNode && d->m_currentNode->projectNode()) {
        QList<ProjectNode::ProjectAction> actions =
                d->m_currentNode->projectNode()->supportedActions(d->m_currentNode);

        if (ProjectNode *pn = qobject_cast<ProjectNode *>(d->m_currentNode)) {
            if (pn == d->m_currentProject->rootProjectNode()) {
                d->m_runActionContextMenu->setVisible(true);
            } else {
                QList<RunConfiguration *> runConfigs = pn->runConfigurationsFor(pn);
                if (runConfigs.count() == 1) {
                    d->m_runActionContextMenu->setVisible(true);
                    d->m_runActionContextMenu->setData(QVariant::fromValue(runConfigs.first()));
                } else if (runConfigs.count() > 1) {
                    foreach (RunConfiguration *rc, runConfigs) {
                        QAction *act = new QAction(runMenu->menu());
                        act->setData(QVariant::fromValue(rc));
                        act->setText(tr("Run %1").arg(rc->displayName()));
                        runMenu->menu()->addAction(act);
                        connect(act, SIGNAL(triggered()),
                                this, SLOT(runProjectContextMenu()));
                    }
                }
            }
        }
        if (qobject_cast<FolderNode*>(d->m_currentNode)) {
            // Also handles ProjectNode
            d->m_addNewFileAction->setEnabled(actions.contains(ProjectNode::AddNewFile));
            d->m_addNewSubprojectAction->setEnabled(d->m_currentNode->nodeType() == ProjectNodeType
                                                    && actions.contains(ProjectNode::AddSubProject));
            d->m_addExistingFilesAction->setEnabled(actions.contains(ProjectNode::AddExistingFile));
            d->m_renameFileAction->setEnabled(actions.contains(ProjectNode::Rename));
        } else if (qobject_cast<FileNode*>(d->m_currentNode)) {
            // Enable and show remove / delete in magic ways:
            // If both are disabled show Remove
            // If both are enabled show both (can't happen atm)
            // If only removeFile is enabled only show it
            // If only deleteFile is enable only show it
            bool enableRemove = actions.contains(ProjectNode::RemoveFile);
            d->m_removeFileAction->setEnabled(enableRemove);
            bool enableDelete = actions.contains(ProjectNode::EraseFile);
            d->m_deleteFileAction->setEnabled(enableDelete);
            d->m_deleteFileAction->setVisible(enableDelete);

            d->m_removeFileAction->setVisible(!enableDelete || enableRemove);
            d->m_renameFileAction->setEnabled(actions.contains(ProjectNode::Rename));
        }
    }
}

QString pathOrDirectoryFor(Node *node, bool dir)
{
    QString path = node->path();
    QString location;
    FolderNode *folder = qobject_cast<FolderNode *>(node);
    if (node->nodeType() == ProjectExplorer::VirtualFolderNodeType && folder) {
        // Virtual Folder case
        // If there are files directly below or no subfolders, take the folder path
        if (!folder->fileNodes().isEmpty() || folder->subFolderNodes().isEmpty()) {
            location = path;
        } else {
            // Otherwise we figure out a commonPath from the subfolders
            QStringList list;
            foreach (FolderNode *f, folder->subFolderNodes())
                list << f->path() + QLatin1Char('/');
            location = Utils::commonPath(list);
        }
    } else {
        QFileInfo fi(path);
        if (dir)
            location = fi.isDir() ? fi.absoluteFilePath() : fi.absolutePath();
        else
            location = fi.absoluteFilePath();
    }
    return location;
}

QString ProjectExplorerPlugin::pathFor(Node *node)
{
    return pathOrDirectoryFor(node, false);
}

QString ProjectExplorerPlugin::directoryFor(Node *node)
{
    return pathOrDirectoryFor(node, true);
}

void ProjectExplorerPlugin::addNewFile()
{
    QTC_ASSERT(d->m_currentNode, return);
    QString location = directoryFor(d->m_currentNode);

    QVariantMap map;
    map.insert(QLatin1String(Constants::PREFERED_PROJECT_NODE), d->m_currentNode->projectNode()->path());
    if (d->m_currentProject) {
        QList<Core::Id> profileIds;
        foreach (Target *target, d->m_currentProject->targets())
            profileIds << target->id();
        map.insert(QLatin1String(Constants::PROJECT_KIT_IDS), QVariant::fromValue(profileIds));
    }
    Core::ICore::showNewItemDialog(tr("New File", "Title of dialog"),
                               Core::IWizard::wizardsOfKind(Core::IWizard::FileWizard)
                               + Core::IWizard::wizardsOfKind(Core::IWizard::ClassWizard),
                               location, map);
}

void ProjectExplorerPlugin::addNewSubproject()
{
    QTC_ASSERT(d->m_currentNode, return);
    QString location = directoryFor(d->m_currentNode);

    if (d->m_currentNode->nodeType() == ProjectNodeType
            && d->m_currentNode->projectNode()->supportedActions(
                d->m_currentNode->projectNode()).contains(ProjectNode::AddSubProject)) {
        QVariantMap map;
        map.insert(QLatin1String(Constants::PREFERED_PROJECT_NODE), d->m_currentNode->projectNode()->path());
        if (d->m_currentProject) {
            QList<Core::Id> profileIds;
            foreach (Target *target, d->m_currentProject->targets())
                profileIds << target->id();
            map.insert(QLatin1String(Constants::PROJECT_KIT_IDS), QVariant::fromValue(profileIds));
        }

        Core::ICore::showNewItemDialog(tr("New Subproject", "Title of dialog"),
                              Core::IWizard::wizardsOfKind(Core::IWizard::ProjectWizard),
                              location, map);
    }
}

void ProjectExplorerPlugin::addExistingFiles()
{
    QTC_ASSERT(d->m_currentNode, return);

    QStringList fileNames = QFileDialog::getOpenFileNames(Core::ICore::mainWindow(),
        tr("Add Existing Files"), directoryFor(d->m_currentNode));
    if (fileNames.isEmpty())
        return;
    addExistingFiles(fileNames);
}

void ProjectExplorerPlugin::addExistingFiles(const QStringList &filePaths)
{
    ProjectNode *projectNode = qobject_cast<ProjectNode*>(d->m_currentNode->projectNode());
    addExistingFiles(projectNode, filePaths);
}

void ProjectExplorerPlugin::addExistingFiles(ProjectNode *projectNode, const QStringList &filePaths)
{
    if (!projectNode) // can happen when project is not yet parsed
        return;

    const QString dir = directoryFor(projectNode);
    QStringList fileNames = filePaths;
    QHash<FileType, QString> fileTypeToFiles;
    foreach (const QString &fileName, fileNames) {
        FileType fileType = typeForFileName(Core::ICore::mimeDatabase(), QFileInfo(fileName));
        fileTypeToFiles.insertMulti(fileType, fileName);
    }

    QStringList notAdded;
    foreach (const FileType type, fileTypeToFiles.uniqueKeys()) {
        projectNode->addFiles(type, fileTypeToFiles.values(type), &notAdded);
    }
    if (!notAdded.isEmpty()) {
        QString message = tr("Could not add following files to project %1:\n").arg(projectNode->displayName());
        QString files = notAdded.join(QString(QLatin1Char('\n')));
        QMessageBox::warning(Core::ICore::mainWindow(), tr("Adding Files to Project Failed"),
                             message + files);
        foreach (const QString &file, notAdded)
            fileNames.removeOne(file);
    }

    Core::ICore::vcsManager()->promptToAdd(dir, fileNames);
}

void ProjectExplorerPlugin::removeProject()
{
    ProjectNode *subProjectNode = qobject_cast<ProjectNode*>(d->m_currentNode->projectNode());
    ProjectNode *projectNode = qobject_cast<ProjectNode *>(subProjectNode->parentFolderNode());
    if (projectNode) {
        Core::RemoveFileDialog removeFileDialog(subProjectNode->path(), Core::ICore::mainWindow());
        removeFileDialog.setDeleteFileVisible(false);
        if (removeFileDialog.exec() == QDialog::Accepted)
            projectNode->removeSubProjects(QStringList() << subProjectNode->path());
    }
}

void ProjectExplorerPlugin::openFile()
{
    QTC_ASSERT(d->m_currentNode, return);
    Core::EditorManager::openEditor(d->m_currentNode->path(), Core::Id(), Core::EditorManager::ModeSwitch);
}

void ProjectExplorerPlugin::searchOnFileSystem()
{
    QTC_ASSERT(d->m_currentNode, return);
    FolderNavigationWidget::findOnFileSystem(pathFor(d->m_currentNode));
}

void ProjectExplorerPlugin::showInGraphicalShell()
{
    QTC_ASSERT(d->m_currentNode, return);
    Core::FileUtils::showInGraphicalShell(Core::ICore::mainWindow(),
                                                    pathFor(d->m_currentNode));
}

void ProjectExplorerPlugin::openTerminalHere()
{
    QTC_ASSERT(d->m_currentNode, return);
    Core::FileUtils::openTerminal(directoryFor(d->m_currentNode));
}

void ProjectExplorerPlugin::removeFile()
{
    QTC_ASSERT(d->m_currentNode && d->m_currentNode->nodeType() == FileNodeType, return);

    FileNode *fileNode = qobject_cast<FileNode*>(d->m_currentNode);

    QString filePath = d->m_currentNode->path();
    Core::RemoveFileDialog removeFileDialog(filePath, Core::ICore::mainWindow());

    if (removeFileDialog.exec() == QDialog::Accepted) {
        const bool deleteFile = removeFileDialog.isDeleteFileChecked();

        // remove from project
        ProjectNode *projectNode = fileNode->projectNode();
        Q_ASSERT(projectNode);

        if (!projectNode->removeFiles(fileNode->fileType(), QStringList(filePath))) {
            QMessageBox::warning(Core::ICore::mainWindow(), tr("Removing File Failed"),
                                 tr("Could not remove file %1 from project %2.").arg(filePath).arg(projectNode->displayName()));
            return;
        }

        Core::FileUtils::removeFile(filePath, deleteFile);
    }
}

void ProjectExplorerPlugin::deleteFile()
{
    QTC_ASSERT(d->m_currentNode && d->m_currentNode->nodeType() == FileNodeType, return);

    FileNode *fileNode = qobject_cast<FileNode*>(d->m_currentNode);

    QString filePath = d->m_currentNode->path();
    QMessageBox::StandardButton button =
            QMessageBox::question(Core::ICore::mainWindow(),
                                  tr("Delete File"),
                                  tr("Delete %1 from file system?").arg(filePath),
                                  QMessageBox::Yes | QMessageBox::No);
    if (button != QMessageBox::Yes)
        return;

    ProjectNode *projectNode = fileNode->projectNode();
    QTC_ASSERT(projectNode, return);

    projectNode->deleteFiles(fileNode->fileType(), QStringList(filePath));

    Core::DocumentManager::expectFileChange(filePath);
    if (Core::IVersionControl *vc =
            Core::ICore::vcsManager()->findVersionControlForDirectory(QFileInfo(filePath).absolutePath())) {
        vc->vcsDelete(filePath);
    }
    QFile file(filePath);
    if (file.exists()) {
        if (!file.remove())
            QMessageBox::warning(Core::ICore::mainWindow(), tr("Deleting File Failed"),
                                 tr("Could not delete file %1.").arg(filePath));
    }
    Core::DocumentManager::unexpectFileChange(filePath);
}

void ProjectExplorerPlugin::renameFile()
{
    QWidget *focusWidget = QApplication::focusWidget();
    while (focusWidget) {
        ProjectTreeWidget *treeWidget = qobject_cast<ProjectTreeWidget*>(focusWidget);
        if (treeWidget) {
            treeWidget->editCurrentItem();
            return;
        }
        focusWidget = focusWidget->parentWidget();
    }
}

void ProjectExplorerPlugin::renameFile(Node *node, const QString &to)
{
    FileNode *fileNode = qobject_cast<FileNode *>(node);
    if (!fileNode)
        return;
    QString orgFilePath = QFileInfo(node->path()).absoluteFilePath();
    QString dir = QFileInfo(orgFilePath).absolutePath();
    QString newFilePath = dir + QLatin1Char('/') + to;

    if (Core::FileUtils::renameFile(orgFilePath, newFilePath)) {
        // Tell the project plugin about rename
        ProjectNode *projectNode = fileNode->projectNode();
        if (!projectNode->renameFile(fileNode->fileType(), orgFilePath, newFilePath)) {
            QMessageBox::warning(Core::ICore::mainWindow(), tr("Project Editing Failed"),
                                 tr("The file %1 was renamed to %2, but the project file %3 could not be automatically changed.")
                                 .arg(orgFilePath)
                                 .arg(newFilePath)
                                 .arg(projectNode->displayName()));
        } else {
            setCurrent(d->m_session->projectForFile(newFilePath), newFilePath, 0);
        }
    }
}

void ProjectExplorerPlugin::setStartupProject()
{
    setStartupProject(d->m_currentProject);
}

void ProjectExplorerPlugin::populateOpenWithMenu()
{
    Core::DocumentManager::populateOpenWithMenu(d->m_openWithMenu, currentNode()->path());
}

void ProjectExplorerPlugin::updateSessionMenu()
{
    d->m_sessionMenu->clear();
    QActionGroup *ag = new QActionGroup(d->m_sessionMenu);
    connect(ag, SIGNAL(triggered(QAction*)), this, SLOT(setSession(QAction*)));
    const QString &activeSession = d->m_session->activeSession();
    foreach (const QString &session, d->m_session->sessions()) {
        QAction *act = ag->addAction(session);
        act->setCheckable(true);
        if (session == activeSession)
            act->setChecked(true);
    }
    d->m_sessionMenu->addActions(ag->actions());
    d->m_sessionMenu->setEnabled(true);
}

void ProjectExplorerPlugin::setSession(QAction *action)
{
    QString session = action->text();
    if (session != d->m_session->activeSession())
        d->m_session->loadSession(session);
}

void ProjectExplorerPlugin::setProjectExplorerSettings(const Internal::ProjectExplorerSettings &pes)
{
    if (d->m_projectExplorerSettings == pes)
        return;
    d->m_projectExplorerSettings = pes;
    emit settingsChanged();
}

Internal::ProjectExplorerSettings ProjectExplorerPlugin::projectExplorerSettings() const
{
    return d->m_projectExplorerSettings;
}

QStringList ProjectExplorerPlugin::projectFilePatterns()
{
    QStringList patterns;
    const Core::MimeDatabase *mdb = Core::ICore::mimeDatabase();
    foreach (const IProjectManager *pm, allProjectManagers())
        if (const Core::MimeType mt = mdb->findByType(pm->mimeType()))
            foreach (const Core::MimeGlobPattern &gp, mt.globPatterns())
                patterns += gp.regExp().pattern();
    return patterns;
}

void ProjectExplorerPlugin::openOpenProjectDialog()
{
    const QString path = Core::DocumentManager::useProjectsDirectory() ? Core::DocumentManager::projectsDirectory() : QString();
    const QStringList files = Core::DocumentManager::getOpenFileNames(d->m_projectFilterString, path);
    if (!files.isEmpty())
        Core::ICore::openFiles(files, Core::ICore::SwitchMode);
}

QList<QPair<QString, QString> > ProjectExplorerPlugin::recentProjects()
{
    return d->m_recentProjects;
}

Q_EXPORT_PLUGIN(ProjectExplorerPlugin)
