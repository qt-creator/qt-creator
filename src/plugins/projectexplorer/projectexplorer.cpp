/***************************************************************************
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

#include "projectexplorer.h"

#include "buildsteplist.h"
#include "configtaskhandler.h"
#include "customexecutablerunconfiguration.h"
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
#include "projectexplorersettingspage.h"
#include "projectmanager.h"
#include "removetaskhandler.h"
#include "kitfeatureprovider.h"
#include "kitmanager.h"
#include "kitoptionspage.h"
#include "target.h"
#include "toolchainmanager.h"
#include "toolchainoptionspage.h"
#include "copytaskhandler.h"
#include "showineditortaskhandler.h"
#include "vcsannotatetaskhandler.h"
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
#include "appoutputpane.h"
#include "processstep.h"
#include "kitinformation.h"
#include "projectfilewizardextension.h"
#include "projectmanager.h"
#include "projecttreewidget.h"
#include "projectwindow.h"
#include "runsettingspropertiespage.h"
#include "session.h"
#include "projectnodes.h"
#include "sessiondialog.h"
#include "buildconfiguration.h"
#include "miniprojecttargetselector.h"
#include "taskhub.h"
#include "customtoolchain.h"
#include "selectablefilesmodel.h"
#include "customwizard/customwizard.h"
#include "devicesupport/desktopdevice.h"
#include "devicesupport/desktopdevicefactory.h"
#include "devicesupport/devicemanager.h"
#include "devicesupport/devicesettingspage.h"
#include "targetsettingspanel.h"
#include "projectpanelfactory.h"
#include "waitforstopdialog.h"
#include "projectexplorericons.h"

#include "windebuginterface.h"
#include "msvctoolchain.h"

#include "projecttree.h"
#include "projectwelcomepage.h"

#include <app/app_version.h>
#include <extensionsystem/pluginspec.h>
#include <extensionsystem/pluginmanager.h>
#include <coreplugin/icore.h>
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
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/findplaceholder.h>
#include <coreplugin/vcsmanager.h>
#include <coreplugin/iversioncontrol.h>
#include <coreplugin/fileutils.h>
#include <coreplugin/removefiledialog.h>
#include <coreplugin/diffservice.h>
#include <texteditor/findinfiles.h>
#include <texteditor/textdocument.h>
#include <texteditor/texteditorconstants.h>
#include <ssh/sshconnection.h>

#include <utils/algorithm.h>
#include <utils/fileutils.h>
#include <utils/macroexpander.h>
#include <utils/mimetypes/mimedatabase.h>
#include <utils/objectpool.h>
#include <utils/parameteraction.h>
#include <utils/processhandle.h>
#include <utils/qtcassert.h>
#include <utils/stringutils.h>
#include <utils/utilsicons.h>

#include <QtPlugin>
#include <QFileInfo>
#include <QSettings>

#include <QAction>
#include <QApplication>
#include <QDir>
#include <QFileDialog>
#include <QMenu>
#include <QMessageBox>
#include <QThreadPool>
#include <QTimer>

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

using namespace Core;
using namespace ProjectExplorer::Internal;

namespace ProjectExplorer {

namespace Constants {
const int  P_MODE_SESSION         = 85;

// Actions
const char NEWPROJECT[]           = "ProjectExplorer.NewProject";
const char LOAD[]                 = "ProjectExplorer.Load";
const char UNLOAD[]               = "ProjectExplorer.Unload";
const char UNLOADCM[]             = "ProjectExplorer.UnloadCM";
const char CLEARSESSION[]         = "ProjectExplorer.ClearSession";
const char BUILDPROJECTONLY[]     = "ProjectExplorer.BuildProjectOnly";
const char BUILDCM[]              = "ProjectExplorer.BuildCM";
const char BUILDDEPENDCM[]        = "ProjectExplorer.BuildDependenciesCM";
const char BUILDSESSION[]         = "ProjectExplorer.BuildSession";
const char REBUILDPROJECTONLY[]   = "ProjectExplorer.RebuildProjectOnly";
const char REBUILD[]              = "ProjectExplorer.Rebuild";
const char REBUILDCM[]            = "ProjectExplorer.RebuildCM";
const char REBUILDDEPENDCM[]      = "ProjectExplorer.RebuildDependenciesCM";
const char REBUILDSESSION[]       = "ProjectExplorer.RebuildSession";
const char DEPLOYPROJECTONLY[]    = "ProjectExplorer.DeployProjectOnly";
const char DEPLOY[]               = "ProjectExplorer.Deploy";
const char DEPLOYCM[]             = "ProjectExplorer.DeployCM";
const char DEPLOYSESSION[]        = "ProjectExplorer.DeploySession";
const char CLEANPROJECTONLY[]     = "ProjectExplorer.CleanProjectOnly";
const char CLEAN[]                = "ProjectExplorer.Clean";
const char CLEANCM[]              = "ProjectExplorer.CleanCM";
const char CLEANDEPENDCM[]        = "ProjectExplorer.CleanDependenciesCM";
const char CLEANSESSION[]         = "ProjectExplorer.CleanSession";
const char CANCELBUILD[]          = "ProjectExplorer.CancelBuild";
const char RUN[]                  = "ProjectExplorer.Run";
const char RUNWITHOUTDEPLOY[]     = "ProjectExplorer.RunWithoutDeploy";
const char RUNCONTEXTMENU[]       = "ProjectExplorer.RunContextMenu";
const char ADDNEWFILE[]           = "ProjectExplorer.AddNewFile";
const char ADDEXISTINGFILES[]     = "ProjectExplorer.AddExistingFiles";
const char ADDEXISTINGDIRECTORY[] = "ProjectExplorer.AddExistingDirectory";
const char ADDNEWSUBPROJECT[]     = "ProjectExplorer.AddNewSubproject";
const char REMOVEPROJECT[]        = "ProjectExplorer.RemoveProject";
const char OPENFILE[]             = "ProjectExplorer.OpenFile";
const char SEARCHONFILESYSTEM[]   = "ProjectExplorer.SearchOnFileSystem";
const char SHOWINGRAPHICALSHELL[] = "ProjectExplorer.ShowInGraphicalShell";
const char OPENTERMIANLHERE[]     = "ProjectExplorer.OpenTerminalHere";
const char REMOVEFILE[]           = "ProjectExplorer.RemoveFile";
const char DUPLICATEFILE[]        = "ProjectExplorer.DuplicateFile";
const char DELETEFILE[]           = "ProjectExplorer.DeleteFile";
const char RENAMEFILE[]           = "ProjectExplorer.RenameFile";
const char DIFFFILE[]             = "ProjectExplorer.DiffFile";
const char SETSTARTUP[]           = "ProjectExplorer.SetStartup";
const char PROJECTTREE_COLLAPSE_ALL[] = "ProjectExplorer.CollapseAll";

const char SELECTTARGET[]         = "ProjectExplorer.SelectTarget";
const char SELECTTARGETQUICK[]    = "ProjectExplorer.SelectTargetQuick";

// Action priorities
const int  P_ACTION_RUN            = 100;
const int  P_ACTION_BUILDPROJECT   = 80;

// Context
const char C_PROJECTEXPLORER[]    = "Project Explorer";

// Menus
const char M_RECENTPROJECTS[]     = "ProjectExplorer.Menu.Recent";
const char M_UNLOADPROJECTS[]     = "ProjectExplorer.Menu.Unload";
const char M_SESSION[]            = "ProjectExplorer.Menu.Session";

const char RUNMENUCONTEXTMENU[]   = "Project.RunMenu";
const char FOLDER_OPEN_LOCATIONS_CONTEXT_MENU[]  = "Project.F.OpenLocation.CtxMenu";
const char PROJECT_OPEN_LOCATIONS_CONTEXT_MENU[]  = "Project.P.OpenLocation.CtxMenu";

} // namespace Constants

static Target *activeTarget()
{
    Project *project = ProjectTree::currentProject();
    return project ? project->activeTarget() : nullptr;
}

static BuildConfiguration *activeBuildConfiguration()
{
    Target *target = activeTarget();
    return target ? target->activeBuildConfiguration() : nullptr;
}

static Kit *currentKit()
{
    Target *target = activeTarget();
    return target ? target->kit() : nullptr;
}

static bool isTextFile(const QString &fileName)
{
    return Utils::mimeTypeForFile(fileName).inherits(
                TextEditor::Constants::C_TEXTEDITOR_MIMETYPE_TEXT);
}

static bool isDiffServiceAvailable()
{
    return ExtensionSystem::PluginManager::getObject<DiffService>();
}

class ProjectExplorerPluginPrivate : public QObject
{
    Q_DECLARE_TR_FUNCTIONS(ProjectExplorer::ProjectExplorerPlugin)

public:
    void deploy(QList<Project *>);
    int queue(QList<Project *>, QList<Id> stepIds);
    void updateContextMenuActions();
    void updateLocationSubMenus();
    void executeRunConfiguration(RunConfiguration *, Core::Id mode);
    QPair<bool, QString> buildSettingsEnabledForSession();
    QPair<bool, QString> buildSettingsEnabled(const Project *pro);

    void addToRecentProjects(const QString &fileName, const QString &displayName);
    void startRunControl(RunControl *runControl);

    void updateActions();
    void updateContext();
    void updateDeployActions();
    void updateRunWithoutDeployMenu();

    void buildQueueFinished(bool success);

    void buildStateChanged(ProjectExplorer::Project * pro);
    void loadAction();
    void handleUnloadProject();
    void unloadProjectContextMenu();
    void closeAllProjects();
    void showSessionManager();
    void updateSessionMenu();
    void setSession(QAction *action);

    void determineSessionToRestoreAtStartup();
    void restoreSession();
    void runProjectContextMenu();
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
    void duplicateFile();
    void deleteFile();
    void handleRenameFile();
    void handleDiffFile();
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
    void activeBuildConfigurationChanged();

    void slotUpdateRunActions();

    void currentModeChanged(Core::Id mode, Core::Id oldMode);

    void updateWelcomePage();

    void runConfigurationConfigurationFinished();

    void checkForShutdown();
    void timerEvent(QTimerEvent *) override;

    QList<QPair<QString, QString> > recentProjects() const;

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
    QAction *m_buildDependenciesActionContextMenu;
    QAction *m_buildSessionAction;
    QAction *m_rebuildProjectOnlyAction;
    Utils::ParameterAction *m_rebuildAction;
    QAction *m_rebuildActionContextMenu;
    QAction *m_rebuildDependenciesActionContextMenu;
    QAction *m_rebuildSessionAction;
    QAction *m_cleanProjectOnlyAction;
    QAction *m_deployProjectOnlyAction;
    Utils::ParameterAction *m_deployAction;
    QAction *m_deployActionContextMenu;
    QAction *m_deploySessionAction;
    Utils::ParameterAction *m_cleanAction;
    QAction *m_cleanActionContextMenu;
    QAction *m_cleanDependenciesActionContextMenu;
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
    QAction *m_duplicateFileAction;
    QAction *m_removeProjectAction;
    QAction *m_deleteFileAction;
    QAction *m_renameFileAction;
    QAction *m_diffFileAction;
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

    ProjectWindow *m_proWindow = nullptr;
    QString m_sessionToRestoreAtStartup;

    QStringList m_profileMimeTypes;
    AppOutputPane *m_outputPane = nullptr;
    int m_activeRunControlCount = 0;
    int m_shutdownWatchDogId = -1;

    QHash<QString, std::function<Project *(const Utils::FileName &)>> m_projectCreators;
    QList<QPair<QString, QString> > m_recentProjects; // pair of filename, displayname
    static const int m_maxRecentProjects = 25;

    QString m_lastOpenDirectory;
    QPointer<RunConfiguration> m_delayedRunConfiguration;
    QList<QPair<RunConfiguration *, Core::Id>> m_delayedRunConfigurationForRun;
    QString m_projectFilterString;
    MiniProjectTargetSelector * m_targetSelector;
    ProjectExplorerSettings m_projectExplorerSettings;
    bool m_shouldHaveRunConfiguration = false;
    bool m_shuttingDown = false;
    Core::Id m_runMode = Constants::NO_RUN_MODE;
    ProjectWelcomePage *m_welcomePage = nullptr;
    IMode *m_projectsMode = nullptr;

    TaskHub *m_taskHub = nullptr;
    KitManager *m_kitManager = nullptr;
    ToolChainManager *m_toolChainManager = nullptr;
    QStringList m_arguments;
#ifdef WITH_JOURNALD
    JournaldWatcher *m_journalWatcher = nullptr;
#endif
    QThreadPool m_threadPool;
};

class ProjectsMode : public IMode
{
public:
    ProjectsMode(QWidget *proWindow)
    {
        setWidget(proWindow);
        setContext(Context(Constants::C_PROJECTEXPLORER));
        setDisplayName(QCoreApplication::translate("ProjectExplorer::ProjectsMode", "Projects"));
        setIcon(Utils::Icon::modeIcon(Icons::MODE_PROJECT_CLASSIC,
                                      Icons::MODE_PROJECT_FLAT, Icons::MODE_PROJECT_FLAT_ACTIVE));
        setPriority(Constants::P_MODE_SESSION);
        setId(Constants::MODE_SESSION);
        setContextHelpId(QLatin1String("Managing Projects"));
    }
};

static ProjectExplorerPlugin *m_instance = nullptr;
static ProjectExplorerPluginPrivate *dd = nullptr;

ProjectExplorerPlugin::ProjectExplorerPlugin()
{
    m_instance = this;
    dd = new ProjectExplorerPluginPrivate;
}

ProjectExplorerPlugin::~ProjectExplorerPlugin()
{
    delete dd->m_proWindow; // Needs access to the kit manager.
    JsonWizardFactory::destroyAllFactories();

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
    if (Utils::HostOsInfo::isWindowsHost()) {
        addAutoReleasedObject(new WinDebugInterface);
        addAutoReleasedObject(new MsvcToolChainFactory);
    } else {
        addAutoReleasedObject(new LinuxIccToolChainFactory);
    }
    if (!Utils::HostOsInfo::isMacHost())
        addAutoReleasedObject(new MingwToolChainFactory); // Mingw offers cross-compiling to windows
    addAutoReleasedObject(new GccToolChainFactory);
    addAutoReleasedObject(new ClangToolChainFactory);
    addAutoReleasedObject(new CustomToolChainFactory);

    addAutoReleasedObject(new DesktopDeviceFactory);

    dd->m_kitManager = new KitManager; // register before ToolChainManager
    dd->m_toolChainManager = new ToolChainManager;

    // Register languages
    ToolChainManager::registerLanguage(Constants::C_LANGUAGE_ID, tr("C"));
    ToolChainManager::registerLanguage(Constants::CXX_LANGUAGE_ID, tr("C++"));

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

    IWizardFactory::registerFactoryCreator([]() -> QList<IWizardFactory *> {
        QList<IWizardFactory *> result;
        result << CustomWizard::createWizards();
        result << JsonWizardFactory::createWizardFactories();
        return result;
    });

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
    connect(sessionManager, &SessionManager::dependencyChanged,
            dd, &ProjectExplorerPluginPrivate::updateActions);
    connect(sessionManager, &SessionManager::sessionLoaded,
            dd, &ProjectExplorerPluginPrivate::updateActions);
    connect(sessionManager, &SessionManager::sessionLoaded,
            dd, &ProjectExplorerPluginPrivate::updateWelcomePage);

    ProjectTree *tree = new ProjectTree(this);
    connect(tree, &ProjectTree::currentProjectChanged,
            dd, &ProjectExplorerPluginPrivate::updateContextMenuActions);
    connect(tree, &ProjectTree::currentNodeChanged,
            dd, &ProjectExplorerPluginPrivate::updateContextMenuActions);
    connect(tree, &ProjectTree::currentProjectChanged,
            dd, &ProjectExplorerPluginPrivate::updateActions);
    connect(tree, &ProjectTree::currentProjectChanged, this, [](Project *project) {
        TextEditor::FindInFiles::instance()->setBaseDirectory(project ? project->projectDirectory()
                                                                      : Utils::FileName());
    });

    addAutoReleasedObject(new CustomWizardMetaFactory<CustomProjectWizard>(IWizardFactory::ProjectWizard));
    addAutoReleasedObject(new CustomWizardMetaFactory<CustomWizard>(IWizardFactory::FileWizard));

    // For JsonWizard:
    JsonWizardFactory::registerPageFactory(new FieldPageFactory);
    JsonWizardFactory::registerPageFactory(new FilePageFactory);
    JsonWizardFactory::registerPageFactory(new KitsPageFactory);
    JsonWizardFactory::registerPageFactory(new ProjectPageFactory);
    JsonWizardFactory::registerPageFactory(new SummaryPageFactory);

    JsonWizardFactory::registerGeneratorFactory(new FileGeneratorFactory);
    JsonWizardFactory::registerGeneratorFactory(new ScannerGeneratorFactory);

    dd->m_proWindow = new ProjectWindow;

    Context projecTreeContext(Constants::C_PROJECT_TREE);

    dd->m_projectsMode = new ProjectsMode(dd->m_proWindow);
    dd->m_projectsMode->setEnabled(false);
    addAutoReleasedObject(dd->m_projectsMode);

    addAutoReleasedObject(new CopyTaskHandler);
    addAutoReleasedObject(new ShowInEditorTaskHandler);
    addAutoReleasedObject(new VcsAnnotateTaskHandler);
    addAutoReleasedObject(new RemoveTaskHandler);
    addAutoReleasedObject(new ConfigTaskHandler(Task::compilerMissingTask(),
                                                Constants::KITS_SETTINGS_PAGE_ID));

    ICore::addPreCloseListener([]() -> bool { return coreAboutToClose(); });

    dd->m_outputPane = new AppOutputPane;
    addAutoReleasedObject(dd->m_outputPane);
    connect(SessionManager::instance(), &SessionManager::projectRemoved,
            dd->m_outputPane, &AppOutputPane::projectRemoved);

    addAutoReleasedObject(new AllProjectsFilter);
    addAutoReleasedObject(new CurrentProjectFilter);

    // ProjectPanelFactories
    auto panelFactory = new ProjectPanelFactory;
    panelFactory->setPriority(30);
    panelFactory->setDisplayName(QCoreApplication::translate("EditorSettingsPanelFactory", "Editor"));
    panelFactory->setIcon(":/projectexplorer/images/EditorSettings.png");
    panelFactory->setCreateWidgetFunction([](Project *project) { return new EditorSettingsWidget(project); });
    ProjectPanelFactory::registerFactory(panelFactory);

    panelFactory = new ProjectPanelFactory;
    panelFactory->setPriority(40);
    panelFactory->setDisplayName(QCoreApplication::translate("CodeStyleSettingsPanelFactory", "Code Style"));
    panelFactory->setIcon(":/projectexplorer/images/CodeStyleSettings.png");
    panelFactory->setCreateWidgetFunction([](Project *project) { return new CodeStyleSettingsWidget(project); });
    ProjectPanelFactory::registerFactory(panelFactory);

    panelFactory = new ProjectPanelFactory;
    panelFactory->setPriority(50);
    panelFactory->setDisplayName(QCoreApplication::translate("DependenciesPanelFactory", "Dependencies"));
    panelFactory->setIcon(":/projectexplorer/images/ProjectDependencies.png");
    panelFactory->setCreateWidgetFunction([](Project *project) { return new DependenciesWidget(project); });
    ProjectPanelFactory::registerFactory(panelFactory);

    addAutoReleasedObject(new ProcessStepFactory);

    addAutoReleasedObject(new AllProjectsFind);
    addAutoReleasedObject(new CurrentProjectFind);

    auto constraint = [](RunConfiguration *runConfiguration) {
        const Runnable runnable = runConfiguration->runnable();
        if (!runnable.is<StandardRunnable>())
            return false;
        const IDevice::ConstPtr device = runnable.as<StandardRunnable>().device;
        if (device && device->type() == ProjectExplorer::Constants::DESKTOP_DEVICE_TYPE)
            return true;
        Target *target = runConfiguration->target();
        Kit *kit = target ? target->kit() : nullptr;
        return DeviceTypeKitInformation::deviceTypeId(kit) == ProjectExplorer::Constants::DESKTOP_DEVICE_TYPE;
    };
    RunControl::registerWorker<SimpleTargetRunner>(Constants::NORMAL_RUN_MODE, constraint);

    addAutoReleasedObject(new CustomExecutableRunConfigurationFactory);

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

    // context menu sub menus:
    ActionContainer *folderOpenLocationCtxMenu =
            ActionManager::createMenu(Constants::FOLDER_OPEN_LOCATIONS_CONTEXT_MENU);
    folderOpenLocationCtxMenu->menu()->setTitle(tr("Open..."));
    folderOpenLocationCtxMenu->setOnAllDisabledBehavior(ActionContainer::Show);

    ActionContainer *projectOpenLocationCtxMenu =
            ActionManager::createMenu(Constants::PROJECT_OPEN_LOCATIONS_CONTEXT_MENU);
    projectOpenLocationCtxMenu->menu()->setTitle(tr("Open..."));
    projectOpenLocationCtxMenu->setOnAllDisabledBehavior(ActionContainer::Show);

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
    mprojectContextMenu->appendGroup(Constants::G_FOLDER_LOCATIONS);
    mprojectContextMenu->appendGroup(Constants::G_PROJECT_BUILD);
    mprojectContextMenu->appendGroup(Constants::G_PROJECT_RUN);
    mprojectContextMenu->appendGroup(Constants::G_PROJECT_REBUILD);
    mprojectContextMenu->appendGroup(Constants::G_PROJECT_FILES);
    mprojectContextMenu->appendGroup(Constants::G_PROJECT_LAST);
    mprojectContextMenu->appendGroup(Constants::G_PROJECT_TREE);

    mprojectContextMenu->addMenu(projectOpenLocationCtxMenu, Constants::G_FOLDER_LOCATIONS);
    connect(mprojectContextMenu->menu(), &QMenu::aboutToShow,
            dd, &ProjectExplorerPluginPrivate::updateLocationSubMenus);

    msubProjectContextMenu->appendGroup(Constants::G_PROJECT_FIRST);
    msubProjectContextMenu->appendGroup(Constants::G_FOLDER_LOCATIONS);
    msubProjectContextMenu->appendGroup(Constants::G_PROJECT_BUILD);
    msubProjectContextMenu->appendGroup(Constants::G_PROJECT_RUN);
    msubProjectContextMenu->appendGroup(Constants::G_PROJECT_FILES);
    msubProjectContextMenu->appendGroup(Constants::G_PROJECT_LAST);
    msubProjectContextMenu->appendGroup(Constants::G_PROJECT_TREE);

    connect(msubProjectContextMenu->menu(), &QMenu::aboutToShow,
            dd, &ProjectExplorerPluginPrivate::updateLocationSubMenus);

    ActionContainer *runMenu = ActionManager::createMenu(Constants::RUNMENUCONTEXTMENU);
    runMenu->setOnAllDisabledBehavior(ActionContainer::Hide);
    const QIcon runSideBarIcon = Utils::Icon::sideBarIcon(Icons::RUN, Icons::RUN_FLAT);
    const QIcon runIcon = Utils::Icon::combinedIcon({Utils::Icons::RUN_SMALL.icon(),
                                                     runSideBarIcon});

    runMenu->menu()->setIcon(runIcon);
    runMenu->menu()->setTitle(tr("Run"));
    msubProjectContextMenu->addMenu(runMenu, ProjectExplorer::Constants::G_PROJECT_RUN);

    mfolderContextMenu->appendGroup(Constants::G_FOLDER_LOCATIONS);
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

    mfolderContextMenu->addMenu(folderOpenLocationCtxMenu, Constants::G_FOLDER_LOCATIONS);
    connect(mfolderContextMenu->menu(), &QMenu::aboutToShow,
            dd, &ProjectExplorerPluginPrivate::updateLocationSubMenus);

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
    if (!Utils::HostOsInfo::isMacHost())
        cmd->setDefaultKeySequence(QKeySequence(tr("Ctrl+Shift+O")));
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
    msession->menu()->setTitle(tr("S&essions"));
    msession->setOnAllDisabledBehavior(ActionContainer::Show);
    mfile->addMenu(msession, Core::Constants::G_FILE_OPEN);
    dd->m_sessionMenu = msession->menu();
    connect(mfile->menu(), &QMenu::aboutToShow,
            dd, &ProjectExplorerPluginPrivate::updateSessionMenu);

    // session manager action
    dd->m_sessionManagerAction = new QAction(tr("&Manage..."), this);
    dd->m_sessionMenu->addAction(dd->m_sessionManagerAction);
    dd->m_sessionMenu->addSeparator();
    cmd->setDefaultKeySequence(QKeySequence());


    // unload action
    dd->m_unloadAction = new Utils::ParameterAction(tr("Close Project"), tr("Close Pro&ject \"%1\""),
                                                      Utils::ParameterAction::AlwaysEnabled, this);
    cmd = ActionManager::registerAction(dd->m_unloadAction, Constants::UNLOAD);
    cmd->setAttribute(Command::CA_UpdateText);
    cmd->setDescription(dd->m_unloadAction->text());
    mfile->addAction(cmd, Core::Constants::G_FILE_PROJECT);

    ActionContainer *munload =
        ActionManager::createMenu(Constants::M_UNLOADPROJECTS);
    munload->menu()->setTitle(tr("Close Pro&ject"));
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
    const QIcon sideBarIcon = Utils::Icon::sideBarIcon(Icons::BUILD, Icons::BUILD_FLAT);
    const QIcon buildIcon = Utils::Icon::combinedIcon({Icons::BUILD_SMALL.icon(), sideBarIcon});
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
    dd->m_rebuildSessionAction = new QAction(Icons::REBUILD.icon(), tr("Rebuild All"), this);
    cmd = ActionManager::registerAction(dd->m_rebuildSessionAction, Constants::REBUILDSESSION);
    mbuild->addAction(cmd, Constants::G_BUILD_REBUILD);
    msessionContextMenu->addAction(cmd, Constants::G_SESSION_REBUILD);

    // clean session
    dd->m_cleanSessionAction = new QAction(Utils::Icons::CLEAN.icon(), tr("Clean All"), this);
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
    dd->m_cancelBuildAction = new QAction(Utils::Icons::STOP_SMALL.icon(), tr("Cancel Build"),
                                          this);
    cmd = ActionManager::registerAction(dd->m_cancelBuildAction, Constants::CANCELBUILD);
    cmd->setDefaultKeySequence(QKeySequence(UseMacShortcuts ? tr("Meta+Backspace") : tr("Alt+Backspace")));
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

    // build action with dependencies (context menu)
    dd->m_buildDependenciesActionContextMenu = new QAction(tr("Build"), this);
    cmd = ActionManager::registerAction(dd->m_buildDependenciesActionContextMenu, Constants::BUILDDEPENDCM, projecTreeContext);
    cmd->setAttribute(Command::CA_UpdateText);
    mprojectContextMenu->addAction(cmd, Constants::G_PROJECT_BUILD);

    // build action (context menu)
    dd->m_buildActionContextMenu = new QAction(tr("Build Without Dependencies"), this);
    cmd = ActionManager::registerAction(dd->m_buildActionContextMenu, Constants::BUILDCM, projecTreeContext);
    cmd->setAttribute(Command::CA_UpdateText);
    mprojectContextMenu->addAction(cmd, Constants::G_PROJECT_BUILD);

    // rebuild action with dependencies (context menu)
    dd->m_rebuildDependenciesActionContextMenu = new QAction(tr("Rebuild"), this);
    cmd = ActionManager::registerAction(dd->m_rebuildDependenciesActionContextMenu, Constants::REBUILDDEPENDCM, projecTreeContext);
    cmd->setAttribute(Command::CA_UpdateText);
    mprojectContextMenu->addAction(cmd, Constants::G_PROJECT_REBUILD);

    // rebuild action (context menu)
    dd->m_rebuildActionContextMenu = new QAction(tr("Rebuild Without Dependencies"), this);
    cmd = ActionManager::registerAction(dd->m_rebuildActionContextMenu, Constants::REBUILDCM, projecTreeContext);
    cmd->setAttribute(Command::CA_UpdateText);
    mprojectContextMenu->addAction(cmd, Constants::G_PROJECT_REBUILD);

    // clean action with dependencies (context menu)
    dd->m_cleanDependenciesActionContextMenu = new QAction(tr("Clean"), this);
    cmd = ActionManager::registerAction(dd->m_cleanDependenciesActionContextMenu, Constants::CLEANDEPENDCM, projecTreeContext);
    cmd->setAttribute(Command::CA_UpdateText);
    mprojectContextMenu->addAction(cmd, Constants::G_PROJECT_REBUILD);

    // clean action (context menu)
    dd->m_cleanActionContextMenu = new QAction(tr("Clean Without Dependencies"), this);
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

    // duplicate file action
    dd->m_duplicateFileAction = new QAction(tr("Duplicate File..."), this);
    cmd = ActionManager::registerAction(dd->m_duplicateFileAction, Constants::DUPLICATEFILE,
                       projecTreeContext);
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

    // diff file action
    dd->m_diffFileAction = new QAction(tr("Diff Against Current File"), this);
    cmd = ActionManager::registerAction(dd->m_diffFileAction, Constants::DIFFFILE, projecTreeContext);
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
    connect(dd->m_projectSelectorAction, &QAction::triggered,
            dd->m_targetSelector, &QWidget::show);
    ModeManager::addProjectSelector(dd->m_projectSelectorAction);

    dd->m_projectSelectorActionMenu = new QAction(this);
    dd->m_projectSelectorActionMenu->setEnabled(false);
    dd->m_projectSelectorActionMenu->setText(tr("Open Build and Run Kit Selector..."));
    connect(dd->m_projectSelectorActionMenu, &QAction::triggered,
            dd->m_targetSelector,
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
    connect(EditorManager::instance(), &EditorManager::autoSaved, this, [] {
        if (!dd->m_shuttingDown && !SessionManager::loadingSession())
            SessionManager::save();
    });
    connect(qApp, &QApplication::applicationStateChanged, this, [](Qt::ApplicationState state) {
        if (!dd->m_shuttingDown && state == Qt::ApplicationActive)
            dd->updateWelcomePage();
    });

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
            s->value(QLatin1String("ProjectExplorer/Settings/MaxAppOutputLines"), Core::Constants::DEFAULT_MAX_LINE_COUNT).toInt();
    dd->m_projectExplorerSettings.maxBuildOutputLines =
            s->value(QLatin1String("ProjectExplorer/Settings/MaxBuildOutputLines"), Core::Constants::DEFAULT_MAX_LINE_COUNT).toInt();
    dd->m_projectExplorerSettings.environmentId =
            QUuid(s->value(QLatin1String("ProjectExplorer/Settings/EnvironmentId")).toByteArray());
    if (dd->m_projectExplorerSettings.environmentId.isNull())
        dd->m_projectExplorerSettings.environmentId = QUuid::createUuid();
    int tmp = s->value(QLatin1String("ProjectExplorer/Settings/StopBeforeBuild"),
                            Utils::HostOsInfo::isWindowsHost() ? 1 : 0).toInt();
    if (tmp < 0 || tmp > ProjectExplorerSettings::StopSameBuildDir)
        tmp = Utils::HostOsInfo::isWindowsHost() ? 1 : 0;
    dd->m_projectExplorerSettings.stopBeforeBuild = ProjectExplorerSettings::StopBeforeBuild(tmp);

    auto buildManager = new BuildManager(this, dd->m_cancelBuildAction);
    connect(buildManager, &BuildManager::buildStateChanged,
            dd, &ProjectExplorerPluginPrivate::buildStateChanged);
    connect(buildManager, &BuildManager::buildQueueFinished,
            dd, &ProjectExplorerPluginPrivate::buildQueueFinished, Qt::QueuedConnection);

    connect(dd->m_sessionManagerAction, &QAction::triggered,
            dd, &ProjectExplorerPluginPrivate::showSessionManager);
    connect(dd->m_newAction, &QAction::triggered,
            dd, &ProjectExplorerPlugin::openNewProjectDialog);
    connect(dd->m_loadAction, &QAction::triggered,
            dd, &ProjectExplorerPluginPrivate::loadAction);
    connect(dd->m_buildProjectOnlyAction, &QAction::triggered, dd, [] {
        dd->queue({ SessionManager::startupProject() }, { Id(Constants::BUILDSTEPS_BUILD) });
    });
    connect(dd->m_buildAction, &QAction::triggered, dd, [] {
        dd->queue(SessionManager::projectOrder(SessionManager::startupProject()),
                  { Id(Constants::BUILDSTEPS_BUILD) });
    });
    connect(dd->m_buildActionContextMenu, &QAction::triggered, dd, [] {
        dd->queue({ ProjectTree::currentProject() }, { Id(Constants::BUILDSTEPS_BUILD) });
    });
    connect(dd->m_buildDependenciesActionContextMenu, &QAction::triggered, dd, [] {
        dd->queue(SessionManager::projectOrder(ProjectTree::currentProject()),
                  { Id(Constants::BUILDSTEPS_BUILD) });
    });
    connect(dd->m_buildSessionAction, &QAction::triggered, dd, [] {
        dd->queue(SessionManager::projectOrder(), { Id(Constants::BUILDSTEPS_BUILD) });
    });
    connect(dd->m_rebuildProjectOnlyAction, &QAction::triggered, dd, [] {
        dd->queue({ SessionManager::startupProject() },
                  { Id(Constants::BUILDSTEPS_CLEAN), Id(Constants::BUILDSTEPS_BUILD) });
    });
    connect(dd->m_rebuildAction, &QAction::triggered, dd, [] {
        dd->queue(SessionManager::projectOrder(SessionManager::startupProject()),
                  { Id(Constants::BUILDSTEPS_CLEAN), Id(Constants::BUILDSTEPS_BUILD) });
    });
    connect(dd->m_rebuildActionContextMenu, &QAction::triggered, dd, [] {
        dd->queue({ ProjectTree::currentProject() },
                  { Id(Constants::BUILDSTEPS_CLEAN), Id(Constants::BUILDSTEPS_BUILD) });
    });
    connect(dd->m_rebuildDependenciesActionContextMenu, &QAction::triggered, dd, [] {
        dd->queue(SessionManager::projectOrder(ProjectTree::currentProject()),
                  { Id(Constants::BUILDSTEPS_CLEAN), Id(Constants::BUILDSTEPS_BUILD) });
    });
    connect(dd->m_rebuildSessionAction, &QAction::triggered, dd, [] {
        dd->queue(SessionManager::projectOrder(),
                  { Id(Constants::BUILDSTEPS_CLEAN), Id(Constants::BUILDSTEPS_BUILD) });
    });
    connect(dd->m_deployProjectOnlyAction, &QAction::triggered, dd, [] {
        dd->deploy({ SessionManager::startupProject() });
    });
    connect(dd->m_deployAction, &QAction::triggered, dd, [] {
        dd->deploy(SessionManager::projectOrder(SessionManager::startupProject()));
    });
    connect(dd->m_deployActionContextMenu, &QAction::triggered, dd, [] {
        dd->deploy({ ProjectTree::currentProject() });
    });
    connect(dd->m_deploySessionAction, &QAction::triggered, dd, [] {
        dd->deploy(SessionManager::projectOrder());
    });
    connect(dd->m_cleanProjectOnlyAction, &QAction::triggered, dd, [] {
        dd->queue({ SessionManager::startupProject() }, { Id(Constants::BUILDSTEPS_CLEAN) });
    });
    connect(dd->m_cleanAction, &QAction::triggered, dd, [] {
        dd->queue(SessionManager::projectOrder(SessionManager::startupProject()),
                  { Id(Constants::BUILDSTEPS_CLEAN) });
    });
    connect(dd->m_cleanActionContextMenu, &QAction::triggered, dd, [] {
        dd->queue({ ProjectTree::currentProject() }, { Id(Constants::BUILDSTEPS_CLEAN) });
    });
    connect(dd->m_cleanDependenciesActionContextMenu, &QAction::triggered, dd, [] {
        dd->queue(SessionManager::projectOrder(ProjectTree::currentProject()),
                  { Id(Constants::BUILDSTEPS_CLEAN) });
    });
    connect(dd->m_cleanSessionAction, &QAction::triggered, dd, [] {
        dd->queue(SessionManager::projectOrder(), { Id(Constants::BUILDSTEPS_CLEAN) });
    });
    connect(dd->m_runAction, &QAction::triggered,
            dd, []() { m_instance->runStartupProject(Constants::NORMAL_RUN_MODE); });
    connect(dd->m_runActionContextMenu, &QAction::triggered,
            dd, &ProjectExplorerPluginPrivate::runProjectContextMenu);
    connect(dd->m_runWithoutDeployAction, &QAction::triggered,
            dd, []() { m_instance->runStartupProject(Constants::NORMAL_RUN_MODE, true); });
    connect(dd->m_cancelBuildAction, &QAction::triggered,
            BuildManager::instance(), &BuildManager::cancel);
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
    connect(dd->m_duplicateFileAction, &QAction::triggered,
            dd, &ProjectExplorerPluginPrivate::duplicateFile);
    connect(dd->m_deleteFileAction, &QAction::triggered,
            dd, &ProjectExplorerPluginPrivate::deleteFile);
    connect(dd->m_renameFileAction, &QAction::triggered,
            dd, &ProjectExplorerPluginPrivate::handleRenameFile);
    connect(dd->m_diffFileAction, &QAction::triggered,
            dd, &ProjectExplorerPluginPrivate::handleDiffFile);
    connect(dd->m_setStartupProjectAction, &QAction::triggered,
            dd, &ProjectExplorerPluginPrivate::handleSetStartupProject);
    connect(dd->m_projectTreeCollapseAllAction, &QAction::triggered,
            ProjectTree::instance(), &ProjectTree::collapseAll);

    connect(this, &ProjectExplorerPlugin::updateRunActions,
            dd, &ProjectExplorerPluginPrivate::slotUpdateRunActions);
    connect(this, &ProjectExplorerPlugin::settingsChanged,
            dd, &ProjectExplorerPluginPrivate::updateRunWithoutDeployMenu);

    connect(ICore::instance(), &ICore::newItemDialogStateChanged,
            dd, &ProjectExplorerPluginPrivate::updateContextMenuActions);

    dd->updateWelcomePage();

    // FIXME: These are mostly "legacy"/"convenience" entries, relying on
    // the global entry point ProjectExplorer::currentProject(). They should
    // not be used in the Run/Build configuration pages.
    Utils::MacroExpander *expander = Utils::globalMacroExpander();
    expander->registerFileVariables(Constants::VAR_CURRENTPROJECT_PREFIX,
        tr("Current project's main file."),
        []() -> QString {
            Utils::FileName projectFilePath;
            if (Project *project = ProjectTree::currentProject())
                projectFilePath = project->projectFilePath();
            return projectFilePath.toString();
        });

    expander->registerVariable("CurrentProject:BuildPath",
        tr("Full build path of the current project's active build configuration."),
        []() -> QString {
            BuildConfiguration *bc = activeBuildConfiguration();
            return bc ? bc->buildDirectory().toUserOutput() : QString();
        });

    expander->registerVariable(Constants::VAR_CURRENTPROJECT_NAME,
        tr("The name of the current project."),
        []() -> QString {
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
        tr("The name of the currently active kit as a filesystem-friendly version."),
        []() -> QString {
            Kit *kit = currentKit();
            return kit ? kit->fileSystemFriendlyName() : QString();
        });

    expander->registerVariable(Constants::VAR_CURRENTKIT_ID,
        tr("The ID of the currently active kit."),
        []() -> QString {
            Kit *kit = currentKit();
            return kit ? kit->id().toString() : QString();
        });

    expander->registerVariable("CurrentDevice:HostAddress",
        tr("The host address of the device in the currently active kit."),
        []() -> QString {
            Kit *kit = currentKit();
            const IDevice::ConstPtr device = DeviceKitInformation::device(kit);
            return device ? device->sshParameters().host : QString();
        });

    expander->registerVariable("CurrentDevice:SshPort",
        tr("The SSH port of the device in the currently active kit."),
        []() -> QString {
            Kit *kit = currentKit();
            const IDevice::ConstPtr device = DeviceKitInformation::device(kit);
            return device ? QString::number(device->sshParameters().port) : QString();
        });

    expander->registerVariable("CurrentDevice:UserName",
        tr("The username with which to log into the device in the currently active kit."),
        []() -> QString {
            Kit *kit = currentKit();
            const IDevice::ConstPtr device = DeviceKitInformation::device(kit);
            return device ? device->sshParameters().userName : QString();
        });


    expander->registerVariable("CurrentDevice:PrivateKeyFile",
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

    expander->registerVariable(Constants::VAR_CURRENTRUN_NAME,
        tr("The currently active run configuration's name."),
        []() -> QString {
            if (Target *target = activeTarget()) {
                if (RunConfiguration *rc = target->activeRunConfiguration())
                    return rc->displayName();
            }
            return QString();
        });

    expander->registerFileVariables("CurrentRun:Executable",
        tr("The currently active run configuration's executable (if applicable)."),
        []() -> QString {
            if (Target *target = activeTarget()) {
                if (RunConfiguration *rc = target->activeRunConfiguration()) {
                    if (rc->runnable().is<StandardRunnable>())
                        return rc->runnable().as<StandardRunnable>().executable;
                }
            }
            return QString();
        });

    expander->registerVariable(Constants::VAR_CURRENTBUILD_TYPE,
        tr("The currently active build configuration's type."),
        [&]() -> QString {
            BuildConfiguration *bc = activeBuildConfiguration();
            const BuildConfiguration::BuildType type
                                   = bc ? bc->buildType() : BuildConfiguration::Unknown;
            return BuildConfiguration::buildTypeName(type);
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

    ProjectExplorerPlugin::OpenProjectResult result = ProjectExplorerPlugin::openProject(filename);
    if (!result)
        ProjectExplorerPlugin::showOpenProjectError(result);

    updateActions();
}

void ProjectExplorerPluginPrivate::unloadProjectContextMenu()
{
    if (Project *p = ProjectTree::currentProject())
        ProjectExplorerPlugin::unloadProject(p);
}

void ProjectExplorerPluginPrivate::handleUnloadProject()
{
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
    dd->updateActions();
}

void ProjectExplorerPluginPrivate::closeAllProjects()
{
    if (!EditorManager::closeAllEditors())
        return; // Action has been cancelled

    SessionManager::closeAllProjects();
    updateActions();

    ModeManager::activateMode(Core::Constants::MODE_WELCOME);
}

void ProjectExplorerPlugin::extensionsInitialized()
{
    // Register factories for all project managers
    QStringList allGlobPatterns;

    const QString filterSeparator = QLatin1String(";;");
    QStringList filterStrings;

    auto factory = new IDocumentFactory;
    factory->setOpener([](QString fileName) -> IDocument* {
        const QFileInfo fi(fileName);
        if (fi.isDir())
            fileName = FolderNavigationWidget::projectFilesInDirectory(fi.absoluteFilePath()).value(0, fileName);

        OpenProjectResult result = ProjectExplorerPlugin::openProject(fileName);
        if (!result)
            showOpenProjectError(result);
        return nullptr;
    });

    factory->addMimeType(QStringLiteral("inode/directory"));
    for (const QString &mimeType : dd->m_projectCreators.keys()) {
        factory->addMimeType(mimeType);
        Utils::MimeType mime = Utils::mimeTypeForName(mimeType);
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
}

bool ProjectExplorerPlugin::delayedInitialize()
{
    dd->determineSessionToRestoreAtStartup();
    DeviceManager::instance()->load();
    ToolChainManager::restoreToolChains();
    dd->m_kitManager->restoreKits();
    QTimer::singleShot(0, dd, &ProjectExplorerPluginPrivate::restoreSession); // delay a bit...
    return true;
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
    SessionManager::closeAllProjects();
    dd->m_projectsMode = nullptr;
    dd->m_shuttingDown = true;
    // Attempt to synchronously shutdown all run controls.
    // If that fails, fall back to asynchronous shutdown (Debugger run controls
    // might shutdown asynchronously).
    removeObject(dd->m_welcomePage);
    delete dd->m_welcomePage;
    removeObject(this);

    if (dd->m_activeRunControlCount == 0)
        return SynchronousShutdown;

    dd->m_outputPane->closeTabs(AppOutputPane::CloseTabNoPrompt /* No prompt any more */);
    dd->m_shutdownWatchDogId = dd->startTimer(10 * 1000); // Make sure we shutdown *somehow*
    return AsynchronousShutdown;
}

void ProjectExplorerPlugin::openNewProjectDialog()
{
    if (!ICore::isNewItemDialogRunning()) {
        ICore::showNewItemDialog(tr("New Project", "Title of dialog"),
                                 Utils::filtered(IWizardFactory::allWizardFactories(),
                                 [](IWizardFactory *f) { return !f->supportedProjectTypes().isEmpty(); }));
    } else {
        ICore::raiseWindow(ICore::newItemDialog());
    }
}

void ProjectExplorerPluginPrivate::showSessionManager()
{
    SessionManager::save();
    SessionDialog sessionDialog(ICore::mainWindow());
    sessionDialog.setAutoLoadSession(dd->m_projectExplorerSettings.autorestoreLastSession);
    sessionDialog.exec();
    dd->m_projectExplorerSettings.autorestoreLastSession = sessionDialog.autoLoadSession();

    updateActions();

    if (ModeManager::currentMode() == Core::Constants::MODE_WELCOME)
        updateWelcomePage();
}

void ProjectExplorerPluginPrivate::setStartupProject(Project *project)
{
    if (!project)
        return;
    SessionManager::setStartupProject(project);
    updateActions();
}

void ProjectExplorerPluginPrivate::savePersistentSettings()
{
    if (dd->m_shuttingDown)
        return;

    if (!SessionManager::loadingSession())  {
        for (Project *pro : SessionManager::projects())
            pro->saveSettings();

        SessionManager::save();
    }

    QSettings *s = ICore::settings();
    if (!SessionManager::isDefaultVirgin())
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
    s->setValue(QLatin1String("ProjectExplorer/Settings/MaxBuildOutputLines"), dd->m_projectExplorerSettings.maxBuildOutputLines);
    s->setValue(QLatin1String("ProjectExplorer/Settings/EnvironmentId"), dd->m_projectExplorerSettings.environmentId.toByteArray());
    s->setValue(QLatin1String("ProjectExplorer/Settings/StopBeforeBuild"), dd->m_projectExplorerSettings.stopBeforeBuild);
}

void ProjectExplorerPlugin::openProjectWelcomePage(const QString &fileName)
{
    OpenProjectResult result = openProject(fileName);
    if (!result)
        showOpenProjectError(result);
}

ProjectExplorerPlugin::OpenProjectResult ProjectExplorerPlugin::openProject(const QString &fileName)
{
    OpenProjectResult result = openProjects(QStringList(fileName));
    Project *project = result.project();
    if (!project)
        return result;
    dd->addToRecentProjects(fileName, project->displayName());
    SessionManager::setStartupProject(project);
    project->projectLoaded();
    return result;
}

void ProjectExplorerPlugin::showOpenProjectError(const OpenProjectResult &result)
{
    if (result)
        return;

    // Potentially both errorMessage and alreadyOpen could contain information
    // that should be shown to the user.
    // BUT, if Creator opens only a single project, this can lead
    // to either
    // - No error
    // - A errorMessage
    // - A single project in alreadyOpen

    // The only place where multiple projects are opened is in session restore
    // where the already open case should never happen, thus
    // the following code uses those assumptions to make the code simpler

    QString errorMessage = result.errorMessage();
    if (!errorMessage.isEmpty()) {
        // ignore alreadyOpen
        QMessageBox::critical(ICore::mainWindow(), tr("Failed to Open Project"), errorMessage);
    } else {
        // ignore multiple alreadyOpen
        Project *alreadyOpen = result.alreadyOpen().first();
        ProjectTree::highlightProject(alreadyOpen,
                                      tr("<h3>Project already open</h3>"));
    }
}

static void appendError(QString &errorString, const QString &error)
{
    if (error.isEmpty())
        return;

    if (!errorString.isEmpty())
        errorString.append(QLatin1Char('\n'));
    errorString.append(error);
}

ProjectExplorerPlugin::OpenProjectResult ProjectExplorerPlugin::openProjects(const QStringList &fileNames)
{
    QList<Project*> openedPro;
    QList<Project *> alreadyOpen;
    QString errorString;
    foreach (const QString &fileName, fileNames) {
        QTC_ASSERT(!fileName.isEmpty(), continue);

        const QFileInfo fi(fileName);
        const QString filePath = fi.absoluteFilePath();
        bool found = false;
        for (Project *pi : SessionManager::projects()) {
            if (filePath == pi->projectFilePath().toString()) {
                alreadyOpen.append(pi);
                found = true;
                break;
            }
        }
        if (found) {
            SessionManager::reportProjectLoadingProgress();
            continue;
        }

        Utils::MimeType mt = Utils::mimeTypeForFile(fileName);
        if (mt.isValid()) {
            if (ProjectManager::canOpenProjectForMimeType(mt)) {
                if (!QFileInfo(filePath).isFile()) {
                    appendError(errorString,
                                tr("Failed opening project \"%1\": Project is not a file.").arg(fileName));
                } else if (Project *pro = ProjectManager::openProject(mt, Utils::FileName::fromString(filePath))) {
                    QObject::connect(pro, &Project::parsingFinished, [pro]() {
                        emit SessionManager::instance()->projectFinishedParsing(pro);
                    });
                    QString restoreError;
                    Project::RestoreResult restoreResult = pro->restoreSettings(&restoreError);
                    if (restoreResult == Project::RestoreResult::Ok) {
                        connect(pro, &Project::fileListChanged,
                                m_instance, &ProjectExplorerPlugin::fileListChanged);
                        SessionManager::addProject(pro);
                        openedPro += pro;
                    } else {
                        if (restoreResult == Project::RestoreResult::Error)
                            appendError(errorString, restoreError);
                        delete pro;
                    }
                }
            } else {
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
    dd->updateActions();

    bool switchToProjectsMode = Utils::anyOf(openedPro, &Project::needsConfiguration);

    if (!openedPro.isEmpty()) {
        if (switchToProjectsMode)
            ModeManager::activateMode(Constants::MODE_SESSION);
        else
            ModeManager::activateMode(Core::Constants::MODE_EDIT);
        ModeManager::setFocusToCurrentMode();
    }

    return OpenProjectResult(openedPro, alreadyOpen, errorString);
}

void ProjectExplorerPluginPrivate::updateWelcomePage()
{
    m_welcomePage->reloadWelcomeScreenData();
}

void ProjectExplorerPluginPrivate::currentModeChanged(Id mode, Id oldMode)
{
    if (oldMode == Constants::MODE_SESSION)
        ICore::saveSettings();
    if (mode == Core::Constants::MODE_WELCOME)
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
    for (const QString &mt : dd->m_projectCreators.keys()) {
        Utils::MimeType mimeType = Utils::mimeTypeForName(mt);
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

QThreadPool *ProjectExplorerPlugin::sharedThreadPool()
{
    return &(dd->m_threadPool);
}

/*!
    This function is connected to the ICore::coreOpened signal.  If
    there was no session explicitly loaded, it creates an empty new
    default session and puts the list of recent projects and sessions
    onto the welcome page.
*/
void ProjectExplorerPluginPrivate::restoreSession()
{
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
                }
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
    connect(dd->m_welcomePage, &ProjectWelcomePage::requestProject,
            m_instance, &ProjectExplorerPlugin::openProjectWelcomePage);
    dd->m_arguments = arguments;
    // delay opening projects from the command line even more
    QTimer::singleShot(0, m_instance, []() {
        ICore::openFiles(dd->m_arguments, ICore::OpenFilesFlags(ICore::CanContainLineAndColumnNumbers | ICore::SwitchMode));
        m_instance->finishedInitialization();
    });
    updateActions();
}

void ProjectExplorerPluginPrivate::buildStateChanged(Project * pro)
{
    Q_UNUSED(pro)
    updateActions();
}

void ProjectExplorerPluginPrivate::executeRunConfiguration(RunConfiguration *runConfiguration, Core::Id runMode)
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

    RunControl::WorkerCreator producer = RunControl::producer(runConfiguration, runMode);

    QTC_ASSERT(producer, return);
    auto runControl = new RunControl(runConfiguration, runMode);

    // A user needed interaction may have cancelled the run
    // (by example asking for a process pid or server url).
    if (!producer(runControl)) {
        delete runControl;
        return;
    }

    emit m_instance->aboutToExecuteProject(runConfiguration->target()->project(), runMode);

    startRunControl(runControl);
}

void ProjectExplorerPlugin::showRunErrorMessage(const QString &errorMessage)
{
    // Empty, non-null means 'canceled' (custom executable dialog for libraries), whereas
    // empty, null means an error occurred, but message was not set
    if (!errorMessage.isEmpty() || errorMessage.isNull())
        QMessageBox::critical(ICore::mainWindow(), errorMessage.isNull() ? tr("Unknown error") : tr("Could Not Run"), errorMessage);
}

void ProjectExplorerPlugin::startRunControl(RunControl *runControl)
{
    dd->startRunControl(runControl);
}

void ProjectExplorerPluginPrivate::startRunControl(RunControl *runControl)
{
    m_outputPane->createNewOutputWindow(runControl);
    m_outputPane->flash(); // one flash for starting
    m_outputPane->showTabFor(runControl);
    Core::Id runMode = runControl->runMode();
    bool popup = (runMode == Constants::NORMAL_RUN_MODE && dd->m_projectExplorerSettings.showRunOutput)
            || (runMode == Constants::DEBUG_RUN_MODE && m_projectExplorerSettings.showDebugOutput);
    m_outputPane->setBehaviorOnOutput(runControl, popup ? AppOutputPane::Popup : AppOutputPane::Flash);
    connect(runControl, &QObject::destroyed, this, &ProjectExplorerPluginPrivate::checkForShutdown,
            Qt::QueuedConnection);
    ++m_activeRunControlCount;
    runControl->initiateStart();
    emit m_instance->updateRunActions();
}

void ProjectExplorerPluginPrivate::checkForShutdown()
{
    --m_activeRunControlCount;
    QTC_ASSERT(m_activeRunControlCount >= 0, m_activeRunControlCount = 0);
    if (m_shuttingDown && m_activeRunControlCount == 0)
        m_instance->asynchronousShutdownFinished();
}

void ProjectExplorerPluginPrivate::timerEvent(QTimerEvent *ev)
{
    if (m_shutdownWatchDogId == ev->timerId())
        m_instance->asynchronousShutdownFinished();
}

void ProjectExplorerPlugin::initiateInlineRenaming()
{
    dd->handleRenameFile();
}

void ProjectExplorerPluginPrivate::buildQueueFinished(bool success)
{
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
    m_delayedRunConfiguration = nullptr;
    m_shouldHaveRunConfiguration = false;
    m_runMode = Constants::NO_RUN_MODE;
}

void ProjectExplorerPluginPrivate::runConfigurationConfigurationFinished()
{
    auto rc = qobject_cast<RunConfiguration *>(sender());
    Core::Id runMode = Constants::NO_RUN_MODE;
    for (int i = 0; i < m_delayedRunConfigurationForRun.size(); ++i) {
        if (m_delayedRunConfigurationForRun.at(i).first == rc) {
            runMode = m_delayedRunConfigurationForRun.at(i).second;
            m_delayedRunConfigurationForRun.removeAt(i);
            break;
        }
    }
    if (runMode != Constants::NO_RUN_MODE && rc->isConfigured())
        executeRunConfiguration(rc, runMode);
}

QList<QPair<QString, QString> > ProjectExplorerPluginPrivate::recentProjects() const
{
    return Utils::filtered(dd->m_recentProjects, [](const QPair<QString, QString> &p) {
        return QFileInfo(p.first).isFile();
    });
}

static QString pathOrDirectoryFor(const Node *node, bool dir)
{
    Utils::FileName path = node->filePath();
    QString location;
    const FolderNode *folder = node->asFolderNode();
    if (node->nodeType() == NodeType::VirtualFolder && folder) {
        // Virtual Folder case
        // If there are files directly below or no subfolders, take the folder path
        if (!folder->fileNodes().isEmpty() || folder->folderNodes().isEmpty()) {
            location = path.toString();
        } else {
            // Otherwise we figure out a commonPath from the subfolders
            QStringList list;
            foreach (FolderNode *f, folder->folderNodes())
                list << f->filePath().toString() + QLatin1Char('/');
            location = Utils::commonPath(list);
        }

        QFileInfo fi(location);
        while ((!fi.exists() || !fi.isDir()) && !fi.isRoot())
            fi.setFile(fi.absolutePath());
        location = fi.absoluteFilePath();
    } else if (!path.isEmpty()) {
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

static QString pathFor(const Node *node)
{
    return pathOrDirectoryFor(node, false);
}

static QString directoryFor(const Node *node)
{
    return pathOrDirectoryFor(node, true);
}

QString ProjectExplorerPlugin::directoryFor(Node *node)
{
    return ProjectExplorer::directoryFor(node);
}

void ProjectExplorerPluginPrivate::updateActions()
{
    const Project *const project = SessionManager::startupProject();
    const Project *const currentProject = ProjectTree::currentProject(); // for context menu actions

    const QPair<bool, QString> buildActionState = buildSettingsEnabled(project);
    const QPair<bool, QString> buildActionContextState = buildSettingsEnabled(currentProject);
    const QPair<bool, QString> buildSessionState = buildSettingsEnabledForSession();

    const QString projectName = project ? project->displayName() : QString();
    const QString projectNameContextMenu = currentProject ? currentProject->displayName() : QString();

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
    m_setStartupProjectAction->setVisible(currentProject != project);

    const bool hasDependencies = SessionManager::projectOrder(currentProject).size() > 1;
    m_buildActionContextMenu->setVisible(hasDependencies);
    m_rebuildActionContextMenu->setVisible(hasDependencies);
    m_cleanActionContextMenu->setVisible(hasDependencies);

    m_buildActionContextMenu->setEnabled(buildActionContextState.first);
    m_rebuildActionContextMenu->setEnabled(buildActionContextState.first);
    m_cleanActionContextMenu->setEnabled(buildActionContextState.first);

    m_buildDependenciesActionContextMenu->setEnabled(buildActionContextState.first);
    m_rebuildDependenciesActionContextMenu->setEnabled(buildActionContextState.first);
    m_cleanDependenciesActionContextMenu->setEnabled(buildActionContextState.first);

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
    if (!m_instance->saveModifiedFiles())
        return -1;

    if (m_projectExplorerSettings.stopBeforeBuild != ProjectExplorerSettings::StopNone
            && stepIds.contains(Constants::BUILDSTEPS_BUILD)) {
        ProjectExplorerSettings::StopBeforeBuild stopCondition = m_projectExplorerSettings.stopBeforeBuild;
        const QList<RunControl *> toStop
                = Utils::filtered(m_outputPane->allRunControls(), [&projects, stopCondition](RunControl *rc) -> bool {
                                      if (!rc->isRunning())
                                          return false;

                                      switch (stopCondition) {
                                      case ProjectExplorerSettings::StopNone:
                                          return false;
                                      case ProjectExplorerSettings::StopAll:
                                          return true;
                                      case ProjectExplorerSettings::StopSameProject:
                                          return projects.contains(rc->project());
                                      case ProjectExplorerSettings::StopSameBuildDir:
                                          return Utils::contains(projects, [rc](Project *p) {
                                              Target *t = p ? p->activeTarget() : nullptr;
                                              BuildConfiguration *bc = t ? t->activeBuildConfiguration() : nullptr;
                                              if (!bc)
                                                  return false;
                                              if (!rc->runnable().is<StandardRunnable>())
                                                  return false;
                                              if (!Utils::FileName::fromString(rc->runnable().as<StandardRunnable>().executable).isChildOf(bc->buildDirectory()))
                                                  return false;
                                              IDevice::ConstPtr device = rc->runnable().as<StandardRunnable>().device;
                                              if (device.isNull())
                                                  device = DeviceKitInformation::device(t->kit());
                                              return !device.isNull() && device->type() == Core::Id(Constants::DESKTOP_DEVICE_TYPE);
                                          });
                                      }
                                      return false; // Can't get here!
                                  });

        if (!toStop.isEmpty()) {
            bool stopThem = true;
            if (m_projectExplorerSettings.prompToStopRunControl) {
                QStringList names = Utils::transform(toStop, &RunControl::displayName);
                if (QMessageBox::question(ICore::mainWindow(), tr("Stop Applications"),
                                          tr("Stop these applications before building?")
                                          + QLatin1String("\n\n")
                                          + names.join(QLatin1Char('\n')))
                        == QMessageBox::No) {
                    stopThem = false;
                }
            }

            if (stopThem) {
                foreach (RunControl *rc, toStop)
                    rc->initiateStop();

                WaitForStopDialog dialog(toStop);
                dialog.exec();

                if (dialog.canceled())
                    return -1;
            }
        }
    }

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
            BuildStepList *bsl = nullptr;
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

void ProjectExplorerPlugin::buildProject(Project *p)
{
    dd->queue(SessionManager::projectOrder(p), { Id(Constants::BUILDSTEPS_BUILD) });
}

void ProjectExplorerPluginPrivate::runProjectContextMenu()
{
    const Node *node = ProjectTree::findCurrentNode();
    const ProjectNode *projectNode = node ? node->asProjectNode() : nullptr;
    if (projectNode == ProjectTree::currentProject()->rootProjectNode() || !projectNode) {
        m_instance->runProject(ProjectTree::currentProject(), Constants::NORMAL_RUN_MODE);
    } else {
        auto act = qobject_cast<QAction *>(sender());
        if (!act)
            return;
        RunConfiguration *rc = act->data().value<RunConfiguration *>();
        if (!rc)
            return;
        m_instance->runRunConfiguration(rc, Constants::NORMAL_RUN_MODE);
    }
}

static bool hasBuildSettings(const Project *pro)
{
    return Utils::anyOf(SessionManager::projectOrder(pro), [](const Project *project) {
        return project
                && project->activeTarget()
                && project->activeTarget()->activeBuildConfiguration();
    });
}

QPair<bool, QString> ProjectExplorerPluginPrivate::buildSettingsEnabled(const Project *pro)
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
    } else if (!hasBuildSettings(nullptr)) {
        result.first = false;
        result.second = tr("Project has no build settings.");
    } else {
        foreach (Project *project, SessionManager::projectOrder(nullptr)) {
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
        box.setWindowTitle(tr("Close %1?").arg(Core::Constants::IDE_DISPLAY_NAME));
        box.setText(tr("A project is currently being built."));
        box.setInformativeText(tr("Do you want to cancel the build process and close %1 anyway?")
                               .arg(Core::Constants::IDE_DISPLAY_NAME));
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

void ProjectExplorerPlugin::runProject(Project *pro, Core::Id mode, const bool forceSkipDeploy)
{
    if (!pro)
        return;

    if (Target *target = pro->activeTarget())
        if (RunConfiguration *rc = target->activeRunConfiguration())
            runRunConfiguration(rc, mode, forceSkipDeploy);
}

void ProjectExplorerPlugin::runStartupProject(Core::Id runMode, bool forceSkipDeploy)
{
    runProject(SessionManager::startupProject(), runMode, forceSkipDeploy);
}

void ProjectExplorerPlugin::runRunConfiguration(RunConfiguration *rc,
                                                Core::Id runMode,
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

QList<QPair<Runnable, Utils::ProcessHandle>> ProjectExplorerPlugin::runningRunControlProcesses()
{
    QList<QPair<Runnable, Utils::ProcessHandle>> processes;
    foreach (RunControl *rc, dd->m_outputPane->allRunControls()) {
        if (rc->isRunning())
            processes << qMakePair(rc->runnable(), rc->applicationProcessHandle());
    }
    return processes;
}

void ProjectExplorerPluginPrivate::projectAdded(Project *pro)
{
    if (m_projectsMode)
        m_projectsMode->setEnabled(true);
    // more specific action en and disabling ?
    pro->subscribeSignal(&BuildConfiguration::enabledChanged, this, [this]() {
        auto bc = qobject_cast<BuildConfiguration *>(sender());
        if (bc && bc->isActive() && bc->project() == SessionManager::startupProject()) {
            updateActions();
            emit m_instance->updateRunActions();
        }
    });
    pro->subscribeSignal(&RunConfiguration::requestRunActionsUpdate, this, [this]() {
        auto rc = qobject_cast<RunConfiguration *>(sender());
        if (rc && rc->isActive() && rc->project() == SessionManager::startupProject())
            emit m_instance->updateRunActions();
    });
}

void ProjectExplorerPluginPrivate::projectRemoved(Project *pro)
{
    Q_UNUSED(pro);
    if (m_projectsMode)
        m_projectsMode->setEnabled(SessionManager::hasProjects());
}

void ProjectExplorerPluginPrivate::projectDisplayNameChanged(Project *pro)
{
    addToRecentProjects(pro->projectFilePath().toString(), pro->displayName());
    updateActions();
}

void ProjectExplorerPluginPrivate::startupProjectChanged()
{
    static QPointer<Project> previousStartupProject = nullptr;
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
    static QPointer<Target> previousTarget = nullptr;
    Target *target = nullptr;
    Project *startupProject = SessionManager::startupProject();
    if (startupProject)
        target = startupProject->activeTarget();
    if (target == previousTarget)
        return;

    if (previousTarget) {
        disconnect(previousTarget.data(), &Target::activeRunConfigurationChanged,
                   this, &ProjectExplorerPluginPrivate::activeRunConfigurationChanged);
        disconnect(previousTarget.data(), &Target::activeBuildConfigurationChanged,
                   this, &ProjectExplorerPluginPrivate::activeBuildConfigurationChanged);
    }
    previousTarget = target;
    if (target) {
        connect(target, &Target::activeRunConfigurationChanged,
                this, &ProjectExplorerPluginPrivate::activeRunConfigurationChanged);
        connect(previousTarget.data(), &Target::activeBuildConfigurationChanged,
                this, &ProjectExplorerPluginPrivate::activeBuildConfigurationChanged);
    }

    activeBuildConfigurationChanged();
    activeRunConfigurationChanged();
    updateDeployActions();
}

void ProjectExplorerPluginPrivate::activeRunConfigurationChanged()
{
    static QPointer<RunConfiguration> previousRunConfiguration = nullptr;
    RunConfiguration *rc = nullptr;
    Project *startupProject = SessionManager::startupProject();
    if (startupProject && startupProject->activeTarget())
        rc = startupProject->activeTarget()->activeRunConfiguration();
    if (rc == previousRunConfiguration)
        return;
    emit m_instance->updateRunActions();
}

void ProjectExplorerPluginPrivate::activeBuildConfigurationChanged()
{
    static QPointer<BuildConfiguration> previousBuildConfiguration = nullptr;
    BuildConfiguration *bc = nullptr;
    Project *startupProject = SessionManager::startupProject();
    if (startupProject && startupProject->activeTarget())
        bc = startupProject->activeTarget()->activeBuildConfiguration();
    if (bc == previousBuildConfiguration)
        return;
    updateActions();
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

        if (Utils::anyOf(SessionManager::projectOrder(nullptr), hasDisabledBuildConfiguration))
            enableDeploySessionAction = false;
    }
    if (!hasProjects || !hasDeploySettings(nullptr) || BuildManager::isBuilding())
        enableDeploySessionAction = false;
    m_deploySessionAction->setEnabled(enableDeploySessionAction);

    emit m_instance->updateRunActions();
}

bool ProjectExplorerPlugin::canRunStartupProject(Core::Id runMode, QString *whyNot)
{
    Project *project = SessionManager::startupProject();
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
    RunControl::WorkerCreator producer = RunControl::producer(activeRC, runMode);
    if (!producer) {
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
    QString whyNot;
    const bool state = ProjectExplorerPlugin::canRunStartupProject(Constants::NORMAL_RUN_MODE, &whyNot);
    m_runAction->setEnabled(state);
    m_runAction->setToolTip(whyNot);
    m_runWithoutDeployAction->setEnabled(state);
}

void ProjectExplorerPluginPrivate::addToRecentProjects(const QString &fileName, const QString &displayName)
{
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
    for (Project *project : SessionManager::projects()) {
        QAction *action = menu->addAction(tr("Close Project \"%1\"").arg(project->displayName()));
        connect(action, &QAction::triggered,
                [project] { ProjectExplorerPlugin::unloadProject(project); } );
    }
}

void ProjectExplorerPluginPrivate::updateRecentProjectMenu()
{
    typedef QList<QPair<QString, QString> >::const_iterator StringPairListConstIterator;
    ActionContainer *aci = ActionManager::actionContainer(Constants::M_RECENTPROJECTS);
    QMenu *menu = aci->menu();
    menu->clear();

    int acceleratorKey = 1;
    auto projects = recentProjects();
    //projects (ignore sessions, they used to be in this list)
    const StringPairListConstIterator end = projects.constEnd();
    for (StringPairListConstIterator it = projects.constBegin(); it != end; ++it, ++acceleratorKey) {
        const QString fileName = it->first;
        if (fileName.endsWith(QLatin1String(".qws")))
            continue;

        const QString actionText = ActionManager::withNumberAccelerator(
                    Utils::withTildeHomePath(fileName), acceleratorKey);
        QAction *action = menu->addAction(actionText);
        connect(action, &QAction::triggered, this, [this, fileName] {
            openRecentProject(fileName);
        });
    }
    const bool hasRecentProjects = !projects.empty();
    menu->setEnabled(hasRecentProjects);

    // add the Clear Menu item
    if (hasRecentProjects) {
        menu->addSeparator();
        QAction *action = menu->addAction(QCoreApplication::translate(
                                          "Core", Core::Constants::TR_CLEAR_MENU));
        connect(action, &QAction::triggered,
                this, &ProjectExplorerPluginPrivate::clearRecentProjects);
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
    if (!fileName.isEmpty()) {
        ProjectExplorerPlugin::OpenProjectResult result
                = ProjectExplorerPlugin::openProject(fileName);
        if (!result)
            ProjectExplorerPlugin::showOpenProjectError(result);
    }
}

void ProjectExplorerPluginPrivate::invalidateProject(Project *project)
{
    disconnect(project, &Project::fileListChanged,
               m_instance, &ProjectExplorerPlugin::fileListChanged);
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
    m_duplicateFileAction->setEnabled(false);
    m_deleteFileAction->setEnabled(false);
    m_renameFileAction->setEnabled(false);
    m_diffFileAction->setEnabled(false);

    m_addExistingFilesAction->setVisible(true);
    m_addExistingDirectoryAction->setVisible(true);
    m_addNewFileAction->setVisible(true);
    m_addNewSubprojectAction->setVisible(true);
    m_removeProjectAction->setVisible(true);
    m_removeFileAction->setVisible(true);
    m_duplicateFileAction->setVisible(false);
    m_deleteFileAction->setVisible(true);
    m_runActionContextMenu->setVisible(false);
    m_diffFileAction->setVisible(isDiffServiceAvailable());

    m_openTerminalHere->setVisible(true);
    m_showInGraphicalShell->setVisible(true);
    m_searchOnFileSystem->setVisible(true);

    ActionContainer *runMenu = ActionManager::actionContainer(Constants::RUNMENUCONTEXTMENU);
    runMenu->menu()->clear();
    runMenu->menu()->menuAction()->setVisible(false);

    const Node *currentNode = ProjectTree::findCurrentNode();

    if (currentNode && currentNode->managingProject()) {
        ProjectNode *pn;
        if (const ContainerNode *cn = currentNode->asContainerNode())
            pn = cn->rootProjectNode();
        else
            pn = const_cast<ProjectNode*>(currentNode->asProjectNode());

        if (pn) {
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

        auto supports = [currentNode](ProjectAction action) {
            return currentNode->supportsAction(action, currentNode);
        };

        if (currentNode->asFolderNode()) {
            // Also handles ProjectNode
            m_addNewFileAction->setEnabled(currentNode->supportsAction(AddNewFile, currentNode)
                                              && !ICore::isNewItemDialogRunning());
            m_addNewSubprojectAction->setEnabled(currentNode->nodeType() == NodeType::Project
                                                    && supports(AddSubProject)
                                                    && !ICore::isNewItemDialogRunning());
            m_removeProjectAction->setEnabled(currentNode->nodeType() == NodeType::Project
                                                    && supports(RemoveSubProject));
            m_addExistingFilesAction->setEnabled(supports(AddExistingFile));
            m_addExistingDirectoryAction->setEnabled(supports(AddExistingDirectory));
            m_renameFileAction->setEnabled(supports(Rename));
        } else if (currentNode->asFileNode()) {
            // Enable and show remove / delete in magic ways:
            // If both are disabled show Remove
            // If both are enabled show both (can't happen atm)
            // If only removeFile is enabled only show it
            // If only deleteFile is enable only show it
            bool enableRemove = supports(RemoveFile);
            m_removeFileAction->setEnabled(enableRemove);
            bool enableDelete = supports(EraseFile);
            m_deleteFileAction->setEnabled(enableDelete);
            m_deleteFileAction->setVisible(enableDelete);

            m_removeFileAction->setVisible(!enableDelete || enableRemove);
            m_renameFileAction->setEnabled(supports(Rename));
            const bool currentNodeIsTextFile = isTextFile(
                        currentNode->filePath().toString());
            m_diffFileAction->setEnabled(isDiffServiceAvailable()
                        && currentNodeIsTextFile && TextEditor::TextDocument::currentTextDocument());

            m_duplicateFileAction->setVisible(supports(DuplicateFile));
            m_duplicateFileAction->setEnabled(supports(DuplicateFile));

            EditorManager::populateOpenWithMenu(m_openWithMenu,
                                                currentNode->filePath().toString());
        }

        if (supports(HidePathActions)) {
            m_openTerminalHere->setVisible(false);
            m_showInGraphicalShell->setVisible(false);
            m_searchOnFileSystem->setVisible(false);
        }

        if (supports(HideFileActions)) {
            m_deleteFileAction->setVisible(false);
            m_removeFileAction->setVisible(false);
        }

        if (supports(HideFolderActions)) {
            m_addNewFileAction->setVisible(false);
            m_addNewSubprojectAction->setVisible(false);
            m_removeProjectAction->setVisible(false);
            m_addExistingFilesAction->setVisible(false);
            m_addExistingDirectoryAction->setVisible(false);
        }
    }
}

void ProjectExplorerPluginPrivate::updateLocationSubMenus()
{
    static QList<QAction *> actions;
    qDeleteAll(actions); // This will also remove these actions from the menus!
    actions.clear();

    ActionContainer *projectMenuContainer
            = ActionManager::actionContainer(Constants::PROJECT_OPEN_LOCATIONS_CONTEXT_MENU);
    QMenu *projectMenu = projectMenuContainer->menu();
    QTC_CHECK(projectMenu->actions().isEmpty());

    ActionContainer *folderMenuContainer
            = ActionManager::actionContainer(Constants::FOLDER_OPEN_LOCATIONS_CONTEXT_MENU);
    QMenu *folderMenu = folderMenuContainer->menu();
    QTC_CHECK(folderMenu->actions().isEmpty());

    const FolderNode *const fn
            = ProjectTree::findCurrentNode() ? ProjectTree::findCurrentNode()->asFolderNode() : nullptr;
    const QList<FolderNode::LocationInfo> locations
            = fn ? fn->locationInfo() : QList<FolderNode::LocationInfo>();

    const bool isVisible = !locations.isEmpty();
    projectMenu->menuAction()->setVisible(isVisible);
    folderMenu->menuAction()->setVisible(isVisible);

    if (!isVisible)
        return;

    for (const FolderNode::LocationInfo &li : locations) {
        const int line = li.line;
        const Utils::FileName path = li.path;
        QAction *action = new QAction(li.displayName, nullptr);
        connect(action, &QAction::triggered, this, [line, path]() {
            Core::EditorManager::openEditorAt(path.toString(), line);
        });

        projectMenu->addAction(action);
        folderMenu->addAction(action);

        actions.append(action);
    }
}

void ProjectExplorerPluginPrivate::addNewFile()
{
    Node* currentNode = ProjectTree::findCurrentNode();
    QTC_ASSERT(currentNode, return);
    QString location = directoryFor(currentNode);

    QVariantMap map;
    // store void pointer to avoid QVariant to use qobject_cast, which might core-dump when trying
    // to access meta data on an object that get deleted in the meantime:
    map.insert(QLatin1String(Constants::PREFERRED_PROJECT_NODE), QVariant::fromValue(static_cast<void *>(currentNode)));
    map.insert(Constants::PREFERRED_PROJECT_NODE_PATH, currentNode->filePath().toString());
    if (Project *p = ProjectTree::currentProject()) {
        QList<Id> profileIds = Utils::transform(p->targets(), &Target::id);
        map.insert(QLatin1String(Constants::PROJECT_KIT_IDS), QVariant::fromValue(profileIds));
        map.insert(Constants::PROJECT_POINTER, QVariant::fromValue(static_cast<void *>(p)));
    }
    ICore::showNewItemDialog(tr("New File", "Title of dialog"),
                             Utils::filtered(IWizardFactory::allWizardFactories(),
                                             [](IWizardFactory *f) {
                                 return f->supportedProjectTypes().isEmpty();
                             }),
                             location, map);
}

void ProjectExplorerPluginPrivate::addNewSubproject()
{
    Node* currentNode = ProjectTree::findCurrentNode();
    QTC_ASSERT(currentNode, return);
    QString location = directoryFor(currentNode);

    if (currentNode->nodeType() == NodeType::Project
            && currentNode->supportsAction(AddSubProject, currentNode)) {
        QVariantMap map;
        map.insert(QLatin1String(Constants::PREFERRED_PROJECT_NODE), QVariant::fromValue(currentNode));
        Project *project = ProjectTree::currentProject();
        Core::Id projectType;
        if (project) {
            QList<Id> profileIds = Utils::transform(ProjectTree::currentProject()->targets(), &Target::id);
            map.insert(QLatin1String(Constants::PROJECT_KIT_IDS), QVariant::fromValue(profileIds));
            projectType = project->id();
        }

        ICore::showNewItemDialog(tr("New Subproject", "Title of dialog"),
                                 Utils::filtered(IWizardFactory::allWizardFactories(),
                                                 [projectType](IWizardFactory *f) {
                                                     return projectType.isValid() ? f->supportedProjectTypes().contains(projectType)
                                                                                  : !f->supportedProjectTypes().isEmpty(); }),
                                 location, map);
    }
}

void ProjectExplorerPluginPrivate::handleAddExistingFiles()
{
    Node *node = ProjectTree::findCurrentNode();
    FolderNode *folderNode = node ? node->asFolderNode() : nullptr;

    QTC_ASSERT(folderNode, return);

    QStringList fileNames = QFileDialog::getOpenFileNames(ICore::mainWindow(),
        tr("Add Existing Files"), directoryFor(node));
    if (fileNames.isEmpty())
        return;

    ProjectExplorerPlugin::addExistingFiles(folderNode, fileNames);
}

void ProjectExplorerPluginPrivate::addExistingDirectory()
{
    Node *node = ProjectTree::findCurrentNode();
    FolderNode *folderNode = node ? node->asFolderNode() : nullptr;

    QTC_ASSERT(folderNode, return);

    SelectableFilesDialogAddDirectory dialog(Utils::FileName::fromString(directoryFor(node)),
                                             Utils::FileNameList(), ICore::mainWindow());
    dialog.setAddFileFilter(folderNode->addFileFilter());

    if (dialog.exec() == QDialog::Accepted)
        ProjectExplorerPlugin::addExistingFiles(folderNode, Utils::transform(dialog.selectedFiles(), &Utils::FileName::toString));
}

void ProjectExplorerPlugin::addExistingFiles(FolderNode *folderNode, const QStringList &filePaths)
{
    // can happen when project is not yet parsed or finished parsing while the dialog was open:
    if (!folderNode || !ProjectTree::hasNode(folderNode))
        return;

    const QString dir = directoryFor(folderNode);
    QStringList fileNames = filePaths;
    QStringList notAdded;
    folderNode->addFiles(fileNames, &notAdded);

    if (!notAdded.isEmpty()) {
        const QString message = tr("Could not add following files to project %1:")
                .arg(folderNode->managingProject()->displayName()) + QLatin1Char('\n');
        const QStringList nativeFiles
                = Utils::transform(notAdded,
                                   [](const QString &f) { return QDir::toNativeSeparators(f); });
        QMessageBox::warning(ICore::mainWindow(), tr("Adding Files to Project Failed"),
                             message + nativeFiles.join(QLatin1Char('\n')));
        fileNames = Utils::filtered(fileNames,
                                    [&notAdded](const QString &f) { return !notAdded.contains(f); });
    }

    VcsManager::promptToAdd(dir, fileNames);
}

void ProjectExplorerPluginPrivate::removeProject()
{
    Node *node = ProjectTree::findCurrentNode();
    if (!node)
        return;
    ProjectNode *subProjectNode = node->managingProject();
    if (!subProjectNode)
        return;
    ProjectNode *projectNode = subProjectNode->managingProject();
    if (projectNode) {
        RemoveFileDialog removeFileDialog(subProjectNode->filePath().toString(), ICore::mainWindow());
        removeFileDialog.setDeleteFileVisible(false);
        if (removeFileDialog.exec() == QDialog::Accepted)
            projectNode->removeSubProject(subProjectNode->filePath().toString());
    }
}

void ProjectExplorerPluginPrivate::openFile()
{
    const Node *currentNode = ProjectTree::findCurrentNode();
    QTC_ASSERT(currentNode, return);
    EditorManager::openEditor(currentNode->filePath().toString());
}

void ProjectExplorerPluginPrivate::searchOnFileSystem()
{
    const Node *currentNode = ProjectTree::findCurrentNode();
    QTC_ASSERT(currentNode, return);
    TextEditor::FindInFiles::findOnFileSystem(pathFor(currentNode));
}

void ProjectExplorerPluginPrivate::showInGraphicalShell()
{
    Node *currentNode = ProjectTree::findCurrentNode();
    QTC_ASSERT(currentNode, return);
    FileUtils::showInGraphicalShell(ICore::mainWindow(), pathFor(currentNode));
}

void ProjectExplorerPluginPrivate::openTerminalHere()
{
    const Node *currentNode = ProjectTree::findCurrentNode();
    QTC_ASSERT(currentNode, return);
    FileUtils::openTerminal(directoryFor(currentNode));
}

void ProjectExplorerPluginPrivate::removeFile()
{
    const Node *currentNode = ProjectTree::findCurrentNode();
    QTC_ASSERT(currentNode && currentNode->nodeType() == NodeType::File, return);

    const Utils::FileName filePath = currentNode->filePath();
    RemoveFileDialog removeFileDialog(filePath.toString(), ICore::mainWindow());

    if (removeFileDialog.exec() == QDialog::Accepted) {
        const bool deleteFile = removeFileDialog.isDeleteFileChecked();

        // Re-read the current node, in case the project is re-parsed while the dialog is open
        if (currentNode != ProjectTree::findCurrentNode()) {
            currentNode = ProjectTreeWidget::nodeForFile(filePath);
            QTC_ASSERT(currentNode && currentNode->nodeType() == NodeType::File, return);
        }

        // remove from project
        FolderNode *folderNode = currentNode->asFileNode()->parentFolderNode();
        QTC_ASSERT(folderNode, return);

        if (!folderNode->removeFiles(QStringList(filePath.toString()))) {
            QMessageBox::warning(ICore::mainWindow(), tr("Removing File Failed"),
                                 tr("Could not remove file %1 from project %2.")
                                 .arg(filePath.toUserOutput())
                                 .arg(folderNode->managingProject()->displayName()));
            if (!deleteFile)
                return;
        }

        FileChangeBlocker changeGuard(filePath.toString());
        FileUtils::removeFile(filePath.toString(), deleteFile);
    }
}

void ProjectExplorerPluginPrivate::duplicateFile()
{
    Node *currentNode = ProjectTree::findCurrentNode();
    QTC_ASSERT(currentNode && currentNode->nodeType() == NodeType::File, return);

    FileNode *fileNode = currentNode->asFileNode();
    QString filePath = currentNode->filePath().toString();
    QFileInfo sourceFileInfo(filePath);
    QString baseName = sourceFileInfo.baseName();

    QString newFilePath = filePath;
    int copyTokenIndex = filePath.lastIndexOf(baseName)+baseName.length();
    newFilePath.insert(copyTokenIndex, tr("_copy"));

    // Build a new file name till a non-existing file is not found.
    uint counter = 0;
    while (QFileInfo::exists(newFilePath)) {
        newFilePath = filePath;
        newFilePath.insert(copyTokenIndex, tr("_copy%1").arg(++counter));
    }

    // Create a copy and add the file to the parent folder node.
    FolderNode *folderNode = fileNode->parentFolderNode();
    QTC_ASSERT(folderNode, return);
    if (!(QFile::copy(filePath, newFilePath) && folderNode->addFiles(QStringList(newFilePath)))) {
        QMessageBox::warning(ICore::mainWindow(), tr("Duplicating File Failed"),
                             tr("Could not duplicate the file %1.")
                             .arg(QDir::toNativeSeparators(filePath)));
    }
}

void ProjectExplorerPluginPrivate::deleteFile()
{
    Node *currentNode = ProjectTree::findCurrentNode();
    QTC_ASSERT(currentNode && currentNode->nodeType() == NodeType::File, return);

    FileNode *fileNode = currentNode->asFileNode();

    QString filePath = currentNode->filePath().toString();
    QMessageBox::StandardButton button =
            QMessageBox::question(ICore::mainWindow(),
                                  tr("Delete File"),
                                  tr("Delete %1 from file system?")
                                  .arg(QDir::toNativeSeparators(filePath)),
                                  QMessageBox::Yes | QMessageBox::No);
    if (button != QMessageBox::Yes)
        return;

    FolderNode *folderNode = fileNode->parentFolderNode();
    QTC_ASSERT(folderNode, return);

    folderNode->deleteFiles(QStringList(filePath));

    FileChangeBlocker changeGuard(filePath);
    if (IVersionControl *vc =
            VcsManager::findVersionControlForDirectory(QFileInfo(filePath).absolutePath())) {
        vc->vcsDelete(filePath);
    }
    QFile file(filePath);
    if (file.exists()) {
        if (!file.remove())
            QMessageBox::warning(ICore::mainWindow(), tr("Deleting File Failed"),
                                 tr("Could not delete file %1.")
                                 .arg(QDir::toNativeSeparators(filePath)));
    }
}

void ProjectExplorerPluginPrivate::handleRenameFile()
{
    QWidget *focusWidget = QApplication::focusWidget();
    while (focusWidget) {
        auto treeWidget = qobject_cast<ProjectTreeWidget*>(focusWidget);
        if (treeWidget) {
            treeWidget->editCurrentItem();
            return;
        }
        focusWidget = focusWidget->parentWidget();
    }
}

void ProjectExplorerPluginPrivate::handleDiffFile()
{
    // current editor's file
    auto textDocument = TextEditor::TextDocument::currentTextDocument();

    if (!textDocument)
        return;

    const QString leftFileName = textDocument->filePath().toString();

    if (leftFileName.isEmpty())
        return;

    // current item's file
    Node *currentNode = ProjectTree::findCurrentNode();
    QTC_ASSERT(currentNode && currentNode->nodeType() == NodeType::File, return);

    FileNode *fileNode = currentNode->asFileNode();

    if (!fileNode)
        return;

    const QString rightFileName = currentNode->filePath().toString();
    if (rightFileName.isEmpty())
        return;

    if (!isTextFile(rightFileName))
        return;

    if (auto diffService = ExtensionSystem::PluginManager::getObject<DiffService>())
        diffService->diffFiles(leftFileName, rightFileName);
}

void ProjectExplorerPlugin::renameFile(Node *node, const QString &newFilePath)
{
    const QString oldFilePath = node->filePath().toFileInfo().absoluteFilePath();
    FolderNode *folderNode = node->parentFolderNode();
    QTC_ASSERT(folderNode, return);
    const QString projectFileName = folderNode->managingProject()->filePath().fileName();

    if (oldFilePath == newFilePath)
        return;

    if (!folderNode->canRenameFile(oldFilePath, newFilePath)) {
        QTimer::singleShot(0, [oldFilePath, newFilePath, projectFileName] {
            int res = QMessageBox::question(ICore::mainWindow(),
                                            tr("Project Editing Failed"),
                                            tr("The project file %1 cannot be automatically changed.\n\n"
                                               "Rename %2 to %3 anyway?")
                                            .arg(projectFileName)
                                            .arg(QDir::toNativeSeparators(oldFilePath))
                                            .arg(QDir::toNativeSeparators(newFilePath)));
            if (res == QMessageBox::Yes) {
                QTC_CHECK(FileUtils::renameFile(oldFilePath, newFilePath));
            }

        });
        return;
    }

    if (FileUtils::renameFile(oldFilePath, newFilePath)) {
        // Tell the project plugin about rename
        if (!folderNode->renameFile(oldFilePath, newFilePath)) {
            const QString renameFileError
                    = tr("The file %1 was renamed to %2, but the project file %3 could not be automatically changed.")
                    .arg(QDir::toNativeSeparators(oldFilePath))
                    .arg(QDir::toNativeSeparators(newFilePath))
                    .arg(projectFileName);

            QTimer::singleShot(0, [renameFileError]() {
                QMessageBox::warning(ICore::mainWindow(),
                                     tr("Project Editing Failed"),
                                     renameFileError);
            });
        }
    } else {
        const QString renameFileError = tr("The file %1 could not be renamed %2.")
                .arg(QDir::toNativeSeparators(oldFilePath))
                .arg(QDir::toNativeSeparators(newFilePath));

        QTimer::singleShot(0, [renameFileError]() {
            QMessageBox::warning(ICore::mainWindow(),
                                 tr("Cannot Rename File"),
                                 renameFileError);
        });
    }
}

void ProjectExplorerPluginPrivate::handleSetStartupProject()
{
    setStartupProject(ProjectTree::currentProject());
}

void ProjectExplorerPluginPrivate::updateSessionMenu()
{
    m_sessionMenu->clear();
    dd->m_sessionMenu->addAction(dd->m_sessionManagerAction);
    dd->m_sessionMenu->addSeparator();
    QActionGroup *ag = new QActionGroup(m_sessionMenu);
    connect(ag, &QActionGroup::triggered, this, &ProjectExplorerPluginPrivate::setSession);
    const QString activeSession = SessionManager::activeSession();

    const QStringList sessions = SessionManager::sessions();
    for (int i = 0; i < sessions.size(); ++i) {
        const QString &session = sessions[i];

        const QString actionText = ActionManager::withNumberAccelerator(session, i + 1);
        QAction *act = ag->addAction(actionText);
        act->setData(session);
        act->setCheckable(true);
        if (session == activeSession)
            act->setChecked(true);
    }
    m_sessionMenu->addActions(ag->actions());
    m_sessionMenu->setEnabled(true);
}

void ProjectExplorerPluginPrivate::setSession(QAction *action)
{
    QString session = action->data().toString();
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
    for (const QString &mime : dd->m_projectCreators.keys()) {
        Utils::MimeType mt = Utils::mimeTypeForName(mime);
        if (mt.isValid())
            patterns.append(mt.globPatterns());
    }
    return patterns;
}

bool ProjectExplorerPlugin::isProjectFile(const Utils::FileName &filePath)
{
    Utils::MimeType mt = Utils::mimeTypeForFile(filePath.toString());
    for (const QString &mime : dd->m_projectCreators.keys()) {
        if (mt.inherits(mime))
            return true;
    }
    return false;
}

void ProjectExplorerPlugin::openOpenProjectDialog()
{
    const QString path = DocumentManager::useProjectsDirectory()
                             ? DocumentManager::projectsDirectory().toString()
                             : QString();
    const QStringList files = DocumentManager::getOpenFileNames(dd->m_projectFilterString, path);
    if (!files.isEmpty())
        ICore::openFiles(files, ICore::SwitchMode);
}

QList<QPair<QString, QString> > ProjectExplorerPlugin::recentProjects()
{
    return dd->recentProjects();
}

void ProjectManager::registerProjectCreator(const QString &mimeType,
    const std::function<Project *(const Utils::FileName &)> &creator)
{
    dd->m_projectCreators[mimeType] = creator;
}

Project *ProjectManager::openProject(const Utils::MimeType &mt, const Utils::FileName &fileName)
{
    if (mt.isValid()) {
        for (const QString &mimeType : dd->m_projectCreators.keys()) {
            if (mt.matchesName(mimeType))
                return dd->m_projectCreators[mimeType](fileName);
        }
    }
    return nullptr;
}

bool ProjectManager::canOpenProjectForMimeType(const Utils::MimeType &mt)
{
    if (mt.isValid()) {
        for (const QString &mimeType : dd->m_projectCreators.keys()) {
            if (mt.matchesName(mimeType))
                return true;
        }
    }
    return false;
}

} // namespace ProjectExplorer
