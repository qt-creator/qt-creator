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

#include "projectexplorer.h"

#include "buildsteplist.h"
#include "configtaskhandler.h"
#include "customwizard/customwizard.h"
#include "deployablefile.h"
#include "deployconfiguration.h"
#include "gcctoolchainfactories.h"
#ifdef WITH_JOURNALD
#include "journaldwatcher.h"
#endif
#include "jsonwizard/jsonwizardfactory.h"
#include "jsonwizard/jsonwizardgeneratorfactory.h"
#include "jsonwizard/jsonwizardpagefactory_p.h"
#include "project.h"
#include "projectexplorersettings.h"
#include "removetaskhandler.h"
#include "unconfiguredprojectpanel.h"
#include "kitfeatureprovider.h"
#include "kitmanager.h"
#include "kitoptionspage.h"
#include "target.h"
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
#include "nodesvisitor.h"
#include "appoutputpane.h"
#include "processstep.h"
#include "kitinformation.h"
#include "projectfilewizardextension.h"
#include "projecttreewidget.h"
#include "projectwindow.h"
#include "runsettingspropertiespage.h"
#include "session.h"
#include "projectnodes.h"
#include "sessiondialog.h"
#include "projectexplorersettingspage.h"
#include "corelistenercheckingforrunningbuild.h"
#include "buildconfiguration.h"
#include "miniprojecttargetselector.h"
#include "taskhub.h"
#include "customtoolchain.h"
#include "selectablefilesmodel.h"
#include <projectexplorer/customwizard/customwizard.h>
#include "devicesupport/desktopdevice.h"
#include "devicesupport/desktopdevicefactory.h"
#include "devicesupport/devicemanager.h"
#include "devicesupport/devicesettingspage.h"
#include "targetsettingspanel.h"
#include "projectpanelfactory.h"

#ifdef Q_OS_WIN
#    include "windebuginterface.h"
#    include "msvctoolchain.h"
#    include "wincetoolchain.h"
#endif

#include "projecttree.h"
#include "projectwelcomepage.h"

#include <extensionsystem/pluginspec.h>
#include <extensionsystem/pluginmanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/id.h>
#include <coreplugin/idocumentfactory.h>
#include <coreplugin/idocument.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/documentmanager.h>
#include <coreplugin/imode.h>
#include <coreplugin/modemanager.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/infobar.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/findplaceholder.h>
#include <coreplugin/vcsmanager.h>
#include <coreplugin/iversioncontrol.h>
#include <coreplugin/fileutils.h>
#include <coreplugin/removefiledialog.h>
#include <texteditor/findinfiles.h>
#include <ssh/sshconnection.h>

#include <utils/algorithm.h>
#include <utils/fileutils.h>
#include <utils/macroexpander.h>
#include <utils/mimetypes/mimedatabase.h>
#include <utils/parameteraction.h>
#include <utils/qtcassert.h>
#include <utils/stringutils.h>

#include <QtPlugin>
#include <QDebug>
#include <QFileInfo>
#include <QSettings>

#include <QAction>
#include <QApplication>
#include <QFileDialog>
#include <QMenu>
#include <QMessageBox>
#include <QTimer>
#include <QWizard>

/*!
    \namespace ProjectExplorer
    The ProjectExplorer namespace contains the classes to explore projects.
*/

/*!
    \namespace ProjectExplorer::Internal
    The ProjectExplorer::Internal namespace is the internal namespace of the
    ProjectExplorer plugin.
    \internal
*/

/*!
    \class ProjectExplorer::ProjectExplorerPlugin

    \brief The ProjectExplorerPlugin class contains static accessor and utility
    functions to obtain the current project, open projects, and so on.
*/

namespace {
bool debug = false;
}

using namespace Core;
using namespace ProjectExplorer::Internal;

namespace ProjectExplorer {

static Target *activeTarget()
{
    Project *project = ProjectTree::currentProject();
    return project ? project->activeTarget() : 0;
}

static BuildConfiguration *activeBuildConfiguration()
{
    Target *target = activeTarget();
    return target ? target->activeBuildConfiguration() : 0;
}

static Kit *currentKit()
{
    Target *target = activeTarget();
    return target ? target->kit() : 0;
}

class ProjectExplorerPluginPrivate : public QObject
{
    Q_DECLARE_TR_FUNCTIONS(ProjectExplorer::ProjectExplorerPlugin)

public:
    ProjectExplorerPluginPrivate();

    void deploy(QList<Project *>);
    int queue(QList<Project *>, QList<Id> stepIds);
    void updateContextMenuActions();
    void executeRunConfiguration(RunConfiguration *, RunMode mode);
    QPair<bool, QString> buildSettingsEnabledForSession();
    QPair<bool, QString> buildSettingsEnabled(Project *pro);

    void addToRecentProjects(const QString &fileName, const QString &displayName);
    void startRunControl(RunControl *runControl, RunMode runMode);

    void updateActions();
    void updateContext();
    void updateDeployActions();
    void updateRunWithoutDeployMenu();

    void buildQueueFinished(bool success);

    void buildStateChanged(ProjectExplorer::Project * pro);
    void buildProjectOnly();
    void handleBuildProject();
    void buildProjectContextMenu();
    void buildSession();
    void rebuildProjectOnly();
    void rebuildProject();
    void rebuildProjectContextMenu();
    void rebuildSession();
    void deployProjectOnly();
    void deployProject();
    void deployProjectContextMenu();
    void deploySession();
    void cleanProjectOnly();
    void cleanProject();
    void cleanProjectContextMenu();
    void cleanSession();
    void cancelBuild();
    void loadAction();
    void handleUnloadProject();
    void unloadProjectContextMenu();
    void closeAllProjects();
    void newProject();
    void showSessionManager();
    void updateSessionMenu();
    void setSession(QAction *action);

    void determineSessionToRestoreAtStartup();
    void restoreSession();
    void loadSession(const QString &session);
    void handleRunProject();
    void runProjectContextMenu();
    void runProjectWithoutDeploy();
    void savePersistentSettings();

    void addNewFile();
    void handleAddExistingFiles();
    void addExistingDirectory();
    void addNewSubproject();
    void removeProject();
    void openFile();
    void searchOnFileSystem();
    void showInGraphicalShell();
    void removeFile();
    void deleteFile();
    void handleRenameFile();
    void handleSetStartupProject();
    void setStartupProject(ProjectExplorer::Project *project);

    void updateRecentProjectMenu();
    void clearRecentProjects();
    void openRecentProject(const QString &fileName);
    void updateUnloadProjectMenu();
    void openTerminalHere();

    void invalidateProject(ProjectExplorer::Project *project);

    void projectAdded(ProjectExplorer::Project *pro);
    void projectRemoved(ProjectExplorer::Project *pro);
    void projectDisplayNameChanged(ProjectExplorer::Project *pro);
    void startupProjectChanged(); // Calls updateRunAction
    void activeTargetChanged();
    void activeRunConfigurationChanged();

    void slotUpdateRunActions();

    void currentModeChanged(Core::IMode *mode, Core::IMode *oldMode);
    void loadCustomWizards();

    void updateWelcomePage();

    void runConfigurationConfigurationFinished();

public:
    QMenu *m_sessionMenu;
    QMenu *m_openWithMenu;

    QMultiMap<int, QObject*> m_actionMap;
    QAction *m_sessionManagerAction;
    QAction *m_newAction;
    QAction *m_loadAction;
    Utils::ParameterAction *m_unloadAction;
    Utils::ParameterAction *m_unloadActionContextMenu;
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
    Utils::ParameterAction *m_cleanAction;
    QAction *m_cleanActionContextMenu;
    QAction *m_cleanSessionAction;
    QAction *m_runAction;
    QAction *m_runActionContextMenu;
    QAction *m_runWithoutDeployAction;
    QAction *m_cancelBuildAction;
    QAction *m_addNewFileAction;
    QAction *m_addExistingFilesAction;
    QAction *m_addExistingDirectoryAction;
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

    ProjectWindow *m_proWindow;
    QString m_sessionToRestoreAtStartup;

    QStringList m_profileMimeTypes;
    AppOutputPane *m_outputPane;

    QList<QPair<QString, QString> > m_recentProjects; // pair of filename, displayname
    static const int m_maxRecentProjects = 25;

    QString m_lastOpenDirectory;
    QPointer<RunConfiguration> m_delayedRunConfiguration;
    QList<QPair<RunConfiguration *, RunMode>> m_delayedRunConfigurationForRun;
    bool m_shouldHaveRunConfiguration;
    RunMode m_runMode;
    QString m_projectFilterString;
    MiniProjectTargetSelector * m_targetSelector;
    ProjectExplorerSettings m_projectExplorerSettings;
    ProjectWelcomePage *m_welcomePage;
    IMode *m_projectsMode;

    TaskHub *m_taskHub;
    KitManager *m_kitManager;
    ToolChainManager *m_toolChainManager;
    bool m_shuttingDown;
    QStringList m_arguments;
    QList<ProjectPanelFactory *> m_panelFactories;
    QString m_renameFileError;
#ifdef WITH_JOURNALD
    JournaldWatcher *m_journalWatcher;
#endif
};

ProjectExplorerPluginPrivate::ProjectExplorerPluginPrivate() :
    m_shouldHaveRunConfiguration(false),
    m_runMode(NoRunMode),
    m_projectsMode(0),
    m_kitManager(0),
    m_toolChainManager(0),
    m_shuttingDown(false)
#ifdef WITH_JOURNALD
    , m_journalWatcher(0)
#endif
{
}

class ProjectsMode : public IMode
{
public:
    ProjectsMode(QWidget *proWindow)
    {
        setWidget(proWindow);
        setContext(Context(Constants::C_PROJECTEXPLORER));
        setDisplayName(QCoreApplication::translate("ProjectExplorer::ProjectsMode", "Projects"));
        setIcon(QIcon(QLatin1String(":/projectexplorer/images/mode_project.png")));
        setPriority(Constants::P_MODE_SESSION);
        setId(Constants::MODE_SESSION);
        setContextHelpId(QLatin1String("Managing Projects"));
    }
};

static ProjectExplorerPlugin *m_instance = 0;
static ProjectExplorerPluginPrivate *dd = 0;

ProjectExplorerPlugin::ProjectExplorerPlugin()
{
    m_instance = this;
    dd = new ProjectExplorerPluginPrivate;
}

ProjectExplorerPlugin::~ProjectExplorerPlugin()
{
    JsonWizardFactory::destroyAllFactories();

    removeObject(dd->m_welcomePage);
    delete dd->m_welcomePage;

    removeObject(this);
    // Force sequence of deletion:
    delete dd->m_kitManager; // remove all the profile information
    delete dd->m_toolChainManager;
#ifdef WITH_JOURNALD
    delete dd->m_journalWatcher;
#endif
    ProjectPanelFactory::destroyFactories();
    delete dd;
}

ProjectExplorerPlugin *ProjectExplorerPlugin::instance()
{
    return m_instance;
}

static void updateActions()
{
    dd->updateActions();
}

bool ProjectExplorerPlugin::initialize(const QStringList &arguments, QString *error)
{
    Q_UNUSED(error);

    qRegisterMetaType<ProjectExplorer::RunControl *>();
    qRegisterMetaType<ProjectExplorer::DeployableFile>("ProjectExplorer::DeployableFile");

    CustomWizard::setVerbose(arguments.count(QLatin1String("-customwizard-verbose")));
    JsonWizardFactory::setVerbose(arguments.count(QLatin1String("-customwizard-verbose")));

    addObject(this);

    addAutoReleasedObject(new DeviceManager);

#ifdef WITH_JOURNALD
    dd->m_journalWatcher = new JournaldWatcher;
#endif

    // Add ToolChainFactories:
#ifdef Q_OS_WIN
    addAutoReleasedObject(new WinDebugInterface);

    addAutoReleasedObject(new MsvcToolChainFactory);
    addAutoReleasedObject(new WinCEToolChainFactory);
#else
    addAutoReleasedObject(new LinuxIccToolChainFactory);
#endif
#ifndef Q_OS_MAC
    addAutoReleasedObject(new MingwToolChainFactory); // Mingw offers cross-compiling to windows
#endif
    addAutoReleasedObject(new GccToolChainFactory);
    addAutoReleasedObject(new ClangToolChainFactory);
    addAutoReleasedObject(new CustomToolChainFactory);

    addAutoReleasedObject(new DesktopDeviceFactory);

    dd->m_kitManager = new KitManager; // register before ToolChainManager
    dd->m_toolChainManager = new ToolChainManager;

    IWizardFactory::registerFeatureProvider(new KitFeatureProvider);

    // Register KitInformation:
    KitManager::registerKitInformation(new DeviceTypeKitInformation);
    KitManager::registerKitInformation(new DeviceKitInformation);
    KitManager::registerKitInformation(new ToolChainKitInformation);
    KitManager::registerKitInformation(new SysRootKitInformation);
    KitManager::registerKitInformation(new EnvironmentKitInformation);

    addAutoReleasedObject(new ToolChainOptionsPage);
    addAutoReleasedObject(new KitOptionsPage);

    addAutoReleasedObject(new TaskHub);

    connect(ICore::instance(), &ICore::newItemsDialogRequested,
            dd, &ProjectExplorerPluginPrivate::loadCustomWizards);

    dd->m_welcomePage = new ProjectWelcomePage;
    connect(dd->m_welcomePage, &ProjectWelcomePage::manageSessions,
            dd, &ProjectExplorerPluginPrivate::showSessionManager);
    addObject(dd->m_welcomePage);

    auto sessionManager = new SessionManager(this);

    connect(sessionManager, &SessionManager::projectAdded,
            this, &ProjectExplorerPlugin::fileListChanged);
    connect(sessionManager, &SessionManager::aboutToRemoveProject,
            dd, &ProjectExplorerPluginPrivate::invalidateProject);
    connect(sessionManager, &SessionManager::projectRemoved,
            this, &ProjectExplorerPlugin::fileListChanged);
    connect(sessionManager, &SessionManager::projectAdded,
            dd, &ProjectExplorerPluginPrivate::projectAdded);
    connect(sessionManager, &SessionManager::projectRemoved,
            dd, &ProjectExplorerPluginPrivate::projectRemoved);
    connect(sessionManager, &SessionManager::startupProjectChanged,
            dd, &ProjectExplorerPluginPrivate::startupProjectChanged);
    connect(sessionManager, &SessionManager::projectDisplayNameChanged,
            dd, &ProjectExplorerPluginPrivate::projectDisplayNameChanged);
    connect(sessionManager, &SessionManager::dependencyChanged, updateActions);
    connect(sessionManager, &SessionManager::sessionLoaded, updateActions);
    connect(sessionManager, &SessionManager::sessionLoaded,
            dd, &ProjectExplorerPluginPrivate::updateWelcomePage);

    ProjectTree *tree = new ProjectTree(this);
    connect(tree, &ProjectTree::currentProjectChanged,
            dd, &ProjectExplorerPluginPrivate::updateContextMenuActions);
    connect(tree, &ProjectTree::currentNodeChanged,
            dd, &ProjectExplorerPluginPrivate::updateContextMenuActions);
    connect(tree, &ProjectTree::currentProjectChanged, updateActions);

    addAutoReleasedObject(new CustomWizardMetaFactory<CustomProjectWizard>(IWizardFactory::ProjectWizard));
    addAutoReleasedObject(new CustomWizardMetaFactory<CustomWizard>(IWizardFactory::FileWizard));
    addAutoReleasedObject(new CustomWizardMetaFactory<CustomWizard>(IWizardFactory::ClassWizard));

    // For JsonWizard:
    JsonWizardFactory::registerPageFactory(new FieldPageFactory);
    JsonWizardFactory::registerPageFactory(new FilePageFactory);
    JsonWizardFactory::registerPageFactory(new KitsPageFactory);
    JsonWizardFactory::registerPageFactory(new ProjectPageFactory);
    JsonWizardFactory::registerPageFactory(new SummaryPageFactory);

    JsonWizardFactory::registerGeneratorFactory(new FileGeneratorFactory);

    dd->m_proWindow = new ProjectWindow;
    addAutoReleasedObject(dd->m_proWindow);

    Context projecTreeContext(Constants::C_PROJECT_TREE);

    dd->m_projectsMode = new ProjectsMode(dd->m_proWindow);
    dd->m_projectsMode->setEnabled(false);
    addAutoReleasedObject(dd->m_projectsMode);
    dd->m_proWindow->layout()->addWidget(new FindToolBarPlaceHolder(dd->m_proWindow));

    addAutoReleasedObject(new CopyTaskHandler);
    addAutoReleasedObject(new ShowInEditorTaskHandler);
    addAutoReleasedObject(new VcsAnnotateTaskHandler);
    addAutoReleasedObject(new RemoveTaskHandler);
    addAutoReleasedObject(new ConfigTaskHandler(Task::compilerMissingTask(),
                                                Constants::KITS_SETTINGS_PAGE_ID));
    addAutoReleasedObject(new CoreListener);

    dd->m_outputPane = new AppOutputPane;
    addAutoReleasedObject(dd->m_outputPane);
    connect(SessionManager::instance(), &SessionManager::projectRemoved,
            dd->m_outputPane, &AppOutputPane::projectRemoved);

    connect(dd->m_outputPane, &AppOutputPane::runControlStarted,
            this, &ProjectExplorerPlugin::runControlStarted);
    connect(dd->m_outputPane, &AppOutputPane::runControlFinished,
            this, &ProjectExplorerPlugin::runControlFinished);
    connect(dd->m_outputPane, &AppOutputPane::runControlFinished,
            this, &ProjectExplorerPlugin::updateRunActions);

    addAutoReleasedObject(new AllProjectsFilter);
    addAutoReleasedObject(new CurrentProjectFilter);

    // ProjectPanelFactories
    auto editorSettingsPanelFactory = new ProjectPanelFactory;
    editorSettingsPanelFactory->setPriority(30);
    QString displayName = QCoreApplication::translate("EditorSettingsPanelFactory", "Editor");
    editorSettingsPanelFactory->setDisplayName(displayName);
    QIcon icon = QIcon(QLatin1String(":/projectexplorer/images/EditorSettings.png"));
    editorSettingsPanelFactory->setSimpleCreateWidgetFunction<EditorSettingsWidget>(icon);
    ProjectPanelFactory::registerFactory(editorSettingsPanelFactory);

    auto codeStyleSettingsPanelFactory = new ProjectPanelFactory;
    codeStyleSettingsPanelFactory->setPriority(40);
    displayName = QCoreApplication::translate("CodeStyleSettingsPanelFactory", "Code Style");
    codeStyleSettingsPanelFactory->setDisplayName(displayName);
    icon = QIcon(QLatin1String(":/projectexplorer/images/CodeStyleSettings.png"));
    codeStyleSettingsPanelFactory->setSimpleCreateWidgetFunction<CodeStyleSettingsWidget>(icon);
    ProjectPanelFactory::registerFactory(codeStyleSettingsPanelFactory);

    auto dependenciesPanelFactory = new ProjectPanelFactory;
    dependenciesPanelFactory->setPriority(50);
    displayName = QCoreApplication::translate("DependenciesPanelFactory", "Dependencies");
    dependenciesPanelFactory->setDisplayName(displayName);
    icon = QIcon(QLatin1String(":/projectexplorer/images/ProjectDependencies.png"));
    dependenciesPanelFactory->setSimpleCreateWidgetFunction<DependenciesWidget>(icon);
    ProjectPanelFactory::registerFactory(dependenciesPanelFactory);

    auto unconfiguredProjectPanel = new ProjectPanelFactory;
    unconfiguredProjectPanel->setPriority(-10);
    unconfiguredProjectPanel->setDisplayName(tr("Configure Project"));
    unconfiguredProjectPanel->setSupportsFunction([](Project *project){
        return project->targets().isEmpty() && !project->requiresTargetPanel();
    });
    icon = QIcon(QLatin1String(":/projectexplorer/images/unconfigured.png"));
    unconfiguredProjectPanel->setSimpleCreateWidgetFunction<TargetSetupPageWrapper>(icon);
    ProjectPanelFactory::registerFactory(unconfiguredProjectPanel);

    auto targetSettingsPanelFactory = new ProjectPanelFactory;
    targetSettingsPanelFactory->setPriority(-10);
    displayName = QCoreApplication::translate("TargetSettingsPanelFactory", "Build & Run");
    targetSettingsPanelFactory->setDisplayName(displayName);
    targetSettingsPanelFactory->setSupportsFunction([](Project *project) {
        return !project->targets().isEmpty()
                || project->requiresTargetPanel();
    });
    targetSettingsPanelFactory->setCreateWidgetFunction([](Project *project) {
        return new TargetSettingsPanelWidget(project);
    });
    ProjectPanelFactory::registerFactory(targetSettingsPanelFactory);

    addAutoReleasedObject(new ProcessStepFactory);

    addAutoReleasedObject(new AllProjectsFind);
    addAutoReleasedObject(new CurrentProjectFind);

    addAutoReleasedObject(new LocalApplicationRunControlFactory);

    addAutoReleasedObject(new ProjectFileWizardExtension);

    // Settings pages
    addAutoReleasedObject(new ProjectExplorerSettingsPage);
    addAutoReleasedObject(new DeviceSettingsPage);

    // context menus
    ActionContainer *msessionContextMenu =
        ActionManager::createMenu(Constants::M_SESSIONCONTEXT);
    ActionContainer *mprojectContextMenu =
        ActionManager::createMenu(Constants::M_PROJECTCONTEXT);
    ActionContainer *msubProjectContextMenu =
        ActionManager::createMenu(Constants::M_SUBPROJECTCONTEXT);
    ActionContainer *mfolderContextMenu =
        ActionManager::createMenu(Constants::M_FOLDERCONTEXT);
    ActionContainer *mfileContextMenu =
        ActionManager::createMenu(Constants::M_FILECONTEXT);

    ActionContainer *mfile =
        ActionManager::actionContainer(Core::Constants::M_FILE);
    ActionContainer *menubar =
        ActionManager::actionContainer(Core::Constants::MENU_BAR);

    // build menu
    ActionContainer *mbuild =
        ActionManager::createMenu(Constants::M_BUILDPROJECT);
    mbuild->menu()->setTitle(tr("&Build"));
    menubar->addMenu(mbuild, Core::Constants::G_VIEW);

    // debug menu
    ActionContainer *mdebug =
        ActionManager::createMenu(Constants::M_DEBUG);
    mdebug->menu()->setTitle(tr("&Debug"));
    menubar->addMenu(mdebug, Core::Constants::G_VIEW);

    ActionContainer *mstartdebugging =
        ActionManager::createMenu(Constants::M_DEBUG_STARTDEBUGGING);
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

    ActionContainer *runMenu = ActionManager::createMenu(Constants::RUNMENUCONTEXTMENU);
    runMenu->setOnAllDisabledBehavior(ActionContainer::Hide);
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
    ActionContainer * const openWith =
            ActionManager::createMenu(ProjectExplorer::Constants::M_OPENFILEWITHCONTEXT);
    openWith->setOnAllDisabledBehavior(ActionContainer::Show);
    dd->m_openWithMenu = openWith->menu();
    dd->m_openWithMenu->setTitle(tr("Open With"));

    connect(dd->m_openWithMenu, &QMenu::triggered,
            DocumentManager::instance(), &DocumentManager::executeOpenWithMenuAction);

    //
    // Separators
    //

    Command *cmd;

    msessionContextMenu->addSeparator(projecTreeContext, Constants::G_SESSION_REBUILD);

    msessionContextMenu->addSeparator(projecTreeContext, Constants::G_SESSION_FILES);
    mprojectContextMenu->addSeparator(projecTreeContext, Constants::G_PROJECT_FILES);
    msubProjectContextMenu->addSeparator(projecTreeContext, Constants::G_PROJECT_FILES);
    mfile->addSeparator(Core::Constants::G_FILE_PROJECT);
    mbuild->addSeparator(Constants::G_BUILD_REBUILD);
    msessionContextMenu->addSeparator(Constants::G_SESSION_OTHER);
    mbuild->addSeparator(Constants::G_BUILD_CANCEL);
    mbuild->addSeparator(Constants::G_BUILD_RUN);
    mprojectContextMenu->addSeparator(Constants::G_PROJECT_REBUILD);

    //
    // Actions
    //

    // new action
    dd->m_newAction = new QAction(tr("New Project..."), this);
    cmd = ActionManager::registerAction(dd->m_newAction, Constants::NEWPROJECT);
    cmd->setDefaultKeySequence(QKeySequence(tr("Ctrl+Shift+N")));
    msessionContextMenu->addAction(cmd, Constants::G_SESSION_FILES);

    // open action
    dd->m_loadAction = new QAction(tr("Load Project..."), this);
    cmd = ActionManager::registerAction(dd->m_loadAction, Constants::LOAD);
#ifndef Q_OS_MAC
    cmd->setDefaultKeySequence(QKeySequence(tr("Ctrl+Shift+O")));
#endif
    msessionContextMenu->addAction(cmd, Constants::G_SESSION_FILES);

    // Default open action
    dd->m_openFileAction = new QAction(tr("Open File"), this);
    cmd = ActionManager::registerAction(dd->m_openFileAction, Constants::OPENFILE,
                       projecTreeContext);
    mfileContextMenu->addAction(cmd, Constants::G_FILE_OPEN);

    dd->m_searchOnFileSystem = new QAction(FileUtils::msgFindInDirectory(), this);
    cmd = ActionManager::registerAction(dd->m_searchOnFileSystem, Constants::SEARCHONFILESYSTEM, projecTreeContext);

    mfileContextMenu->addAction(cmd, Constants::G_FILE_OTHER);
    mfolderContextMenu->addAction(cmd, Constants::G_FOLDER_CONFIG);
    msubProjectContextMenu->addAction(cmd, Constants::G_PROJECT_LAST);
    mprojectContextMenu->addAction(cmd, Constants::G_PROJECT_LAST);

    dd->m_showInGraphicalShell = new QAction(FileUtils::msgGraphicalShellAction(), this);
    cmd = ActionManager::registerAction(dd->m_showInGraphicalShell, Constants::SHOWINGRAPHICALSHELL,
                       projecTreeContext);
    mfileContextMenu->addAction(cmd, Constants::G_FILE_OPEN);
    mfolderContextMenu->addAction(cmd, Constants::G_FOLDER_FILES);

    dd->m_openTerminalHere = new QAction(FileUtils::msgTerminalAction(), this);
    cmd = ActionManager::registerAction(dd->m_openTerminalHere, Constants::OPENTERMIANLHERE,
                       projecTreeContext);
    mfileContextMenu->addAction(cmd, Constants::G_FILE_OPEN);
    mfolderContextMenu->addAction(cmd, Constants::G_FOLDER_FILES);

    // Open With menu
    mfileContextMenu->addMenu(openWith, Constants::G_FILE_OPEN);

    // recent projects menu
    ActionContainer *mrecent =
        ActionManager::createMenu(Constants::M_RECENTPROJECTS);
    mrecent->menu()->setTitle(tr("Recent P&rojects"));
    mrecent->setOnAllDisabledBehavior(ActionContainer::Show);
    mfile->addMenu(mrecent, Core::Constants::G_FILE_OPEN);
    connect(mfile->menu(), &QMenu::aboutToShow,
            dd, &ProjectExplorerPluginPrivate::updateRecentProjectMenu);

    // session menu
    ActionContainer *msession = ActionManager::createMenu(Constants::M_SESSION);
    msession->menu()->setTitle(tr("Sessions"));
    msession->setOnAllDisabledBehavior(ActionContainer::Show);
    mfile->addMenu(msession, Core::Constants::G_FILE_OPEN);
    dd->m_sessionMenu = msession->menu();
    connect(mfile->menu(), &QMenu::aboutToShow,
            dd, &ProjectExplorerPluginPrivate::updateSessionMenu);

    // session manager action
    dd->m_sessionManagerAction = new QAction(tr("Session Manager..."), this);
    cmd = ActionManager::registerAction(dd->m_sessionManagerAction, Constants::NEWSESSION);
    mfile->addAction(cmd, Core::Constants::G_FILE_OPEN);
    cmd->setDefaultKeySequence(QKeySequence());


    // unload action
    dd->m_unloadAction = new Utils::ParameterAction(tr("Close Project"), tr("Close Project \"%1\""),
                                                      Utils::ParameterAction::AlwaysEnabled, this);
    cmd = ActionManager::registerAction(dd->m_unloadAction, Constants::UNLOAD);
    cmd->setAttribute(Command::CA_UpdateText);
    cmd->setDescription(dd->m_unloadAction->text());
    mfile->addAction(cmd, Core::Constants::G_FILE_PROJECT);

    ActionContainer *munload =
        ActionManager::createMenu(Constants::M_UNLOADPROJECTS);
    munload->menu()->setTitle(tr("Close Project"));
    munload->setOnAllDisabledBehavior(ActionContainer::Show);
    mfile->addMenu(munload, Core::Constants::G_FILE_PROJECT);
    connect(mfile->menu(), &QMenu::aboutToShow,
            dd, &ProjectExplorerPluginPrivate::updateUnloadProjectMenu);

    // unload session action
    dd->m_closeAllProjects = new QAction(tr("Close All Projects and Editors"), this);
    cmd = ActionManager::registerAction(dd->m_closeAllProjects, Constants::CLEARSESSION);
    mfile->addAction(cmd, Core::Constants::G_FILE_PROJECT);
    msessionContextMenu->addAction(cmd, Constants::G_SESSION_FILES);

    // build session action
    QIcon buildIcon = QIcon(QLatin1String(Constants::ICON_BUILD));
    buildIcon.addFile(QLatin1String(Constants::ICON_BUILD_SMALL));
    dd->m_buildSessionAction = new QAction(buildIcon, tr("Build All"), this);
    cmd = ActionManager::registerAction(dd->m_buildSessionAction, Constants::BUILDSESSION);
    cmd->setDefaultKeySequence(QKeySequence(tr("Ctrl+Shift+B")));
    mbuild->addAction(cmd, Constants::G_BUILD_BUILD);
    msessionContextMenu->addAction(cmd, Constants::G_SESSION_BUILD);

    // deploy session
    dd->m_deploySessionAction = new QAction(tr("Deploy All"), this);
    cmd = ActionManager::registerAction(dd->m_deploySessionAction, Constants::DEPLOYSESSION);
    mbuild->addAction(cmd, Constants::G_BUILD_DEPLOY);
    msessionContextMenu->addAction(cmd, Constants::G_SESSION_BUILD);

    // rebuild session action
    QIcon rebuildIcon = QIcon(QLatin1String(Constants::ICON_REBUILD));
    rebuildIcon.addFile(QLatin1String(Constants::ICON_REBUILD_SMALL));
    dd->m_rebuildSessionAction = new QAction(rebuildIcon, tr("Rebuild All"), this);
    cmd = ActionManager::registerAction(dd->m_rebuildSessionAction, Constants::REBUILDSESSION);
    mbuild->addAction(cmd, Constants::G_BUILD_REBUILD);
    msessionContextMenu->addAction(cmd, Constants::G_SESSION_REBUILD);

    // clean session
    QIcon cleanIcon = QIcon(QLatin1String(Constants::ICON_CLEAN));
    cleanIcon.addFile(QLatin1String(Constants::ICON_CLEAN_SMALL));
    dd->m_cleanSessionAction = new QAction(cleanIcon, tr("Clean All"), this);
    cmd = ActionManager::registerAction(dd->m_cleanSessionAction, Constants::CLEANSESSION);
    mbuild->addAction(cmd, Constants::G_BUILD_CLEAN);
    msessionContextMenu->addAction(cmd, Constants::G_SESSION_REBUILD);

    // build action
    dd->m_buildAction = new Utils::ParameterAction(tr("Build Project"), tr("Build Project \"%1\""),
                                                     Utils::ParameterAction::AlwaysEnabled, this);
    dd->m_buildAction->setIcon(buildIcon);
    cmd = ActionManager::registerAction(dd->m_buildAction, Constants::BUILD);
    cmd->setAttribute(Command::CA_UpdateText);
    cmd->setDescription(dd->m_buildAction->text());
    cmd->setDefaultKeySequence(QKeySequence(tr("Ctrl+B")));
    mbuild->addAction(cmd, Constants::G_BUILD_BUILD);

    // Add to mode bar
    ModeManager::addAction(cmd->action(), Constants::P_ACTION_BUILDPROJECT);

    // deploy action
    dd->m_deployAction = new Utils::ParameterAction(tr("Deploy Project"), tr("Deploy Project \"%1\""),
                                                     Utils::ParameterAction::AlwaysEnabled, this);
    cmd = ActionManager::registerAction(dd->m_deployAction, Constants::DEPLOY);
    cmd->setAttribute(Command::CA_UpdateText);
    cmd->setDescription(dd->m_deployAction->text());
    mbuild->addAction(cmd, Constants::G_BUILD_DEPLOY);

    // rebuild action
    dd->m_rebuildAction = new Utils::ParameterAction(tr("Rebuild Project"), tr("Rebuild Project \"%1\""),
                                                       Utils::ParameterAction::AlwaysEnabled, this);
    cmd = ActionManager::registerAction(dd->m_rebuildAction, Constants::REBUILD);
    cmd->setAttribute(Command::CA_UpdateText);
    cmd->setDescription(dd->m_rebuildAction->text());
    mbuild->addAction(cmd, Constants::G_BUILD_REBUILD);

    // clean action
    dd->m_cleanAction = new Utils::ParameterAction(tr("Clean Project"), tr("Clean Project \"%1\""),
                                                     Utils::ParameterAction::AlwaysEnabled, this);
    cmd = ActionManager::registerAction(dd->m_cleanAction, Constants::CLEAN);
    cmd->setAttribute(Command::CA_UpdateText);
    cmd->setDescription(dd->m_cleanAction->text());
    mbuild->addAction(cmd, Constants::G_BUILD_CLEAN);

    // cancel build action
    QIcon stopIcon = QIcon(QLatin1String(Constants::ICON_STOP));
    stopIcon.addFile(QLatin1String(Constants::ICON_STOP_SMALL));
    dd->m_cancelBuildAction = new QAction(stopIcon, tr("Cancel Build"), this);
    cmd = ActionManager::registerAction(dd->m_cancelBuildAction, Constants::CANCELBUILD);
    mbuild->addAction(cmd, Constants::G_BUILD_CANCEL);

    // run action
    dd->m_runAction = new QAction(runIcon, tr("Run"), this);
    cmd = ActionManager::registerAction(dd->m_runAction, Constants::RUN);
    cmd->setAttribute(Command::CA_UpdateText);

    cmd->setDefaultKeySequence(QKeySequence(tr("Ctrl+R")));
    mbuild->addAction(cmd, Constants::G_BUILD_RUN);

    ModeManager::addAction(cmd->action(), Constants::P_ACTION_RUN);

    // Run without deployment action
    dd->m_runWithoutDeployAction = new QAction(tr("Run Without Deployment"), this);
    cmd = ActionManager::registerAction(dd->m_runWithoutDeployAction, Constants::RUNWITHOUTDEPLOY);
    mbuild->addAction(cmd, Constants::G_BUILD_RUN);

    // build action (context menu)
    dd->m_buildActionContextMenu = new QAction(tr("Build"), this);
    cmd = ActionManager::registerAction(dd->m_buildActionContextMenu, Constants::BUILDCM, projecTreeContext);
    cmd->setAttribute(Command::CA_UpdateText);
    mprojectContextMenu->addAction(cmd, Constants::G_PROJECT_BUILD);

    // rebuild action (context menu)
    dd->m_rebuildActionContextMenu = new QAction(tr("Rebuild"), this);
    cmd = ActionManager::registerAction(dd->m_rebuildActionContextMenu, Constants::REBUILDCM, projecTreeContext);
    cmd->setAttribute(Command::CA_UpdateText);
    mprojectContextMenu->addAction(cmd, Constants::G_PROJECT_REBUILD);

    // clean action (context menu)
    dd->m_cleanActionContextMenu = new QAction(tr("Clean"), this);
    cmd = ActionManager::registerAction(dd->m_cleanActionContextMenu, Constants::CLEANCM, projecTreeContext);
    cmd->setAttribute(Command::CA_UpdateText);
    mprojectContextMenu->addAction(cmd, Constants::G_PROJECT_REBUILD);

    // build without dependencies action
    dd->m_buildProjectOnlyAction = new QAction(tr("Build Without Dependencies"), this);
    ActionManager::registerAction(dd->m_buildProjectOnlyAction, Constants::BUILDPROJECTONLY);

    // rebuild without dependencies action
    dd->m_rebuildProjectOnlyAction = new QAction(tr("Rebuild Without Dependencies"), this);
    ActionManager::registerAction(dd->m_rebuildProjectOnlyAction, Constants::REBUILDPROJECTONLY);

    // deploy without dependencies action
    dd->m_deployProjectOnlyAction = new QAction(tr("Deploy Without Dependencies"), this);
    ActionManager::registerAction(dd->m_deployProjectOnlyAction, Constants::DEPLOYPROJECTONLY);

    // clean without dependencies action
    dd->m_cleanProjectOnlyAction = new QAction(tr("Clean Without Dependencies"), this);
    ActionManager::registerAction(dd->m_cleanProjectOnlyAction, Constants::CLEANPROJECTONLY);

    // deploy action (context menu)
    dd->m_deployActionContextMenu = new QAction(tr("Deploy"), this);
    cmd = ActionManager::registerAction(dd->m_deployActionContextMenu, Constants::DEPLOYCM, projecTreeContext);
    mprojectContextMenu->addAction(cmd, Constants::G_PROJECT_RUN);

    dd->m_runActionContextMenu = new QAction(runIcon, tr("Run"), this);
    cmd = ActionManager::registerAction(dd->m_runActionContextMenu, Constants::RUNCONTEXTMENU, projecTreeContext);
    mprojectContextMenu->addAction(cmd, Constants::G_PROJECT_RUN);
    msubProjectContextMenu->addAction(cmd, Constants::G_PROJECT_RUN);

    // add new file action
    dd->m_addNewFileAction = new QAction(tr("Add New..."), this);
    cmd = ActionManager::registerAction(dd->m_addNewFileAction, Constants::ADDNEWFILE,
                       projecTreeContext);
    mprojectContextMenu->addAction(cmd, Constants::G_PROJECT_FILES);
    msubProjectContextMenu->addAction(cmd, Constants::G_PROJECT_FILES);
    mfolderContextMenu->addAction(cmd, Constants::G_FOLDER_FILES);

    // add existing file action
    dd->m_addExistingFilesAction = new QAction(tr("Add Existing Files..."), this);
    cmd = ActionManager::registerAction(dd->m_addExistingFilesAction, Constants::ADDEXISTINGFILES,
                       projecTreeContext);
    mprojectContextMenu->addAction(cmd, Constants::G_PROJECT_FILES);
    msubProjectContextMenu->addAction(cmd, Constants::G_PROJECT_FILES);
    mfolderContextMenu->addAction(cmd, Constants::G_FOLDER_FILES);

    // add existing directory action
    dd->m_addExistingDirectoryAction = new QAction(tr("Add Existing Directory..."), this);
    cmd = ActionManager::registerAction(dd->m_addExistingDirectoryAction,
                                              Constants::ADDEXISTINGDIRECTORY,
                                              projecTreeContext);
    mprojectContextMenu->addAction(cmd, Constants::G_PROJECT_FILES);
    msubProjectContextMenu->addAction(cmd, Constants::G_PROJECT_FILES);
    mfolderContextMenu->addAction(cmd, Constants::G_FOLDER_FILES);

    // new subproject action
    dd->m_addNewSubprojectAction = new QAction(tr("New Subproject..."), this);
    cmd = ActionManager::registerAction(dd->m_addNewSubprojectAction, Constants::ADDNEWSUBPROJECT,
                       projecTreeContext);
    mprojectContextMenu->addAction(cmd, Constants::G_PROJECT_FILES);
    msubProjectContextMenu->addAction(cmd, Constants::G_PROJECT_FILES);

    // unload project again, in right position
    dd->m_unloadActionContextMenu = new Utils::ParameterAction(tr("Close Project"), tr("Close Project \"%1\""),
                                                              Utils::ParameterAction::EnabledWithParameter, this);
    cmd = ActionManager::registerAction(dd->m_unloadActionContextMenu, Constants::UNLOADCM);
    cmd->setAttribute(Command::CA_UpdateText);
    cmd->setDescription(dd->m_unloadActionContextMenu->text());
    mprojectContextMenu->addAction(cmd, Constants::G_PROJECT_LAST);

    // remove file action
    dd->m_removeFileAction = new QAction(tr("Remove File..."), this);
    cmd = ActionManager::registerAction(dd->m_removeFileAction, Constants::REMOVEFILE,
                       projecTreeContext);
    cmd->setDefaultKeySequence(QKeySequence::Delete);
    mfileContextMenu->addAction(cmd, Constants::G_FILE_OTHER);

    //: Remove project from parent profile (Project explorer view); will not physically delete any files.
    dd->m_removeProjectAction = new QAction(tr("Remove Project..."), this);
    cmd = ActionManager::registerAction(dd->m_removeProjectAction, Constants::REMOVEPROJECT,
                       projecTreeContext);
    msubProjectContextMenu->addAction(cmd, Constants::G_PROJECT_FILES);

    // delete file action
    dd->m_deleteFileAction = new QAction(tr("Delete File..."), this);
    cmd = ActionManager::registerAction(dd->m_deleteFileAction, Constants::DELETEFILE,
                             projecTreeContext);
    cmd->setDefaultKeySequence(QKeySequence::Delete);
    mfileContextMenu->addAction(cmd, Constants::G_FILE_OTHER);

    // renamefile action
    dd->m_renameFileAction = new QAction(tr("Rename..."), this);
    cmd = ActionManager::registerAction(dd->m_renameFileAction, Constants::RENAMEFILE,
                       projecTreeContext);
    mfileContextMenu->addAction(cmd, Constants::G_FILE_OTHER);
    // Not yet used by anyone, so hide for now
//    mfolder->addAction(cmd, Constants::G_FOLDER_FILES);
//    msubProject->addAction(cmd, Constants::G_FOLDER_FILES);
//    mproject->addAction(cmd, Constants::G_FOLDER_FILES);

    // set startup project action
    dd->m_setStartupProjectAction = new Utils::ParameterAction(tr("Set as Active Project"),
                                                              tr("Set \"%1\" as Active Project"),
                                                              Utils::ParameterAction::AlwaysEnabled, this);
    cmd = ActionManager::registerAction(dd->m_setStartupProjectAction, Constants::SETSTARTUP,
                             projecTreeContext);
    cmd->setAttribute(Command::CA_UpdateText);
    cmd->setDescription(dd->m_setStartupProjectAction->text());
    mprojectContextMenu->addAction(cmd, Constants::G_PROJECT_FIRST);

    // Collapse All.
    dd->m_projectTreeCollapseAllAction = new QAction(tr("Collapse All"), this);
    cmd = ActionManager::registerAction(dd->m_projectTreeCollapseAllAction, Constants::PROJECTTREE_COLLAPSE_ALL,
                             projecTreeContext);
    const Id treeGroup = Constants::G_PROJECT_TREE;
    mfileContextMenu->addSeparator(treeGroup);
    mfileContextMenu->addAction(cmd, treeGroup);
    msubProjectContextMenu->addSeparator(treeGroup);
    msubProjectContextMenu->addAction(cmd, treeGroup);
    mfolderContextMenu->addSeparator(treeGroup);
    mfolderContextMenu->addAction(cmd, treeGroup);
    mprojectContextMenu->addSeparator(treeGroup);
    mprojectContextMenu->addAction(cmd, treeGroup);
    msessionContextMenu->addSeparator(treeGroup);
    msessionContextMenu->addAction(cmd, treeGroup);

    // target selector
    dd->m_projectSelectorAction = new QAction(this);
    dd->m_projectSelectorAction->setCheckable(true);
    dd->m_projectSelectorAction->setEnabled(false);
    QWidget *mainWindow = ICore::mainWindow();
    dd->m_targetSelector = new MiniProjectTargetSelector(dd->m_projectSelectorAction, mainWindow);
    connect(dd->m_projectSelectorAction, &QAction::triggered, dd->m_targetSelector, &QWidget::show);
    ModeManager::addProjectSelector(dd->m_projectSelectorAction);

    dd->m_projectSelectorActionMenu = new QAction(this);
    dd->m_projectSelectorActionMenu->setEnabled(false);
    dd->m_projectSelectorActionMenu->setText(tr("Open Build and Run Kit Selector..."));
    connect(dd->m_projectSelectorActionMenu, &QAction::triggered, dd->m_targetSelector,
            &MiniProjectTargetSelector::toggleVisible);
    cmd = ActionManager::registerAction(dd->m_projectSelectorActionMenu, Constants::SELECTTARGET);
    mbuild->addAction(cmd, Constants::G_BUILD_RUN);

    dd->m_projectSelectorActionQuick = new QAction(this);
    dd->m_projectSelectorActionQuick->setEnabled(false);
    dd->m_projectSelectorActionQuick->setText(tr("Quick Switch Kit Selector"));
    connect(dd->m_projectSelectorActionQuick, &QAction::triggered,
            dd->m_targetSelector, &MiniProjectTargetSelector::nextOrShow);
    cmd = ActionManager::registerAction(dd->m_projectSelectorActionQuick, Constants::SELECTTARGETQUICK);
    cmd->setDefaultKeySequence(QKeySequence(tr("Ctrl+T")));

    connect(ICore::instance(), &ICore::saveSettingsRequested,
            dd, &ProjectExplorerPluginPrivate::savePersistentSettings);

    addAutoReleasedObject(new ProjectTreeWidgetFactory);
    addAutoReleasedObject(new FolderNavigationWidgetFactory);
    addAutoReleasedObject(new DefaultDeployConfigurationFactory);

    QSettings *s = ICore::settings();
    const QStringList fileNames =
            s->value(QLatin1String("ProjectExplorer/RecentProjects/FileNames")).toStringList();
    const QStringList displayNames =
            s->value(QLatin1String("ProjectExplorer/RecentProjects/DisplayNames")).toStringList();
    if (fileNames.size() == displayNames.size()) {
        for (int i = 0; i < fileNames.size(); ++i) {
            if (QFileInfo(fileNames.at(i)).isFile())
                dd->m_recentProjects.append(qMakePair(fileNames.at(i), displayNames.at(i)));
        }
    }

    dd->m_projectExplorerSettings.buildBeforeDeploy =
            s->value(QLatin1String("ProjectExplorer/Settings/BuildBeforeDeploy"), true).toBool();
    dd->m_projectExplorerSettings.deployBeforeRun =
            s->value(QLatin1String("ProjectExplorer/Settings/DeployBeforeRun"), true).toBool();
    dd->m_projectExplorerSettings.saveBeforeBuild =
            s->value(QLatin1String("ProjectExplorer/Settings/SaveBeforeBuild"), false).toBool();
    dd->m_projectExplorerSettings.showCompilerOutput =
            s->value(QLatin1String("ProjectExplorer/Settings/ShowCompilerOutput"), false).toBool();
    dd->m_projectExplorerSettings.showRunOutput =
            s->value(QLatin1String("ProjectExplorer/Settings/ShowRunOutput"), true).toBool();
    dd->m_projectExplorerSettings.showDebugOutput =
            s->value(QLatin1String("ProjectExplorer/Settings/ShowDebugOutput"), false).toBool();
    dd->m_projectExplorerSettings.cleanOldAppOutput =
            s->value(QLatin1String("ProjectExplorer/Settings/CleanOldAppOutput"), false).toBool();
    dd->m_projectExplorerSettings.mergeStdErrAndStdOut =
            s->value(QLatin1String("ProjectExplorer/Settings/MergeStdErrAndStdOut"), false).toBool();
    dd->m_projectExplorerSettings.wrapAppOutput =
            s->value(QLatin1String("ProjectExplorer/Settings/WrapAppOutput"), true).toBool();
    dd->m_projectExplorerSettings.useJom =
            s->value(QLatin1String("ProjectExplorer/Settings/UseJom"), true).toBool();
    dd->m_projectExplorerSettings.autorestoreLastSession =
            s->value(QLatin1String("ProjectExplorer/Settings/AutoRestoreLastSession"), false).toBool();
    dd->m_projectExplorerSettings.prompToStopRunControl =
            s->value(QLatin1String("ProjectExplorer/Settings/PromptToStopRunControl"), false).toBool();
    dd->m_projectExplorerSettings.maxAppOutputLines =
            s->value(QLatin1String("ProjectExplorer/Settings/MaxAppOutputLines"), 100000).toInt();
    dd->m_projectExplorerSettings.environmentId =
            QUuid(s->value(QLatin1String("ProjectExplorer/Settings/EnvironmentId")).toByteArray());
    if (dd->m_projectExplorerSettings.environmentId.isNull())
        dd->m_projectExplorerSettings.environmentId = QUuid::createUuid();

    connect(dd->m_sessionManagerAction, &QAction::triggered,
            dd, &ProjectExplorerPluginPrivate::showSessionManager);
    connect(dd->m_newAction, &QAction::triggered,
            dd, &ProjectExplorerPluginPrivate::newProject);
    connect(dd->m_loadAction, &QAction::triggered,
            dd, &ProjectExplorerPluginPrivate::loadAction);
    connect(dd->m_buildProjectOnlyAction, &QAction::triggered,
            dd, &ProjectExplorerPluginPrivate::buildProjectOnly);
    connect(dd->m_buildAction, &QAction::triggered,
            dd, &ProjectExplorerPluginPrivate::handleBuildProject);
    connect(dd->m_buildActionContextMenu, &QAction::triggered,
            dd, &ProjectExplorerPluginPrivate::buildProjectContextMenu);
    connect(dd->m_buildSessionAction, &QAction::triggered,
            dd, &ProjectExplorerPluginPrivate::buildSession);
    connect(dd->m_rebuildProjectOnlyAction, &QAction::triggered,
            dd, &ProjectExplorerPluginPrivate::rebuildProjectOnly);
    connect(dd->m_rebuildAction, &QAction::triggered,
            dd, &ProjectExplorerPluginPrivate::rebuildProject);
    connect(dd->m_rebuildActionContextMenu, &QAction::triggered,
            dd, &ProjectExplorerPluginPrivate::rebuildProjectContextMenu);
    connect(dd->m_rebuildSessionAction, &QAction::triggered,
            dd, &ProjectExplorerPluginPrivate::rebuildSession);
    connect(dd->m_deployProjectOnlyAction, &QAction::triggered,
            dd, &ProjectExplorerPluginPrivate::deployProjectOnly);
    connect(dd->m_deployAction, &QAction::triggered,
            dd, &ProjectExplorerPluginPrivate::deployProject);
    connect(dd->m_deployActionContextMenu, &QAction::triggered,
            dd, &ProjectExplorerPluginPrivate::deployProjectContextMenu);
    connect(dd->m_deploySessionAction, &QAction::triggered,
            dd, &ProjectExplorerPluginPrivate::deploySession);
    connect(dd->m_cleanProjectOnlyAction, &QAction::triggered,
            dd, &ProjectExplorerPluginPrivate::cleanProjectOnly);
    connect(dd->m_cleanAction, &QAction::triggered,
            dd, &ProjectExplorerPluginPrivate::cleanProject);
    connect(dd->m_cleanActionContextMenu, &QAction::triggered,
            dd, &ProjectExplorerPluginPrivate::cleanProjectContextMenu);
    connect(dd->m_cleanSessionAction, &QAction::triggered,
            dd, &ProjectExplorerPluginPrivate::cleanSession);
    connect(dd->m_runAction, &QAction::triggered,
            dd, &ProjectExplorerPluginPrivate::handleRunProject);
    connect(dd->m_runActionContextMenu, &QAction::triggered,
            dd, &ProjectExplorerPluginPrivate::runProjectContextMenu);
    connect(dd->m_runWithoutDeployAction, &QAction::triggered,
            dd, &ProjectExplorerPluginPrivate::runProjectWithoutDeploy);
    connect(dd->m_cancelBuildAction, &QAction::triggered,
            dd, &ProjectExplorerPluginPrivate::cancelBuild);
    connect(dd->m_unloadAction, &QAction::triggered,
            dd, &ProjectExplorerPluginPrivate::handleUnloadProject);
    connect(dd->m_unloadActionContextMenu, &QAction::triggered,
            dd, &ProjectExplorerPluginPrivate::unloadProjectContextMenu);
    connect(dd->m_closeAllProjects, &QAction::triggered,
            dd, &ProjectExplorerPluginPrivate::closeAllProjects);
    connect(dd->m_addNewFileAction, &QAction::triggered,
            dd, &ProjectExplorerPluginPrivate::addNewFile);
    connect(dd->m_addExistingFilesAction, &QAction::triggered,
            dd, &ProjectExplorerPluginPrivate::handleAddExistingFiles);
    connect(dd->m_addExistingDirectoryAction, &QAction::triggered,
            dd, &ProjectExplorerPluginPrivate::addExistingDirectory);
    connect(dd->m_addNewSubprojectAction, &QAction::triggered,
            dd, &ProjectExplorerPluginPrivate::addNewSubproject);
    connect(dd->m_removeProjectAction, &QAction::triggered,
            dd, &ProjectExplorerPluginPrivate::removeProject);
    connect(dd->m_openFileAction, &QAction::triggered,
            dd, &ProjectExplorerPluginPrivate::openFile);
    connect(dd->m_searchOnFileSystem, &QAction::triggered,
            dd, &ProjectExplorerPluginPrivate::searchOnFileSystem);
    connect(dd->m_showInGraphicalShell, &QAction::triggered,
            dd, &ProjectExplorerPluginPrivate::showInGraphicalShell);
    connect(dd->m_openTerminalHere, &QAction::triggered,
            dd, &ProjectExplorerPluginPrivate::openTerminalHere);
    connect(dd->m_removeFileAction, &QAction::triggered,
            dd, &ProjectExplorerPluginPrivate::removeFile);
    connect(dd->m_deleteFileAction, &QAction::triggered,
            dd, &ProjectExplorerPluginPrivate::deleteFile);
    connect(dd->m_renameFileAction, &QAction::triggered,
            dd, &ProjectExplorerPluginPrivate::handleRenameFile);
    connect(dd->m_setStartupProjectAction, &QAction::triggered,
            dd, &ProjectExplorerPluginPrivate::handleSetStartupProject);
    connect(dd->m_projectTreeCollapseAllAction, &QAction::triggered,
            ProjectTree::instance(), &ProjectTree::collapseAll);

    connect(this, &ProjectExplorerPlugin::updateRunActions,
            dd, &ProjectExplorerPluginPrivate::slotUpdateRunActions);
    connect(this, &ProjectExplorerPlugin::settingsChanged,
            dd, &ProjectExplorerPluginPrivate::updateRunWithoutDeployMenu);

    auto buildManager = new BuildManager(this, dd->m_cancelBuildAction);
    connect(buildManager, &BuildManager::buildStateChanged,
            dd, &ProjectExplorerPluginPrivate::buildStateChanged);
    connect(buildManager, &BuildManager::buildQueueFinished,
            dd, &ProjectExplorerPluginPrivate::buildQueueFinished, Qt::QueuedConnection);

    connect(ICore::instance(), &ICore::coreAboutToOpen,
            dd, &ProjectExplorerPluginPrivate::determineSessionToRestoreAtStartup);
    connect(ICore::instance(), &ICore::coreOpened,
            dd, &ProjectExplorerPluginPrivate::restoreSession);
    connect(ICore::instance(), &ICore::newItemDialogRunningChanged, updateActions);

    dd->updateWelcomePage();

    // FIXME: These are mostly "legacy"/"convenience" entries, relying on
    // the global entry point ProjectExplorer::currentProject(). They should
    // not be used in the Run/Build configuration pages.
    Utils::MacroExpander *expander = Utils::globalMacroExpander();
    expander->registerFileVariables(Constants::VAR_CURRENTPROJECT_PREFIX,
        tr("Current project's main file."),
        [this]() -> QString {
            Utils::FileName projectFilePath;
            if (Project *project = ProjectTree::currentProject())
                if (IDocument *doc = project->document())
                    projectFilePath = doc->filePath();
            return projectFilePath.toString();
        });

    expander->registerVariable(Constants::VAR_CURRENTPROJECT_BUILDPATH,
        tr("Full build path of the current project's active build configuration."),
        []() -> QString {
            BuildConfiguration *bc = activeBuildConfiguration();
            return bc ? bc->buildDirectory().toUserOutput() : QString();
        });

    expander->registerVariable(Constants::VAR_CURRENTPROJECT_NAME,
        tr("The name of the current project."),
        [this]() -> QString {
            Project *project = ProjectTree::currentProject();
            return project ? project->displayName() : QString();
        });

    expander->registerVariable(Constants::VAR_CURRENTKIT_NAME,
        tr("The name of the currently active kit."),
        []() -> QString {
            Kit *kit = currentKit();
            return kit ? kit->displayName() : QString();
        });

    expander->registerVariable(Constants::VAR_CURRENTKIT_FILESYSTEMNAME,
        tr("The name of the currently active kit in a filesystem-friendly version."),
        []() -> QString {
            Kit *kit = currentKit();
            return kit ? kit->fileSystemFriendlyName() : QString();
        });

    expander->registerVariable(Constants::VAR_CURRENTKIT_ID,
        tr("The id of the currently active kit."),
        []() -> QString {
            Kit *kit = currentKit();
            return kit ? kit->id().toString() : QString();
        });

    expander->registerVariable(Constants::VAR_CURRENTDEVICE_HOSTADDRESS,
        tr("The host address of the device in the currently active kit."),
        []() -> QString {
            Kit *kit = currentKit();
            const IDevice::ConstPtr device = DeviceKitInformation::device(kit);
            return device ? device->sshParameters().host : QString();
        });

    expander->registerVariable(Constants::VAR_CURRENTDEVICE_SSHPORT,
        tr("The SSH port of the device in the currently active kit."),
        []() -> QString {
            Kit *kit = currentKit();
            const IDevice::ConstPtr device = DeviceKitInformation::device(kit);
            return device ? QString::number(device->sshParameters().port) : QString();
        });

    expander->registerVariable(Constants::VAR_CURRENTDEVICE_USERNAME,
        tr("The username with which to log into the device in the currently active kit."),
        []() -> QString {
            Kit *kit = currentKit();
            const IDevice::ConstPtr device = DeviceKitInformation::device(kit);
            return device ? device->sshParameters().userName : QString();
        });


    expander->registerVariable(Constants::VAR_CURRENTDEVICE_PRIVATEKEYFILE,
        tr("The private key file with which to authenticate when logging into the device "
           "in the currently active kit."),
        []() -> QString {
            Kit *kit = currentKit();
            const IDevice::ConstPtr device = DeviceKitInformation::device(kit);
            return device ? device->sshParameters().privateKeyFile : QString();
        });

    expander->registerVariable(Constants::VAR_CURRENTBUILD_NAME,
        tr("The currently active build configuration's name."),
        [&]() -> QString {
            BuildConfiguration *bc = activeBuildConfiguration();
            return bc ? bc->displayName() : QString();
        });


    expander->registerVariable(Constants::VAR_CURRENTBUILD_TYPE,
        tr("The currently active build configuration's type."),
        [&]() -> QString {
            if (BuildConfiguration *bc = activeBuildConfiguration()) {
                BuildConfiguration::BuildType type = bc->buildType();
                if (type == BuildConfiguration::Debug)
                    return tr("debug");
                if (type == BuildConfiguration::Release)
                    return tr("release");
            }
            return tr("unknown");
        });


    QString fileDescription = tr("File where current session is saved.");
    auto fileHandler = [] { return SessionManager::sessionNameToFileName(SessionManager::activeSession()).toString(); };
    expander->registerFileVariables("Session", fileDescription, fileHandler);
    expander->registerFileVariables("CurrentSession", fileDescription, fileHandler, false);

    QString nameDescription = tr("Name of current session.");
    auto nameHandler = [] { return SessionManager::activeSession(); };
    expander->registerVariable("Session:Name", nameDescription, nameHandler);
    expander->registerVariable("CurrentSession:Name", nameDescription, nameHandler, false);

    return true;
}

void ProjectExplorerPluginPrivate::loadAction()
{
    if (debug)
        qDebug() << "ProjectExplorerPlugin::loadAction";


    QString dir = dd->m_lastOpenDirectory;

    // for your special convenience, we preselect a pro file if it is
    // the current file
    if (const IDocument *document = EditorManager::currentDocument()) {
        const QString fn = document->filePath().toString();
        const bool isProject = dd->m_profileMimeTypes.contains(document->mimeType());
        dir = isProject ? fn : QFileInfo(fn).absolutePath();
    }

    QString filename = QFileDialog::getOpenFileName(ICore::dialogParent(),
                                                    tr("Load Project"), dir,
                                                    dd->m_projectFilterString);
    if (filename.isEmpty())
        return;
    QString errorMessage;
    ProjectExplorerPlugin::openProject(filename, &errorMessage);

    if (!errorMessage.isEmpty())
        QMessageBox::critical(ICore::mainWindow(), tr("Failed to open project."), errorMessage);
    updateActions();
}

void ProjectExplorerPluginPrivate::unloadProjectContextMenu()
{
    if (debug)
        qDebug() << "ProjectExplorerPlugin::unloadProjectContextMenu";

    if (Project *p = ProjectTree::currentProject())
        ProjectExplorerPlugin::unloadProject(p);
}

void ProjectExplorerPluginPrivate::handleUnloadProject()
{
    if (debug)
        qDebug() << "ProjectExplorerPlugin::unloadProject";

    QList<Project *> projects = SessionManager::projects();
    QTC_ASSERT(!projects.isEmpty(), return);

    ProjectExplorerPlugin::unloadProject(projects.first());
}

void ProjectExplorerPlugin::unloadProject(Project *project)
{
    if (BuildManager::isBuilding(project)) {
        QMessageBox box;
        QPushButton *closeAnyway = box.addButton(tr("Cancel Build && Unload"), QMessageBox::AcceptRole);
        QPushButton *cancelClose = box.addButton(tr("Do Not Unload"), QMessageBox::RejectRole);
        box.setDefaultButton(cancelClose);
        box.setWindowTitle(tr("Unload Project %1?").arg(project->displayName()));
        box.setText(tr("The project %1 is currently being built.").arg(project->displayName()));
        box.setInformativeText(tr("Do you want to cancel the build process and unload the project anyway?"));
        box.exec();
        if (box.clickedButton() != closeAnyway)
            return;
        BuildManager::cancel();
    }

    IDocument *document = project->document();

    if (!document || document->filePath().isEmpty()) //nothing to save?
        return;

    if (!DocumentManager::saveModifiedDocumentSilently(document))
        return;

    dd->addToRecentProjects(document->filePath().toString(), project->displayName());

    SessionManager::removeProject(project);
    updateActions();
}

void ProjectExplorerPluginPrivate::closeAllProjects()
{
    if (debug)
        qDebug() << "ProjectExplorerPlugin::closeAllProject";

    if (!EditorManager::closeAllEditors())
        return; // Action has been cancelled

    SessionManager::closeAllProjects();
    updateActions();

    ModeManager::activateMode(Core::Constants::MODE_WELCOME);
}

void ProjectExplorerPlugin::extensionsInitialized()
{
    // Register factories for all project managers
    QList<IProjectManager*> projectManagers =
        ExtensionSystem::PluginManager::getObjects<IProjectManager>();

    QStringList allGlobPatterns;

    const QString filterSeparator = QLatin1String(";;");
    QStringList filterStrings;

    auto factory = new IDocumentFactory;
    factory->setOpener([this](const QString &fileName) -> IDocument* {
        QString errorMessage;
        ProjectExplorerPlugin::openProject(fileName, &errorMessage);
        if (!errorMessage.isEmpty())
            QMessageBox::critical(ICore::mainWindow(),
                tr("Failed to open project."), errorMessage);
        return 0;
    });

    Utils::MimeDatabase mdb;
    foreach (IProjectManager *manager, projectManagers) {
        const QString mimeType = manager->mimeType();
        factory->addMimeType(mimeType);
        Utils::MimeType mime = mdb.mimeTypeForName(mimeType);
        allGlobPatterns.append(mime.globPatterns());
        filterStrings.append(mime.filterString());

        dd->m_profileMimeTypes += mimeType;
    }

    addAutoReleasedObject(factory);

    QString allProjectsFilter = tr("All Projects");
    allProjectsFilter += QLatin1String(" (") + allGlobPatterns.join(QLatin1Char(' '))
            + QLatin1Char(')');
    filterStrings.prepend(allProjectsFilter);
    dd->m_projectFilterString = filterStrings.join(filterSeparator);

    BuildManager::extensionsInitialized();

    DeviceManager::instance()->addDevice(IDevice::Ptr(new DesktopDevice));
    DeviceManager::instance()->load();
    ToolChainManager::restoreToolChains();
    dd->m_kitManager->restoreKits();
}

void ProjectExplorerPluginPrivate::loadCustomWizards()
{
    // Add custom wizards, for which other plugins might have registered
    // class factories
    static bool firstTime = true;
    if (firstTime) {
        firstTime = false;
        foreach (IWizardFactory *cpw, CustomWizard::createWizards())
            m_instance->addAutoReleasedObject(cpw);
        foreach (IWizardFactory *cpw, JsonWizardFactory::createWizardFactories())
            m_instance->addAutoReleasedObject(cpw);
    }
}

void ProjectExplorerPluginPrivate::updateRunWithoutDeployMenu()
{
    m_runWithoutDeployAction->setVisible(m_projectExplorerSettings.deployBeforeRun);
}

ExtensionSystem::IPlugin::ShutdownFlag ProjectExplorerPlugin::aboutToShutdown()
{
    disconnect(ModeManager::instance(), &ModeManager::currentModeChanged,
               dd, &ProjectExplorerPluginPrivate::currentModeChanged);
    ProjectTree::aboutToShutDown();
    dd->m_proWindow->aboutToShutdown(); // disconnect from session
    SessionManager::closeAllProjects();
    dd->m_projectsMode = 0;
    dd->m_shuttingDown = true;
    // Attempt to synchronously shutdown all run controls.
    // If that fails, fall back to asynchronous shutdown (Debugger run controls
    // might shutdown asynchronously).
    if (dd->m_outputPane->closeTabs(AppOutputPane::CloseTabNoPrompt /* No prompt any more */))
        return SynchronousShutdown;
    connect(dd->m_outputPane, &AppOutputPane::allRunControlsFinished,
            this, &IPlugin::asynchronousShutdownFinished);
    return AsynchronousShutdown;
}

void ProjectExplorerPluginPrivate::newProject()
{
    if (debug)
        qDebug() << "ProjectExplorerPlugin::newProject";

    ICore::showNewItemDialog(tr("New Project", "Title of dialog"),
                              IWizardFactory::wizardFactoriesOfKind(IWizardFactory::ProjectWizard));
    updateActions();
}

void ProjectExplorerPluginPrivate::showSessionManager()
{
    if (debug)
        qDebug() << "ProjectExplorerPlugin::showSessionManager";

    if (SessionManager::isDefaultVirgin()) {
        // do not save new virgin default sessions
    } else {
        SessionManager::save();
    }
    SessionDialog sessionDialog(ICore::mainWindow());
    sessionDialog.setAutoLoadSession(dd->m_projectExplorerSettings.autorestoreLastSession);
    sessionDialog.exec();
    dd->m_projectExplorerSettings.autorestoreLastSession = sessionDialog.autoLoadSession();

    updateActions();

    IMode *welcomeMode = ModeManager::mode(Core::Constants::MODE_WELCOME);
    if (ModeManager::currentMode() == welcomeMode)
        updateWelcomePage();
}

void ProjectExplorerPluginPrivate::setStartupProject(Project *project)
{
    if (debug)
        qDebug() << "ProjectExplorerPlugin::setStartupProject";

    if (!project)
        return;
    SessionManager::setStartupProject(project);
    updateActions();
}

void ProjectExplorerPluginPrivate::savePersistentSettings()
{
    if (debug)
        qDebug()<<"ProjectExplorerPlugin::savePersistentSettings()";

    if (dd->m_shuttingDown)
        return;

    if (!SessionManager::loadingSession())  {
        foreach (Project *pro, SessionManager::projects())
            pro->saveSettings();

        if (SessionManager::isDefaultVirgin()) {
            // do not save new virgin default sessions
        } else {
            SessionManager::save();
        }
    }

    QSettings *s = ICore::settings();
    s->setValue(QLatin1String("ProjectExplorer/StartupSession"), SessionManager::activeSession());
    s->remove(QLatin1String("ProjectExplorer/RecentProjects/Files"));

    QStringList fileNames;
    QStringList displayNames;
    QList<QPair<QString, QString> >::const_iterator it, end;
    end = dd->m_recentProjects.constEnd();
    for (it = dd->m_recentProjects.constBegin(); it != end; ++it) {
        fileNames << (*it).first;
        displayNames << (*it).second;
    }

    s->setValue(QLatin1String("ProjectExplorer/RecentProjects/FileNames"), fileNames);
    s->setValue(QLatin1String("ProjectExplorer/RecentProjects/DisplayNames"), displayNames);

    s->setValue(QLatin1String("ProjectExplorer/Settings/BuildBeforeDeploy"), dd->m_projectExplorerSettings.buildBeforeDeploy);
    s->setValue(QLatin1String("ProjectExplorer/Settings/DeployBeforeRun"), dd->m_projectExplorerSettings.deployBeforeRun);
    s->setValue(QLatin1String("ProjectExplorer/Settings/SaveBeforeBuild"), dd->m_projectExplorerSettings.saveBeforeBuild);
    s->setValue(QLatin1String("ProjectExplorer/Settings/ShowCompilerOutput"), dd->m_projectExplorerSettings.showCompilerOutput);
    s->setValue(QLatin1String("ProjectExplorer/Settings/ShowRunOutput"), dd->m_projectExplorerSettings.showRunOutput);
    s->setValue(QLatin1String("ProjectExplorer/Settings/ShowDebugOutput"), dd->m_projectExplorerSettings.showDebugOutput);
    s->setValue(QLatin1String("ProjectExplorer/Settings/CleanOldAppOutput"), dd->m_projectExplorerSettings.cleanOldAppOutput);
    s->setValue(QLatin1String("ProjectExplorer/Settings/MergeStdErrAndStdOut"), dd->m_projectExplorerSettings.mergeStdErrAndStdOut);
    s->setValue(QLatin1String("ProjectExplorer/Settings/WrapAppOutput"), dd->m_projectExplorerSettings.wrapAppOutput);
    s->setValue(QLatin1String("ProjectExplorer/Settings/UseJom"), dd->m_projectExplorerSettings.useJom);
    s->setValue(QLatin1String("ProjectExplorer/Settings/AutoRestoreLastSession"), dd->m_projectExplorerSettings.autorestoreLastSession);
    s->setValue(QLatin1String("ProjectExplorer/Settings/PromptToStopRunControl"), dd->m_projectExplorerSettings.prompToStopRunControl);
    s->setValue(QLatin1String("ProjectExplorer/Settings/MaxAppOutputLines"), dd->m_projectExplorerSettings.maxAppOutputLines);
    s->setValue(QLatin1String("ProjectExplorer/Settings/EnvironmentId"), dd->m_projectExplorerSettings.environmentId.toByteArray());
}

void ProjectExplorerPlugin::openProjectWelcomePage(const QString &fileName)
{
    QString errorMessage;
    openProject(fileName, &errorMessage);
    if (!errorMessage.isEmpty())
        QMessageBox::critical(ICore::mainWindow(), tr("Failed to Open Project"), errorMessage);
}

Project *ProjectExplorerPlugin::openProject(const QString &fileName, QString *errorString)
{
    if (debug)
        qDebug() << "ProjectExplorerPlugin::openProject";

    QList<Project *> list = openProjects(QStringList() << fileName, errorString);
    if (list.isEmpty())
        return 0;
    dd->addToRecentProjects(fileName, list.first()->displayName());
    SessionManager::setStartupProject(list.first());
    return list.first();
}

static QList<IProjectManager*> allProjectManagers()
{
    return ExtensionSystem::PluginManager::getObjects<IProjectManager>();
}

static void appendError(QString *errorString, const QString &error)
{
    if (!errorString || error.isEmpty())
        return;

    if (!errorString->isEmpty())
        errorString->append(QLatin1Char('\n'));
    errorString->append(error);
}

QList<Project *> ProjectExplorerPlugin::openProjects(const QStringList &fileNames, QString *errorString)
{
    if (debug)
        qDebug() << "ProjectExplorerPlugin - opening projects " << fileNames;

    const QList<IProjectManager*> projectManagers = allProjectManagers();

    QList<Project*> openedPro;
    foreach (const QString &fileName, fileNames) {
        QTC_ASSERT(!fileName.isEmpty(), continue);

        QFileInfo fi = QFileInfo(fileName);
        QString filePath = fileName;
        if (fi.exists()) // canonicalFilePath will be empty otherwise!
            filePath = fi.canonicalFilePath();
        bool found = false;
        foreach (Project *pi, SessionManager::projects()) {
            if (filePath == pi->projectFilePath().toString()) {
                found = true;
                break;
            }
        }
        if (found) {
            appendError(errorString, tr("Failed opening project \"%1\": Project already open.")
                        .arg(QDir::toNativeSeparators(fileName)));
            SessionManager::reportProjectLoadingProgress();
            continue;
        }

        Utils::MimeDatabase mdb;
        Utils::MimeType mt = mdb.mimeTypeForFile(fileName);
        if (mt.isValid()) {
            bool foundProjectManager = false;
            foreach (IProjectManager *manager, projectManagers) {
                if (mt.matchesName(manager->mimeType())) {
                    foundProjectManager = true;
                    QString tmp;
                    if (Project *pro = manager->openProject(filePath, &tmp)) {
                        if (pro->restoreSettings()) {
                            connect(pro, &Project::fileListChanged,
                                    m_instance, &ProjectExplorerPlugin::fileListChanged);
                            SessionManager::addProject(pro);
                            openedPro += pro;
                        } else {
                            appendError(errorString, tr("Failed opening project \"%1\": Settings could not be restored.")
                                        .arg(QDir::toNativeSeparators(fileName)));
                            delete pro;
                        }
                    }
                    if (!tmp.isEmpty())
                        appendError(errorString, tmp);
                    break;
                }
            }
            if (!foundProjectManager) {
                appendError(errorString, tr("Failed opening project \"%1\": No plugin can open project type \"%2\".")
                            .arg(QDir::toNativeSeparators(fileName))
                            .arg(mt.name()));
            }
        } else {
            appendError(errorString, tr("Failed opening project \"%1\": Unknown project type.")
                        .arg(QDir::toNativeSeparators(fileName)));
        }
        if (fileNames.size() > 1)
            SessionManager::reportProjectLoadingProgress();
    }
    updateActions();

    bool switchToProjectsMode = Utils::anyOf(openedPro, &Project::needsConfiguration);

    if (!openedPro.isEmpty()) {
        if (switchToProjectsMode)
            ModeManager::activateMode(Constants::MODE_SESSION);
        else
            ModeManager::activateMode(Core::Constants::MODE_EDIT);
        ModeManager::setFocusToCurrentMode();
    }

    return openedPro;
}

void ProjectExplorerPluginPrivate::updateWelcomePage()
{
    m_welcomePage->reloadWelcomeScreenData();
}

void ProjectExplorerPluginPrivate::currentModeChanged(IMode *mode, IMode *oldMode)
{
    if (oldMode && oldMode->id() == Constants::MODE_SESSION)
        ICore::saveSettings();
    if (mode && mode->id() == Core::Constants::MODE_WELCOME)
        updateWelcomePage();
}

void ProjectExplorerPluginPrivate::determineSessionToRestoreAtStartup()
{
    // Process command line arguments first:
    if (m_instance->pluginSpec()->arguments().contains(QLatin1String("-lastsession")))
        m_sessionToRestoreAtStartup = SessionManager::lastSession();
    QStringList arguments = ExtensionSystem::PluginManager::arguments();
    if (m_sessionToRestoreAtStartup.isNull()) {
        QStringList sessions = SessionManager::sessions();
        // We have command line arguments, try to find a session in them
        // Default to no session loading
        foreach (const QString &arg, arguments) {
            if (sessions.contains(arg)) {
                // Session argument
                m_sessionToRestoreAtStartup = arg;
                break;
            }
        }
    }
    // Handle settings only after command line arguments:
    if (m_sessionToRestoreAtStartup.isNull()
        && m_projectExplorerSettings.autorestoreLastSession)
        m_sessionToRestoreAtStartup = SessionManager::lastSession();

    if (!m_sessionToRestoreAtStartup.isNull())
        ModeManager::activateMode(Core::Constants::MODE_EDIT);
}

// Return a list of glob patterns for project files ("*.pro", etc), use first, main pattern only.
QStringList ProjectExplorerPlugin::projectFileGlobs()
{
    QStringList result;
    Utils::MimeDatabase mdb;
    foreach (const IProjectManager *ipm, ExtensionSystem::PluginManager::getObjects<IProjectManager>()) {
        Utils::MimeType mimeType = mdb.mimeTypeForName(ipm->mimeType());
        if (mimeType.isValid()) {
            const QStringList patterns = mimeType.globPatterns();
            if (!patterns.isEmpty())
                result.append(patterns.front());
        }
    }
    return result;
}

void ProjectExplorerPlugin::updateContextMenuActions()
{
    dd->updateContextMenuActions();
}

/*!
    This function is connected to the ICore::coreOpened signal.  If
    there was no session explicitly loaded, it creates an empty new
    default session and puts the list of recent projects and sessions
    onto the welcome page.
*/
void ProjectExplorerPluginPrivate::restoreSession()
{
    if (debug)
        qDebug() << "ProjectExplorerPlugin::restoreSession";

    // We have command line arguments, try to find a session in them
    QStringList arguments = ExtensionSystem::PluginManager::arguments();
    if (!dd->m_sessionToRestoreAtStartup.isEmpty() && !arguments.isEmpty())
        arguments.removeOne(dd->m_sessionToRestoreAtStartup);

    // Massage the argument list.
    // Be smart about directories: If there is a session of that name, load it.
    //   Other than that, look for project files in it. The idea is to achieve
    //   'Do what I mean' functionality when starting Creator in a directory with
    //   the single command line argument '.' and avoid editor warnings about not
    //   being able to open directories.
    // In addition, convert "filename" "+45" or "filename" ":23" into
    //   "filename+45"   and "filename:23".
    if (!arguments.isEmpty()) {
        const QStringList sessions = SessionManager::sessions();
        QStringList projectGlobs = ProjectExplorerPlugin::projectFileGlobs();
        for (int a = 0; a < arguments.size(); ) {
            const QString &arg = arguments.at(a);
            const QFileInfo fi(arg);
            if (fi.isDir()) {
                const QDir dir(fi.absoluteFilePath());
                // Does the directory name match a session?
                if (dd->m_sessionToRestoreAtStartup.isEmpty()
                    && sessions.contains(dir.dirName())) {
                    dd->m_sessionToRestoreAtStartup = dir.dirName();
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
    if (!dd->m_sessionToRestoreAtStartup.isEmpty())
        SessionManager::loadSession(dd->m_sessionToRestoreAtStartup);

    // update welcome page
    connect(ModeManager::instance(), &ModeManager::currentModeChanged,
            dd, &ProjectExplorerPluginPrivate::currentModeChanged);
    connect(dd->m_welcomePage, &ProjectWelcomePage::requestSession,
            dd, &ProjectExplorerPluginPrivate::loadSession);
    connect(dd->m_welcomePage, &ProjectWelcomePage::requestProject,
            m_instance, &ProjectExplorerPlugin::openProjectWelcomePage);
    dd->m_arguments = arguments;
    QTimer::singleShot(0, m_instance, SLOT(restoreSession2()));
    updateActions();
}

void ProjectExplorerPluginPrivate::loadSession(const QString &session)
{
    if (debug)
        qDebug() << "ProjectExplorerPlugin::loadSession" << session;
    SessionManager::loadSession(session);
}

void ProjectExplorerPlugin::restoreSession2()
{
    QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
    ICore::openFiles(dd->m_arguments, ICore::OpenFilesFlags(ICore::CanContainLineAndColumnNumbers | ICore::SwitchMode));
}

void ProjectExplorerPluginPrivate::buildStateChanged(Project * pro)
{
    if (debug) {
        qDebug() << "buildStateChanged";
        qDebug() << pro->projectFilePath() << "isBuilding()" << BuildManager::isBuilding(pro);
    }
    Q_UNUSED(pro)
    updateActions();
}

// NBS TODO implement more than one runner
static IRunControlFactory *findRunControlFactory(RunConfiguration *config, RunMode mode)
{
    return ExtensionSystem::PluginManager::getObject<IRunControlFactory>(
        [&config, &mode](IRunControlFactory *factory) {
            return factory->canRun(config, mode);
        });
}

void ProjectExplorerPluginPrivate::executeRunConfiguration(RunConfiguration *runConfiguration, RunMode runMode)
{
    if (!runConfiguration->isConfigured()) {
        QString errorMessage;
        RunConfiguration::ConfigurationState state = runConfiguration->ensureConfigured(&errorMessage);

        if (state == RunConfiguration::UnConfigured) {
            m_instance->showRunErrorMessage(errorMessage);
            return;
        } else if (state == RunConfiguration::Waiting) {
            connect(runConfiguration, &RunConfiguration::configurationFinished,
                    this, &ProjectExplorerPluginPrivate::runConfigurationConfigurationFinished);
            m_delayedRunConfigurationForRun.append(qMakePair(runConfiguration, runMode));
            return;
        }
    }

    if (IRunControlFactory *runControlFactory = findRunControlFactory(runConfiguration, runMode)) {
        emit m_instance->aboutToExecuteProject(runConfiguration->target()->project(), runMode);

        QString errorMessage;
        RunControl *control = runControlFactory->create(runConfiguration, runMode, &errorMessage);
        if (!control) {
            m_instance->showRunErrorMessage(errorMessage);
            return;
        }
        startRunControl(control, runMode);
    }
}

void ProjectExplorerPlugin::showRunErrorMessage(const QString &errorMessage)
{
    // Empty, non-null means 'canceled' (custom executable dialog for libraries), whereas
    // empty, null means an error occurred, but message was not set
    if (!errorMessage.isEmpty() || errorMessage.isNull())
        QMessageBox::critical(ICore::mainWindow(), errorMessage.isNull() ? tr("Unknown error") : tr("Could Not Run"), errorMessage);
}

void ProjectExplorerPlugin::startRunControl(RunControl *runControl, RunMode runMode)
{
    dd->startRunControl(runControl, runMode);
}

void ProjectExplorerPluginPrivate::startRunControl(RunControl *runControl, RunMode runMode)
{
    m_outputPane->createNewOutputWindow(runControl);
    m_outputPane->flash(); // one flash for starting
    m_outputPane->showTabFor(runControl);
    bool popup = (runMode == NormalRunMode && dd->m_projectExplorerSettings.showRunOutput)
            || ((runMode == DebugRunMode || runMode == DebugRunModeWithBreakOnMain)
                && m_projectExplorerSettings.showDebugOutput);
    m_outputPane->setBehaviorOnOutput(runControl, popup ? AppOutputPane::Popup : AppOutputPane::Flash);
    runControl->start();
    emit m_instance->updateRunActions();
}

void ProjectExplorerPlugin::initiateInlineRenaming()
{
    dd->handleRenameFile();
}

void ProjectExplorerPluginPrivate::buildQueueFinished(bool success)
{
    if (debug)
        qDebug() << "buildQueueFinished()" << success;

    updateActions();

    bool ignoreErrors = true;
    if (!m_delayedRunConfiguration.isNull() && success && BuildManager::getErrorTaskCount() > 0) {
        ignoreErrors = QMessageBox::question(ICore::dialogParent(),
                                             ProjectExplorerPlugin::tr("Ignore All Errors?"),
                                             ProjectExplorerPlugin::tr("Found some build errors in current task.\n"
                                                "Do you want to ignore them?"),
                                             QMessageBox::Yes | QMessageBox::No,
                                             QMessageBox::No) == QMessageBox::Yes;
    }
    if (m_delayedRunConfiguration.isNull() && m_shouldHaveRunConfiguration) {
        QMessageBox::warning(ICore::dialogParent(),
                             ProjectExplorerPlugin::tr("Run Configuration Removed"),
                             ProjectExplorerPlugin::tr("The configuration that was supposed to run is no longer "
                                "available."), QMessageBox::Ok);
    }

    if (success && ignoreErrors && !m_delayedRunConfiguration.isNull()) {
        executeRunConfiguration(m_delayedRunConfiguration.data(), m_runMode);
    } else {
        if (BuildManager::tasksAvailable())
            BuildManager::showTaskWindow();
    }
    m_delayedRunConfiguration = 0;
    m_shouldHaveRunConfiguration = false;
    m_runMode = NoRunMode;
}

void ProjectExplorerPluginPrivate::runConfigurationConfigurationFinished()
{
    RunConfiguration *rc = qobject_cast<RunConfiguration *>(sender());
    RunMode runMode = NoRunMode;
    for (int i = 0; i < m_delayedRunConfigurationForRun.size(); ++i) {
        if (m_delayedRunConfigurationForRun.at(i).first == rc) {
            runMode = m_delayedRunConfigurationForRun.at(i).second;
            m_delayedRunConfigurationForRun.removeAt(i);
            break;
        }
    }
    if (runMode != NoRunMode && rc->isConfigured())
        executeRunConfiguration(rc, runMode);
}

static QString pathOrDirectoryFor(Node *node, bool dir)
{
    Utils::FileName path = node->path();
    QString location;
    FolderNode *folder = node->asFolderNode();
    if (node->nodeType() == VirtualFolderNodeType && folder) {
        // Virtual Folder case
        // If there are files directly below or no subfolders, take the folder path
        if (!folder->fileNodes().isEmpty() || folder->subFolderNodes().isEmpty()) {
            location = path.toString();
        } else {
            // Otherwise we figure out a commonPath from the subfolders
            QStringList list;
            foreach (FolderNode *f, folder->subFolderNodes())
                list << f->path().toString() + QLatin1Char('/');
            location = Utils::commonPath(list);
        }

        QFileInfo fi(location);
        while ((!fi.exists() || !fi.isDir()) && !fi.isRoot())
            fi.setFile(fi.absolutePath());
        location = fi.absoluteFilePath();
    } else {
        QFileInfo fi = path.toFileInfo();
        // remove any /suffixes, which e.g. ResourceNode uses
        // Note this should be removed again by making node->path() a true path again
        // That requires changes in both the VirtualFolderNode and ResourceNode
        while (!fi.exists() && !fi.isRoot())
            fi.setFile(fi.absolutePath());

        if (dir)
            location = fi.isDir() ? fi.absoluteFilePath() : fi.absolutePath();
        else
            location = fi.absoluteFilePath();
    }
    return location;
}

static QString pathFor(Node *node)
{
    return pathOrDirectoryFor(node, false);
}

static QString directoryFor(Node *node)
{
    return pathOrDirectoryFor(node, true);
}

QString ProjectExplorerPlugin::directoryFor(Node *node)
{
    return ProjectExplorer::directoryFor(node);
}

void ProjectExplorerPluginPrivate::updateActions()
{
    if (debug)
        qDebug() << "ProjectExplorerPlugin::updateActions";

    m_newAction->setEnabled(!ICore::isNewItemDialogRunning());

    Project *project = SessionManager::startupProject();
    Project *currentProject = ProjectTree::currentProject(); // for context menu actions

    QPair<bool, QString> buildActionState = buildSettingsEnabled(project);
    QPair<bool, QString> buildActionContextState = buildSettingsEnabled(currentProject);
    QPair<bool, QString> buildSessionState = buildSettingsEnabledForSession();

    QString projectName = project ? project->displayName() : QString();
    QString projectNameContextMenu = currentProject ? currentProject->displayName() : QString();

    m_unloadAction->setParameter(projectName);
    m_unloadActionContextMenu->setParameter(projectNameContextMenu);

    // Normal actions
    m_buildAction->setParameter(projectName);
    m_rebuildAction->setParameter(projectName);
    m_cleanAction->setParameter(projectName);

    m_buildAction->setEnabled(buildActionState.first);
    m_rebuildAction->setEnabled(buildActionState.first);
    m_cleanAction->setEnabled(buildActionState.first);

    m_buildAction->setToolTip(buildActionState.second);
    m_rebuildAction->setToolTip(buildActionState.second);
    m_cleanAction->setToolTip(buildActionState.second);

    // Context menu actions
    m_setStartupProjectAction->setParameter(projectNameContextMenu);

    bool hasDependencies = SessionManager::projectOrder(currentProject).size() > 1;
    if (hasDependencies) {
        m_buildActionContextMenu->setText(tr("Build Without Dependencies"));
        m_rebuildActionContextMenu->setText(tr("Rebuild Without Dependencies"));
        m_cleanActionContextMenu->setText(tr("Clean Without Dependencies"));
    } else {
        m_buildActionContextMenu->setText(tr("Build"));
        m_rebuildActionContextMenu->setText(tr("Rebuild"));
        m_cleanActionContextMenu->setText(tr("Clean"));
    }

    m_buildActionContextMenu->setEnabled(buildActionContextState.first);
    m_rebuildActionContextMenu->setEnabled(buildActionContextState.first);
    m_cleanActionContextMenu->setEnabled(buildActionContextState.first);

    m_buildActionContextMenu->setToolTip(buildActionState.second);
    m_rebuildActionContextMenu->setToolTip(buildActionState.second);
    m_cleanActionContextMenu->setToolTip(buildActionState.second);

    // build project only
    m_buildProjectOnlyAction->setEnabled(buildActionState.first);
    m_rebuildProjectOnlyAction->setEnabled(buildActionState.first);
    m_cleanProjectOnlyAction->setEnabled(buildActionState.first);

    m_buildProjectOnlyAction->setToolTip(buildActionState.second);
    m_rebuildProjectOnlyAction->setToolTip(buildActionState.second);
    m_cleanProjectOnlyAction->setToolTip(buildActionState.second);

    // Session actions
    m_closeAllProjects->setEnabled(SessionManager::hasProjects());
    m_unloadAction->setVisible(SessionManager::projects().size() <= 1);
    m_unloadAction->setEnabled(SessionManager::projects().size() == 1);
    m_unloadActionContextMenu->setEnabled(SessionManager::hasProjects());

    ActionContainer *aci =
        ActionManager::actionContainer(Constants::M_UNLOADPROJECTS);
    aci->menu()->menuAction()->setVisible(SessionManager::projects().size() > 1);

    m_buildSessionAction->setEnabled(buildSessionState.first);
    m_rebuildSessionAction->setEnabled(buildSessionState.first);
    m_cleanSessionAction->setEnabled(buildSessionState.first);

    m_buildSessionAction->setToolTip(buildSessionState.second);
    m_rebuildSessionAction->setToolTip(buildSessionState.second);
    m_cleanSessionAction->setToolTip(buildSessionState.second);

    m_cancelBuildAction->setEnabled(BuildManager::isBuilding());

    const bool hasProjects = SessionManager::hasProjects();
    m_projectSelectorAction->setEnabled(hasProjects);
    m_projectSelectorActionMenu->setEnabled(hasProjects);
    m_projectSelectorActionQuick->setEnabled(hasProjects);

    updateDeployActions();
    updateRunWithoutDeployMenu();
}

bool ProjectExplorerPlugin::saveModifiedFiles()
{
    if (debug)
        qDebug() << "ProjectExplorerPlugin::saveModifiedFiles";

    QList<IDocument *> documentsToSave = DocumentManager::modifiedDocuments();
    if (!documentsToSave.isEmpty()) {
        if (dd->m_projectExplorerSettings.saveBeforeBuild) {
            bool cancelled = false;
            DocumentManager::saveModifiedDocumentsSilently(documentsToSave, &cancelled);
            if (cancelled)
                return false;
        } else {
            bool cancelled = false;
            bool alwaysSave = false;
            if (!DocumentManager::saveModifiedDocuments(documentsToSave, QString(), &cancelled,
                                                        tr("Always save files before build"), &alwaysSave)) {
                if (cancelled)
                    return false;
            }

            if (alwaysSave)
                dd->m_projectExplorerSettings.saveBeforeBuild = true;
        }
    }
    return true;
}

//NBS handle case where there is no activeBuildConfiguration
// because someone delete all build configurations

void ProjectExplorerPluginPrivate::deploy(QList<Project *> projects)
{
    QList<Id> steps;
    if (m_projectExplorerSettings.buildBeforeDeploy)
        steps << Id(Constants::BUILDSTEPS_BUILD);
    steps << Id(Constants::BUILDSTEPS_DEPLOY);
    queue(projects, steps);
}

QString ProjectExplorerPlugin::displayNameForStepId(Id stepId)
{
    if (stepId == Constants::BUILDSTEPS_CLEAN)
        return tr("Clean");
    if (stepId == Constants::BUILDSTEPS_BUILD)
        return tr("Build", "Build step");
    if (stepId == Constants::BUILDSTEPS_DEPLOY)
        return tr("Deploy");
    return tr("Build", "Build step");
}

int ProjectExplorerPluginPrivate::queue(QList<Project *> projects, QList<Id> stepIds)
{
    if (debug) {
        QStringList projectNames, stepNames;
        foreach (const Project *p, projects)
            projectNames << p->displayName();
        foreach (const Id id, stepIds)
            stepNames << id.toString();
        qDebug() << "Building" << stepNames << "for projects" << projectNames;
    }

    if (!m_instance->saveModifiedFiles())
        return -1;

    QList<BuildStepList *> stepLists;
    QStringList names;
    QStringList preambleMessage;

    foreach (Project *pro, projects)
        if (pro && pro->needsConfiguration())
            preambleMessage.append(tr("The project %1 is not configured, skipping it.")
                                   .arg(pro->displayName()) + QLatin1Char('\n'));
    foreach (Id id, stepIds) {
        foreach (Project *pro, projects) {
            if (!pro || pro->needsConfiguration())
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
            names << m_instance->displayNameForStepId(id);
        }
    }

    if (stepLists.isEmpty())
        return 0;

    if (!BuildManager::buildLists(stepLists, names, preambleMessage))
        return -1;
    return stepLists.count();
}

void ProjectExplorerPluginPrivate::buildProjectOnly()
{
    queue(QList<Project *>() << SessionManager::startupProject(), QList<Id>() << Id(Constants::BUILDSTEPS_BUILD));
}

void ProjectExplorerPlugin::buildProject(Project *p)
{
    dd->queue(SessionManager::projectOrder(p),
          QList<Id>() << Id(Constants::BUILDSTEPS_BUILD));
}

void ProjectExplorerPlugin::requestProjectModeUpdate(Project *p)
{
    dd->m_proWindow->projectUpdated(p);
}

void ProjectExplorerPluginPrivate::handleBuildProject()
{
    queue(SessionManager::projectOrder(SessionManager::startupProject()),
          QList<Id>() << Id(Constants::BUILDSTEPS_BUILD));
}

void ProjectExplorerPluginPrivate::buildProjectContextMenu()
{
    queue(QList<Project *>() <<  ProjectTree::currentProject(),
          QList<Id>() << Id(Constants::BUILDSTEPS_BUILD));
}

void ProjectExplorerPluginPrivate::buildSession()
{
    queue(SessionManager::projectOrder(),
          QList<Id>() << Id(Constants::BUILDSTEPS_BUILD));
}

void ProjectExplorerPluginPrivate::rebuildProjectOnly()
{
    queue(QList<Project *>() << SessionManager::startupProject(),
          QList<Id>() << Id(Constants::BUILDSTEPS_CLEAN) << Id(Constants::BUILDSTEPS_BUILD));
}

void ProjectExplorerPluginPrivate::rebuildProject()
{
    queue(SessionManager::projectOrder(SessionManager::startupProject()),
          QList<Id>() << Id(Constants::BUILDSTEPS_CLEAN) << Id(Constants::BUILDSTEPS_BUILD));
}

void ProjectExplorerPluginPrivate::rebuildProjectContextMenu()
{
    queue(QList<Project *>() <<  ProjectTree::currentProject(),
          QList<Id>() << Id(Constants::BUILDSTEPS_CLEAN) << Id(Constants::BUILDSTEPS_BUILD));
}

void ProjectExplorerPluginPrivate::rebuildSession()
{
    queue(SessionManager::projectOrder(),
          QList<Id>() << Id(Constants::BUILDSTEPS_CLEAN) << Id(Constants::BUILDSTEPS_BUILD));
}

void ProjectExplorerPluginPrivate::deployProjectOnly()
{
    deploy(QList<Project *>() << SessionManager::startupProject());
}

void ProjectExplorerPluginPrivate::deployProject()
{
    deploy(SessionManager::projectOrder(SessionManager::startupProject()));
}

void ProjectExplorerPluginPrivate::deployProjectContextMenu()
{
    deploy(QList<Project *>() << ProjectTree::currentProject());
}

void ProjectExplorerPluginPrivate::deploySession()
{
    deploy(SessionManager::projectOrder());
}

void ProjectExplorerPluginPrivate::cleanProjectOnly()
{
    queue(QList<Project *>() << SessionManager::startupProject(),
          QList<Id>() << Id(Constants::BUILDSTEPS_CLEAN));
}

void ProjectExplorerPluginPrivate::cleanProject()
{
    queue(SessionManager::projectOrder(SessionManager::startupProject()),
          QList<Id>() << Id(Constants::BUILDSTEPS_CLEAN));
}

void ProjectExplorerPluginPrivate::cleanProjectContextMenu()
{
    queue(QList<Project *>() <<  ProjectTree::currentProject(),
          QList<Id>() << Id(Constants::BUILDSTEPS_CLEAN));
}

void ProjectExplorerPluginPrivate::cleanSession()
{
    queue(SessionManager::projectOrder(),
          QList<Id>() << Id(Constants::BUILDSTEPS_CLEAN));
}

void ProjectExplorerPluginPrivate::handleRunProject()
{
    m_instance->runStartupProject(NormalRunMode);
}

void ProjectExplorerPluginPrivate::runProjectWithoutDeploy()
{
    m_instance->runStartupProject(NormalRunMode, true);
}

void ProjectExplorerPluginPrivate::runProjectContextMenu()
{
    Node *node = ProjectTree::currentNode();
    ProjectNode *projectNode = node ? node->asProjectNode() : 0;
    if (projectNode == ProjectTree::currentProject()->rootProjectNode() || !projectNode) {
        m_instance->runProject(ProjectTree::currentProject(), NormalRunMode);
    } else {
        QAction *act = qobject_cast<QAction *>(sender());
        if (!act)
            return;
        RunConfiguration *rc = act->data().value<RunConfiguration *>();
        if (!rc)
            return;
        m_instance->runRunConfiguration(rc, NormalRunMode);
    }
}

static bool hasBuildSettings(Project *pro)
{
    return Utils::anyOf(SessionManager::projectOrder(pro), [](Project *project) {
        return project
                && project->activeTarget()
                && project->activeTarget()->activeBuildConfiguration();
    });
}

QPair<bool, QString> ProjectExplorerPluginPrivate::buildSettingsEnabled(Project *pro)
{
    QPair<bool, QString> result;
    result.first = true;
    if (!pro) {
        result.first = false;
        result.second = tr("No project loaded.");
    } else if (BuildManager::isBuilding(pro)) {
        result.first = false;
        result.second = tr("Currently building the active project.");
    } else if (pro->needsConfiguration()) {
        result.first = false;
        result.second = tr("The project %1 is not configured.").arg(pro->displayName());
    } else if (!hasBuildSettings(pro)) {
        result.first = false;
        result.second = tr("Project has no build settings.");
    } else {
        const QList<Project *> & projects = SessionManager::projectOrder(pro);
        foreach (Project *project, projects) {
            if (project
                    && project->activeTarget()
                    && project->activeTarget()->activeBuildConfiguration()
                    && !project->activeTarget()->activeBuildConfiguration()->isEnabled()) {
                result.first = false;
                result.second += tr("Building \"%1\" is disabled: %2<br>")
                        .arg(project->displayName(),
                             project->activeTarget()->activeBuildConfiguration()->disabledReason());
            }
        }
    }
    return result;
}

QPair<bool, QString> ProjectExplorerPluginPrivate::buildSettingsEnabledForSession()
{
    QPair<bool, QString> result;
    result.first = true;
    if (!SessionManager::hasProjects()) {
        result.first = false;
        result.second = tr("No project loaded.");
    } else if (BuildManager::isBuilding()) {
        result.first = false;
        result.second = tr("A build is in progress.");
    } else if (!hasBuildSettings(0)) {
        result.first = false;
        result.second = tr("Project has no build settings.");
    } else {
        foreach (Project *project, SessionManager::projectOrder(0)) {
            if (project
                    && project->activeTarget()
                    && project->activeTarget()->activeBuildConfiguration()
                    && !project->activeTarget()->activeBuildConfiguration()->isEnabled()) {
                result.first = false;
                result.second += tr("Building \"%1\" is disabled: %2")
                        .arg(project->displayName(),
                             project->activeTarget()->activeBuildConfiguration()->disabledReason());
                result.second += QLatin1Char('\n');
            }
        }
    }
    return result;
}

bool ProjectExplorerPlugin::coreAboutToClose()
{
    if (BuildManager::isBuilding()) {
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
    if (!dd->m_outputPane->aboutToClose())
        return false;
    return true;
}

static bool hasDeploySettings(Project *pro)
{
    return Utils::anyOf(SessionManager::projectOrder(pro), [](Project *project) {
        return project->activeTarget()
                && project->activeTarget()->activeDeployConfiguration()
                && !project->activeTarget()->activeDeployConfiguration()->stepList()->isEmpty();
    });
}

void ProjectExplorerPlugin::runProject(Project *pro, RunMode mode, const bool forceSkipDeploy)
{
    if (!pro)
        return;

    if (Target *target = pro->activeTarget())
        if (RunConfiguration *rc = target->activeRunConfiguration())
            runRunConfiguration(rc, mode, forceSkipDeploy);
}

void ProjectExplorerPlugin::runStartupProject(RunMode runMode, bool forceSkipDeploy)
{
    runProject(SessionManager::startupProject(), runMode, forceSkipDeploy);
}

void ProjectExplorerPlugin::runRunConfiguration(RunConfiguration *rc,
                                                RunMode runMode,
                                                const bool forceSkipDeploy)
{
    if (!rc->isEnabled())
        return;

    QList<Id> stepIds;
    if (!forceSkipDeploy && dd->m_projectExplorerSettings.deployBeforeRun) {
        if (dd->m_projectExplorerSettings.buildBeforeDeploy)
            stepIds << Id(Constants::BUILDSTEPS_BUILD);
        stepIds << Id(Constants::BUILDSTEPS_DEPLOY);
    }

    Project *pro = rc->target()->project();
    int queueCount = dd->queue(SessionManager::projectOrder(pro), stepIds);

    if (queueCount < 0) // something went wrong
        return;

    if (queueCount > 0) {
        // delay running till after our queued steps were processed
        dd->m_runMode = runMode;
        dd->m_delayedRunConfiguration = rc;
        dd->m_shouldHaveRunConfiguration = true;
    } else {
        dd->executeRunConfiguration(rc, runMode);
    }
    emit m_instance->updateRunActions();
}

void ProjectExplorerPluginPrivate::projectAdded(Project *pro)
{
    if (m_projectsMode)
        m_projectsMode->setEnabled(true);
    // more specific action en and disabling ?
    connect(pro, &Project::buildConfigurationEnabledChanged,
            this, &ProjectExplorerPluginPrivate::updateActions);
}

void ProjectExplorerPluginPrivate::projectRemoved(Project * pro)
{
    if (m_projectsMode)
        m_projectsMode->setEnabled(SessionManager::hasProjects());
    // more specific action en and disabling ?
    disconnect(pro, &Project::buildConfigurationEnabledChanged,
               this, &ProjectExplorerPluginPrivate::updateActions);
}

void ProjectExplorerPluginPrivate::projectDisplayNameChanged(Project *pro)
{
    addToRecentProjects(pro->projectFilePath().toString(), pro->displayName());
    updateActions();
}

void ProjectExplorerPluginPrivate::startupProjectChanged()
{
    static QPointer<Project> previousStartupProject = 0;
    Project *project = SessionManager::startupProject();
    if (project == previousStartupProject)
        return;

    if (previousStartupProject) {
        disconnect(previousStartupProject.data(), &Project::activeTargetChanged,
                   this, &ProjectExplorerPluginPrivate::activeTargetChanged);
    }

    previousStartupProject = project;

    if (project) {
        connect(project, &Project::activeTargetChanged,
                this, &ProjectExplorerPluginPrivate::activeTargetChanged);
    }

    activeTargetChanged();
    updateActions();
}

void ProjectExplorerPluginPrivate::activeTargetChanged()
{
    static QPointer<Target> previousTarget = 0;
    Target *target = 0;
    Project *startupProject = SessionManager::startupProject();
    if (startupProject)
        target = startupProject->activeTarget();
    if (target == previousTarget)
        return;

    if (previousTarget) {
        disconnect(previousTarget.data(), &Target::activeRunConfigurationChanged,
                   this, &ProjectExplorerPluginPrivate::activeRunConfigurationChanged);
    }
    previousTarget = target;
    if (target) {
        connect(target, &Target::activeRunConfigurationChanged,
                this, &ProjectExplorerPluginPrivate::activeRunConfigurationChanged);
    }

    activeRunConfigurationChanged();
    updateDeployActions();
}

void ProjectExplorerPluginPrivate::activeRunConfigurationChanged()
{
    static QPointer<RunConfiguration> previousRunConfiguration = 0;
    RunConfiguration *rc = 0;
    Project *startupProject = SessionManager::startupProject();
    if (startupProject && startupProject->activeTarget())
        rc = startupProject->activeTarget()->activeRunConfiguration();
    if (rc == previousRunConfiguration)
        return;
    if (previousRunConfiguration) {
        disconnect(previousRunConfiguration.data(), &RunConfiguration::requestRunActionsUpdate,
                   m_instance, &ProjectExplorerPlugin::updateRunActions);
    }
    previousRunConfiguration = rc;
    if (rc) {
        connect(rc, &RunConfiguration::requestRunActionsUpdate,
                m_instance, &ProjectExplorerPlugin::updateRunActions);
    }
    emit m_instance->updateRunActions();
}

void ProjectExplorerPluginPrivate::updateDeployActions()
{
    Project *project = SessionManager::startupProject();

    bool enableDeployActions = project
            && !BuildManager::isBuilding(project)
            && hasDeploySettings(project);
    Project *currentProject = ProjectTree::currentProject();
    bool enableDeployActionsContextMenu = currentProject
                              && !BuildManager::isBuilding(currentProject)
                              && hasDeploySettings(currentProject);

    if (m_projectExplorerSettings.buildBeforeDeploy) {
        if (hasBuildSettings(project)
                && !buildSettingsEnabled(project).first)
            enableDeployActions = false;
        if (hasBuildSettings(currentProject)
                && !buildSettingsEnabled(currentProject).first)
            enableDeployActionsContextMenu = false;
    }

    const QString projectName = project ? project->displayName() : QString();
    bool hasProjects = SessionManager::hasProjects();

    m_deployAction->setParameter(projectName);
    m_deployAction->setEnabled(enableDeployActions);

    m_deployActionContextMenu->setEnabled(enableDeployActionsContextMenu);

    m_deployProjectOnlyAction->setEnabled(enableDeployActions);

    bool enableDeploySessionAction = true;
    if (m_projectExplorerSettings.buildBeforeDeploy) {
        auto hasDisabledBuildConfiguration = [](Project *project) {
            return project && project->activeTarget()
                    && project->activeTarget()->activeBuildConfiguration()
                    && !project->activeTarget()->activeBuildConfiguration()->isEnabled();
        };

        if (Utils::anyOf(SessionManager::projectOrder(0), hasDisabledBuildConfiguration))
            enableDeploySessionAction = false;
    }
    if (!hasProjects || !hasDeploySettings(0) || BuildManager::isBuilding())
        enableDeploySessionAction = false;
    m_deploySessionAction->setEnabled(enableDeploySessionAction);

    emit m_instance->updateRunActions();
}

bool ProjectExplorerPlugin::canRun(Project *project, RunMode runMode, QString *whyNot)
{
    if (!project) {
        if (whyNot)
            *whyNot = tr("No active project.");
        return false;
    }

    if (project->needsConfiguration()) {
        if (whyNot)
            *whyNot = tr("The project \"%1\" is not configured.").arg(project->displayName());
        return false;
    }

    Target *target = project->activeTarget();
    if (!target) {
        if (whyNot)
            *whyNot = tr("The project \"%1\" has no active kit.").arg(project->displayName());
        return false;
    }

    RunConfiguration *activeRC = target->activeRunConfiguration();
    if (!activeRC) {
        if (whyNot)
            *whyNot = tr("The kit \"%1\" for the project \"%2\" has no active run configuration.")
                .arg(target->displayName(), project->displayName());
        return false;
    }

    if (!activeRC->isEnabled()) {
        if (whyNot)
            *whyNot = activeRC->disabledReason();
        return false;
    }


    if (dd->m_projectExplorerSettings.buildBeforeDeploy
            && dd->m_projectExplorerSettings.deployBeforeRun
            && hasBuildSettings(project)) {
        QPair<bool, QString> buildState = dd->buildSettingsEnabled(project);
        if (!buildState.first) {
            if (whyNot)
                *whyNot = buildState.second;
            return false;
        }
    }

    // shouldn't actually be shown to the user...
    if (!findRunControlFactory(activeRC, runMode)) {
        if (whyNot)
            *whyNot = tr("Cannot run \"%1\".").arg(activeRC->displayName());
        return false;
    }

    if (BuildManager::isBuilding()) {
        if (whyNot)
            *whyNot = tr("A build is still in progress.");
        return false;
    }

    return true;
}

void ProjectExplorerPluginPrivate::slotUpdateRunActions()
{
    Project *project = SessionManager::startupProject();
    QString whyNot;
    const bool state = ProjectExplorerPlugin::canRun(project, NormalRunMode, &whyNot);
    m_runAction->setEnabled(state);
    m_runAction->setToolTip(whyNot);
    m_runWithoutDeployAction->setEnabled(state);
}

void ProjectExplorerPluginPrivate::cancelBuild()
{
    if (debug)
        qDebug() << "ProjectExplorerPlugin::cancelBuild";

    if (BuildManager::isBuilding())
        BuildManager::cancel();
}

void ProjectExplorerPluginPrivate::addToRecentProjects(const QString &fileName, const QString &displayName)
{
    if (debug)
        qDebug() << "ProjectExplorerPlugin::addToRecentProjects(" << fileName << ")";

    if (fileName.isEmpty())
        return;
    QString prettyFileName(QDir::toNativeSeparators(fileName));

    QList<QPair<QString, QString> >::iterator it;
    for (it = m_recentProjects.begin(); it != m_recentProjects.end();)
        if ((*it).first == prettyFileName)
            it = m_recentProjects.erase(it);
        else
            ++it;

    if (m_recentProjects.count() > m_maxRecentProjects)
        m_recentProjects.removeLast();
    m_recentProjects.prepend(qMakePair(prettyFileName, displayName));
    QFileInfo fi(prettyFileName);
    m_lastOpenDirectory = fi.absolutePath();
    emit m_instance->recentProjectsChanged();
}

void ProjectExplorerPluginPrivate::updateUnloadProjectMenu()
{
    ActionContainer *aci = ActionManager::actionContainer(Constants::M_UNLOADPROJECTS);
    QMenu *menu = aci->menu();
    menu->clear();
    foreach (Project *project, SessionManager::projects()) {
        QAction *action = menu->addAction(tr("Close Project \"%1\"").arg(project->displayName()));
        connect(action, &QAction::triggered,
                [project] { ProjectExplorerPlugin::unloadProject(project); } );
    }
}

void ProjectExplorerPluginPrivate::updateRecentProjectMenu()
{
    typedef QList<QPair<QString, QString> >::const_iterator StringPairListConstIterator;
    if (debug)
        qDebug() << "ProjectExplorerPlugin::updateRecentProjectMenu";

    ActionContainer *aci =
        ActionManager::actionContainer(Constants::M_RECENTPROJECTS);
    QMenu *menu = aci->menu();
    menu->clear();

    bool hasRecentProjects = false;
    //projects (ignore sessions, they used to be in this list)
    const StringPairListConstIterator end = dd->m_recentProjects.constEnd();
    for (StringPairListConstIterator it = dd->m_recentProjects.constBegin(); it != end; ++it) {
        const QString fileName = it->first;
        if (fileName.endsWith(QLatin1String(".qws")))
            continue;
        QAction *action = menu->addAction(Utils::withTildeHomePath(fileName));
        connect(action, &QAction::triggered, this, [this, fileName] {
            openRecentProject(fileName);
        });
        hasRecentProjects = true;
    }
    menu->setEnabled(hasRecentProjects);

    // add the Clear Menu item
    if (hasRecentProjects) {
        menu->addSeparator();
        QAction *action = menu->addAction(QCoreApplication::translate(
                                          "Core", Core::Constants::TR_CLEAR_MENU));
        connect(action, &QAction::triggered, this, &ProjectExplorerPluginPrivate::clearRecentProjects);
    }
    emit m_instance->recentProjectsChanged();
}

void ProjectExplorerPluginPrivate::clearRecentProjects()
{
    m_recentProjects.clear();
    updateWelcomePage();
}

void ProjectExplorerPluginPrivate::openRecentProject(const QString &fileName)
{
    if (debug)
        qDebug() << "ProjectExplorerPlugin::openRecentProject()";

    if (!fileName.isEmpty()) {
        QString errorMessage;
        ProjectExplorerPlugin::openProject(fileName, &errorMessage);
        if (!errorMessage.isEmpty())
            QMessageBox::critical(ICore::mainWindow(), tr("Failed to open project."), errorMessage);
    }
}

void ProjectExplorerPluginPrivate::invalidateProject(Project *project)
{
    if (debug)
        qDebug() << "ProjectExplorerPlugin::invalidateProject" << project->displayName();

    disconnect(project, &Project::fileListChanged, m_instance, &ProjectExplorerPlugin::fileListChanged);
    updateActions();
}

void ProjectExplorerPluginPrivate::updateContextMenuActions()
{
    m_addExistingFilesAction->setEnabled(false);
    m_addExistingDirectoryAction->setEnabled(false);
    m_addNewFileAction->setEnabled(false);
    m_addNewSubprojectAction->setEnabled(false);
    m_removeProjectAction->setEnabled(false);
    m_removeFileAction->setEnabled(false);
    m_deleteFileAction->setEnabled(false);
    m_renameFileAction->setEnabled(false);

    m_addExistingFilesAction->setVisible(true);
    m_addExistingDirectoryAction->setVisible(true);
    m_addNewFileAction->setVisible(true);
    m_addNewSubprojectAction->setVisible(true);
    m_removeProjectAction->setVisible(true);
    m_removeFileAction->setVisible(true);
    m_deleteFileAction->setVisible(true);
    m_runActionContextMenu->setVisible(false);

    m_openTerminalHere->setVisible(true);
    m_showInGraphicalShell->setVisible(true);
    m_searchOnFileSystem->setVisible(true);

    ActionContainer *runMenu = ActionManager::actionContainer(Constants::RUNMENUCONTEXTMENU);
    runMenu->menu()->clear();
    runMenu->menu()->menuAction()->setVisible(false);

    Node *currentNode = ProjectTree::currentNode();

    if (currentNode && currentNode->projectNode()) {
        QList<ProjectAction> actions = currentNode->supportedActions(currentNode);

        if (ProjectNode *pn = currentNode->asProjectNode()) {
            if (ProjectTree::currentProject() && pn == ProjectTree::currentProject()->rootProjectNode()) {
                m_runActionContextMenu->setVisible(true);
            } else {
                QList<RunConfiguration *> runConfigs = pn->runConfigurations();
                if (runConfigs.count() == 1) {
                    m_runActionContextMenu->setVisible(true);
                    m_runActionContextMenu->setData(QVariant::fromValue(runConfigs.first()));
                } else if (runConfigs.count() > 1) {
                    runMenu->menu()->menuAction()->setVisible(true);
                    foreach (RunConfiguration *rc, runConfigs) {
                        QAction *act = new QAction(runMenu->menu());
                        act->setData(QVariant::fromValue(rc));
                        act->setText(tr("Run %1").arg(rc->displayName()));
                        runMenu->menu()->addAction(act);
                        connect(act, &QAction::triggered,
                                this, &ProjectExplorerPluginPrivate::runProjectContextMenu);
                    }
                }
            }
        }
        if (currentNode->asFolderNode()) {
            // Also handles ProjectNode
            m_addNewFileAction->setEnabled(actions.contains(AddNewFile)
                                              && !ICore::isNewItemDialogRunning());
            m_addNewSubprojectAction->setEnabled(currentNode->nodeType() == ProjectNodeType
                                                    && actions.contains(AddSubProject)
                                                    && !ICore::isNewItemDialogRunning());
            m_removeProjectAction->setEnabled(currentNode->nodeType() == ProjectNodeType
                                                    && actions.contains(RemoveSubProject));
            m_addExistingFilesAction->setEnabled(actions.contains(AddExistingFile));
            m_addExistingDirectoryAction->setEnabled(actions.contains(AddExistingDirectory));
            m_renameFileAction->setEnabled(actions.contains(Rename));
        } else if (currentNode->asFileNode()) {
            // Enable and show remove / delete in magic ways:
            // If both are disabled show Remove
            // If both are enabled show both (can't happen atm)
            // If only removeFile is enabled only show it
            // If only deleteFile is enable only show it
            bool enableRemove = actions.contains(RemoveFile);
            m_removeFileAction->setEnabled(enableRemove);
            bool enableDelete = actions.contains(EraseFile);
            m_deleteFileAction->setEnabled(enableDelete);
            m_deleteFileAction->setVisible(enableDelete);

            m_removeFileAction->setVisible(!enableDelete || enableRemove);
            m_renameFileAction->setEnabled(actions.contains(Rename));

            DocumentManager::populateOpenWithMenu(m_openWithMenu, ProjectTree::currentNode()->path().toString());
        }

        if (actions.contains(HidePathActions)) {
            m_openTerminalHere->setVisible(false);
            m_showInGraphicalShell->setVisible(false);
            m_searchOnFileSystem->setVisible(false);
        }

        if (actions.contains(HideFileActions)) {
            m_deleteFileAction->setVisible(false);
            m_removeFileAction->setVisible(false);
        }

        if (actions.contains(HideFolderActions)) {
            m_addNewFileAction->setVisible(false);
            m_addNewSubprojectAction->setVisible(false);
            m_removeProjectAction->setVisible(false);
            m_addExistingFilesAction->setVisible(false);
            m_addExistingDirectoryAction->setVisible(false);
        }
    }
}

void ProjectExplorerPluginPrivate::addNewFile()
{
    QTC_ASSERT(ProjectTree::currentNode(), return);
    QString location = directoryFor(ProjectTree::currentNode());

    QVariantMap map;
    map.insert(QLatin1String(Constants::PREFERRED_PROJECT_NODE), QVariant::fromValue(ProjectTree::currentNode()));
    if (ProjectTree::currentProject()) {
        QList<Id> profileIds = Utils::transform(ProjectTree::currentProject()->targets(), &Target::id);
        map.insert(QLatin1String(Constants::PROJECT_KIT_IDS), QVariant::fromValue(profileIds));
    }
    ICore::showNewItemDialog(tr("New File", "Title of dialog"),
                               IWizardFactory::wizardFactoriesOfKind(IWizardFactory::FileWizard)
                               + IWizardFactory::wizardFactoriesOfKind(IWizardFactory::ClassWizard),
                               location, map);
}

void ProjectExplorerPluginPrivate::addNewSubproject()
{
    QTC_ASSERT(ProjectTree::currentNode(), return);
    Node *currentNode = ProjectTree::currentNode();
    QString location = directoryFor(currentNode);

    if (currentNode->nodeType() == ProjectNodeType
            && currentNode->supportedActions(
                currentNode).contains(AddSubProject)) {
        QVariantMap map;
        map.insert(QLatin1String(Constants::PREFERRED_PROJECT_NODE), QVariant::fromValue(currentNode));
        if (ProjectTree::currentProject()) {
            QList<Id> profileIds = Utils::transform(ProjectTree::currentProject()->targets(), &Target::id);
            map.insert(QLatin1String(Constants::PROJECT_KIT_IDS), QVariant::fromValue(profileIds));
        }

        ICore::showNewItemDialog(tr("New Subproject", "Title of dialog"),
                              IWizardFactory::wizardFactoriesOfKind(IWizardFactory::ProjectWizard),
                              location, map);
    }
}

void ProjectExplorerPluginPrivate::handleAddExistingFiles()
{
    QTC_ASSERT(ProjectTree::currentNode(), return);

    QStringList fileNames = QFileDialog::getOpenFileNames(ICore::mainWindow(),
        tr("Add Existing Files"), directoryFor(ProjectTree::currentNode()));
    if (fileNames.isEmpty())
        return;
    ProjectExplorerPlugin::addExistingFiles(fileNames);
}

void ProjectExplorerPluginPrivate::addExistingDirectory()
{
    QTC_ASSERT(ProjectTree::currentNode(), return);

    SelectableFilesDialogAddDirectory dialog(directoryFor(ProjectTree::currentNode()), QStringList(), ICore::mainWindow());

    if (dialog.exec() == QDialog::Accepted)
        ProjectExplorerPlugin::addExistingFiles(dialog.selectedFiles());
}

void ProjectExplorerPlugin::addExistingFiles(const QStringList &filePaths)
{
    Node *node = ProjectTree::currentNode();
    FolderNode *folderNode = node ? node->asFolderNode() : 0;
    addExistingFiles(folderNode, filePaths);
}

void ProjectExplorerPlugin::addExistingFiles(FolderNode *folderNode, const QStringList &filePaths)
{
    if (!folderNode) // can happen when project is not yet parsed
        return;

    const QString dir = directoryFor(folderNode);
    QStringList fileNames = filePaths;
    QStringList notAdded;
    folderNode->addFiles(fileNames, &notAdded);

    if (!notAdded.isEmpty()) {
        QString message = tr("Could not add following files to project %1:").arg(folderNode->projectNode()->displayName());
        message += QLatin1Char('\n');
        QString files = notAdded.join(QLatin1Char('\n'));
        QMessageBox::warning(ICore::mainWindow(), tr("Adding Files to Project Failed"),
                             message + files);
        foreach (const QString &file, notAdded)
            fileNames.removeOne(file);
    }

    VcsManager::promptToAdd(dir, fileNames);
}

void ProjectExplorerPluginPrivate::removeProject()
{
    Node *node = ProjectTree::currentNode();
    if (!node)
        return;
    ProjectNode *subProjectNode = node->projectNode();
    if (!subProjectNode)
        return;
    ProjectNode *projectNode = subProjectNode->parentFolderNode()->asProjectNode();
    if (projectNode) {
        RemoveFileDialog removeFileDialog(subProjectNode->path().toString(), ICore::mainWindow());
        removeFileDialog.setDeleteFileVisible(false);
        if (removeFileDialog.exec() == QDialog::Accepted)
            projectNode->removeSubProjects(QStringList() << subProjectNode->path().toString());
    }
}

void ProjectExplorerPluginPrivate::openFile()
{
    QTC_ASSERT(ProjectTree::currentNode(), return);
    EditorManager::openEditor(ProjectTree::currentNode()->path().toString());
}

void ProjectExplorerPluginPrivate::searchOnFileSystem()
{
    QTC_ASSERT(ProjectTree::currentNode(), return);
    TextEditor::FindInFiles::findOnFileSystem(pathFor(ProjectTree::currentNode()));
}

void ProjectExplorerPluginPrivate::showInGraphicalShell()
{
    QTC_ASSERT(ProjectTree::currentNode(), return);
    FileUtils::showInGraphicalShell(ICore::mainWindow(), pathFor(ProjectTree::currentNode()));
}

void ProjectExplorerPluginPrivate::openTerminalHere()
{
    QTC_ASSERT(ProjectTree::currentNode(), return);
    FileUtils::openTerminal(directoryFor(ProjectTree::currentNode()));
}

void ProjectExplorerPluginPrivate::removeFile()
{
    Node *currentNode = ProjectTree::currentNode();
    QTC_ASSERT(currentNode && currentNode->nodeType() == FileNodeType, return);

    FileNode *fileNode = currentNode->asFileNode();

    QString filePath = currentNode->path().toString();
    RemoveFileDialog removeFileDialog(filePath, ICore::mainWindow());

    if (removeFileDialog.exec() == QDialog::Accepted) {
        const bool deleteFile = removeFileDialog.isDeleteFileChecked();

        // remove from project
        FolderNode *folderNode = fileNode->parentFolderNode();
        Q_ASSERT(folderNode);

        if (!folderNode->removeFiles(QStringList(filePath))) {
            QMessageBox::warning(ICore::mainWindow(), tr("Removing File Failed"),
                                 tr("Could not remove file %1 from project %2.").arg(filePath).arg(folderNode->projectNode()->displayName()));
            return;
        }

        DocumentManager::expectFileChange(filePath);
        FileUtils::removeFile(filePath, deleteFile);
        DocumentManager::unexpectFileChange(filePath);
    }
}

void ProjectExplorerPluginPrivate::deleteFile()
{
    Node *currentNode = ProjectTree::currentNode();
    QTC_ASSERT(currentNode && currentNode->nodeType() == FileNodeType, return);

    FileNode *fileNode = currentNode->asFileNode();

    QString filePath = currentNode->path().toString();
    QMessageBox::StandardButton button =
            QMessageBox::question(ICore::mainWindow(),
                                  tr("Delete File"),
                                  tr("Delete %1 from file system?").arg(filePath),
                                  QMessageBox::Yes | QMessageBox::No);
    if (button != QMessageBox::Yes)
        return;

    FolderNode *folderNode = fileNode->parentFolderNode();
    QTC_ASSERT(folderNode, return);

    folderNode->deleteFiles(QStringList(filePath));

    DocumentManager::expectFileChange(filePath);
    if (IVersionControl *vc =
            VcsManager::findVersionControlForDirectory(QFileInfo(filePath).absolutePath())) {
        vc->vcsDelete(filePath);
    }
    QFile file(filePath);
    if (file.exists()) {
        if (!file.remove())
            QMessageBox::warning(ICore::mainWindow(), tr("Deleting File Failed"),
                                 tr("Could not delete file %1.").arg(filePath));
    }
    DocumentManager::unexpectFileChange(filePath);
}

void ProjectExplorerPluginPrivate::handleRenameFile()
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

void ProjectExplorerPlugin::renameFile(Node *node, const QString &newFilePath)
{
    QString orgFilePath = node->path().toFileInfo().absoluteFilePath();

    if (FileUtils::renameFile(orgFilePath, newFilePath)) {
        // Tell the project plugin about rename
        FolderNode *folderNode = node->parentFolderNode();
        QString projectDisplayName = folderNode->projectNode()->displayName();
        if (!folderNode->renameFile(orgFilePath, newFilePath)) {
            dd->m_renameFileError = tr("The file %1 was renamed to %2, but the project file %3 could not be automatically changed.")
                    .arg(orgFilePath)
                    .arg(newFilePath)
                    .arg(projectDisplayName);

            QTimer::singleShot(0, m_instance, SLOT(showRenameFileError()));
        }
    }
}

void ProjectExplorerPluginPrivate::handleSetStartupProject()
{
    setStartupProject(ProjectTree::currentProject());
}

void ProjectExplorerPlugin::showRenameFileError()
{
    QMessageBox::warning(ICore::mainWindow(), tr("Project Editing Failed"), dd->m_renameFileError);
}

void ProjectExplorerPluginPrivate::updateSessionMenu()
{
    m_sessionMenu->clear();
    QActionGroup *ag = new QActionGroup(m_sessionMenu);
    connect(ag, &QActionGroup::triggered, this, &ProjectExplorerPluginPrivate::setSession);
    const QString activeSession = SessionManager::activeSession();
    foreach (const QString &session, SessionManager::sessions()) {
        QAction *act = ag->addAction(session);
        act->setCheckable(true);
        if (session == activeSession)
            act->setChecked(true);
    }
    m_sessionMenu->addActions(ag->actions());
    m_sessionMenu->setEnabled(true);
}

void ProjectExplorerPluginPrivate::setSession(QAction *action)
{
    QString session = action->text();
    if (session != SessionManager::activeSession())
        SessionManager::loadSession(session);
}

void ProjectExplorerPlugin::setProjectExplorerSettings(const ProjectExplorerSettings &pes)
{
    QTC_ASSERT(dd->m_projectExplorerSettings.environmentId == pes.environmentId, return);

    if (dd->m_projectExplorerSettings == pes)
        return;
    dd->m_projectExplorerSettings = pes;
    emit m_instance->settingsChanged();
}

ProjectExplorerSettings ProjectExplorerPlugin::projectExplorerSettings()
{
    return dd->m_projectExplorerSettings;
}

QStringList ProjectExplorerPlugin::projectFilePatterns()
{
    QStringList patterns;
    Utils::MimeDatabase mdb;
    foreach (const IProjectManager *pm, allProjectManagers()) {
        Utils::MimeType mt = mdb.mimeTypeForName(pm->mimeType());
        if (mt.isValid())
            patterns.append(mt.globPatterns());
    }
    return patterns;
}

void ProjectExplorerPlugin::openOpenProjectDialog()
{
    const QString path = DocumentManager::useProjectsDirectory() ? DocumentManager::projectsDirectory() : QString();
    const QStringList files = DocumentManager::getOpenFileNames(dd->m_projectFilterString, path);
    if (!files.isEmpty())
        ICore::openFiles(files, ICore::SwitchMode);
}

QList<QPair<QString, QString> > ProjectExplorerPlugin::recentProjects()
{
    return dd->m_recentProjects;
}

} // namespace ProjectExplorer
