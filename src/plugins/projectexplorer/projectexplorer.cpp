/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "projectexplorer.h"
#include "project.h"
#include "projectexplorersettings.h"
#include "applicationrunconfiguration.h"
#include "allprojectsfilter.h"
#include "allprojectsfind.h"
#include "buildmanager.h"
#include "buildsettingspropertiespage.h"
#include "currentprojectfind.h"
#include "currentprojectfilter.h"
#include "customexecutablerunconfiguration.h"
#include "editorsettingspropertiespage.h"
#include "dependenciespanel.h"
#include "foldernavigationwidget.h"
#include "iprojectmanager.h"
#include "metatypedeclarations.h"
#include "nodesvisitor.h"
#include "outputwindow.h"
#include "persistentsettings.h"
#include "pluginfilefactory.h"
#include "processstep.h"
#include "projectexplorerconstants.h"
#include "projectfilewizardextension.h"
#include "projecttreewidget.h"
#include "projectwindow.h"
#include "removefiledialog.h"
#include "runsettingspropertiespage.h"
#include "scriptwrappers.h"
#include "session.h"
#include "sessiondialog.h"
#include "buildparserfactory.h"
#include "projectexplorersettingspage.h"
#include "projectwelcomepage.h"
#include "projectwelcomepagewidget.h"
#include "corelistenercheckingforrunningbuild.h"
#include "buildconfiguration.h"

#include <coreplugin/basemode.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/filemanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/mainwindow.h>
#include <coreplugin/mimedatabase.h>
#include <coreplugin/modemanager.h>
#include <coreplugin/uniqueidmanager.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/editormanager/ieditorfactory.h>
#include <coreplugin/editormanager/iexternaleditor.h>
#include <coreplugin/findplaceholder.h>
#include <coreplugin/basefilewizard.h>
#include <coreplugin/mainwindow.h>
#include <coreplugin/vcsmanager.h>
#include <coreplugin/iversioncontrol.h>
#include <coreplugin/vcsmanager.h>
#include <welcome/welcomemode.h>
#include <extensionsystem/pluginmanager.h>
#include <utils/qtcassert.h>
#include <utils/parameteraction.h>

#include <QtCore/QtPlugin>
#include <QtCore/QDateTime>
#include <QtCore/QDebug>
#include <QtCore/QSettings>

#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QFileDialog>
#include <QtGui/QMenu>
#include <QtGui/QMessageBox>

Q_DECLARE_METATYPE(QSharedPointer<ProjectExplorer::RunConfiguration>);
Q_DECLARE_METATYPE(Core::IEditorFactory*);
Q_DECLARE_METATYPE(Core::IExternalEditor*);

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
#if 0
    QAction *m_loadAction;
#endif
    Core::Utils::ParameterAction *m_unloadAction;
    QAction *m_clearSession;
    QAction *m_buildProjectOnlyAction;
    Core::Utils::ParameterAction *m_buildAction;
    QAction *m_buildSessionAction;
    QAction *m_rebuildProjectOnlyAction;
    Core::Utils::ParameterAction *m_rebuildAction;
    QAction *m_rebuildSessionAction;
    QAction *m_cleanProjectOnlyAction;
    Core::Utils::ParameterAction *m_cleanAction;
    QAction *m_cleanSessionAction;
    QAction *m_runAction;
    QAction *m_runActionContextMenu;
    QAction *m_cancelBuildAction;
    QAction *m_debugAction;
    QAction *m_addNewFileAction;
    QAction *m_addExistingFilesAction;
    QAction *m_openFileAction;
    QAction *m_showInGraphicalShell;
    QAction *m_removeFileAction;
    QAction *m_renameFileAction;

    QMenu *m_buildConfigurationMenu;
    QActionGroup *m_buildConfigurationActionGroup;
    QMenu *m_runConfigurationMenu;
    QActionGroup *m_runConfigurationActionGroup;

    Internal::ProjectWindow *m_proWindow;
    SessionManager *m_session;
    QString m_sessionToRestoreAtStartup;

    Project *m_currentProject;
    Node *m_currentNode;

    BuildManager *m_buildManager;

    QList<Internal::ProjectFileFactory*> m_fileFactories;
    QStringList m_profileMimeTypes;
    Internal::OutputPane *m_outputPane;

    QList<QPair<QString, QString> > m_recentProjects; // pair of filename, displayname
    static const int m_maxRecentProjects = 7;

    QString m_lastOpenDirectory;
    QSharedPointer<RunConfiguration> m_delayedRunConfiguration;
    RunControl *m_debuggingRunControl;
    QString m_runMode;
    QString m_projectFilterString;
    Internal::ProjectExplorerSettings m_projectExplorerSettings;
    Internal::ProjectWelcomePage *m_welcomePlugin;
    Internal::ProjectWelcomePageWidget *m_welcomePage;
};

ProjectExplorerPluginPrivate::ProjectExplorerPluginPrivate() :
    m_buildConfigurationActionGroup(0),
    m_runConfigurationActionGroup(0),
    m_currentProject(0),
    m_currentNode(0),
    m_delayedRunConfiguration(0),
    m_debuggingRunControl(0)
{
}

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
    removeObject(d->m_welcomePlugin);
    removeObject(this);
    delete d;
}

ProjectExplorerPlugin *ProjectExplorerPlugin::instance()
{
    return m_instance;
}

bool ProjectExplorerPlugin::initialize(const QStringList &arguments, QString *error)
{
    Q_UNUSED(arguments)
    Q_UNUSED(error)

    Core::ICore *core = Core::ICore::instance();
    Core::ActionManager *am = core->actionManager();

    d->m_welcomePlugin = new ProjectWelcomePage;
    d->m_welcomePage = qobject_cast<Internal::ProjectWelcomePageWidget*>(d->m_welcomePlugin->page());
    Q_ASSERT(d->m_welcomePage);
    connect(d->m_welcomePage, SIGNAL(manageSessions()), this, SLOT(showSessionManager()));
    addObject(d->m_welcomePlugin);
    addObject(this);

    connect(core->fileManager(), SIGNAL(currentFileChanged(QString)),
            this, SLOT(setCurrentFile(QString)));

    d->m_session = new SessionManager(this);

    connect(d->m_session, SIGNAL(projectAdded(ProjectExplorer::Project *)),
            this, SIGNAL(fileListChanged()));
    connect(d->m_session, SIGNAL(aboutToRemoveProject(ProjectExplorer::Project *)),
            this, SLOT(invalidateProject(ProjectExplorer::Project *)));
    connect(d->m_session, SIGNAL(projectRemoved(ProjectExplorer::Project *)),
            this, SIGNAL(fileListChanged()));
    connect(d->m_session, SIGNAL(startupProjectChanged(ProjectExplorer::Project *)),
            this, SLOT(startupProjectChanged()));

    d->m_proWindow = new ProjectWindow;

    QList<int> globalcontext;
    globalcontext.append(Core::Constants::C_GLOBAL_ID);

    QList<int> pecontext;
    pecontext << core->uniqueIDManager()->uniqueIdentifier(Constants::C_PROJECTEXPLORER);

    Core::BaseMode *mode = new Core::BaseMode;
    mode->setName(tr("Projects"));
    mode->setUniqueModeName(Constants::MODE_SESSION);
    mode->setIcon(QIcon(QLatin1String(":/fancyactionbar/images/mode_Project.png")));
    mode->setPriority(Constants::P_MODE_SESSION);
    mode->setWidget(d->m_proWindow);
    mode->setContext(QList<int>() << pecontext);
    addAutoReleasedObject(mode);
    d->m_proWindow->layout()->addWidget(new Core::FindToolBarPlaceHolder(d->m_proWindow));

    d->m_buildManager = new BuildManager(this);
    connect(d->m_buildManager, SIGNAL(buildStateChanged(ProjectExplorer::Project *)),
            this, SLOT(buildStateChanged(ProjectExplorer::Project *)));
    connect(d->m_buildManager, SIGNAL(buildQueueFinished(bool)),
            this, SLOT(buildQueueFinished(bool)));

    addAutoReleasedObject(new CoreListenerCheckingForRunningBuild(d->m_buildManager));

    d->m_outputPane = new OutputPane;
    addAutoReleasedObject(d->m_outputPane);
    connect(d->m_session, SIGNAL(projectRemoved(ProjectExplorer::Project *)),
            d->m_outputPane, SLOT(projectRemoved()));

    AllProjectsFilter *allProjectsFilter = new AllProjectsFilter(this);
    addAutoReleasedObject(allProjectsFilter);

    CurrentProjectFilter *currentProjectFilter = new CurrentProjectFilter(this);
    addAutoReleasedObject(currentProjectFilter);

    addAutoReleasedObject(new BuildSettingsPanelFactory);
    addAutoReleasedObject(new RunSettingsPanelFactory);
    addAutoReleasedObject(new EditorSettingsPanelFactory);
    addAutoReleasedObject(new DependenciesPanelFactory(d->m_session));

    ProcessStepFactory *processStepFactory = new ProcessStepFactory;
    addAutoReleasedObject(processStepFactory);

    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    AllProjectsFind *allProjectsFind = new AllProjectsFind(this,
        pm->getObject<Find::SearchResultWindow>());
    addAutoReleasedObject(allProjectsFind);

    CurrentProjectFind *currentProjectFind = new CurrentProjectFind(this,
        pm->getObject<Find::SearchResultWindow>());
    addAutoReleasedObject(currentProjectFind);

    addAutoReleasedObject(new LocalApplicationRunControlFactory);
    addAutoReleasedObject(new CustomExecutableRunConfigurationFactory);

    addAutoReleasedObject(new ProjectFileWizardExtension);

    // Build parsers
    addAutoReleasedObject(new GccParserFactory);
    addAutoReleasedObject(new MsvcParserFactory);

    // Settings page
    addAutoReleasedObject(new ProjectExplorerSettingsPage);

    // context menus
    Core::ActionContainer *msessionContextMenu =
        am->createMenu(Constants::M_SESSIONCONTEXT);
    Core::ActionContainer *mproject =
        am->createMenu(Constants::M_PROJECTCONTEXT);
    Core::ActionContainer *msubProject =
        am->createMenu(Constants::M_SUBPROJECTCONTEXT);
    Core::ActionContainer *mfolder =
        am->createMenu(Constants::M_FOLDERCONTEXT);
    Core::ActionContainer *mfilec =
        am->createMenu(Constants::M_FILECONTEXT);

    d->m_sessionContextMenu = msessionContextMenu->menu();
    d->m_projectMenu = mproject->menu();
    d->m_subProjectMenu = msubProject->menu();
    d->m_folderMenu = mfolder->menu();
    d->m_fileMenu = mfilec->menu();

    Core::ActionContainer *mfile =
        am->actionContainer(Core::Constants::M_FILE);
    Core::ActionContainer *menubar =
        am->actionContainer(Core::Constants::MENU_BAR);

    // mode manager (for fancy actions)
    Core::ModeManager *modeManager = core->modeManager();

    // build menu
    Core::ActionContainer *mbuild =
        am->createMenu(Constants::M_BUILDPROJECT);
    mbuild->menu()->setTitle(tr("&Build"));
    menubar->addMenu(mbuild, Core::Constants::G_VIEW);

    // debug menu
    Core::ActionContainer *mdebug =
        am->createMenu(Constants::M_DEBUG);
    mdebug->menu()->setTitle(tr("&Debug"));
    menubar->addMenu(mdebug, Core::Constants::G_VIEW);

    Core::ActionContainer *mstartdebugging =
        am->createMenu(Constants::M_DEBUG_STARTDEBUGGING);
    mstartdebugging->menu()->setTitle(tr("&Start Debugging"));
    mdebug->addMenu(mstartdebugging, Core::Constants::G_DEFAULT_ONE);

    //
    // Groups
    //

    mbuild->appendGroup(Constants::G_BUILD_SESSION);
    mbuild->appendGroup(Constants::G_BUILD_PROJECT);
    mbuild->appendGroup(Constants::G_BUILD_OTHER);
    mbuild->appendGroup(Constants::G_BUILD_CANCEL);
    mbuild->appendGroup(Constants::G_BUILD_RUN);

    msessionContextMenu->appendGroup(Constants::G_SESSION_BUILD);
    msessionContextMenu->appendGroup(Constants::G_SESSION_FILES);
    msessionContextMenu->appendGroup(Constants::G_SESSION_OTHER);
    msessionContextMenu->appendGroup(Constants::G_SESSION_CONFIG);

    mproject->appendGroup(Constants::G_PROJECT_OPEN);
    mproject->appendGroup(Constants::G_PROJECT_NEW);
    mproject->appendGroup(Constants::G_PROJECT_BUILD);
    mproject->appendGroup(Constants::G_PROJECT_RUN);
    mproject->appendGroup(Constants::G_PROJECT_FILES);
    mproject->appendGroup(Constants::G_PROJECT_OTHER);
    mproject->appendGroup(Constants::G_PROJECT_CONFIG);

    msubProject->appendGroup(Constants::G_PROJECT_OPEN);
    msubProject->appendGroup(Constants::G_PROJECT_FILES);
    msubProject->appendGroup(Constants::G_PROJECT_OTHER);
    msubProject->appendGroup(Constants::G_PROJECT_CONFIG);

    mfolder->appendGroup(Constants::G_FOLDER_FILES);
    mfolder->appendGroup(Constants::G_FOLDER_OTHER);
    mfolder->appendGroup(Constants::G_FOLDER_CONFIG);

    mfilec->appendGroup(Constants::G_FILE_OPEN);
    mfilec->appendGroup(Constants::G_FILE_OTHER);
    mfilec->appendGroup(Constants::G_FILE_CONFIG);

    // "open with" submenu
    Core::ActionContainer * const openWith =
            am->createMenu(ProjectExplorer::Constants::M_OPENFILEWITHCONTEXT);
    d->m_openWithMenu = openWith->menu();
    d->m_openWithMenu->setTitle(tr("Open With"));

    connect(mfilec->menu(), SIGNAL(aboutToShow()), this, SLOT(populateOpenWithMenu()));
    connect(d->m_openWithMenu, SIGNAL(triggered(QAction *)),
            this, SLOT(openWithMenuTriggered(QAction *)));

    //
    // Separators
    //

    Core::Command *cmd;
    QAction *sep;

    sep = new QAction(this);
    sep->setSeparator(true);
    cmd = am->registerAction(sep, QLatin1String("ProjectExplorer.Build.Sep"), globalcontext);
    mbuild->addAction(cmd, Constants::G_BUILD_PROJECT);

    sep = new QAction(this);
    sep->setSeparator(true);
    cmd = am->registerAction(sep, QLatin1String("ProjectExplorer.Files.Sep"), globalcontext);
    msessionContextMenu->addAction(cmd, Constants::G_SESSION_FILES);
    mproject->addAction(cmd, Constants::G_PROJECT_FILES);
    msubProject->addAction(cmd, Constants::G_PROJECT_FILES);

    sep = new QAction(this);
    sep->setSeparator(true);
    cmd = am->registerAction(sep, QLatin1String("ProjectExplorer.New.Sep"), globalcontext);
    mproject->addAction(cmd, Constants::G_PROJECT_NEW);

    sep = new QAction(this);
    sep->setSeparator(true);
    cmd = am->registerAction(sep, QLatin1String("ProjectExplorer.Config.Sep"), globalcontext);
    msessionContextMenu->addAction(cmd, Constants::G_SESSION_CONFIG);
    mproject->addAction(cmd, Constants::G_PROJECT_CONFIG);
    msubProject->addAction(cmd, Constants::G_PROJECT_CONFIG);

    sep = new QAction(this);
    sep->setSeparator(true);
    cmd = am->registerAction(sep, QLatin1String("ProjectExplorer.Projects.Sep"), globalcontext);
    mfile->addAction(cmd, Core::Constants::G_FILE_PROJECT);

    sep = new QAction(this);
    sep->setSeparator(true);
    cmd = am->registerAction(sep, QLatin1String("ProjectExplorer.Other.Sep"), globalcontext);
    mbuild->addAction(cmd, Constants::G_BUILD_OTHER);
    msessionContextMenu->addAction(cmd, Constants::G_SESSION_OTHER);
    mproject->addAction(cmd, Constants::G_PROJECT_OTHER);
    msubProject->addAction(cmd, Constants::G_PROJECT_OTHER);

    sep = new QAction(this);
    sep->setSeparator(true);
    cmd = am->registerAction(sep, QLatin1String("ProjectExplorer.Run.Sep"), globalcontext);
    mbuild->addAction(cmd, Constants::G_BUILD_RUN);
    mproject->addAction(cmd, Constants::G_PROJECT_RUN);

    sep = new QAction(this);
    sep->setSeparator(true);
    cmd = am->registerAction(sep, QLatin1String("ProjectExplorer.CancelBuild.Sep"), globalcontext);
    mbuild->addAction(cmd, Constants::G_BUILD_CANCEL);

    //
    // Actions
    //

    // new session action
    d->m_sessionManagerAction = new QAction(tr("Session Manager..."), this);
    cmd = am->registerAction(d->m_sessionManagerAction, Constants::NEWSESSION, globalcontext);
    cmd->setDefaultKeySequence(QKeySequence());

    // new action
    d->m_newAction = new QAction(tr("New Project..."), this);
    cmd = am->registerAction(d->m_newAction, Constants::NEWPROJECT, globalcontext);
    cmd->setDefaultKeySequence(QKeySequence(tr("Ctrl+Shift+N")));
    msessionContextMenu->addAction(cmd, Constants::G_SESSION_FILES);

#if 0
    // open action
    d->m_loadAction = new QAction(tr("Load Project..."), this);
    cmd = am->registerAction(d->m_loadAction, Constants::LOAD, globalcontext);
    cmd->setDefaultKeySequence(QKeySequence(tr("Ctrl+Shift+O")));
    mfile->addAction(cmd, Core::Constants::G_FILE_PROJECT);
    msessionContextMenu->addAction(cmd, Constants::G_SESSION_FILES);
#endif

    // Default open action
    d->m_openFileAction = new QAction(tr("Open File"), this);
    cmd = am->registerAction(d->m_openFileAction, ProjectExplorer::Constants::OPENFILE,
                       globalcontext);
    mfilec->addAction(cmd, Constants::G_FILE_OPEN);

#if defined(Q_OS_WIN)
    d->m_showInGraphicalShell = new QAction(tr("Show in Explorer..."), this);
#elif defined(Q_OS_MAC)
    d->m_showInGraphicalShell = new QAction(tr("Show in Finder..."), this);
#else
    d->m_showInGraphicalShell = new QAction(tr("Show containing folder..."), this);
#endif
    cmd = am->registerAction(d->m_showInGraphicalShell, ProjectExplorer::Constants::SHOWINGRAPHICALSHELL,
                       globalcontext);
    mfilec->addAction(cmd, Constants::G_FILE_OPEN);
    mfolder->addAction(cmd, Constants::G_FOLDER_FILES);

    // Open With menu
    mfilec->addMenu(openWith, ProjectExplorer::Constants::G_FILE_OPEN);

    // recent projects menu
    Core::ActionContainer *mrecent =
        am->createMenu(Constants::M_RECENTPROJECTS);
    mrecent->menu()->setTitle(tr("Recent Projects"));
    mfile->addMenu(mrecent, Core::Constants::G_FILE_OPEN);
    connect(mfile->menu(), SIGNAL(aboutToShow()),
        this, SLOT(updateRecentProjectMenu()));

    // unload action
    d->m_unloadAction = new Core::Utils::ParameterAction(tr("Close Project"), tr("Close Project \"%1\""),
                                                      Core::Utils::ParameterAction::EnabledWithParameter, this);
    cmd = am->registerAction(d->m_unloadAction, Constants::UNLOAD, globalcontext);
    cmd->setAttribute(Core::Command::CA_UpdateText);
    cmd->setDefaultText(d->m_unloadAction->text());
    mfile->addAction(cmd, Core::Constants::G_FILE_PROJECT);
    mproject->addAction(cmd, Constants::G_PROJECT_FILES);

    // unload session action
    d->m_clearSession = new QAction(tr("Close All Projects"), this);
    cmd = am->registerAction(d->m_clearSession, Constants::CLEARSESSION, globalcontext);
    mfile->addAction(cmd, Core::Constants::G_FILE_PROJECT);
    msessionContextMenu->addAction(cmd, Constants::G_SESSION_FILES);

    // session menu
    Core::ActionContainer *msession = am->createMenu(Constants::M_SESSION);
    msession->menu()->setTitle(tr("Session"));
    mfile->addMenu(msession, Core::Constants::G_FILE_PROJECT);
    d->m_sessionMenu = msession->menu();
    connect(mfile->menu(), SIGNAL(aboutToShow()),
            this, SLOT(updateSessionMenu()));

    // build menu
    Core::ActionContainer *mbc =
        am->createMenu(Constants::BUILDCONFIGURATIONMENU);
    d->m_buildConfigurationMenu = mbc->menu();
    d->m_buildConfigurationMenu->setTitle(tr("Set Build Configuration"));
    //TODO this means it is build twice, rrr
    connect(mproject->menu(), SIGNAL(aboutToShow()), this, SLOT(populateBuildConfigurationMenu()));
    connect(mbuild->menu(), SIGNAL(aboutToShow()), this, SLOT(populateBuildConfigurationMenu()));
    connect(d->m_buildConfigurationMenu, SIGNAL(aboutToShow()), this, SLOT(populateBuildConfigurationMenu()));
    connect(d->m_buildConfigurationMenu, SIGNAL(triggered(QAction *)), this, SLOT(buildConfigurationMenuTriggered(QAction *)));

    // build session action
    QIcon buildIcon(Constants::ICON_BUILD);
    buildIcon.addFile(Constants::ICON_BUILD_SMALL);
    d->m_buildSessionAction = new QAction(buildIcon, tr("Build All"), this);
    cmd = am->registerAction(d->m_buildSessionAction, Constants::BUILDSESSION, globalcontext);
    cmd->setDefaultKeySequence(QKeySequence(tr("Ctrl+Shift+B")));
    mbuild->addAction(cmd, Constants::G_BUILD_SESSION);
    msessionContextMenu->addAction(cmd, Constants::G_SESSION_BUILD);
    // Add to mode bar
    modeManager->addAction(cmd, Constants::P_ACTION_BUILDSESSION, d->m_buildConfigurationMenu);

    // rebuild session action
    QIcon rebuildIcon(Constants::ICON_REBUILD);
    rebuildIcon.addFile(Constants::ICON_REBUILD_SMALL);
    d->m_rebuildSessionAction = new QAction(rebuildIcon, tr("Rebuild All"), this);
    cmd = am->registerAction(d->m_rebuildSessionAction, Constants::REBUILDSESSION, globalcontext);
    mbuild->addAction(cmd, Constants::G_BUILD_SESSION);
    msessionContextMenu->addAction(cmd, Constants::G_SESSION_BUILD);

    // clean session
    QIcon cleanIcon(Constants::ICON_CLEAN);
    cleanIcon.addFile(Constants::ICON_CLEAN_SMALL);
    d->m_cleanSessionAction = new QAction(cleanIcon, tr("Clean All"), this);
    cmd = am->registerAction(d->m_cleanSessionAction, Constants::CLEANSESSION, globalcontext);
    mbuild->addAction(cmd, Constants::G_BUILD_SESSION);
    msessionContextMenu->addAction(cmd, Constants::G_SESSION_BUILD);

    // build action
    d->m_buildAction = new Core::Utils::ParameterAction(tr("Build Project"), tr("Build Project \"%1\""),
                                                     Core::Utils::ParameterAction::EnabledWithParameter, this);
    cmd = am->registerAction(d->m_buildAction, Constants::BUILD, globalcontext);
    cmd->setAttribute(Core::Command::CA_UpdateText);
    cmd->setDefaultText(d->m_buildAction->text());
    cmd->setDefaultKeySequence(QKeySequence(tr("Ctrl+B")));
    mbuild->addAction(cmd, Constants::G_BUILD_PROJECT);
    mproject->addAction(cmd, Constants::G_PROJECT_BUILD);

    // rebuild action
    d->m_rebuildAction = new Core::Utils::ParameterAction(tr("Rebuild Project"), tr("Rebuild Project \"%1\""),
                                                       Core::Utils::ParameterAction::EnabledWithParameter, this);
    cmd = am->registerAction(d->m_rebuildAction, Constants::REBUILD, globalcontext);
    cmd->setAttribute(Core::Command::CA_UpdateText);
    cmd->setDefaultText(d->m_rebuildAction->text());
    mbuild->addAction(cmd, Constants::G_BUILD_PROJECT);
    mproject->addAction(cmd, Constants::G_PROJECT_BUILD);

    // clean action
    d->m_cleanAction = new Core::Utils::ParameterAction(tr("Clean Project"), tr("Clean Project \"%1\""),
                                                     Core::Utils::ParameterAction::EnabledWithParameter, this);
    cmd = am->registerAction(d->m_cleanAction, Constants::CLEAN, globalcontext);
    cmd->setAttribute(Core::Command::CA_UpdateText);
    cmd->setDefaultText(d->m_cleanAction->text());
    mbuild->addAction(cmd, Constants::G_BUILD_PROJECT);
    mproject->addAction(cmd, Constants::G_PROJECT_BUILD);

    // build without dependencies action
    d->m_buildProjectOnlyAction = new QAction(tr("Build Without Dependencies"), this);
    cmd = am->registerAction(d->m_buildProjectOnlyAction, Constants::BUILDPROJECTONLY, globalcontext);

    // rebuild without dependencies action
    d->m_rebuildProjectOnlyAction = new QAction(tr("Rebuild Without Dependencies"), this);
    cmd = am->registerAction(d->m_rebuildProjectOnlyAction, Constants::REBUILDPROJECTONLY, globalcontext);

    // clean without dependencies action
    d->m_cleanProjectOnlyAction = new QAction(tr("Clean Without Dependencies"), this);
    cmd = am->registerAction(d->m_cleanProjectOnlyAction, Constants::CLEANPROJECTONLY, globalcontext);

    // Add Set Build Configuration to menu
    mbuild->addMenu(mbc, Constants::G_BUILD_OTHER);
    mproject->addMenu(mbc, Constants::G_PROJECT_CONFIG);

    // run action
    QIcon runIcon(Constants::ICON_RUN);
    runIcon.addFile(Constants::ICON_RUN_SMALL);
    d->m_runAction = new QAction(runIcon, tr("Run"), this);
    cmd = am->registerAction(d->m_runAction, Constants::RUN, globalcontext);
    cmd->setDefaultKeySequence(QKeySequence(tr("Ctrl+R")));
    mbuild->addAction(cmd, Constants::G_BUILD_RUN);

    Core::ActionContainer *mrc = am->createMenu(Constants::RUNCONFIGURATIONMENU);
    d->m_runConfigurationMenu = mrc->menu();
    d->m_runConfigurationMenu->setTitle(tr("Set Run Configuration"));
    mbuild->addMenu(mrc, Constants::G_BUILD_RUN);
    mproject->addMenu(mrc, Constants::G_PROJECT_CONFIG);
    // TODO this recreates the menu twice if shown in the Build or context menu
    connect(d->m_runConfigurationMenu, SIGNAL(aboutToShow()), this, SLOT(populateRunConfigurationMenu()));
    connect(mproject->menu(), SIGNAL(aboutToShow()), this, SLOT(populateRunConfigurationMenu()));
    connect(mbuild->menu(), SIGNAL(aboutToShow()), this, SLOT(populateRunConfigurationMenu()));
    connect(d->m_runConfigurationMenu, SIGNAL(triggered(QAction *)), this, SLOT(runConfigurationMenuTriggered(QAction *)));

    modeManager->addAction(cmd, Constants::P_ACTION_RUN, d->m_runConfigurationMenu);

    d->m_runActionContextMenu = new QAction(runIcon, tr("Run"), this);
    cmd = am->registerAction(d->m_runActionContextMenu, Constants::RUNCONTEXTMENU, globalcontext);
    mproject->addAction(cmd, Constants::G_PROJECT_RUN);

    // cancel build action
    d->m_cancelBuildAction = new QAction(tr("Cancel Build"), this);
    cmd = am->registerAction(d->m_cancelBuildAction, Constants::CANCELBUILD, globalcontext);
    mbuild->addAction(cmd, Constants::G_BUILD_CANCEL);

    // debug action
    QIcon debuggerIcon(":/projectexplorer/images/debugger_start_small.png");
    debuggerIcon.addFile(":/projectexplorer/images/debugger_start.png");
    d->m_debugAction = new QAction(debuggerIcon, tr("Start Debugging"), this);
    cmd = am->registerAction(d->m_debugAction, Constants::DEBUG, globalcontext);
    cmd->setAttribute(Core::Command::CA_UpdateText);
    cmd->setAttribute(Core::Command::CA_UpdateIcon);
    cmd->setDefaultText(tr("Start Debugging"));
    cmd->setDefaultKeySequence(QKeySequence(tr("F5")));
    mstartdebugging->addAction(cmd, Core::Constants::G_DEFAULT_ONE);
    modeManager->addAction(cmd, Constants::P_ACTION_DEBUG, d->m_runConfigurationMenu);

    // add new file action
    d->m_addNewFileAction = new QAction(tr("Add New..."), this);
    cmd = am->registerAction(d->m_addNewFileAction, ProjectExplorer::Constants::ADDNEWFILE,
                       globalcontext);
    mproject->addAction(cmd, Constants::G_PROJECT_FILES);
    msubProject->addAction(cmd, Constants::G_PROJECT_FILES);
    mfolder->addAction(cmd, Constants::G_FOLDER_FILES);

    // add existing file action
    d->m_addExistingFilesAction = new QAction(tr("Add Existing Files..."), this);
    cmd = am->registerAction(d->m_addExistingFilesAction, ProjectExplorer::Constants::ADDEXISTINGFILES,
                       globalcontext);
    mproject->addAction(cmd, Constants::G_PROJECT_FILES);
    msubProject->addAction(cmd, Constants::G_PROJECT_FILES);
    mfolder->addAction(cmd, Constants::G_FOLDER_FILES);

    // remove file action
    d->m_removeFileAction = new QAction(tr("Remove File..."), this);
    cmd = am->registerAction(d->m_removeFileAction, ProjectExplorer::Constants::REMOVEFILE,
                       globalcontext);
    mfilec->addAction(cmd, Constants::G_FILE_OTHER);

    // renamefile action (TODO: Not supported yet)
    d->m_renameFileAction = new QAction(tr("Rename"), this);
    cmd = am->registerAction(d->m_renameFileAction, ProjectExplorer::Constants::RENAMEFILE,
                       globalcontext);
    mfilec->addAction(cmd, Constants::G_FILE_OTHER);
    d->m_renameFileAction->setEnabled(false);
    d->m_renameFileAction->setVisible(false);

    connect(core, SIGNAL(saveSettingsRequested()),
        this, SLOT(savePersistentSettings()));

    addAutoReleasedObject(new ProjectTreeWidgetFactory);
    addAutoReleasedObject(new FolderNavigationWidgetFactory);

    // > -- Creator 1.0 compatibility code
    QStringList oldRecentProjects;
    if (QSettings *s = core->settings())
        oldRecentProjects = s->value("ProjectExplorer/RecentProjects/Files", QStringList()).toStringList();
    for (QStringList::iterator it = oldRecentProjects.begin(); it != oldRecentProjects.end(); ) {
        if (QFileInfo(*it).isFile()) {
            ++it;
        } else {
            it = oldRecentProjects.erase(it);
        }
    }

    foreach(const QString &s, oldRecentProjects) {
        d->m_recentProjects.append(qMakePair(s, QFileInfo(s).fileName()));
    }
    // < -- Creator 1.0 compatibility code

    if (QSettings *s = core->settings()) {
        const QStringList fileNames = s->value("ProjectExplorer/RecentProjects/FileNames").toStringList();
        const QStringList displayNames = s->value("ProjectExplorer/RecentProjects/DisplayNames").toStringList();
        if (fileNames.size() == displayNames.size()) {
            for (int i = 0; i < fileNames.size(); ++i) {
                if (QFileInfo(fileNames.at(i)).isFile())
                    d->m_recentProjects.append(qMakePair(fileNames.at(i), displayNames.at(i)));
            }
        }
    }

    if (QSettings *s = core->settings()) {
        d->m_projectExplorerSettings.buildBeforeRun = s->value("ProjectExplorer/Settings/BuildBeforeRun", true).toBool();
        d->m_projectExplorerSettings.saveBeforeBuild = s->value("ProjectExplorer/Settings/SaveBeforeBuild", false).toBool();
        d->m_projectExplorerSettings.showCompilerOutput = s->value("ProjectExplorer/Settings/ShowCompilerOutput", false).toBool();
        d->m_projectExplorerSettings.useJom = s->value("ProjectExplorer/Settings/UseJom", true).toBool();
    }

    connect(d->m_sessionManagerAction, SIGNAL(triggered()), this, SLOT(showSessionManager()));
    connect(d->m_newAction, SIGNAL(triggered()), this, SLOT(newProject()));
#if 0
    connect(d->m_loadAction, SIGNAL(triggered()), this, SLOT(loadAction()));
#endif
    connect(d->m_buildProjectOnlyAction, SIGNAL(triggered()), this, SLOT(buildProjectOnly()));
    connect(d->m_buildAction, SIGNAL(triggered()), this, SLOT(buildProject()));
    connect(d->m_buildSessionAction, SIGNAL(triggered()), this, SLOT(buildSession()));
    connect(d->m_rebuildProjectOnlyAction, SIGNAL(triggered()), this, SLOT(rebuildProjectOnly()));
    connect(d->m_rebuildAction, SIGNAL(triggered()), this, SLOT(rebuildProject()));
    connect(d->m_rebuildSessionAction, SIGNAL(triggered()), this, SLOT(rebuildSession()));
    connect(d->m_cleanProjectOnlyAction, SIGNAL(triggered()), this, SLOT(cleanProjectOnly()));
    connect(d->m_cleanAction, SIGNAL(triggered()), this, SLOT(cleanProject()));
    connect(d->m_cleanSessionAction, SIGNAL(triggered()), this, SLOT(cleanSession()));
    connect(d->m_runAction, SIGNAL(triggered()), this, SLOT(runProject()));
    connect(d->m_runActionContextMenu, SIGNAL(triggered()), this, SLOT(runProjectContextMenu()));
    connect(d->m_cancelBuildAction, SIGNAL(triggered()), this, SLOT(cancelBuild()));
    connect(d->m_debugAction, SIGNAL(triggered()), this, SLOT(debugProject()));
    connect(d->m_unloadAction, SIGNAL(triggered()), this, SLOT(unloadProject()));
    connect(d->m_clearSession, SIGNAL(triggered()), this, SLOT(clearSession()));
    connect(d->m_addNewFileAction, SIGNAL(triggered()), this, SLOT(addNewFile()));
    connect(d->m_addExistingFilesAction, SIGNAL(triggered()), this, SLOT(addExistingFiles()));
    connect(d->m_openFileAction, SIGNAL(triggered()), this, SLOT(openFile()));
    connect(d->m_showInGraphicalShell, SIGNAL(triggered()), this, SLOT(showInGraphicalShell()));
    connect(d->m_removeFileAction, SIGNAL(triggered()), this, SLOT(removeFile()));
    connect(d->m_renameFileAction, SIGNAL(triggered()), this, SLOT(renameFile()));

    updateActions();

    connect(Core::ICore::instance(), SIGNAL(coreAboutToOpen()),
            this, SLOT(determineSessionToRestoreAtStartup()));
    connect(Core::ICore::instance(), SIGNAL(coreOpened()), this, SLOT(restoreSession()));

    updateWelcomePage();

    return true;
}

// Find a factory by file mime type in a sequence of factories
template <class Factory, class Iterator>
    Factory *findFactory(const QString &mimeType, Iterator i1, Iterator i2)
{
    for ( ; i1 != i2; ++i2) {
        Factory *f = *i1;
        if (f->mimeTypes().contains(mimeType))
            return f;
    }
    return 0;
}

ProjectFileFactory * ProjectExplorerPlugin::findProjectFileFactory(const QString &filename) const
{
    // Find factory
    if (const Core::MimeType mt = Core::ICore::instance()->mimeDatabase()->findByFile(QFileInfo(filename)))
        if (ProjectFileFactory *pf = findFactory<ProjectFileFactory>(mt.type(), d->m_fileFactories.constBegin(), d->m_fileFactories.constEnd()))
            return pf;
    qWarning("Unable to find project file factory for '%s'", filename.toUtf8().constData());
    return 0;
}

void ProjectExplorerPlugin::loadAction()
{
    if (debug)
        qDebug() << "ProjectExplorerPlugin::loadAction";


    QString dir = d->m_lastOpenDirectory;

    // for your special convenience, we preselect a pro file if it is
    // the current file
    if (Core::IEditor *editor = Core::EditorManager::instance()->currentEditor()) {
        if (const Core::IFile *file = editor->file()) {
            const QString fn = file->fileName();
            const bool isProject = d->m_profileMimeTypes.contains(file->mimeType());
            dir = isProject ? fn : QFileInfo(fn).absolutePath();
        }
    }

    QString filename = QFileDialog::getOpenFileName(0, tr("Load Project"),
                                                    dir,
                                                    d->m_projectFilterString);
    if (filename.isEmpty())
        return;
    if (ProjectFileFactory *pf = findProjectFileFactory(filename))
        pf->open(filename);
    updateActions();
}

void ProjectExplorerPlugin::unloadProject()
{
    if (debug)
        qDebug() << "ProjectExplorerPlugin::unloadProject";

    Core::IFile *fi = d->m_currentProject->file();

    if (!fi || fi->fileName().isEmpty()) //nothing to save?
        return;

    QList<Core::IFile*> filesToSave;
    filesToSave << fi;
    // FIXME: What we want here is to check whether we need to safe any of the pro/pri files in this project

    // check the number of modified files
    int readonlycount = 0;
    foreach (const Core::IFile *file, filesToSave) {
        if (file->isReadOnly())
            ++readonlycount;
    }

    bool success = false;
    if (readonlycount > 0)
        success = Core::ICore::instance()->fileManager()->saveModifiedFiles(filesToSave).isEmpty();
    else
        success = Core::ICore::instance()->fileManager()->saveModifiedFilesSilently(filesToSave).isEmpty();

    if (!success)
        return;

    addToRecentProjects(fi->fileName(), d->m_currentProject->name());
    d->m_session->removeProject(d->m_currentProject);
    updateActions();
}

void ProjectExplorerPlugin::clearSession()
{
    if (debug)
        qDebug() << "ProjectExplorerPlugin::clearSession";

    if (!d->m_session->clear())
        return; // Action has been cancelled
    updateActions();
}

void ProjectExplorerPlugin::extensionsInitialized()
{
    d->m_fileFactories = ProjectFileFactory::createFactories(&d->m_projectFilterString);
    foreach (ProjectFileFactory *pf, d->m_fileFactories) {
        d->m_profileMimeTypes += pf->mimeTypes();
        addAutoReleasedObject(pf);
    }
}

void ProjectExplorerPlugin::shutdown()
{
    d->m_session->clear();
//    d->m_proWindow->saveConfigChanges();
}

void ProjectExplorerPlugin::newProject()
{
    if (debug)
        qDebug() << "ProjectExplorerPlugin::newProject";

    QString defaultLocation;
    if (currentProject()) {
        const QFileInfo file(currentProject()->file()->fileName());
        QDir dir = file.dir();
        dir.cdUp();
        defaultLocation = dir.absolutePath();
    }

    Core::ICore::instance()->showNewItemDialog(tr("New Project", "Title of dialog"),
                              Core::IWizard::wizardsOfKind(Core::IWizard::ProjectWizard),
                              defaultLocation);
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
    SessionDialog sessionDialog(d->m_session, d->m_session->activeSession(), false);
    sessionDialog.exec();

    updateActions();

    Core::ModeManager *modeManager = Core::ModeManager::instance();
    Core::IMode *welcomeMode = modeManager->mode(Core::Constants::MODE_WELCOME);
    if (modeManager->currentMode() == welcomeMode)
        updateWelcomePage();
}

void ProjectExplorerPlugin::setStartupProject(Project *project)
{
    if (debug)
        qDebug() << "ProjectExplorerPlugin::setStartupProject";

    if (!project)
        project = d->m_currentProject;
    if (!project)
        return;
    d->m_session->setStartupProject(project);
    // NPE: Visually mark startup project
    updateActions();
}

void ProjectExplorerPlugin::savePersistentSettings()
{
    if (debug)
        qDebug()<<"ProjectExplorerPlugin::savePersistentSettings()";

    foreach (Project *pro, d->m_session->projects())
        pro->saveSettings();

    if (d->m_session->isDefaultVirgin()) {
        // do not save new virgin default sessions
    } else {
        d->m_session->save();
    }

    QSettings *s = Core::ICore::instance()->settings();
    if (s) {
        s->setValue("ProjectExplorer/StartupSession", d->m_session->file()->fileName());
        s->remove("ProjectExplorer/RecentProjects/Files");

        QStringList fileNames;
        QStringList displayNames;
        QList<QPair<QString, QString> >::const_iterator it, end;
        end = d->m_recentProjects.constEnd();
        for (it = d->m_recentProjects.constBegin(); it != end; ++it) {
            fileNames << (*it).first;
            displayNames << (*it).second;
        }

        s->setValue("ProjectExplorer/RecentProjects/FileNames", fileNames);
        s->setValue("ProjectExplorer/RecentProjects/DisplayNames", displayNames);

        s->setValue("ProjectExplorer/Settings/BuildBeforeRun", d->m_projectExplorerSettings.buildBeforeRun);
        s->setValue("ProjectExplorer/Settings/SaveBeforeBuild", d->m_projectExplorerSettings.saveBeforeBuild);
        s->setValue("ProjectExplorer/Settings/ShowCompilerOutput", d->m_projectExplorerSettings.showCompilerOutput);
        s->setValue("ProjectExplorer/Settings/UseJom", d->m_projectExplorerSettings.useJom);
    }
}

bool ProjectExplorerPlugin::openProject(const QString &fileName)
{
    if (debug)
        qDebug() << "ProjectExplorerPlugin::openProject";

    QList<Project *> list = openProjects(QStringList() << fileName);
    if (!list.isEmpty()) {
        addToRecentProjects(fileName, list.first()->name());
        return true;
    }
    return false;
}

QList<Project *> ProjectExplorerPlugin::openProjects(const QStringList &fileNames)
{
    if (debug)
        qDebug() << "ProjectExplorerPlugin - opening projects " << fileNames;

    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    QList<IProjectManager*> projectManagers = pm->getObjects<IProjectManager>();

    //QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
   // bool blocked = blockSignals(true);
    QList<Project*> openedPro;
    foreach (const QString &fileName, fileNames) {
        if (const Core::MimeType mt = Core::ICore::instance()->mimeDatabase()->findByFile(QFileInfo(fileName))) {
            foreach (IProjectManager *manager, projectManagers)
                if (manager->mimeType() == mt.type()) {
                    if (Project *pro = manager->openProject(fileName))
                        openedPro += pro;
                    d->m_session->reportProjectLoadingProgress();
                    break;
                }
        }
    }
    //blockSignals(blocked);

    if (openedPro.isEmpty()) {
        if (debug)
            qDebug() << "ProjectExplorerPlugin - Could not open any projects!";
        QApplication::restoreOverrideCursor();
        return QList<Project *>();
    }

    QList<Project *>::iterator it, end;
    end = openedPro.end();
    for (it = openedPro.begin(); it != end; ) {
        if (debug)
            qDebug()<<"restoring settings for "<<(*it)->file()->fileName();
        if ((*it)->restoreSettings()) {
            connect(*it, SIGNAL(fileListChanged()), this, SIGNAL(fileListChanged()));
            ++it;
        } else {
            delete  *it;
            it = openedPro.erase(it);
        }
    }

    d->m_session->addProjects(openedPro);

    // Make sure we always have a current project / node
    if (!d->m_currentProject && !openedPro.isEmpty())
        setCurrentNode(openedPro.first()->rootProjectNode());

    updateActions();

    Core::ModeManager::instance()->activateMode(Core::Constants::MODE_EDIT);
    QApplication::restoreOverrideCursor();

    return openedPro;
}

Project *ProjectExplorerPlugin::currentProject() const
{
    if (debug) {
        if (d->m_currentProject)
            qDebug() << "ProjectExplorerPlugin::currentProject returns " << d->m_currentProject->name();
        else
            qDebug() << "ProjectExplorerPlugin::currentProject returns 0";
    }
    return d->m_currentProject;
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

    Project *pro = d->m_session->startupProject();

    if (!pro)
        pro = d->m_currentProject;

    return pro;
}

// update welcome page
void ProjectExplorerPlugin::updateWelcomePage()
{
    ProjectWelcomePageWidget::WelcomePageData welcomePageData;
    welcomePageData.sessionList =  d->m_session->sessions();
    welcomePageData.activeSession = d->m_session->activeSession();
    welcomePageData.previousSession = d->m_session->lastSession();
    welcomePageData.projectList = d->m_recentProjects;
    d->m_welcomePage->updateWelcomePage(welcomePageData);
}

void ProjectExplorerPlugin::currentModeChanged(Core::IMode *)
{
        updateWelcomePage();
}

void ProjectExplorerPlugin::determineSessionToRestoreAtStartup()
{
    QStringList sessions = d->m_session->sessions();
    // We have command line arguments, try to find a session in them
    QStringList arguments = ExtensionSystem::PluginManager::instance()->arguments();
    // Default to no session loading
    d->m_sessionToRestoreAtStartup = QString::null;
    foreach (const QString &arg, arguments) {
        if (sessions.contains(arg)) {
            // Session argument
            d->m_sessionToRestoreAtStartup = arg;
            break;
        }
    }
    if (!d->m_sessionToRestoreAtStartup.isNull())
        Core::ICore::instance()->modeManager()->activateMode(Core::Constants::MODE_EDIT);
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
    QStringList arguments = ExtensionSystem::PluginManager::instance()->arguments();
    arguments.removeOne(d->m_sessionToRestoreAtStartup);

    // Restore latest session or what was passed on the command line
    if (d->m_sessionToRestoreAtStartup == QString::null) {
        d->m_session->createAndLoadNewDefaultSession();
    } else {
        d->m_session->loadSession(d->m_sessionToRestoreAtStartup);
    }

    // update welcome page
    Core::ModeManager *modeManager = Core::ModeManager::instance();
    connect(modeManager, SIGNAL(currentModeChanged(Core::IMode*)), this, SLOT(currentModeChanged(Core::IMode*)));
    connect(d->m_welcomePage, SIGNAL(requestSession(QString)), this, SLOT(loadSession(QString)));
    connect(d->m_welcomePage, SIGNAL(requestProject(QString)), this, SLOT(loadProject(QString)));

    Core::ICore::instance()->openFiles(arguments);
    updateActions();

}

void ProjectExplorerPlugin::loadSession(const QString &session)
{
    if (debug)
        qDebug() << "ProjectExplorerPlugin::loadSession" << session;
    d->m_session->loadSession(session);
}


void ProjectExplorerPlugin::showContextMenu(const QPoint &globalPos, Node *node)
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
    if (contextMenu && contextMenu->actions().count() > 0) {
        contextMenu->popup(globalPos);
    }
}

BuildManager *ProjectExplorerPlugin::buildManager() const
{
    return d->m_buildManager;
}

void ProjectExplorerPlugin::buildStateChanged(Project * pro)
{
    if (debug) {
        qDebug() << "buildStateChanged";
        qDebug() << pro->file()->fileName() << "isBuilding()" << d->m_buildManager->isBuilding(pro);
    }
    Q_UNUSED(pro)
    updateActions();
}

void ProjectExplorerPlugin::executeRunConfiguration(const QSharedPointer<RunConfiguration> &runConfiguration, const QString &runMode)
{
    if (IRunControlFactory *runControlFactory = findRunControlFactory(runConfiguration, runMode)) {
        emit aboutToExecuteProject(runConfiguration->project());

        RunControl *control = runControlFactory->create(runConfiguration, runMode);
        d->m_outputPane->createNewOutputWindow(control);
        if (runMode == ProjectExplorer::Constants::RUNMODE)
            d->m_outputPane->popup(false);
        d->m_outputPane->showTabFor(control);

        connect(control, SIGNAL(addToOutputWindow(RunControl *, const QString &)),
                this, SLOT(addToApplicationOutputWindow(RunControl *, const QString &)));
        connect(control, SIGNAL(addToOutputWindowInline(RunControl *, const QString &)),
                this, SLOT(addToApplicationOutputWindowInline(RunControl *, const QString &)));
        connect(control, SIGNAL(error(RunControl *, const QString &)),
                this, SLOT(addErrorToApplicationOutputWindow(RunControl *, const QString &)));
        connect(control, SIGNAL(finished()),
                this, SLOT(runControlFinished()));

        if (runMode == ProjectExplorer::Constants::DEBUGMODE)
            d->m_debuggingRunControl = control;

        control->start();
        updateRunAction();
    }

}

void ProjectExplorerPlugin::buildQueueFinished(bool success)
{
    if (debug)
        qDebug() << "buildQueueFinished()" << success;

    updateActions();

    if (success && d->m_delayedRunConfiguration) {
        executeRunConfiguration(d->m_delayedRunConfiguration, d->m_runMode);
    } else {
        if (d->m_buildManager->tasksAvailable())
            d->m_buildManager->showTaskWindow();
    }
    d->m_delayedRunConfiguration = QSharedPointer<RunConfiguration>(0);
    d->m_runMode = QString::null;
}

void ProjectExplorerPlugin::setCurrent(Project *project, QString filePath, Node *node)
{
    if (debug)
        qDebug() << "ProjectExplorer - setting path to " << (node ? node->path() : filePath)
                << " and project to " << (project ? project->name() : "0");

    if (node)
        filePath = node->path();
    else
        node = d->m_session->nodeForFile(filePath, project);

    Core::ICore *core = Core::ICore::instance();

    bool projectChanged = false;
    if (d->m_currentProject != project) {
        int oldContext = -1;
        int newContext = -1;
        int oldLanguageID = -1;
        int newLanguageID = -1;
        if (d->m_currentProject) {
            oldContext = d->m_currentProject->projectManager()->projectContext();
            oldLanguageID = d->m_currentProject->projectManager()->projectLanguage();
        }
        if (project) {
            newContext = project->projectManager()->projectContext();
            newLanguageID = project->projectManager()->projectLanguage();
        }
        core->removeAdditionalContext(oldContext);
        core->removeAdditionalContext(oldLanguageID);
        core->addAdditionalContext(newContext);
        core->addAdditionalContext(newLanguageID);
        core->updateContext();

        d->m_currentProject = project;

        projectChanged = true;
    }

    if (projectChanged || d->m_currentNode != node) {
        d->m_currentNode = node;
        if (debug)
            qDebug() << "ProjectExplorer - currentNodeChanged(" << (node ? node->path() : "0") << ", " << (project ? project->name() : "0") << ")";
        emit currentNodeChanged(d->m_currentNode, project);
    }
    if (projectChanged) {
        if (debug)
            qDebug() << "ProjectExplorer - currentProjectChanged(" << (project ? project->name() : "0") << ")";
        // Enable the right VCS
        if (const Core::IFile *projectFile = project ? project->file() : static_cast<const Core::IFile *>(0)) {
            core->vcsManager()->setVCSEnabled(QFileInfo(projectFile->fileName()).absolutePath());
        } else {
            core->vcsManager()->setAllVCSEnabled();
        }

        emit currentProjectChanged(project);
        updateActions();
    }

    core->fileManager()->setCurrentFile(filePath);
}

void ProjectExplorerPlugin::updateActions()
{
    if (debug)
        qDebug() << "ProjectExplorerPlugin::updateActions";

    bool enableBuildActions = d->m_currentProject && ! (d->m_buildManager->isBuilding(d->m_currentProject));
    bool hasProjects = !d->m_session->projects().isEmpty();
    bool building = d->m_buildManager->isBuilding();
    QString projectName = d->m_currentProject ? d->m_currentProject->name() : QString();

    if (debug)
        qDebug() << "BuildManager::isBuilding()" << building;

    d->m_unloadAction->setParameter(projectName);

    d->m_buildAction->setParameter(projectName);
    d->m_rebuildAction->setParameter(projectName);
    d->m_cleanAction->setParameter(projectName);

    d->m_buildProjectOnlyAction->setEnabled(enableBuildActions);
    d->m_rebuildProjectOnlyAction->setEnabled(enableBuildActions);
    d->m_cleanProjectOnlyAction->setEnabled(enableBuildActions);

    d->m_clearSession->setEnabled(hasProjects && !building);
    d->m_buildSessionAction->setEnabled(hasProjects && !building);
    d->m_rebuildSessionAction->setEnabled(hasProjects && !building);
    d->m_cleanSessionAction->setEnabled(hasProjects && !building);
    d->m_cancelBuildAction->setEnabled(building);

    updateRunAction();
}

// NBS TODO check projectOrder()
// what we want here is all the projects pro depends on
QStringList ProjectExplorerPlugin::allFilesWithDependencies(Project *pro)
{
    if (debug)
        qDebug() << "ProjectExplorerPlugin::allFilesWithDependencies(" << pro->file()->fileName() << ")";

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

    QList<Core::IFile *> filesToSave = Core::ICore::instance()->fileManager()->modifiedFiles();
    if (!filesToSave.isEmpty()) {
        if (d->m_projectExplorerSettings.saveBeforeBuild) {
            Core::ICore::instance()->fileManager()->saveModifiedFilesSilently(filesToSave);
        } else {
            bool cancelled = false;
            bool alwaysSave = false;

            Core::FileManager *fm = Core::ICore::instance()->fileManager();
            fm->saveModifiedFiles(filesToSave, &cancelled, QString::null,
                                  "Always save files before build", &alwaysSave);

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

void ProjectExplorerPlugin::buildProjectOnly()
{
    if (debug)
        qDebug() << "ProjectExplorerPlugin::buildProjectOnly";

    if (saveModifiedFiles())
        buildManager()->buildProject(d->m_currentProject, d->m_currentProject->activeBuildConfiguration());
}

static QStringList configurations(const QList<Project *> &projects)
{
    QStringList result;
    foreach (const Project * pro, projects)
        result << pro->activeBuildConfiguration();
    return result;
}

void ProjectExplorerPlugin::buildProject()
{
    if (debug)
        qDebug() << "ProjectExplorerPlugin::buildProject";

    if (saveModifiedFiles()) {
        const QList<Project *> & projects = d->m_session->projectOrder(d->m_currentProject);
        d->m_buildManager->buildProjects(projects, configurations(projects));
    }
}

void ProjectExplorerPlugin::buildSession()
{
    if (debug)
        qDebug() << "ProjectExplorerPlugin::buildSession";

    if (saveModifiedFiles()) {
        const QList<Project *> & projects = d->m_session->projectOrder();
        d->m_buildManager->buildProjects(projects, configurations(projects));
    }
}

void ProjectExplorerPlugin::rebuildProjectOnly()
{
    if (debug)
        qDebug() << "ProjectExplorerPlugin::rebuildProjectOnly";

    if (saveModifiedFiles()) {
        d->m_buildManager->cleanProject(d->m_currentProject, d->m_currentProject->activeBuildConfiguration());
        d->m_buildManager->buildProject(d->m_currentProject, d->m_currentProject->activeBuildConfiguration());
    }
}

void ProjectExplorerPlugin::rebuildProject()
{
    if (debug)
        qDebug() << "ProjectExplorerPlugin::rebuildProject";

    if (saveModifiedFiles()) {
        const QList<Project *> & projects = d->m_session->projectOrder(d->m_currentProject);
        const QStringList configs = configurations(projects);

        d->m_buildManager->cleanProjects(projects, configs);
        d->m_buildManager->buildProjects(projects, configs);
    }
}

void ProjectExplorerPlugin::rebuildSession()
{
    if (debug)
        qDebug() << "ProjectExplorerPlugin::rebuildSession";

    if (saveModifiedFiles()) {
        const QList<Project *> & projects = d->m_session->projectOrder();
        const QStringList configs = configurations(projects);

        d->m_buildManager->cleanProjects(projects, configs);
        d->m_buildManager->buildProjects(projects, configs);
    }
}

void ProjectExplorerPlugin::cleanProjectOnly()
{
    if (debug)
        qDebug() << "ProjectExplorerPlugin::cleanProjectOnly";

    if (saveModifiedFiles())
        d->m_buildManager->cleanProject(d->m_currentProject, d->m_currentProject->activeBuildConfiguration());
}

void ProjectExplorerPlugin::cleanProject()
{
    if (debug)
        qDebug() << "ProjectExplorerPlugin::cleanProject";

    if (saveModifiedFiles()) {
        const QList<Project *> & projects = d->m_session->projectOrder(d->m_currentProject);
        d->m_buildManager->cleanProjects(projects, configurations(projects));
    }
}

void ProjectExplorerPlugin::cleanSession()
{
    if (debug)
        qDebug() << "ProjectExplorerPlugin::cleanSession";

    if (saveModifiedFiles()) {
        const QList<Project *> & projects = d->m_session->projectOrder();
        d->m_buildManager->cleanProjects(projects, configurations(projects));
    }
}

void ProjectExplorerPlugin::runProject()
{
    runProjectImpl(startupProject());
}

void ProjectExplorerPlugin::runProjectContextMenu()
{
    runProjectImpl(d->m_currentProject);
}

void ProjectExplorerPlugin::runProjectImpl(Project *pro)
{
    if (!pro)
        return;

    if (d->m_projectExplorerSettings.buildBeforeRun) {
        if (saveModifiedFiles()) {
            d->m_runMode = ProjectExplorer::Constants::RUNMODE;
            d->m_delayedRunConfiguration = pro->activeRunConfiguration();

            const QList<Project *> & projects = d->m_session->projectOrder(pro);
            d->m_buildManager->buildProjects(projects, configurations(projects));
        }
    } else {
        executeRunConfiguration(pro->activeRunConfiguration(), ProjectExplorer::Constants::RUNMODE);
    }
}

void ProjectExplorerPlugin::debugProject()
{
    Project *pro = startupProject();
    if (!pro || d->m_debuggingRunControl )
        return;

    if (d->m_projectExplorerSettings.buildBeforeRun) {
        if (saveModifiedFiles()) {
            d->m_runMode = ProjectExplorer::Constants::DEBUGMODE;
            d->m_delayedRunConfiguration = pro->activeRunConfiguration();

            const QList<Project *> & projects = d->m_session->projectOrder(pro);
            d->m_buildManager->buildProjects(projects, configurations(projects));

            updateRunAction();
        }
    } else {
        executeRunConfiguration(pro->activeRunConfiguration(), ProjectExplorer::Constants::DEBUGMODE);
    }
}

void ProjectExplorerPlugin::addToApplicationOutputWindow(RunControl *rc, const QString &line)
{
    d->m_outputPane->appendOutput(rc, line);
}

void ProjectExplorerPlugin::addToApplicationOutputWindowInline(RunControl *rc, const QString &line)
{
    d->m_outputPane->appendOutputInline(rc, line);
}

void ProjectExplorerPlugin::addErrorToApplicationOutputWindow(RunControl *rc, const QString &error)
{
    d->m_outputPane->appendOutput(rc, error);
}

void ProjectExplorerPlugin::runControlFinished()
{
    if (sender() == d->m_debuggingRunControl)
        d->m_debuggingRunControl = 0;

    updateRunAction();
}

void ProjectExplorerPlugin::startupProjectChanged()
{
    static QPointer<Project> previousStartupProject = 0;
    Project *project = startupProject();
    if (project == previousStartupProject)
        return;

    if (previousStartupProject) {
        disconnect(previousStartupProject, SIGNAL(activeRunConfigurationChanged()),
                   this, SLOT(updateRunAction()));
    }

    previousStartupProject = project;

    if (project) {
        connect(project, SIGNAL(activeRunConfigurationChanged()),
                this, SLOT(updateRunAction()));
    }

    updateRunAction();
}

// NBS TODO implement more than one runner
IRunControlFactory *ProjectExplorerPlugin::findRunControlFactory(const QSharedPointer<RunConfiguration> &config, const QString &mode)
{
    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    const QList<IRunControlFactory *> factories = pm->getObjects<IRunControlFactory>();
    foreach (IRunControlFactory *f, factories)
        if (f->canRun(config, mode))
            return f;
    return 0;
}

void ProjectExplorerPlugin::updateRunAction()
{
    const Project *project = startupProject();
    bool canRun = project && findRunControlFactory(project->activeRunConfiguration(), ProjectExplorer::Constants::RUNMODE);
    const bool canDebug = project && !d->m_debuggingRunControl && findRunControlFactory(project->activeRunConfiguration(), ProjectExplorer::Constants::DEBUGMODE);
    const bool building = d->m_buildManager->isBuilding();
    d->m_runAction->setEnabled(canRun && !building);

    canRun = d->m_currentProject && findRunControlFactory(d->m_currentProject->activeRunConfiguration(), ProjectExplorer::Constants::RUNMODE);
    d->m_runActionContextMenu->setEnabled(canRun && !building);

    d->m_debugAction->setEnabled(canDebug && !building);
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
    for(it = d->m_recentProjects.begin(); it != d->m_recentProjects.end();)
        if ((*it).first == prettyFileName)
            it = d->m_recentProjects.erase(it);
        else
            ++it;

    if (d->m_recentProjects.count() > d->m_maxRecentProjects)
        d->m_recentProjects.removeLast();
    d->m_recentProjects.prepend(qMakePair(prettyFileName, displayName));
    QFileInfo fi(prettyFileName);
    d->m_lastOpenDirectory = fi.absolutePath();
}

void ProjectExplorerPlugin::updateRecentProjectMenu()
{
    if (debug)
        qDebug() << "ProjectExplorerPlugin::updateRecentProjectMenu";

    Core::ActionContainer *aci =
        Core::ICore::instance()->actionManager()->actionContainer(Constants::M_RECENTPROJECTS);
    QMenu *menu = aci->menu();
    menu->clear();

    menu->setEnabled(!d->m_recentProjects.isEmpty());

    //projects (ignore sessions, they used to be in this list)

    QList<QPair<QString, QString> >::const_iterator it, end;
    end = d->m_recentProjects.constEnd();
    for (it = d->m_recentProjects.constBegin(); it != end; ++it) {
        const QPair<QString, QString> &s = *it;
        if (s.first.endsWith(".qws"))
            continue;
        QAction *action = menu->addAction(s.first);
        action->setData(s.first);
        connect(action, SIGNAL(triggered()), this, SLOT(openRecentProject()));
    }
}

void ProjectExplorerPlugin::openRecentProject()
{
    if (debug)
        qDebug() << "ProjectExplorerPlugin::openRecentProject()";

    QAction *a = qobject_cast<QAction*>(sender());
    if (!a)
        return;
    QString fileName = a->data().toString();
    if (!fileName.isEmpty())
        openProject(fileName);
}

void ProjectExplorerPlugin::invalidateProject(Project *project)
{
    if (debug)
        qDebug() << "ProjectExplorerPlugin::invalidateProject" << project->name();
    if (d->m_currentProject == project) {
        //
        // Workaround for a bug in QItemSelectionModel
        // - currentChanged etc are not emitted if the
        // item is removed from the underlying data model
        //
        setCurrent(0, QString(), 0);
    }

    disconnect(project, SIGNAL(fileListChanged()), this, SIGNAL(fileListChanged()));
}

void ProjectExplorerPlugin::goToTaskWindow()
{
    d->m_buildManager->gotoTaskWindow();
}

void ProjectExplorerPlugin::updateContextMenuActions()
{
    if (ProjectNode *projectNode = qobject_cast<ProjectNode*>(d->m_currentNode)) {
        const bool addFilesEnabled = projectNode->supportedActions().contains(ProjectNode::AddFile);
        d->m_addExistingFilesAction->setEnabled(addFilesEnabled);
        d->m_addNewFileAction->setEnabled(addFilesEnabled);
    } else if (FileNode *fileNode = qobject_cast<FileNode*>(d->m_currentNode)) {
        const bool removeFileEnabled = fileNode->projectNode()->supportedActions().contains(ProjectNode::RemoveFile);
        d->m_removeFileAction->setEnabled(removeFileEnabled);
    }
}

void ProjectExplorerPlugin::addNewFile()
{
    QTC_ASSERT(d->m_currentNode, return)
    QFileInfo fi(d->m_currentNode->path());
    const QString location = (fi.isDir() ? fi.absoluteFilePath() : fi.absolutePath());
    Core::ICore::instance()->showNewItemDialog(tr("New File", "Title of dialog"),
                              Core::IWizard::wizardsOfKind(Core::IWizard::FileWizard)
                              + Core::IWizard::wizardsOfKind(Core::IWizard::ClassWizard),
                              location);
}

void ProjectExplorerPlugin::addExistingFiles()
{
    QTC_ASSERT(d->m_currentNode, return)

    ProjectNode *projectNode = qobject_cast<ProjectNode*>(d->m_currentNode->projectNode());
    Core::ICore *core = Core::ICore::instance();
    QFileInfo fi(d->m_currentNode->path());
    const QString dir = (fi.isDir() ? fi.absoluteFilePath() : fi.absolutePath());
    QStringList fileNames = QFileDialog::getOpenFileNames(core->mainWindow(), tr("Add Existing Files"), dir);
    if (fileNames.isEmpty())
        return;

    QHash<FileType, QString> fileTypeToFiles;
    foreach (const QString &fileName, fileNames) {
        FileType fileType = typeForFileName(core->mimeDatabase(), QFileInfo(fileName));
        fileTypeToFiles.insertMulti(fileType, fileName);
    }

    QStringList notAdded;
    foreach (const FileType type, fileTypeToFiles.uniqueKeys()) {
        projectNode->addFiles(type, fileTypeToFiles.values(type), &notAdded);
    }
    if (!notAdded.isEmpty()) {
        QString message = tr("Could not add following files to project %1:\n").arg(projectNode->name());
        QString files = notAdded.join("\n");
        QMessageBox::warning(core->mainWindow(), tr("Add files to project failed"),
                             message + files);
        foreach (const QString &file, notAdded)
            fileNames.removeOne(file);
    }

    if (Core::IVersionControl *vcManager = core->vcsManager()->findVersionControlForDirectory(dir))
        if (vcManager->supportsOperation(Core::IVersionControl::AddOperation)) {
            const QString files = fileNames.join(QString(QLatin1Char('\n')));
            QMessageBox::StandardButton button =
                QMessageBox::question(core->mainWindow(), tr("Add to Version Control"),
                                      tr("Add files\n%1\nto version control (%2)?").arg(files, vcManager->name()),
                                      QMessageBox::Yes | QMessageBox::No);
            if (button == QMessageBox::Yes) {
                QStringList notAddedToVc;
                foreach (const QString &file, fileNames) {
                    if (!vcManager->vcsAdd(file))
                        notAddedToVc << file;
                }

                if (!notAddedToVc.isEmpty()) {
                    const QString message = tr("Could not add following files to version control (%1)\n").arg(vcManager->name());
                    const QString filesNotAdded = notAddedToVc.join(QString(QLatin1Char('\n')));
                    QMessageBox::warning(core->mainWindow(), tr("Add files to version control failed"),
                                         message + filesNotAdded);
                }
            }
        }
}

void ProjectExplorerPlugin::openFile()
{
    QTC_ASSERT(d->m_currentNode, return)
    Core::EditorManager *em = Core::EditorManager::instance();
    em->openEditor(d->m_currentNode->path());
    em->ensureEditorManagerVisible();
}

void ProjectExplorerPlugin::showInGraphicalShell()
{
    QTC_ASSERT(d->m_currentNode, return)
#if defined(Q_OS_WIN)
    QString explorer = Environment::systemEnvironment().searchInPath("explorer.exe");
    if (explorer.isEmpty()) {
        QMessageBox::warning(Core::ICore::instance()->mainWindow(),
                             tr("Launching Windows Explorer failed"),
                             tr("Could not find explorer.exe in path to launch Windows Explorer."));
        return;
    }
    QProcess::execute(explorer,
                      QStringList() << QString("/select,%1")
                      .arg(QDir::toNativeSeparators(d->m_currentNode->path())));
#elif defined(Q_OS_MAC)
    QProcess::execute("/usr/bin/osascript", QStringList()
                      << "-e"
                      << QString("tell application \"Finder\" to reveal POSIX file \"%1\"")
                      .arg(d->m_currentNode->path()));
    QProcess::execute("/usr/bin/osascript", QStringList()
                      << "-e"
                      << "tell application \"Finder\" to activate");
#else
    // we cannot select a file here, because no file browser really supports it...
    QFileInfo fileInfo(d->m_currentNode->path());
    QString xdgopen = Environment::systemEnvironment().searchInPath("xdg-open");
    if (xdgopen.isEmpty()) {
        QMessageBox::warning(Core::ICore::instance()->mainWindow(),
                             tr("Launching a file explorer failed"),
                             tr("Could not find xdg-open to launch the native file explorer."));
        return;
    }
    QProcess::execute(xdgopen, QStringList() <<  fileInfo.path());
#endif
}

void ProjectExplorerPlugin::removeFile()
{
    QTC_ASSERT(d->m_currentNode && d->m_currentNode->nodeType() == FileNodeType, return)

    FileNode *fileNode = qobject_cast<FileNode*>(d->m_currentNode);
    Core::ICore *core = Core::ICore::instance();

    const QString filePath = d->m_currentNode->path();
    const QString fileDir = QFileInfo(filePath).dir().absolutePath();
    RemoveFileDialog removeFileDialog(filePath, core->mainWindow());

    if (removeFileDialog.exec() == QDialog::Accepted) {
        const bool deleteFile = removeFileDialog.isDeleteFileChecked();

        // remove from project
        ProjectNode *projectNode = fileNode->projectNode();
        Q_ASSERT(projectNode);

        if (!projectNode->removeFiles(fileNode->fileType(), QStringList(filePath))) {
            QMessageBox::warning(core->mainWindow(), tr("Remove file failed"),
                                 tr("Could not remove file %1 from project %2.").arg(filePath).arg(projectNode->name()));
            return;
        }

        // remove from version control
        core->vcsManager()->showDeleteDialog(filePath);

        // remove from file system
        if (deleteFile) {
            QFile file(filePath);

            if (file.exists()) {
                // could have been deleted by vc
                if (!file.remove())
                    QMessageBox::warning(core->mainWindow(), tr("Delete file failed"),
                                         tr("Could not delete file %1.").arg(filePath));
            }
        }
    }
}

void ProjectExplorerPlugin::renameFile()
{
    QWidget *focusWidget = QApplication::focusWidget();
    while (focusWidget) {
        ProjectTreeWidget *treeWidget = qobject_cast<ProjectTreeWidget*>(focusWidget);
        if (treeWidget) {
            treeWidget->editCurrentItem();
            break;
        }
        focusWidget = focusWidget->parentWidget();
    }
}

void ProjectExplorerPlugin::populateBuildConfigurationMenu()
{
    if (debug)
        qDebug() << "ProjectExplorerPlugin::populateBuildConfigurationMenu";

    // delete the old actiongroup and all actions that are children of it
    delete d->m_buildConfigurationActionGroup;
    d->m_buildConfigurationActionGroup = new QActionGroup(d->m_buildConfigurationMenu);
    d->m_buildConfigurationMenu->clear();
    if (Project *pro = d->m_currentProject) {
        const QString &activeBuildConfiguration = pro->activeBuildConfiguration();
        foreach (const QString &buildConfiguration, pro->buildConfigurations()) {
            QString displayName = pro->buildConfiguration(buildConfiguration)->displayName();
            QAction *act = new QAction(displayName, d->m_buildConfigurationActionGroup);
            if (debug)
                qDebug() << "BuildConfiguration " << buildConfiguration << "active: " << activeBuildConfiguration;
            act->setCheckable(true);
            act->setChecked(buildConfiguration == activeBuildConfiguration);
            act->setData(buildConfiguration);
            d->m_buildConfigurationMenu->addAction(act);
        }
        d->m_buildConfigurationMenu->setEnabled(true);
    } else {
        d->m_buildConfigurationMenu->setEnabled(false);
    }
}

void ProjectExplorerPlugin::buildConfigurationMenuTriggered(QAction *action)
{
    if (debug)
        qDebug() << "ProjectExplorerPlugin::buildConfigurationMenuTriggered";

    d->m_currentProject->setActiveBuildConfiguration(action->data().toString());
}

void ProjectExplorerPlugin::populateRunConfigurationMenu()
{
    if (debug)
        qDebug() << "ProjectExplorerPlugin::populateRunConfigurationMenu";

    delete d->m_runConfigurationActionGroup;
    d->m_runConfigurationActionGroup = new QActionGroup(d->m_runConfigurationMenu);
    d->m_runConfigurationMenu->clear();

    const Project *startupProject = d->m_session->startupProject();
    QSharedPointer<RunConfiguration> activeRunConfiguration
            = (startupProject) ? startupProject->activeRunConfiguration() : QSharedPointer<RunConfiguration>(0);

    foreach (const Project *pro, d->m_session->projects()) {
        foreach (QSharedPointer<RunConfiguration> runConfiguration, pro->runConfigurations()) {
            if (runConfiguration->isEnabled()) {
                const QString title = QString("%1 (%2)").arg(pro->name(), runConfiguration->name());
                QAction *act = new QAction(title, d->m_runConfigurationActionGroup);
                act->setCheckable(true);
                act->setData(qVariantFromValue(runConfiguration));
                act->setChecked(runConfiguration == activeRunConfiguration);
                d->m_runConfigurationMenu->addAction(act);
                if (debug)
                    qDebug() << "RunConfiguration" << runConfiguration << "project:" << pro->name()
                             << "active:" << (runConfiguration == activeRunConfiguration);
            }
        }
    }

    d->m_runConfigurationMenu->setDisabled(d->m_runConfigurationMenu->actions().isEmpty());
}

void ProjectExplorerPlugin::runConfigurationMenuTriggered(QAction *action)
{
    if (debug)
        qDebug() << "ProjectExplorerPlugin::runConfigurationMenuTriggered" << action;

        QSharedPointer<RunConfiguration> runConfiguration = qVariantValue<QSharedPointer<RunConfiguration> >(action->data());
        runConfiguration->project()->setActiveRunConfiguration(runConfiguration);
        setStartupProject(runConfiguration->project());
}

void ProjectExplorerPlugin::populateOpenWithMenu()
{
    typedef QList<Core::IEditorFactory*> EditorFactoryList;
    typedef QList<Core::IExternalEditor*> ExternalEditorList;

    d->m_openWithMenu->clear();

    bool anyMatches = false;
    const QString fileName = currentNode()->path();

    Core::ICore *core = Core::ICore::instance();
    if (const Core::MimeType mt = core->mimeDatabase()->findByFile(QFileInfo(fileName))) {
        const EditorFactoryList factories = core->editorManager()->editorFactories(mt, false);
        const ExternalEditorList externalEditors = core->editorManager()->externalEditors(mt, false);
        anyMatches = !factories.empty() || !externalEditors.empty();
        if (anyMatches) {
            const QList<Core::IEditor *> editorsOpenForFile = core->editorManager()->editorsForFileName(fileName);
            // Add all suitable editors
            foreach (Core::IEditorFactory *editorFactory, factories) {
                // Add action to open with this very editor factory
                QString const actionTitle = qApp->translate("OpenWith::Editors", editorFactory->kind().toAscii());
                QAction * const action = d->m_openWithMenu->addAction(actionTitle);
                action->setData(qVariantFromValue(editorFactory));
                // File already open in an editor -> only enable that entry since
                // we currently do not support opening a file in two editors at once
                if (!editorsOpenForFile.isEmpty()) {
                    bool enabled = false;
                    foreach (Core::IEditor * const openEditor, editorsOpenForFile) {
                        if (editorFactory->kind() == QLatin1String(openEditor->kind()))
                            enabled = true;
                        break;
                    }
                    action->setEnabled(enabled);
                }
            } // for editor factories
            // Add all suitable external editors
            foreach (Core::IExternalEditor *externalEditor, externalEditors) {
                QAction * const action = d->m_openWithMenu->addAction(qApp->translate("OpenWith::Editors", externalEditor->kind().toAscii()));
                action->setData(qVariantFromValue(externalEditor));
            }
        } // matches
    }
    d->m_openWithMenu->setEnabled(anyMatches);
}

void ProjectExplorerPlugin::openWithMenuTriggered(QAction *action)
{
    if (!action) {
        qWarning() << "ProjectExplorerPlugin::openWithMenuTriggered no action, can't happen.";
        return;
    }
    Core::EditorManager *em = Core::EditorManager::instance();
    const QVariant data = action->data();
    if (qVariantCanConvert<Core::IEditorFactory *>(data)) {
        Core::IEditorFactory *factory = qVariantValue<Core::IEditorFactory *>(data);
        em->openEditor(currentNode()->path(), factory->kind());
        em->ensureEditorManagerVisible();
        return;
    }
    if (qVariantCanConvert<Core::IExternalEditor *>(data)) {
        Core::IExternalEditor *externalEditor = qVariantValue<Core::IExternalEditor *>(data);
        em->openExternalEditor(currentNode()->path(), externalEditor->kind());
    }
}

void ProjectExplorerPlugin::updateSessionMenu()
{
    d->m_sessionMenu->clear();
    QActionGroup *ag = new QActionGroup(d->m_sessionMenu);
    connect(ag, SIGNAL(triggered(QAction *)), this, SLOT(setSession(QAction *)));
    const QString &activeSession = d->m_session->activeSession();
    foreach (const QString &session, d->m_session->sessions()) {
        QAction *act = ag->addAction(session);
        act->setCheckable(true);
        if (session == activeSession)
            act->setChecked(true);
    }
    d->m_sessionMenu->addActions(ag->actions());
    d->m_sessionMenu->addSeparator();
    d->m_sessionMenu->addAction(d->m_sessionManagerAction);

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

Q_EXPORT_PLUGIN(ProjectExplorerPlugin)
