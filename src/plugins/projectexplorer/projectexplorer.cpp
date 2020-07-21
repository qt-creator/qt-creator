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

#include "appoutputpane.h"
#include "buildpropertiessettings.h"
#include "buildpropertiessettingspage.h"
#include "buildsteplist.h"
#include "buildsystem.h"
#include "compileoutputwindow.h"
#include "configtaskhandler.h"
#include "customexecutablerunconfiguration.h"
#include "customparserssettingspage.h"
#include "customwizard/customwizard.h"
#include "deployablefile.h"
#include "deployconfiguration.h"
#include "desktoprunconfiguration.h"
#include "environmentwidget.h"
#include "extraabi.h"
#include "gcctoolchainfactories.h"
#ifdef WITH_JOURNALD
#include "journaldwatcher.h"
#endif
#include "jsonwizard/jsonwizardfactory.h"
#include "jsonwizard/jsonwizardgeneratorfactory.h"
#include "jsonwizard/jsonwizardpagefactory_p.h"
#include "namedwidget.h"
#include "project.h"
#include "projectexplorersettings.h"
#include "projectexplorersettingspage.h"
#include "projectmanager.h"
#include "removetaskhandler.h"
#include "runconfigurationaspects.h"
#include "kitfeatureprovider.h"
#include "kitmanager.h"
#include "kitoptionspage.h"
#include "parseissuesdialog.h"
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
#include "devicesupport/sshsettingspage.h"
#include "targetsettingspanel.h"
#include "projectpanelfactory.h"
#include "projectexplorericons.h"
#include "simpleprojectwizard.h"

#include "windebuginterface.h"
#include "msvctoolchain.h"

#include "projecttree.h"
#include "projectwelcomepage.h"

#include <app/app_version.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/diffservice.h>
#include <coreplugin/documentmanager.h>
#include <coreplugin/editormanager/documentmodel.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/fileutils.h>
#include <coreplugin/findplaceholder.h>
#include <coreplugin/icore.h>
#include <coreplugin/idocument.h>
#include <coreplugin/idocumentfactory.h>
#include <coreplugin/imode.h>
#include <coreplugin/iversioncontrol.h>
#include <coreplugin/locator/directoryfilter.h>
#include <coreplugin/modemanager.h>
#include <coreplugin/vcsmanager.h>
#include <extensionsystem/pluginmanager.h>
#include <extensionsystem/pluginspec.h>
#include <ssh/sshconnection.h>
#include <ssh/sshsettings.h>
#include <texteditor/findinfiles.h>
#include <texteditor/textdocument.h>
#include <texteditor/texteditorconstants.h>

#include <utils/algorithm.h>
#include <utils/fileutils.h>
#include <utils/macroexpander.h>
#include <utils/mimetypes/mimedatabase.h>
#include <utils/parameteraction.h>
#include <utils/processhandle.h>
#include <utils/proxyaction.h>
#include <utils/qtcassert.h>
#include <utils/removefiledialog.h>
#include <utils/stringutils.h>
#include <utils/utilsicons.h>

#include <QAction>
#include <QActionGroup>
#include <QApplication>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QInputDialog>
#include <QMenu>
#include <QMessageBox>
#include <QPair>
#include <QSettings>
#include <QThreadPool>
#include <QTimer>

#include <functional>

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
using namespace Utils;

namespace ProjectExplorer {

namespace Constants {
const int  P_MODE_SESSION         = 85;

// Actions
const char NEWPROJECT[]           = "ProjectExplorer.NewProject";
const char LOAD[]                 = "ProjectExplorer.Load";
const char UNLOAD[]               = "ProjectExplorer.Unload";
const char UNLOADCM[]             = "ProjectExplorer.UnloadCM";
const char CLEARSESSION[]         = "ProjectExplorer.ClearSession";
const char BUILDALLCONFIGS[]      = "ProjectExplorer.BuildProjectForAllConfigs";
const char BUILDPROJECTONLY[]     = "ProjectExplorer.BuildProjectOnly";
const char BUILDCM[]              = "ProjectExplorer.BuildCM";
const char BUILDDEPENDCM[]        = "ProjectExplorer.BuildDependenciesCM";
const char BUILDSESSION[]         = "ProjectExplorer.BuildSession";
const char BUILDSESSIONALLCONFIGS[] = "ProjectExplorer.BuildSessionForAllConfigs";
const char REBUILDPROJECTONLY[]   = "ProjectExplorer.RebuildProjectOnly";
const char REBUILD[]              = "ProjectExplorer.Rebuild";
const char REBUILDALLCONFIGS[]    = "ProjectExplorer.RebuildProjectForAllConfigs";
const char REBUILDCM[]            = "ProjectExplorer.RebuildCM";
const char REBUILDDEPENDCM[]      = "ProjectExplorer.RebuildDependenciesCM";
const char REBUILDSESSION[]       = "ProjectExplorer.RebuildSession";
const char REBUILDSESSIONALLCONFIGS[] = "ProjectExplorer.RebuildSessionForAllConfigs";
const char DEPLOYPROJECTONLY[]    = "ProjectExplorer.DeployProjectOnly";
const char DEPLOY[]               = "ProjectExplorer.Deploy";
const char DEPLOYCM[]             = "ProjectExplorer.DeployCM";
const char DEPLOYSESSION[]        = "ProjectExplorer.DeploySession";
const char CLEANPROJECTONLY[]     = "ProjectExplorer.CleanProjectOnly";
const char CLEAN[]                = "ProjectExplorer.Clean";
const char CLEANALLCONFIGS[]      = "ProjectExplorer.CleanProjectForAllConfigs";
const char CLEANCM[]              = "ProjectExplorer.CleanCM";
const char CLEANDEPENDCM[]        = "ProjectExplorer.CleanDependenciesCM";
const char CLEANSESSION[]         = "ProjectExplorer.CleanSession";
const char CLEANSESSIONALLCONFIGS[] = "ProjectExplorer.CleanSessionForAllConfigs";
const char CANCELBUILD[]          = "ProjectExplorer.CancelBuild";
const char RUN[]                  = "ProjectExplorer.Run";
const char RUNWITHOUTDEPLOY[]     = "ProjectExplorer.RunWithoutDeploy";
const char RUNCONTEXTMENU[]       = "ProjectExplorer.RunContextMenu";
const char ADDEXISTINGFILES[]     = "ProjectExplorer.AddExistingFiles";
const char ADDEXISTINGDIRECTORY[] = "ProjectExplorer.AddExistingDirectory";
const char ADDNEWSUBPROJECT[]     = "ProjectExplorer.AddNewSubproject";
const char REMOVEPROJECT[]        = "ProjectExplorer.RemoveProject";
const char OPENFILE[]             = "ProjectExplorer.OpenFile";
const char SEARCHONFILESYSTEM[]   = "ProjectExplorer.SearchOnFileSystem";
const char SHOWINGRAPHICALSHELL[] = "ProjectExplorer.ShowInGraphicalShell";
const char OPENTERMINALHERE[]     = "ProjectExplorer.OpenTerminalHere";
const char DUPLICATEFILE[]        = "ProjectExplorer.DuplicateFile";
const char DELETEFILE[]           = "ProjectExplorer.DeleteFile";
const char DIFFFILE[]             = "ProjectExplorer.DiffFile";
const char SETSTARTUP[]           = "ProjectExplorer.SetStartup";
const char PROJECTTREE_COLLAPSE_ALL[] = "ProjectExplorer.CollapseAll";
const char PROJECTTREE_EXPAND_ALL[] = "ProjectExplorer.ExpandAll";

const char SELECTTARGET[]         = "ProjectExplorer.SelectTarget";
const char SELECTTARGETQUICK[]    = "ProjectExplorer.SelectTargetQuick";

// Action priorities
const int  P_ACTION_RUN            = 100;
const int  P_ACTION_BUILDPROJECT   = 80;

// Menus
const char M_RECENTPROJECTS[]     = "ProjectExplorer.Menu.Recent";
const char M_UNLOADPROJECTS[]     = "ProjectExplorer.Menu.Unload";
const char M_SESSION[]            = "ProjectExplorer.Menu.Session";

const char RUNMENUCONTEXTMENU[]   = "Project.RunMenu";
const char FOLDER_OPEN_LOCATIONS_CONTEXT_MENU[]  = "Project.F.OpenLocation.CtxMenu";
const char PROJECT_OPEN_LOCATIONS_CONTEXT_MENU[]  = "Project.P.OpenLocation.CtxMenu";

// Default directories:
const char DEFAULT_BUILD_DIRECTORY_TEMPLATE[] = "../%{JS: Util.asciify(\"build-%{CurrentProject:Name}-%{CurrentKit:FileSystemName}-%{CurrentBuild:Name}\")}";
const char DEFAULT_BUILD_DIRECTORY_TEMPLATE_KEY[] = "Directories/BuildDirectory.Template";

const char BUILD_BEFORE_DEPLOY_SETTINGS_KEY[] = "ProjectExplorer/Settings/BuildBeforeDeploy";
const char DEPLOY_BEFORE_RUN_SETTINGS_KEY[] = "ProjectExplorer/Settings/DeployBeforeRun";
const char SAVE_BEFORE_BUILD_SETTINGS_KEY[] = "ProjectExplorer/Settings/SaveBeforeBuild";
const char USE_JOM_SETTINGS_KEY[] = "ProjectExplorer/Settings/UseJom";
const char AUTO_RESTORE_SESSION_SETTINGS_KEY[] = "ProjectExplorer/Settings/AutoRestoreLastSession";
const char ADD_LIBRARY_PATHS_TO_RUN_ENV_SETTINGS_KEY[] =
        "ProjectExplorer/Settings/AddLibraryPathsToRunEnv";
const char PROMPT_TO_STOP_RUN_CONTROL_SETTINGS_KEY[] =
        "ProjectExplorer/Settings/PromptToStopRunControl";
const char AUTO_CREATE_RUN_CONFIGS_SETTINGS_KEY[] =
        "ProjectExplorer/Settings/AutomaticallyCreateRunConfigurations";
const char ENVIRONMENT_ID_SETTINGS_KEY[] = "ProjectExplorer/Settings/EnvironmentId";
const char STOP_BEFORE_BUILD_SETTINGS_KEY[] = "ProjectExplorer/Settings/StopBeforeBuild";
const char TERMINAL_MODE_SETTINGS_KEY[] = "ProjectExplorer/Settings/TerminalMode";
const char CLOSE_FILES_WITH_PROJECT_SETTINGS_KEY[]
    = "ProjectExplorer/Settings/CloseFilesWithProject";
const char CLEAR_ISSUES_ON_REBUILD_SETTINGS_KEY[] = "ProjectExplorer/Settings/ClearIssuesOnRebuild";
const char ABORT_BUILD_ALL_ON_ERROR_SETTINGS_KEY[]
    = "ProjectExplorer/Settings/AbortBuildAllOnError";
const char LOW_BUILD_PRIORITY_SETTINGS_KEY[] = "ProjectExplorer/Settings/LowBuildPriority";

const char SEPARATE_DEBUG_INFO_SETTINGS_KEY[] = "ProjectExplorer/Settings/SeparateDebugInfo";
const char QML_DEBUGGING_SETTINGS_KEY[] = "ProjectExplorer/Settings/QmlDebugging";
const char QT_QUICK_COMPILER_SETTINGS_KEY[] = "ProjectExplorer/Settings/QtQuickCompiler";

const char CUSTOM_PARSER_COUNT_KEY[] = "ProjectExplorer/Settings/CustomParserCount";
const char CUSTOM_PARSER_PREFIX_KEY[] = "ProjectExplorer/Settings/CustomParser";

} // namespace Constants


static Utils::optional<Utils::Environment> sysEnv(const Project *)
{
    return Utils::Environment::systemEnvironment();
}

static Utils::optional<Utils::Environment> buildEnv(const Project *project)
{
    if (!project || !project->activeTarget() || !project->activeTarget()->activeBuildConfiguration())
        return {};
    return project->activeTarget()->activeBuildConfiguration()->environment();
}

static bool canOpenTerminalWithRunEnv(const Project *project)
{
    if (!project)
        return false;
    const Target * const target = project->activeTarget();
    if (!target)
        return false;
    const RunConfiguration * const runConfig = target->activeRunConfiguration();
    if (!runConfig)
        return false;
    IDevice::ConstPtr device = runConfig->runnable().device;
    if (!device)
        device = DeviceKitAspect::device(target->kit());
    return device && device->canOpenTerminal();
}

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

static RunConfiguration *activeRunConfiguration()
{
    Target *target = activeTarget();
    return target ? target->activeRunConfiguration() : nullptr;
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

class ProjectsMode : public IMode
{
public:
    ProjectsMode()
    {
        setContext(Context(Constants::C_PROJECTEXPLORER));
        setDisplayName(QCoreApplication::translate("ProjectExplorer::ProjectsMode", "Projects"));
        setIcon(Utils::Icon::modeIcon(Icons::MODE_PROJECT_CLASSIC,
                                      Icons::MODE_PROJECT_FLAT, Icons::MODE_PROJECT_FLAT_ACTIVE));
        setPriority(Constants::P_MODE_SESSION);
        setId(Constants::MODE_SESSION);
        setContextHelp("Managing Projects");
    }
};


class ProjectEnvironmentWidget : public NamedWidget
{
    Q_DECLARE_TR_FUNCTIONS(ProjectEnvironmentWidget)

public:
    explicit ProjectEnvironmentWidget(Project *project) : NamedWidget(tr("Project Environment"))
    {
        const auto vbox = new QVBoxLayout(this);
        vbox->setContentsMargins(0, 0, 0, 0);
        const auto envWidget = new EnvironmentWidget(this, EnvironmentWidget::TypeLocal);
        envWidget->setOpenTerminalFunc({});
        envWidget->expand();
        vbox->addWidget(envWidget);
        connect(envWidget, &EnvironmentWidget::userChangesChanged,
                this, [project, envWidget] {
            project->setAdditionalEnvironment(envWidget->userChanges());
        });
        envWidget->setUserChanges(project->additionalEnvironment());
    }
};


class ProjectExplorerPluginPrivate : public QObject
{
    Q_DECLARE_TR_FUNCTIONS(ProjectExplorer::ProjectExplorerPlugin)

public:
    ProjectExplorerPluginPrivate();

    void updateContextMenuActions();
    void updateLocationSubMenus();
    void executeRunConfiguration(RunConfiguration *, Utils::Id mode);
    QPair<bool, QString> buildSettingsEnabledForSession();
    QPair<bool, QString> buildSettingsEnabled(const Project *pro);

    void addToRecentProjects(const QString &fileName, const QString &displayName);
    void startRunControl(RunControl *runControl);
    void showOutputPaneForRunControl(RunControl *runControl);

    void updateActions();
    void updateContext();
    void updateDeployActions();
    void updateRunWithoutDeployMenu();

    void buildQueueFinished(bool success);

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
    void addExistingProjects();
    void removeProject();
    void openFile();
    void searchOnFileSystem();
    void showInGraphicalShell();
    void removeFile();
    void duplicateFile();
    void deleteFile();
    void handleRenameFile();
    void handleSetStartupProject();
    void setStartupProject(ProjectExplorer::Project *project);
    bool closeAllFilesInProject(const Project *project);

    void updateRecentProjectMenu();
    void clearRecentProjects();
    void openRecentProject(const QString &fileName);
    void removeFromRecentProjects(const QString &fileName, const QString &displayName);
    void updateUnloadProjectMenu();
    using EnvironmentGetter = std::function<Utils::optional<Utils::Environment>(const Project *project)>;
    void openTerminalHere(const EnvironmentGetter &env);
    void openTerminalHereWithRunEnv();

    void invalidateProject(ProjectExplorer::Project *project);

    void projectAdded(ProjectExplorer::Project *pro);
    void projectRemoved(ProjectExplorer::Project *pro);
    void projectDisplayNameChanged(ProjectExplorer::Project *pro);

    void doUpdateRunActions();

    void currentModeChanged(Utils::Id mode, Utils::Id oldMode);

    void updateWelcomePage();

    void checkForShutdown();
    void timerEvent(QTimerEvent *) override;

    QList<QPair<QString, QString> > recentProjects() const;

public:
    QMenu *m_sessionMenu;
    QMenu *m_openWithMenu;
    QMenu *m_openTerminalMenu;

    QMultiMap<int, QObject*> m_actionMap;
    QAction *m_sessionManagerAction;
    QAction *m_newAction;
    QAction *m_loadAction;
    Utils::ParameterAction *m_unloadAction;
    Utils::ParameterAction *m_unloadActionContextMenu;
    QAction *m_closeAllProjects;
    QAction *m_buildProjectOnlyAction;
    Utils::ParameterAction *m_buildProjectForAllConfigsAction;
    Utils::ParameterAction *m_buildAction;
    Utils::ParameterAction *m_buildForRunConfigAction;
    Utils::ProxyAction *m_modeBarBuildAction;
    QAction *m_buildActionContextMenu;
    QAction *m_buildDependenciesActionContextMenu;
    QAction *m_buildSessionAction;
    QAction *m_buildSessionForAllConfigsAction;
    QAction *m_rebuildProjectOnlyAction;
    Utils::ParameterAction *m_rebuildAction;
    Utils::ParameterAction *m_rebuildProjectForAllConfigsAction;
    QAction *m_rebuildActionContextMenu;
    QAction *m_rebuildDependenciesActionContextMenu;
    QAction *m_rebuildSessionAction;
    QAction *m_rebuildSessionForAllConfigsAction;
    QAction *m_cleanProjectOnlyAction;
    QAction *m_deployProjectOnlyAction;
    Utils::ParameterAction *m_deployAction;
    QAction *m_deployActionContextMenu;
    QAction *m_deploySessionAction;
    Utils::ParameterAction *m_cleanAction;
    Utils::ParameterAction *m_cleanProjectForAllConfigsAction;
    QAction *m_cleanActionContextMenu;
    QAction *m_cleanDependenciesActionContextMenu;
    QAction *m_cleanSessionAction;
    QAction *m_cleanSessionForAllConfigsAction;
    QAction *m_runAction;
    QAction *m_runActionContextMenu;
    QAction *m_runWithoutDeployAction;
    QAction *m_cancelBuildAction;
    QAction *m_addNewFileAction;
    QAction *m_addExistingFilesAction;
    QAction *m_addExistingDirectoryAction;
    QAction *m_addNewSubprojectAction;
    QAction *m_addExistingProjectsAction;
    QAction *m_removeFileAction;
    QAction *m_duplicateFileAction;
    QAction *m_removeProjectAction;
    QAction *m_deleteFileAction;
    QAction *m_renameFileAction;
    QAction *m_filePropertiesAction = nullptr;
    QAction *m_diffFileAction;
    QAction *m_openFileAction;
    QAction *m_projectTreeCollapseAllAction;
    QAction *m_projectTreeExpandAllAction;
    QAction *m_projectTreeExpandNodeAction = nullptr;
    Utils::ParameterAction *m_closeProjectFilesActionFileMenu;
    Utils::ParameterAction *m_closeProjectFilesActionContextMenu;
    QAction *m_searchOnFileSystem;
    QAction *m_showInGraphicalShell;
    QAction *m_openTerminalHere;
    QAction *m_openTerminalHereBuildEnv;
    QAction *m_openTerminalHereRunEnv;
    Utils::ParameterAction *m_setStartupProjectAction;
    QAction *m_projectSelectorAction;
    QAction *m_projectSelectorActionMenu;
    QAction *m_projectSelectorActionQuick;
    QAction *m_runSubProject;

    ProjectWindow *m_proWindow = nullptr;
    QString m_sessionToRestoreAtStartup;

    QStringList m_profileMimeTypes;
    int m_activeRunControlCount = 0;
    int m_shutdownWatchDogId = -1;

    QHash<QString, std::function<Project *(const Utils::FilePath &)>> m_projectCreators;
    QList<QPair<QString, QString> > m_recentProjects; // pair of filename, displayname
    static const int m_maxRecentProjects = 25;

    QString m_lastOpenDirectory;
    QPointer<RunConfiguration> m_delayedRunConfiguration;
    QString m_projectFilterString;
    MiniProjectTargetSelector * m_targetSelector;
    ProjectExplorerSettings m_projectExplorerSettings;
    BuildPropertiesSettings m_buildPropertiesSettings;
    QList<Internal::CustomParserSettings> m_customParsers;
    bool m_shouldHaveRunConfiguration = false;
    bool m_shuttingDown = false;
    Utils::Id m_runMode = Constants::NO_RUN_MODE;

    ToolChainManager *m_toolChainManager = nullptr;
    QStringList m_arguments;

#ifdef WITH_JOURNALD
    JournaldWatcher m_journalWatcher;
#endif
    QThreadPool m_threadPool;

    DeviceManager m_deviceManager{true};

#ifdef Q_OS_WIN
    WinDebugInterface m_winDebugInterface;
    MsvcToolChainFactory m_mscvToolChainFactory;
    ClangClToolChainFactory m_clangClToolChainFactory;
#else
    LinuxIccToolChainFactory m_linuxToolChainFactory;
#endif

#ifndef Q_OS_MACOS
    MingwToolChainFactory m_mingwToolChainFactory; // Mingw offers cross-compiling to windows
#endif

    GccToolChainFactory m_gccToolChainFactory;
    ClangToolChainFactory m_clangToolChainFactory;
    CustomToolChainFactory m_customToolChainFactory;

    DesktopDeviceFactory m_desktopDeviceFactory;

    ToolChainOptionsPage m_toolChainOptionsPage;
    KitOptionsPage m_kitOptionsPage;

    TaskHub m_taskHub;

    ProjectWelcomePage m_welcomePage;

    CustomWizardMetaFactory<CustomProjectWizard> m_customProjectWizard{IWizardFactory::ProjectWizard};
    CustomWizardMetaFactory<CustomWizard> m_fileWizard{IWizardFactory::FileWizard};

    ProjectsMode m_projectsMode;

    CopyTaskHandler m_copyTaskHandler;
    ShowInEditorTaskHandler m_showInEditorTaskHandler;
    VcsAnnotateTaskHandler m_vcsAnnotateTaskHandler;
    RemoveTaskHandler m_removeTaskHandler;
    ConfigTaskHandler m_configTaskHandler{Task::compilerMissingTask(), Constants::KITS_SETTINGS_PAGE_ID};

    SessionManager m_sessionManager;
    AppOutputPane m_outputPane;

    ProjectTree m_projectTree;

    AllProjectsFilter m_allProjectsFilter;
    CurrentProjectFilter m_currentProjectFilter;
    DirectoryFilter m_allProjectDirectoriesFilter;

    ProcessStepFactory m_processStepFactory;

    AllProjectsFind m_allProjectsFind;
    CurrentProjectFind m_curretProjectFind;

    CustomExecutableRunConfigurationFactory m_customExecutableRunConfigFactory;
    RunWorkerFactory m_customExecutableRunWorkerFactory{
        RunWorkerFactory::make<SimpleTargetRunner>(),
        {Constants::NORMAL_RUN_MODE},
        {m_customExecutableRunConfigFactory.runConfigurationId()}
    };

    ProjectFileWizardExtension m_projectFileWizardExtension;

    // Settings pages
    ProjectExplorerSettingsPage m_projectExplorerSettingsPage;
    BuildPropertiesSettingsPage m_buildPropertiesSettingsPage;
    AppOutputSettingsPage m_appOutputSettingsPage;
    CompileOutputSettingsPage m_compileOutputSettingsPage;
    DeviceSettingsPage m_deviceSettingsPage;
    SshSettingsPage m_sshSettingsPage;
    CustomParsersSettingsPage m_customParsersSettingsPage;

    ProjectTreeWidgetFactory m_projectTreeFactory;
    FolderNavigationWidgetFactory m_folderNavigationWidgetFactory;
    DefaultDeployConfigurationFactory m_defaultDeployConfigFactory;

    IDocumentFactory m_documentFactory;

    DeviceTypeKitAspect deviceTypeKitAspect;
    DeviceKitAspect deviceeKitAspect;
    ToolChainKitAspect toolChainKitAspect;
    SysRootKitAspect sysRootKitAspect;
    EnvironmentKitAspect environmentKitAspect;

    DesktopQmakeRunConfigurationFactory qmakeRunConfigFactory;
    QbsRunConfigurationFactory qbsRunConfigFactory;
    CMakeRunConfigurationFactory cmakeRunConfigFactory;

    RunWorkerFactory desktopRunWorkerFactory{
        RunWorkerFactory::make<SimpleTargetRunner>(),
        {ProjectExplorer::Constants::NORMAL_RUN_MODE},
        {qmakeRunConfigFactory.runConfigurationId(),
         qbsRunConfigFactory.runConfigurationId(),
         cmakeRunConfigFactory.runConfigurationId()}
    };

};

static ProjectExplorerPlugin *m_instance = nullptr;
static ProjectExplorerPluginPrivate *dd = nullptr;

ProjectExplorerPlugin::ProjectExplorerPlugin()
{
    m_instance = this;
}

ProjectExplorerPlugin::~ProjectExplorerPlugin()
{
    delete dd->m_proWindow; // Needs access to the kit manager.
    JsonWizardFactory::destroyAllFactories();

    // Force sequence of deletion:
    KitManager::destroy(); // remove all the profile information
    delete dd->m_toolChainManager;
    ProjectPanelFactory::destroyFactories();
    delete dd;
    dd = nullptr;
    m_instance = nullptr;

#ifdef WITH_TESTS
    deleteTestToolchains();
#endif
}

ProjectExplorerPlugin *ProjectExplorerPlugin::instance()
{
    return m_instance;
}

bool ProjectExplorerPlugin::initialize(const QStringList &arguments, QString *error)
{
    Q_UNUSED(error)

    dd = new ProjectExplorerPluginPrivate;

    qRegisterMetaType<ProjectExplorer::BuildSystem *>();
    qRegisterMetaType<ProjectExplorer::RunControl *>();
    qRegisterMetaType<ProjectExplorer::DeployableFile>("ProjectExplorer::DeployableFile");

    handleCommandLineArguments(arguments);

    dd->m_toolChainManager = new ToolChainManager;

    // Register languages
    ToolChainManager::registerLanguage(Constants::C_LANGUAGE_ID, tr("C"));
    ToolChainManager::registerLanguage(Constants::CXX_LANGUAGE_ID, tr("C++"));

    IWizardFactory::registerFeatureProvider(new KitFeatureProvider);

    IWizardFactory::registerFactoryCreator([]() -> QList<IWizardFactory *> {
        QList<IWizardFactory *> result;
        result << CustomWizard::createWizards();
        result << JsonWizardFactory::createWizardFactories();
        result << new SimpleProjectWizard;
        return result;
    });

    connect(&dd->m_welcomePage, &ProjectWelcomePage::manageSessions,
            dd, &ProjectExplorerPluginPrivate::showSessionManager);

    SessionManager *sessionManager = &dd->m_sessionManager;
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
    connect(sessionManager, &SessionManager::projectDisplayNameChanged,
            dd, &ProjectExplorerPluginPrivate::projectDisplayNameChanged);
    connect(sessionManager, &SessionManager::dependencyChanged,
            dd, &ProjectExplorerPluginPrivate::updateActions);
    connect(sessionManager, &SessionManager::sessionLoaded,
            dd, &ProjectExplorerPluginPrivate::updateActions);
    connect(sessionManager,
            &SessionManager::sessionLoaded,
            dd,
            &ProjectExplorerPluginPrivate::updateWelcomePage);

    connect(sessionManager, &SessionManager::projectAdded, dd, [](ProjectExplorer::Project *project) {
        dd->m_allProjectDirectoriesFilter.addDirectory(project->projectDirectory().toString());
    });
    connect(sessionManager,
            &SessionManager::projectRemoved,
            dd,
            [](ProjectExplorer::Project *project) {
                dd->m_allProjectDirectoriesFilter.removeDirectory(
                    project->projectDirectory().toString());
            });

    ProjectTree *tree = &dd->m_projectTree;
    connect(tree, &ProjectTree::currentProjectChanged,
            dd, &ProjectExplorerPluginPrivate::updateContextMenuActions);
    connect(tree, &ProjectTree::nodeActionsChanged,
            dd, &ProjectExplorerPluginPrivate::updateContextMenuActions);
    connect(tree, &ProjectTree::currentNodeChanged,
            dd, &ProjectExplorerPluginPrivate::updateContextMenuActions);
    connect(tree, &ProjectTree::currentProjectChanged,
            dd, &ProjectExplorerPluginPrivate::updateActions);
    connect(tree, &ProjectTree::currentProjectChanged, this, [](Project *project) {
        TextEditor::FindInFiles::instance()->setBaseDirectory(project ? project->projectDirectory()
                                                                      : Utils::FilePath());
    });

    // For JsonWizard:
    JsonWizardFactory::registerPageFactory(new FieldPageFactory);
    JsonWizardFactory::registerPageFactory(new FilePageFactory);
    JsonWizardFactory::registerPageFactory(new KitsPageFactory);
    JsonWizardFactory::registerPageFactory(new ProjectPageFactory);
    JsonWizardFactory::registerPageFactory(new SummaryPageFactory);

    JsonWizardFactory::registerGeneratorFactory(new FileGeneratorFactory);
    JsonWizardFactory::registerGeneratorFactory(new ScannerGeneratorFactory);

    dd->m_proWindow = new ProjectWindow;

    Context projectTreeContext(Constants::C_PROJECT_TREE);

    dd->m_projectsMode.setWidget(dd->m_proWindow);
    dd->m_projectsMode.setEnabled(false);

    ICore::addPreCloseListener([]() -> bool { return coreAboutToClose(); });

    connect(SessionManager::instance(), &SessionManager::projectRemoved,
            &dd->m_outputPane, &AppOutputPane::projectRemoved);

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

    panelFactory = new ProjectPanelFactory;
    panelFactory->setPriority(60);
    panelFactory->setDisplayName(QCoreApplication::translate("EnvironmentPanelFactory", "Environment"));
    panelFactory->setCreateWidgetFunction([](Project *project) {
        return new ProjectEnvironmentWidget(project);
    });
    ProjectPanelFactory::registerFactory(panelFactory);

    RunConfiguration::registerAspect<CustomParsersAspect>();

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
    folderOpenLocationCtxMenu->setOnAllDisabledBehavior(ActionContainer::Hide);

    ActionContainer *projectOpenLocationCtxMenu =
            ActionManager::createMenu(Constants::PROJECT_OPEN_LOCATIONS_CONTEXT_MENU);
    projectOpenLocationCtxMenu->menu()->setTitle(tr("Open..."));
    projectOpenLocationCtxMenu->setOnAllDisabledBehavior(ActionContainer::Hide);

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
    mprojectContextMenu->appendGroup(Constants::G_FOLDER_LOCATIONS);
    mprojectContextMenu->appendGroup(Constants::G_PROJECT_FILES);
    mprojectContextMenu->appendGroup(Constants::G_PROJECT_LAST);
    mprojectContextMenu->appendGroup(Constants::G_PROJECT_TREE);

    mprojectContextMenu->addMenu(projectOpenLocationCtxMenu, Constants::G_FOLDER_LOCATIONS);
    connect(mprojectContextMenu->menu(), &QMenu::aboutToShow,
            dd, &ProjectExplorerPluginPrivate::updateLocationSubMenus);

    msubProjectContextMenu->appendGroup(Constants::G_PROJECT_FIRST);
    msubProjectContextMenu->appendGroup(Constants::G_PROJECT_BUILD);
    msubProjectContextMenu->appendGroup(Constants::G_PROJECT_RUN);
    msubProjectContextMenu->appendGroup(Constants::G_FOLDER_LOCATIONS);
    msubProjectContextMenu->appendGroup(Constants::G_PROJECT_FILES);
    msubProjectContextMenu->appendGroup(Constants::G_PROJECT_LAST);
    msubProjectContextMenu->appendGroup(Constants::G_PROJECT_TREE);

    msubProjectContextMenu->addMenu(projectOpenLocationCtxMenu, Constants::G_FOLDER_LOCATIONS);
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

    // Open Terminal submenu
    ActionContainer * const openTerminal =
            ActionManager::createMenu(ProjectExplorer::Constants::M_OPENTERMINALCONTEXT);
    openTerminal->setOnAllDisabledBehavior(ActionContainer::Show);
    dd->m_openTerminalMenu = openTerminal->menu();
    dd->m_openTerminalMenu->setTitle(Core::FileUtils::msgTerminalWithAction());

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

    msessionContextMenu->addSeparator(projectTreeContext, Constants::G_SESSION_REBUILD);

    msessionContextMenu->addSeparator(projectTreeContext, Constants::G_SESSION_FILES);
    mprojectContextMenu->addSeparator(projectTreeContext, Constants::G_PROJECT_FILES);
    msubProjectContextMenu->addSeparator(projectTreeContext, Constants::G_PROJECT_FILES);
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
                       projectTreeContext);
    mfileContextMenu->addAction(cmd, Constants::G_FILE_OPEN);

    dd->m_searchOnFileSystem = new QAction(Core::FileUtils::msgFindInDirectory(), this);
    cmd = ActionManager::registerAction(dd->m_searchOnFileSystem, Constants::SEARCHONFILESYSTEM, projectTreeContext);

    mfileContextMenu->addAction(cmd, Constants::G_FILE_OTHER);
    mfolderContextMenu->addAction(cmd, Constants::G_FOLDER_CONFIG);
    msubProjectContextMenu->addAction(cmd, Constants::G_PROJECT_LAST);
    mprojectContextMenu->addAction(cmd, Constants::G_PROJECT_LAST);

    dd->m_showInGraphicalShell = new QAction(Core::FileUtils::msgGraphicalShellAction(), this);
    cmd = ActionManager::registerAction(dd->m_showInGraphicalShell, Constants::SHOWINGRAPHICALSHELL,
                       projectTreeContext);
    mfileContextMenu->addAction(cmd, Constants::G_FILE_OPEN);
    mfolderContextMenu->addAction(cmd, Constants::G_FOLDER_FILES);

    // Open Terminal Here menu
    dd->m_openTerminalHere = new QAction(Core::FileUtils::msgTerminalHereAction(), this);
    cmd = ActionManager::registerAction(dd->m_openTerminalHere, Constants::OPENTERMINALHERE,
                                        projectTreeContext);

    mfileContextMenu->addAction(cmd, Constants::G_FILE_OPEN);
    mfolderContextMenu->addAction(cmd, Constants::G_FOLDER_FILES);

    mfileContextMenu->addMenu(openTerminal, Constants::G_FILE_OPEN);
    mfolderContextMenu->addMenu(openTerminal, Constants::G_FOLDER_FILES);

    dd->m_openTerminalHereBuildEnv = new QAction(tr("Build Environment"), this);
    dd->m_openTerminalHereRunEnv = new QAction(tr("Run Environment"), this);
    cmd = ActionManager::registerAction(dd->m_openTerminalHereBuildEnv,
                                        "ProjectExplorer.OpenTerminalHereBuildEnv",
                                        projectTreeContext);
    dd->m_openTerminalMenu->addAction(dd->m_openTerminalHereBuildEnv);

    cmd = ActionManager::registerAction(dd->m_openTerminalHereRunEnv,
                                        "ProjectExplorer.OpenTerminalHereRunEnv",
                                        projectTreeContext);
    dd->m_openTerminalMenu->addAction(dd->m_openTerminalHereRunEnv);

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

    dd->m_closeProjectFilesActionFileMenu = new Utils::ParameterAction(
                tr("Close All Files in Project"), tr("Close All Files in Project \"%1\""),
                Utils::ParameterAction::AlwaysEnabled, this);
    cmd = ActionManager::registerAction(dd->m_closeProjectFilesActionFileMenu,
                                        "ProjectExplorer.CloseProjectFilesFileMenu");
    cmd->setAttribute(Command::CA_UpdateText);
    cmd->setDescription(dd->m_closeProjectFilesActionFileMenu->text());
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
    dd->m_buildSessionAction = new QAction(buildIcon, tr("Build All Projects"), this);
    cmd = ActionManager::registerAction(dd->m_buildSessionAction, Constants::BUILDSESSION);
    cmd->setDefaultKeySequence(QKeySequence(tr("Ctrl+Shift+B")));
    mbuild->addAction(cmd, Constants::G_BUILD_BUILD);
    msessionContextMenu->addAction(cmd, Constants::G_SESSION_BUILD);

    dd->m_buildSessionForAllConfigsAction
            = new QAction(buildIcon, tr("Build All Projects for All Configurations"), this);
    cmd = ActionManager::registerAction(dd->m_buildSessionForAllConfigsAction,
                                        Constants::BUILDSESSIONALLCONFIGS);
    mbuild->addAction(cmd, Constants::G_BUILD_BUILD);
    msessionContextMenu->addAction(cmd, Constants::G_SESSION_BUILD);

    // deploy session
    dd->m_deploySessionAction = new QAction(tr("Deploy All Projects"), this);
    cmd = ActionManager::registerAction(dd->m_deploySessionAction, Constants::DEPLOYSESSION);
    mbuild->addAction(cmd, Constants::G_BUILD_DEPLOY);
    msessionContextMenu->addAction(cmd, Constants::G_SESSION_BUILD);

    // rebuild session action
    dd->m_rebuildSessionAction = new QAction(Icons::REBUILD.icon(), tr("Rebuild All Projects"),
                                             this);
    cmd = ActionManager::registerAction(dd->m_rebuildSessionAction, Constants::REBUILDSESSION);
    mbuild->addAction(cmd, Constants::G_BUILD_REBUILD);
    msessionContextMenu->addAction(cmd, Constants::G_SESSION_REBUILD);

    dd->m_rebuildSessionForAllConfigsAction
            = new QAction(Icons::REBUILD.icon(), tr("Rebuild All Projects for All Configurations"),
                          this);
    cmd = ActionManager::registerAction(dd->m_rebuildSessionForAllConfigsAction,
                                        Constants::REBUILDSESSIONALLCONFIGS);
    mbuild->addAction(cmd, Constants::G_BUILD_REBUILD);
    msessionContextMenu->addAction(cmd, Constants::G_SESSION_REBUILD);

    // clean session
    dd->m_cleanSessionAction = new QAction(Utils::Icons::CLEAN.icon(), tr("Clean All Projects"),
                                           this);
    cmd = ActionManager::registerAction(dd->m_cleanSessionAction, Constants::CLEANSESSION);
    mbuild->addAction(cmd, Constants::G_BUILD_CLEAN);
    msessionContextMenu->addAction(cmd, Constants::G_SESSION_REBUILD);

    dd->m_cleanSessionForAllConfigsAction = new QAction(Utils::Icons::CLEAN.icon(),
            tr("Clean All Projects for All Configurations"), this);
    cmd = ActionManager::registerAction(dd->m_cleanSessionForAllConfigsAction,
                                        Constants::CLEANSESSIONALLCONFIGS);
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

    dd->m_buildProjectForAllConfigsAction
            = new Utils::ParameterAction(tr("Build Project for All Configurations"),
                                         tr("Build Project \"%1\" for All Configurations"),
                                         Utils::ParameterAction::AlwaysEnabled, this);
    dd->m_buildProjectForAllConfigsAction->setIcon(buildIcon);
    cmd = ActionManager::registerAction(dd->m_buildProjectForAllConfigsAction,
                                        Constants::BUILDALLCONFIGS);
    cmd->setAttribute(Command::CA_UpdateText);
    cmd->setDescription(dd->m_buildProjectForAllConfigsAction->text());
    mbuild->addAction(cmd, Constants::G_BUILD_BUILD);

    // Add to mode bar
    dd->m_modeBarBuildAction = new Utils::ProxyAction(this);
    dd->m_modeBarBuildAction->setObjectName("Build"); // used for UI introduction
    dd->m_modeBarBuildAction->initialize(cmd->action());
    dd->m_modeBarBuildAction->setAttribute(Utils::ProxyAction::UpdateText);
    dd->m_modeBarBuildAction->setAction(cmd->action());
    ModeManager::addAction(dd->m_modeBarBuildAction, Constants::P_ACTION_BUILDPROJECT);

    // build for run config
    dd->m_buildForRunConfigAction = new Utils::ParameterAction(
                tr("Build for Run Configuration"), tr("Build for Run Configuration \"%1\""),
                Utils::ParameterAction::EnabledWithParameter, this);
    dd->m_buildForRunConfigAction->setIcon(buildIcon);
    cmd = ActionManager::registerAction(dd->m_buildForRunConfigAction,
                                        "ProjectExplorer.BuildForRunConfig");
    cmd->setAttribute(Command::CA_UpdateText);
    cmd->setDescription(dd->m_buildForRunConfigAction->text());
    mbuild->addAction(cmd, Constants::G_BUILD_BUILD);

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

    dd->m_rebuildProjectForAllConfigsAction
            = new Utils::ParameterAction(tr("Rebuild Project for All Configurations"),
                                         tr("Rebuild Project \"%1\" for All Configurations"),
                                         Utils::ParameterAction::AlwaysEnabled, this);
    cmd = ActionManager::registerAction(dd->m_rebuildProjectForAllConfigsAction,
                                        Constants::REBUILDALLCONFIGS);
    cmd->setAttribute(Command::CA_UpdateText);
    cmd->setDescription(dd->m_rebuildProjectForAllConfigsAction->text());
    mbuild->addAction(cmd, Constants::G_BUILD_REBUILD);

    // clean action
    dd->m_cleanAction = new Utils::ParameterAction(tr("Clean Project"), tr("Clean Project \"%1\""),
                                                     Utils::ParameterAction::AlwaysEnabled, this);
    cmd = ActionManager::registerAction(dd->m_cleanAction, Constants::CLEAN);
    cmd->setAttribute(Command::CA_UpdateText);
    cmd->setDescription(dd->m_cleanAction->text());
    mbuild->addAction(cmd, Constants::G_BUILD_CLEAN);

    dd->m_cleanProjectForAllConfigsAction
            = new Utils::ParameterAction(tr("Clean Project for All Configurations"),
                                         tr("Clean Project \"%1\" for All Configurations"),
                                         Utils::ParameterAction::AlwaysEnabled, this);
    cmd = ActionManager::registerAction(dd->m_cleanProjectForAllConfigsAction,
                                        Constants::CLEANALLCONFIGS);
    cmd->setAttribute(Command::CA_UpdateText);
    cmd->setDescription(dd->m_cleanProjectForAllConfigsAction->text());
    mbuild->addAction(cmd, Constants::G_BUILD_CLEAN);

    // cancel build action
    dd->m_cancelBuildAction = new QAction(Utils::Icons::STOP_SMALL.icon(), tr("Cancel Build"), this);
    cmd = ActionManager::registerAction(dd->m_cancelBuildAction, Constants::CANCELBUILD);
    cmd->setDefaultKeySequence(QKeySequence(useMacShortcuts ? tr("Meta+Backspace") : tr("Alt+Backspace")));
    mbuild->addAction(cmd, Constants::G_BUILD_CANCEL);

    // run action
    dd->m_runAction = new QAction(runIcon, tr("Run"), this);
    cmd = ActionManager::registerAction(dd->m_runAction, Constants::RUN);
    cmd->setAttribute(Command::CA_UpdateText);

    cmd->setDefaultKeySequence(QKeySequence(tr("Ctrl+R")));
    mbuild->addAction(cmd, Constants::G_BUILD_RUN);

    cmd->action()->setObjectName("Run"); // used for UI introduction
    ModeManager::addAction(cmd->action(), Constants::P_ACTION_RUN);

    // Run without deployment action
    dd->m_runWithoutDeployAction = new QAction(tr("Run Without Deployment"), this);
    cmd = ActionManager::registerAction(dd->m_runWithoutDeployAction, Constants::RUNWITHOUTDEPLOY);
    mbuild->addAction(cmd, Constants::G_BUILD_RUN);

    // build action with dependencies (context menu)
    dd->m_buildDependenciesActionContextMenu = new QAction(tr("Build"), this);
    cmd = ActionManager::registerAction(dd->m_buildDependenciesActionContextMenu, Constants::BUILDDEPENDCM, projectTreeContext);
    cmd->setAttribute(Command::CA_UpdateText);
    mprojectContextMenu->addAction(cmd, Constants::G_PROJECT_BUILD);

    // build action (context menu)
    dd->m_buildActionContextMenu = new QAction(tr("Build Without Dependencies"), this);
    cmd = ActionManager::registerAction(dd->m_buildActionContextMenu, Constants::BUILDCM, projectTreeContext);
    cmd->setAttribute(Command::CA_UpdateText);
    mprojectContextMenu->addAction(cmd, Constants::G_PROJECT_BUILD);

    // rebuild action with dependencies (context menu)
    dd->m_rebuildDependenciesActionContextMenu = new QAction(tr("Rebuild"), this);
    cmd = ActionManager::registerAction(dd->m_rebuildDependenciesActionContextMenu, Constants::REBUILDDEPENDCM, projectTreeContext);
    cmd->setAttribute(Command::CA_UpdateText);
    mprojectContextMenu->addAction(cmd, Constants::G_PROJECT_REBUILD);

    // rebuild action (context menu)
    dd->m_rebuildActionContextMenu = new QAction(tr("Rebuild Without Dependencies"), this);
    cmd = ActionManager::registerAction(dd->m_rebuildActionContextMenu, Constants::REBUILDCM, projectTreeContext);
    cmd->setAttribute(Command::CA_UpdateText);
    mprojectContextMenu->addAction(cmd, Constants::G_PROJECT_REBUILD);

    // clean action with dependencies (context menu)
    dd->m_cleanDependenciesActionContextMenu = new QAction(tr("Clean"), this);
    cmd = ActionManager::registerAction(dd->m_cleanDependenciesActionContextMenu, Constants::CLEANDEPENDCM, projectTreeContext);
    cmd->setAttribute(Command::CA_UpdateText);
    mprojectContextMenu->addAction(cmd, Constants::G_PROJECT_REBUILD);

    // clean action (context menu)
    dd->m_cleanActionContextMenu = new QAction(tr("Clean Without Dependencies"), this);
    cmd = ActionManager::registerAction(dd->m_cleanActionContextMenu, Constants::CLEANCM, projectTreeContext);
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
    cmd = ActionManager::registerAction(dd->m_deployActionContextMenu, Constants::DEPLOYCM, projectTreeContext);
    mprojectContextMenu->addAction(cmd, Constants::G_PROJECT_RUN);

    dd->m_runActionContextMenu = new QAction(runIcon, tr("Run"), this);
    cmd = ActionManager::registerAction(dd->m_runActionContextMenu, Constants::RUNCONTEXTMENU, projectTreeContext);
    mprojectContextMenu->addAction(cmd, Constants::G_PROJECT_RUN);
    msubProjectContextMenu->addAction(cmd, Constants::G_PROJECT_RUN);

    // add new file action
    dd->m_addNewFileAction = new QAction(this);
    cmd = ActionManager::registerAction(dd->m_addNewFileAction, Constants::ADDNEWFILE,
                       projectTreeContext);
    mprojectContextMenu->addAction(cmd, Constants::G_PROJECT_FILES);
    msubProjectContextMenu->addAction(cmd, Constants::G_PROJECT_FILES);
    mfolderContextMenu->addAction(cmd, Constants::G_FOLDER_FILES);

    // add existing file action
    dd->m_addExistingFilesAction = new QAction(tr("Add Existing Files..."), this);
    cmd = ActionManager::registerAction(dd->m_addExistingFilesAction, Constants::ADDEXISTINGFILES,
                       projectTreeContext);
    mprojectContextMenu->addAction(cmd, Constants::G_PROJECT_FILES);
    msubProjectContextMenu->addAction(cmd, Constants::G_PROJECT_FILES);
    mfolderContextMenu->addAction(cmd, Constants::G_FOLDER_FILES);

    // add existing projects action
    dd->m_addExistingProjectsAction = new QAction(tr("Add Existing Projects..."), this);
    cmd = ActionManager::registerAction(dd->m_addExistingProjectsAction,
                                        "ProjectExplorer.AddExistingProjects", projectTreeContext);
    mprojectContextMenu->addAction(cmd, Constants::G_PROJECT_FILES);
    msubProjectContextMenu->addAction(cmd, Constants::G_PROJECT_FILES);

    // add existing directory action
    dd->m_addExistingDirectoryAction = new QAction(tr("Add Existing Directory..."), this);
    cmd = ActionManager::registerAction(dd->m_addExistingDirectoryAction,
                                              Constants::ADDEXISTINGDIRECTORY,
                                              projectTreeContext);
    mprojectContextMenu->addAction(cmd, Constants::G_PROJECT_FILES);
    msubProjectContextMenu->addAction(cmd, Constants::G_PROJECT_FILES);
    mfolderContextMenu->addAction(cmd, Constants::G_FOLDER_FILES);

    // new subproject action
    dd->m_addNewSubprojectAction = new QAction(tr("New Subproject..."), this);
    cmd = ActionManager::registerAction(dd->m_addNewSubprojectAction, Constants::ADDNEWSUBPROJECT,
                       projectTreeContext);
    mprojectContextMenu->addAction(cmd, Constants::G_PROJECT_FILES);
    msubProjectContextMenu->addAction(cmd, Constants::G_PROJECT_FILES);

    dd->m_closeProjectFilesActionContextMenu = new Utils::ParameterAction(
                tr("Close All Files"), tr("Close All Files in Project \"%1\""),
                Utils::ParameterAction::EnabledWithParameter, this);
    cmd = ActionManager::registerAction(dd->m_closeProjectFilesActionContextMenu,
                                        "ProjectExplorer.CloseAllFilesInProjectContextMenu");
    cmd->setAttribute(Command::CA_UpdateText);
    cmd->setDescription(dd->m_closeProjectFilesActionContextMenu->text());
    mprojectContextMenu->addAction(cmd, Constants::G_PROJECT_LAST);

    // unload project again, in right position
    dd->m_unloadActionContextMenu = new Utils::ParameterAction(tr("Close Project"), tr("Close Project \"%1\""),
                                                              Utils::ParameterAction::EnabledWithParameter, this);
    cmd = ActionManager::registerAction(dd->m_unloadActionContextMenu, Constants::UNLOADCM);
    cmd->setAttribute(Command::CA_UpdateText);
    cmd->setDescription(dd->m_unloadActionContextMenu->text());
    mprojectContextMenu->addAction(cmd, Constants::G_PROJECT_LAST);

    // file properties action
    dd->m_filePropertiesAction = new QAction(tr("Properties..."), this);
    cmd = ActionManager::registerAction(dd->m_filePropertiesAction, Constants::FILEPROPERTIES,
                       projectTreeContext);
    mfileContextMenu->addAction(cmd, Constants::G_FILE_OTHER);

    // remove file action
    dd->m_removeFileAction = new QAction(this);
    cmd = ActionManager::registerAction(dd->m_removeFileAction, Constants::REMOVEFILE,
                       projectTreeContext);
    cmd->setDefaultKeySequences({QKeySequence::Delete, QKeySequence::Backspace});
    mfileContextMenu->addAction(cmd, Constants::G_FILE_OTHER);

    // duplicate file action
    dd->m_duplicateFileAction = new QAction(tr("Duplicate File..."), this);
    cmd = ActionManager::registerAction(dd->m_duplicateFileAction, Constants::DUPLICATEFILE,
                       projectTreeContext);
    mfileContextMenu->addAction(cmd, Constants::G_FILE_OTHER);

    //: Remove project from parent profile (Project explorer view); will not physically delete any files.
    dd->m_removeProjectAction = new QAction(tr("Remove Project..."), this);
    cmd = ActionManager::registerAction(dd->m_removeProjectAction, Constants::REMOVEPROJECT,
                       projectTreeContext);
    msubProjectContextMenu->addAction(cmd, Constants::G_PROJECT_FILES);

    // delete file action
    dd->m_deleteFileAction = new QAction(tr("Delete File..."), this);
    cmd = ActionManager::registerAction(dd->m_deleteFileAction, Constants::DELETEFILE,
                             projectTreeContext);
    cmd->setDefaultKeySequences({QKeySequence::Delete, QKeySequence::Backspace});
    mfileContextMenu->addAction(cmd, Constants::G_FILE_OTHER);

    // renamefile action
    dd->m_renameFileAction = new QAction(this);
    cmd = ActionManager::registerAction(dd->m_renameFileAction, Constants::RENAMEFILE,
                       projectTreeContext);
    mfileContextMenu->addAction(cmd, Constants::G_FILE_OTHER);

    // diff file action
    dd->m_diffFileAction = TextEditor::TextDocument::createDiffAgainstCurrentFileAction(
        this, &ProjectTree::currentFilePath);
    cmd = ActionManager::registerAction(dd->m_diffFileAction, Constants::DIFFFILE, projectTreeContext);
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
                             projectTreeContext);
    cmd->setAttribute(Command::CA_UpdateText);
    cmd->setDescription(dd->m_setStartupProjectAction->text());
    mprojectContextMenu->addAction(cmd, Constants::G_PROJECT_FIRST);

    // Collapse & Expand.
    const Id treeGroup = Constants::G_PROJECT_TREE;

    dd->m_projectTreeExpandNodeAction = new QAction(tr("Expand"), this);
    connect(dd->m_projectTreeExpandNodeAction, &QAction::triggered,
            ProjectTree::instance(), &ProjectTree::expandCurrentNodeRecursively);
    Command * const expandNodeCmd = ActionManager::registerAction(
                dd->m_projectTreeExpandNodeAction, "ProjectExplorer.ExpandNode",
                projectTreeContext);
    dd->m_projectTreeCollapseAllAction = new QAction(tr("Collapse All"), this);
    Command * const collapseCmd = ActionManager::registerAction(
                dd->m_projectTreeCollapseAllAction, Constants::PROJECTTREE_COLLAPSE_ALL,
                projectTreeContext);
    dd->m_projectTreeExpandAllAction = new QAction(tr("Expand All"), this);
    Command * const expandCmd = ActionManager::registerAction(
                dd->m_projectTreeExpandAllAction, Constants::PROJECTTREE_EXPAND_ALL,
                projectTreeContext);
    for (Core::ActionContainer * const ac : {mfileContextMenu, msubProjectContextMenu,
         mfolderContextMenu, mprojectContextMenu, msessionContextMenu}) {
        ac->addSeparator(treeGroup);
        ac->addAction(expandNodeCmd, treeGroup);
        ac->addAction(collapseCmd, treeGroup);
        ac->addAction(expandCmd, treeGroup);
    }

    // target selector
    dd->m_projectSelectorAction = new QAction(this);
    dd->m_projectSelectorAction->setObjectName("KitSelector"); // used for UI introduction
    dd->m_projectSelectorAction->setCheckable(true);
    dd->m_projectSelectorAction->setEnabled(false);
    dd->m_targetSelector = new MiniProjectTargetSelector(dd->m_projectSelectorAction, ICore::dialogParent());
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

    const QVariant buildBeforeDeploy = s->value(Constants::BUILD_BEFORE_DEPLOY_SETTINGS_KEY);
    const QString buildBeforeDeployString = buildBeforeDeploy.toString();
    if (buildBeforeDeployString == "true") { // backward compatibility with QtC < 4.12
        dd->m_projectExplorerSettings.buildBeforeDeploy = BuildBeforeRunMode::WholeProject;
    } else if (buildBeforeDeployString == "false") {
        dd->m_projectExplorerSettings.buildBeforeDeploy = BuildBeforeRunMode::Off;
    } else if (buildBeforeDeploy.isValid()) {
        dd->m_projectExplorerSettings.buildBeforeDeploy
                = static_cast<BuildBeforeRunMode>(buildBeforeDeploy.toInt());
    }

    dd->m_projectExplorerSettings.deployBeforeRun =
            s->value(Constants::DEPLOY_BEFORE_RUN_SETTINGS_KEY, true).toBool();
    dd->m_projectExplorerSettings.saveBeforeBuild =
            s->value(Constants::SAVE_BEFORE_BUILD_SETTINGS_KEY, false).toBool();
    dd->m_projectExplorerSettings.useJom = s->value(Constants::USE_JOM_SETTINGS_KEY, true).toBool();
    dd->m_projectExplorerSettings.autorestoreLastSession =
            s->value(Constants::AUTO_RESTORE_SESSION_SETTINGS_KEY, false).toBool();
    dd->m_projectExplorerSettings.addLibraryPathsToRunEnv =
            s->value(Constants::ADD_LIBRARY_PATHS_TO_RUN_ENV_SETTINGS_KEY, true).toBool();
    dd->m_projectExplorerSettings.prompToStopRunControl =
            s->value(Constants::PROMPT_TO_STOP_RUN_CONTROL_SETTINGS_KEY, false).toBool();
    dd->m_projectExplorerSettings.automaticallyCreateRunConfigurations =
            s->value(Constants::AUTO_CREATE_RUN_CONFIGS_SETTINGS_KEY, true).toBool();
    dd->m_projectExplorerSettings.environmentId =
            QUuid(s->value(Constants::ENVIRONMENT_ID_SETTINGS_KEY).toByteArray());
    if (dd->m_projectExplorerSettings.environmentId.isNull())
        dd->m_projectExplorerSettings.environmentId = QUuid::createUuid();
    int tmp = s->value(Constants::STOP_BEFORE_BUILD_SETTINGS_KEY,
                            Utils::HostOsInfo::isWindowsHost() ? 1 : 0).toInt();
    if (tmp < 0 || tmp > int(StopBeforeBuild::SameApp))
        tmp = Utils::HostOsInfo::isWindowsHost() ? 1 : 0;
    dd->m_projectExplorerSettings.stopBeforeBuild = StopBeforeBuild(tmp);
    dd->m_projectExplorerSettings.terminalMode = static_cast<TerminalMode>(s->value(
        Constants::TERMINAL_MODE_SETTINGS_KEY, int(TerminalMode::Off)).toInt());
    dd->m_projectExplorerSettings.closeSourceFilesWithProject
            = s->value(Constants::CLOSE_FILES_WITH_PROJECT_SETTINGS_KEY, true).toBool();
    dd->m_projectExplorerSettings.clearIssuesOnRebuild
            = s->value(Constants::CLEAR_ISSUES_ON_REBUILD_SETTINGS_KEY, true).toBool();
    dd->m_projectExplorerSettings.abortBuildAllOnError
            = s->value(Constants::ABORT_BUILD_ALL_ON_ERROR_SETTINGS_KEY, true).toBool();
    dd->m_projectExplorerSettings.lowBuildPriority
            = s->value(Constants::LOW_BUILD_PRIORITY_SETTINGS_KEY, false).toBool();
    dd->m_buildPropertiesSettings.buildDirectoryTemplate
            = s->value(Constants::DEFAULT_BUILD_DIRECTORY_TEMPLATE_KEY).toString();
    if (dd->m_buildPropertiesSettings.buildDirectoryTemplate.isEmpty())
        dd->m_buildPropertiesSettings.buildDirectoryTemplate = Constants::DEFAULT_BUILD_DIRECTORY_TEMPLATE;

    const auto loadTriStateValue = [&s](const QString &key) {
      return TriState::fromVariant(s->value(key, TriState::Default.toVariant()));
    };
    dd->m_buildPropertiesSettings.separateDebugInfo
            = loadTriStateValue(Constants::SEPARATE_DEBUG_INFO_SETTINGS_KEY);
    dd->m_buildPropertiesSettings.qmlDebugging
            = loadTriStateValue(Constants::QML_DEBUGGING_SETTINGS_KEY);
    dd->m_buildPropertiesSettings.qtQuickCompiler
            = loadTriStateValue(Constants::QT_QUICK_COMPILER_SETTINGS_KEY);

    const int customParserCount = s->value(Constants::CUSTOM_PARSER_COUNT_KEY).toInt();
    for (int i = 0; i < customParserCount; ++i) {
        CustomParserSettings settings;
        settings.fromMap(s->value(Constants::CUSTOM_PARSER_PREFIX_KEY
                                  + QString::number(i)).toMap());
        dd->m_customParsers << settings;
    }

    auto buildManager = new BuildManager(this, dd->m_cancelBuildAction);
    connect(buildManager, &BuildManager::buildStateChanged,
            dd, &ProjectExplorerPluginPrivate::updateActions);
    connect(buildManager, &BuildManager::buildQueueFinished,
            dd, &ProjectExplorerPluginPrivate::buildQueueFinished, Qt::QueuedConnection);

    connect(dd->m_sessionManagerAction, &QAction::triggered,
            dd, &ProjectExplorerPluginPrivate::showSessionManager);
    connect(dd->m_newAction, &QAction::triggered,
            dd, &ProjectExplorerPlugin::openNewProjectDialog);
    connect(dd->m_loadAction, &QAction::triggered,
            dd, &ProjectExplorerPluginPrivate::loadAction);
    connect(dd->m_buildProjectOnlyAction, &QAction::triggered, dd, [] {
        BuildManager::buildProjectWithoutDependencies(SessionManager::startupProject());
    });
    connect(dd->m_buildAction, &QAction::triggered, dd, [] {
        BuildManager::buildProjectWithDependencies(SessionManager::startupProject());
    });
    connect(dd->m_buildProjectForAllConfigsAction, &QAction::triggered, dd, [] {
        BuildManager::buildProjectWithDependencies(SessionManager::startupProject(),
                                                   ConfigSelection::All);
    });
    connect(dd->m_buildActionContextMenu, &QAction::triggered, dd, [] {
        BuildManager::buildProjectWithoutDependencies(ProjectTree::currentProject());
    });
    connect(dd->m_buildForRunConfigAction, &QAction::triggered, dd, [] {
        const Project * const project = SessionManager::startupProject();
        QTC_ASSERT(project, return);
        const Target * const target = project->activeTarget();
        QTC_ASSERT(target, return);
        const RunConfiguration * const runConfig = target->activeRunConfiguration();
        QTC_ASSERT(runConfig, return);
        ProjectNode * const productNode = runConfig->productNode();
        QTC_ASSERT(productNode, return);
        QTC_ASSERT(productNode->isProduct(), return);
        productNode->build();
    });
    connect(dd->m_buildDependenciesActionContextMenu, &QAction::triggered, dd, [] {
        BuildManager::buildProjectWithDependencies(ProjectTree::currentProject());
    });
    connect(dd->m_buildSessionAction, &QAction::triggered, dd, [] {
        BuildManager::buildProjects(SessionManager::projectOrder(), ConfigSelection::Active);
    });
    connect(dd->m_buildSessionForAllConfigsAction, &QAction::triggered, dd, [] {
        BuildManager::buildProjects(SessionManager::projectOrder(), ConfigSelection::All);
    });
    connect(dd->m_rebuildProjectOnlyAction, &QAction::triggered, dd, [] {
        BuildManager::rebuildProjectWithoutDependencies(SessionManager::startupProject());
    });
    connect(dd->m_rebuildAction, &QAction::triggered, dd, [] {
        BuildManager::rebuildProjectWithDependencies(SessionManager::startupProject(),
                                                     ConfigSelection::Active);
    });
    connect(dd->m_rebuildProjectForAllConfigsAction, &QAction::triggered, dd, [] {
        BuildManager::rebuildProjectWithDependencies(SessionManager::startupProject(),
                                                     ConfigSelection::All);
    });
    connect(dd->m_rebuildActionContextMenu, &QAction::triggered, dd, [] {
        BuildManager::rebuildProjectWithoutDependencies(ProjectTree::currentProject());
    });
    connect(dd->m_rebuildDependenciesActionContextMenu, &QAction::triggered, dd, [] {
        BuildManager::rebuildProjectWithDependencies(ProjectTree::currentProject(),
                                                     ConfigSelection::Active);
    });
    connect(dd->m_rebuildSessionAction, &QAction::triggered, dd, [] {
        BuildManager::rebuildProjects(SessionManager::projectOrder(), ConfigSelection::Active);
    });
    connect(dd->m_rebuildSessionForAllConfigsAction, &QAction::triggered, dd, [] {
        BuildManager::rebuildProjects(SessionManager::projectOrder(), ConfigSelection::All);
    });
    connect(dd->m_deployProjectOnlyAction, &QAction::triggered, dd, [] {
        BuildManager::deployProjects({SessionManager::startupProject()});
    });
    connect(dd->m_deployAction, &QAction::triggered, dd, [] {
        BuildManager::deployProjects(SessionManager::projectOrder(SessionManager::startupProject()));
    });
    connect(dd->m_deployActionContextMenu, &QAction::triggered, dd, [] {
        BuildManager::deployProjects({ProjectTree::currentProject()});
    });
    connect(dd->m_deploySessionAction, &QAction::triggered, dd, [] {
        BuildManager::deployProjects(SessionManager::projectOrder());
    });
    connect(dd->m_cleanProjectOnlyAction, &QAction::triggered, dd, [] {
        BuildManager::cleanProjectWithoutDependencies(SessionManager::startupProject());
    });
    connect(dd->m_cleanAction, &QAction::triggered, dd, [] {
        BuildManager::cleanProjectWithDependencies(SessionManager::startupProject(),
                                                   ConfigSelection::Active);
    });
    connect(dd->m_cleanProjectForAllConfigsAction, &QAction::triggered, dd, [] {
        BuildManager::cleanProjectWithDependencies(SessionManager::startupProject(),
                                                   ConfigSelection::All);
    });
    connect(dd->m_cleanActionContextMenu, &QAction::triggered, dd, [] {
        BuildManager::cleanProjectWithoutDependencies(ProjectTree::currentProject());
    });
    connect(dd->m_cleanDependenciesActionContextMenu, &QAction::triggered, dd, [] {
        BuildManager::cleanProjectWithDependencies(ProjectTree::currentProject(),
                                                   ConfigSelection::Active);
    });
    connect(dd->m_cleanSessionAction, &QAction::triggered, dd, [] {
        BuildManager::cleanProjects(SessionManager::projectOrder(), ConfigSelection::Active);
    });
    connect(dd->m_cleanSessionForAllConfigsAction, &QAction::triggered, dd, [] {
        BuildManager::cleanProjects(SessionManager::projectOrder(), ConfigSelection::All);
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
    connect(dd->m_addExistingProjectsAction, &QAction::triggered,
            dd, &ProjectExplorerPluginPrivate::addExistingProjects);
    connect(dd->m_removeProjectAction, &QAction::triggered,
            dd, &ProjectExplorerPluginPrivate::removeProject);
    connect(dd->m_openFileAction, &QAction::triggered,
            dd, &ProjectExplorerPluginPrivate::openFile);
    connect(dd->m_searchOnFileSystem, &QAction::triggered,
            dd, &ProjectExplorerPluginPrivate::searchOnFileSystem);
    connect(dd->m_showInGraphicalShell, &QAction::triggered,
            dd, &ProjectExplorerPluginPrivate::showInGraphicalShell);

    connect(dd->m_openTerminalHere, &QAction::triggered, dd, []() { dd->openTerminalHere(sysEnv); });
    connect(dd->m_openTerminalHereBuildEnv, &QAction::triggered, dd, []() { dd->openTerminalHere(buildEnv); });
    connect(dd->m_openTerminalHereRunEnv, &QAction::triggered, dd, []() { dd->openTerminalHereWithRunEnv(); });

    connect(dd->m_filePropertiesAction, &QAction::triggered, this, []() {
                const Node *currentNode = ProjectTree::currentNode();
                QTC_ASSERT(currentNode && currentNode->asFileNode(), return);
                DocumentManager::showFilePropertiesDialog(currentNode->filePath());
            });
    connect(dd->m_removeFileAction, &QAction::triggered,
            dd, &ProjectExplorerPluginPrivate::removeFile);
    connect(dd->m_duplicateFileAction, &QAction::triggered,
            dd, &ProjectExplorerPluginPrivate::duplicateFile);
    connect(dd->m_deleteFileAction, &QAction::triggered,
            dd, &ProjectExplorerPluginPrivate::deleteFile);
    connect(dd->m_renameFileAction, &QAction::triggered,
            dd, &ProjectExplorerPluginPrivate::handleRenameFile);
    connect(dd->m_setStartupProjectAction, &QAction::triggered,
            dd, &ProjectExplorerPluginPrivate::handleSetStartupProject);
    connect(dd->m_closeProjectFilesActionFileMenu, &QAction::triggered,
            dd, [] { dd->closeAllFilesInProject(SessionManager::projects().first()); });
    connect(dd->m_closeProjectFilesActionContextMenu, &QAction::triggered,
            dd, [] { dd->closeAllFilesInProject(ProjectTree::currentProject()); });
    connect(dd->m_projectTreeCollapseAllAction, &QAction::triggered,
            ProjectTree::instance(), &ProjectTree::collapseAll);
    connect(dd->m_projectTreeExpandAllAction, &QAction::triggered,
            ProjectTree::instance(), &ProjectTree::expandAll);

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
            Utils::FilePath projectFilePath;
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
            const IDevice::ConstPtr device = DeviceKitAspect::device(kit);
            return device ? device->sshParameters().host() : QString();
        });

    expander->registerVariable("CurrentDevice:SshPort",
        tr("The SSH port of the device in the currently active kit."),
        []() -> QString {
            Kit *kit = currentKit();
            const IDevice::ConstPtr device = DeviceKitAspect::device(kit);
            return device ? QString::number(device->sshParameters().port()) : QString();
        });

    expander->registerVariable("CurrentDevice:UserName",
        tr("The username with which to log into the device in the currently active kit."),
        []() -> QString {
            Kit *kit = currentKit();
            const IDevice::ConstPtr device = DeviceKitAspect::device(kit);
            return device ? device->sshParameters().userName() : QString();
        });


    expander->registerVariable("CurrentDevice:PrivateKeyFile",
        tr("The private key file with which to authenticate when logging into the device "
           "in the currently active kit."),
        []() -> QString {
            Kit *kit = currentKit();
            const IDevice::ConstPtr device = DeviceKitAspect::device(kit);
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
                if (RunConfiguration *rc = target->activeRunConfiguration())
                    return rc->commandLine().executable().toString();
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

    expander->registerPrefix(Constants::VAR_CURRENTBUILD_ENV,
                             BuildConfiguration::tr("Variables in the current build environment"),
                             [](const QString &var) {
                                 if (BuildConfiguration *bc = activeBuildConfiguration())
                                     return bc->environment().expandedValueForKey(var);
                                 return QString();
                             });

    Utils::EnvironmentProvider::addProvider(
        {Constants::VAR_CURRENTBUILD_ENV, tr("Current Build Environment"), []() {
             if (BuildConfiguration *bc = activeBuildConfiguration())
                 return bc->environment();
             return Utils::Environment::systemEnvironment();
         }});

    Utils::EnvironmentProvider::addProvider(
        {"CurrentRun:Env", tr("Current Run Environment"), []() {
             if (RunConfiguration *rc = activeRunConfiguration())
                if (auto envAspect = rc->aspect<EnvironmentAspect>())
                    return envAspect->environment();
             return Utils::Environment::systemEnvironment();
         }});

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

    if (projectExplorerSettings().closeSourceFilesWithProject && !dd->closeAllFilesInProject(project))
        return;

    dd->addToRecentProjects(project->projectFilePath().toString(), project->displayName());

    SessionManager::removeProject(project);
    dd->updateActions();
}

void ProjectExplorerPluginPrivate::closeAllProjects()
{
    if (!EditorManager::closeAllDocuments())
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

    dd->m_documentFactory.setOpener([](QString fileName) {
        const QFileInfo fi(fileName);
        if (fi.isDir())
            fileName = FolderNavigationWidget::projectFilesInDirectory(fi.absoluteFilePath()).value(0, fileName);

        OpenProjectResult result = ProjectExplorerPlugin::openProject(fileName);
        if (!result)
            showOpenProjectError(result);
        return nullptr;
    });

    dd->m_documentFactory.addMimeType(QStringLiteral("inode/directory"));
    for (const QString &mimeType : dd->m_projectCreators.keys()) {
        dd->m_documentFactory.addMimeType(mimeType);
        Utils::MimeType mime = Utils::mimeTypeForName(mimeType);
        allGlobPatterns.append(mime.globPatterns());
        filterStrings.append(mime.filterString());
        dd->m_profileMimeTypes += mimeType;
    }

    QString allProjectsFilter = tr("All Projects");
    allProjectsFilter += QLatin1String(" (") + allGlobPatterns.join(QLatin1Char(' '))
            + QLatin1Char(')');
    filterStrings.prepend(allProjectsFilter);
    dd->m_projectFilterString = filterStrings.join(filterSeparator);

    BuildManager::extensionsInitialized();

    DeviceManager::instance()->addDevice(IDevice::Ptr(new DesktopDevice));

    QSsh::SshSettings::loadSettings(Core::ICore::settings());
    const auto searchPathRetriever = [] {
        Utils::FilePaths searchPaths;
        searchPaths << Utils::FilePath::fromString(Core::ICore::libexecPath());
        if (Utils::HostOsInfo::isWindowsHost()) {
            const QString gitBinary = Core::ICore::settings()->value("Git/BinaryPath", "git")
                    .toString();
            const QStringList rawGitSearchPaths = Core::ICore::settings()->value("Git/Path")
                    .toString().split(':', Qt::SkipEmptyParts);
            const Utils::FilePaths gitSearchPaths = Utils::transform(rawGitSearchPaths,
                    [](const QString &rawPath) { return Utils::FilePath::fromString(rawPath); });
            const Utils::FilePath fullGitPath = Utils::Environment::systemEnvironment()
                    .searchInPath(gitBinary, gitSearchPaths);
            if (!fullGitPath.isEmpty()) {
                searchPaths << fullGitPath.parentDir()
                            << fullGitPath.parentDir().parentDir() + "/usr/bin";
            }
        }
        return searchPaths;
    };
    QSsh::SshSettings::setExtraSearchPathRetriever(searchPathRetriever);

    const auto parseIssuesAction = new QAction(tr("Parse Build Output..."), this);
    ActionContainer *mtools = ActionManager::actionContainer(Core::Constants::M_TOOLS);
    Command * const cmd = ActionManager::registerAction(parseIssuesAction,
                                                        "ProjectExplorer.ParseIssuesAction");
    connect(parseIssuesAction, &QAction::triggered, this, [] {
        ParseIssuesDialog dlg(ICore::dialogParent());
        dlg.exec();
    });
    mtools->addAction(cmd);

    // delay restoring kits until UI is shown for improved perceived startup performance
    QTimer::singleShot(0, this, &ProjectExplorerPlugin::restoreKits);
}

void ProjectExplorerPlugin::restoreKits()
{
    dd->determineSessionToRestoreAtStartup();
    ExtraAbi::load(); // Load this before Toolchains!
    DeviceManager::instance()->load();
    ToolChainManager::restoreToolChains();
    KitManager::restoreKits();
    QTimer::singleShot(0, dd, &ProjectExplorerPluginPrivate::restoreSession); // delay a bit...
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
    ToolChainManager::aboutToShutdown();
    SessionManager::closeAllProjects();

    dd->m_shuttingDown = true;

    // Attempt to synchronously shutdown all run controls.
    // If that fails, fall back to asynchronous shutdown (Debugger run controls
    // might shutdown asynchronously).
    if (dd->m_activeRunControlCount == 0)
        return SynchronousShutdown;

    dd->m_outputPane.closeTabs(AppOutputPane::CloseTabNoPrompt /* No prompt any more */);
    dd->m_shutdownWatchDogId = dd->startTimer(10 * 1000); // Make sure we shutdown *somehow*
    return AsynchronousShutdown;
}

void ProjectExplorerPlugin::showSessionManager()
{
    dd->showSessionManager();
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
    SessionDialog sessionDialog(ICore::dialogParent());
    sessionDialog.setAutoLoadSession(dd->m_projectExplorerSettings.autorestoreLastSession);
    sessionDialog.exec();
    dd->m_projectExplorerSettings.autorestoreLastSession = sessionDialog.autoLoadSession();

    updateActions();

    if (ModeManager::currentModeId() == Core::Constants::MODE_WELCOME)
        updateWelcomePage();
}

void ProjectExplorerPluginPrivate::setStartupProject(Project *project)
{
    if (!project)
        return;
    SessionManager::setStartupProject(project);
    updateActions();
}

bool ProjectExplorerPluginPrivate::closeAllFilesInProject(const Project *project)
{
    QTC_ASSERT(project, return false);
    QList<DocumentModel::Entry *> openFiles = DocumentModel::entries();
    Utils::erase(openFiles, [project](const DocumentModel::Entry *entry) {
        return entry->pinned || !project->isKnownFile(entry->fileName());
    });
    for (const Project * const otherProject : SessionManager::projects()) {
        if (otherProject == project)
            continue;
        Utils::erase(openFiles, [otherProject](const DocumentModel::Entry *entry) {
            return otherProject->isKnownFile(entry->fileName());
        });
    }
    return EditorManager::closeDocuments(openFiles);
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
    if (SessionManager::isDefaultVirgin()) {
        s->remove(Constants::STARTUPSESSION_KEY);
    } else {
        s->setValue(Constants::STARTUPSESSION_KEY, SessionManager::activeSession());
        s->setValue(Constants::LASTSESSION_KEY, SessionManager::activeSession());
    }
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

    s->setValue(Constants::BUILD_BEFORE_DEPLOY_SETTINGS_KEY, int(dd->m_projectExplorerSettings.buildBeforeDeploy));
    s->setValue(Constants::DEPLOY_BEFORE_RUN_SETTINGS_KEY, dd->m_projectExplorerSettings.deployBeforeRun);
    s->setValue(Constants::SAVE_BEFORE_BUILD_SETTINGS_KEY, dd->m_projectExplorerSettings.saveBeforeBuild);
    s->setValue(Constants::USE_JOM_SETTINGS_KEY, dd->m_projectExplorerSettings.useJom);
    s->setValue(Constants::AUTO_RESTORE_SESSION_SETTINGS_KEY, dd->m_projectExplorerSettings.autorestoreLastSession);
    s->setValue(Constants::ADD_LIBRARY_PATHS_TO_RUN_ENV_SETTINGS_KEY, dd->m_projectExplorerSettings.addLibraryPathsToRunEnv);
    s->setValue(Constants::PROMPT_TO_STOP_RUN_CONTROL_SETTINGS_KEY, dd->m_projectExplorerSettings.prompToStopRunControl);
    s->setValue(Constants::TERMINAL_MODE_SETTINGS_KEY,
                int(dd->m_projectExplorerSettings.terminalMode));
    s->setValue(Constants::CLOSE_FILES_WITH_PROJECT_SETTINGS_KEY,
                dd->m_projectExplorerSettings.closeSourceFilesWithProject);
    s->setValue(Constants::CLEAR_ISSUES_ON_REBUILD_SETTINGS_KEY,
                dd->m_projectExplorerSettings.clearIssuesOnRebuild);
    s->setValue(Constants::ABORT_BUILD_ALL_ON_ERROR_SETTINGS_KEY,
                dd->m_projectExplorerSettings.abortBuildAllOnError);
    s->setValue(Constants::LOW_BUILD_PRIORITY_SETTINGS_KEY,
                dd->m_projectExplorerSettings.lowBuildPriority);
    s->setValue(Constants::AUTO_CREATE_RUN_CONFIGS_SETTINGS_KEY,
                dd->m_projectExplorerSettings.automaticallyCreateRunConfigurations);
    s->setValue(Constants::ENVIRONMENT_ID_SETTINGS_KEY, dd->m_projectExplorerSettings.environmentId.toByteArray());
    s->setValue(Constants::STOP_BEFORE_BUILD_SETTINGS_KEY, int(dd->m_projectExplorerSettings.stopBeforeBuild));

    // Store this in the Core directory scope for backward compatibility!
    s->setValue(Constants::DEFAULT_BUILD_DIRECTORY_TEMPLATE_KEY,
                dd->m_buildPropertiesSettings.buildDirectoryTemplate);

    s->setValue(Constants::SEPARATE_DEBUG_INFO_SETTINGS_KEY,
                dd->m_buildPropertiesSettings.separateDebugInfo.toVariant());
    s->setValue(Constants::QML_DEBUGGING_SETTINGS_KEY,
                dd->m_buildPropertiesSettings.qmlDebugging.toVariant());
    s->setValue(Constants::QT_QUICK_COMPILER_SETTINGS_KEY,
                dd->m_buildPropertiesSettings.qtQuickCompiler.toVariant());

    s->setValue(Constants::CUSTOM_PARSER_COUNT_KEY, dd->m_customParsers.count());
    for (int i = 0; i < dd->m_customParsers.count(); ++i) {
        s->setValue(Constants::CUSTOM_PARSER_PREFIX_KEY + QString::number(i),
                    dd->m_customParsers.at(i).toMap());
    }
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
        QMessageBox::critical(ICore::dialogParent(), tr("Failed to Open Project"), errorMessage);
    } else {
        // ignore multiple alreadyOpen
        Project *alreadyOpen = result.alreadyOpen().constFirst();
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
        const auto filePath = Utils::FilePath::fromString(fi.absoluteFilePath());
        Project *found = Utils::findOrDefault(SessionManager::projects(),
                                              Utils::equal(&Project::projectFilePath, filePath));
        if (found) {
            alreadyOpen.append(found);
            SessionManager::reportProjectLoadingProgress();
            continue;
        }

        Utils::MimeType mt = Utils::mimeTypeForFile(fileName);
        if (ProjectManager::canOpenProjectForMimeType(mt)) {
            if (!filePath.toFileInfo().isFile()) {
                appendError(errorString,
                            tr("Failed opening project \"%1\": Project is not a file.").arg(fileName));
            } else if (Project *pro = ProjectManager::openProject(mt, filePath)) {
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
    m_welcomePage.reloadWelcomeScreenData();
}

void ProjectExplorerPluginPrivate::currentModeChanged(Id mode, Id oldMode)
{
    if (oldMode == Constants::MODE_SESSION) {
        // Saving settings directly in a mode change is not a good idea, since the mode change
        // can be part of a bigger change. Save settings after that bigger change had a chance to
        // complete.
        QTimer::singleShot(0, ICore::instance(), [] { ICore::saveSettings(ICore::ModeChanged); });
    }
    if (mode == Core::Constants::MODE_WELCOME)
        updateWelcomePage();
}

void ProjectExplorerPluginPrivate::determineSessionToRestoreAtStartup()
{
    // Process command line arguments first:
    const bool lastSessionArg = m_instance->pluginSpec()->arguments().contains("-lastsession");
    m_sessionToRestoreAtStartup = lastSessionArg ? SessionManager::startupSession() : QString();
    QStringList arguments = ExtensionSystem::PluginManager::arguments();
    if (!lastSessionArg) {
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
    if (m_sessionToRestoreAtStartup.isEmpty() && m_projectExplorerSettings.autorestoreLastSession)
        m_sessionToRestoreAtStartup = SessionManager::startupSession();

    if (!m_sessionToRestoreAtStartup.isEmpty())
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

QThreadPool *ProjectExplorerPlugin::sharedThreadPool()
{
    return &(dd->m_threadPool);
}

MiniProjectTargetSelector *ProjectExplorerPlugin::targetSelector()
{
    return dd->m_targetSelector;
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

    SessionManager::loadSession(!dd->m_sessionToRestoreAtStartup.isEmpty()
                                ? dd->m_sessionToRestoreAtStartup : QString(), true);

    // update welcome page
    connect(ModeManager::instance(), &ModeManager::currentModeChanged,
            dd, &ProjectExplorerPluginPrivate::currentModeChanged);
    connect(&dd->m_welcomePage, &ProjectWelcomePage::requestProject,
            m_instance, &ProjectExplorerPlugin::openProjectWelcomePage);
    dd->m_arguments = arguments;
    // delay opening projects from the command line even more
    QTimer::singleShot(0, m_instance, []() {
        ICore::openFiles(dd->m_arguments, ICore::OpenFilesFlags(ICore::CanContainLineAndColumnNumbers | ICore::SwitchMode));
        emit m_instance->finishedInitialization();
    });
    updateActions();
}

void ProjectExplorerPluginPrivate::executeRunConfiguration(RunConfiguration *runConfiguration, Utils::Id runMode)
{
    const Tasks runConfigIssues = runConfiguration->checkForIssues();
    if (!runConfigIssues.isEmpty()) {
        for (const Task &t : runConfigIssues)
            TaskHub::addTask(t);
        // TODO: Insert an extra task with a "link" to the run settings page?
        TaskHub::requestPopup();
        return;
    }

    auto runControl = new RunControl(runMode);
    runControl->setRunConfiguration(runConfiguration);

    // A user needed interaction may have cancelled the run
    // (by example asking for a process pid or server url).
    if (!runControl->createMainWorker()) {
        delete runControl;
        return;
    }

    startRunControl(runControl);
}

void ProjectExplorerPlugin::startRunControl(RunControl *runControl)
{
    dd->startRunControl(runControl);
}

void ProjectExplorerPlugin::showOutputPaneForRunControl(RunControl *runControl)
{
    dd->showOutputPaneForRunControl(runControl);
}

void ProjectExplorerPluginPrivate::startRunControl(RunControl *runControl)
{
    m_outputPane.createNewOutputWindow(runControl);
    m_outputPane.flash(); // one flash for starting
    m_outputPane.showTabFor(runControl);
    Utils::Id runMode = runControl->runMode();
    const auto popupMode = runMode == Constants::NORMAL_RUN_MODE
            ? m_outputPane.settings().runOutputMode
            : runMode == Constants::DEBUG_RUN_MODE
                ? m_outputPane.settings().debugOutputMode
                : AppOutputPaneMode::FlashOnOutput;
    m_outputPane.setBehaviorOnOutput(runControl, popupMode);
    connect(runControl, &QObject::destroyed, this, &ProjectExplorerPluginPrivate::checkForShutdown,
            Qt::QueuedConnection);
    ++m_activeRunControlCount;
    runControl->initiateStart();
    doUpdateRunActions();
}

void ProjectExplorerPluginPrivate::showOutputPaneForRunControl(RunControl *runControl)
{
    m_outputPane.showTabFor(runControl);
    m_outputPane.popup(IOutputPane::NoModeSwitch | IOutputPane::WithFocus);
}

void ProjectExplorerPluginPrivate::checkForShutdown()
{
    --m_activeRunControlCount;
    QTC_ASSERT(m_activeRunControlCount >= 0, m_activeRunControlCount = 0);
    if (m_shuttingDown && m_activeRunControlCount == 0)
        emit m_instance->asynchronousShutdownFinished();
}

void ProjectExplorerPluginPrivate::timerEvent(QTimerEvent *ev)
{
    if (m_shutdownWatchDogId == ev->timerId())
        emit m_instance->asynchronousShutdownFinished();
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
    doUpdateRunActions();
}

QList<QPair<QString, QString> > ProjectExplorerPluginPrivate::recentProjects() const
{
    return Utils::filtered(dd->m_recentProjects, [](const QPair<QString, QString> &p) {
        return QFileInfo(p.first).isFile();
    });
}

void ProjectExplorerPluginPrivate::updateActions()
{
    const Project *const project = SessionManager::startupProject();
    const Project *const currentProject = ProjectTree::currentProject(); // for context menu actions

    const QPair<bool, QString> buildActionState = buildSettingsEnabled(project);
    const QPair<bool, QString> buildActionContextState = buildSettingsEnabled(currentProject);
    const QPair<bool, QString> buildSessionState = buildSettingsEnabledForSession();
    const bool isBuilding = BuildManager::isBuilding(project);

    const QString projectName = project ? project->displayName() : QString();
    const QString projectNameContextMenu = currentProject ? currentProject->displayName() : QString();

    m_unloadAction->setParameter(projectName);
    m_unloadActionContextMenu->setParameter(projectNameContextMenu);
    m_closeProjectFilesActionFileMenu->setParameter(projectName);
    m_closeProjectFilesActionContextMenu->setParameter(projectNameContextMenu);

    // mode bar build action
    QAction * const buildAction = ActionManager::command(Constants::BUILD)->action();
    m_modeBarBuildAction->setAction(isBuilding
                                    ? ActionManager::command(Constants::CANCELBUILD)->action()
                                    : buildAction);
    m_modeBarBuildAction->setIcon(isBuilding
                                  ? Icons::CANCELBUILD_FLAT.icon()
                                  : buildAction->icon());

    const RunConfiguration * const runConfig = project && project->activeTarget()
            ? project->activeTarget()->activeRunConfiguration() : nullptr;

    // Normal actions
    m_buildAction->setParameter(projectName);
    m_buildProjectForAllConfigsAction->setParameter(projectName);
    if (runConfig)
        m_buildForRunConfigAction->setParameter(runConfig->displayName());
    m_rebuildAction->setParameter(projectName);
    m_rebuildProjectForAllConfigsAction->setParameter(projectName);
    m_cleanAction->setParameter(projectName);
    m_cleanProjectForAllConfigsAction->setParameter(projectName);

    m_buildAction->setEnabled(buildActionState.first);
    m_buildProjectForAllConfigsAction->setEnabled(buildActionState.first);
    m_rebuildAction->setEnabled(buildActionState.first);
    m_rebuildProjectForAllConfigsAction->setEnabled(buildActionState.first);
    m_cleanAction->setEnabled(buildActionState.first);
    m_cleanProjectForAllConfigsAction->setEnabled(buildActionState.first);

    // The last condition is there to prevent offering this action for custom run configurations.
    m_buildForRunConfigAction->setEnabled(buildActionState.first
            && runConfig && project->canBuildProducts()
            && !runConfig->buildTargetInfo().projectFilePath.isEmpty());

    m_buildAction->setToolTip(buildActionState.second);
    m_buildProjectForAllConfigsAction->setToolTip(buildActionState.second);
    m_rebuildAction->setToolTip(buildActionState.second);
    m_rebuildProjectForAllConfigsAction->setToolTip(buildActionState.second);
    m_cleanAction->setToolTip(buildActionState.second);
    m_cleanProjectForAllConfigsAction->setToolTip(buildActionState.second);

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
    m_closeProjectFilesActionFileMenu->setVisible(SessionManager::projects().size() <= 1);
    m_closeProjectFilesActionFileMenu->setEnabled(SessionManager::projects().size() == 1);
    m_closeProjectFilesActionContextMenu->setEnabled(SessionManager::hasProjects());

    ActionContainer *aci =
        ActionManager::actionContainer(Constants::M_UNLOADPROJECTS);
    aci->menu()->menuAction()->setVisible(SessionManager::projects().size() > 1);

    m_buildSessionAction->setEnabled(buildSessionState.first);
    m_buildSessionForAllConfigsAction->setEnabled(buildSessionState.first);
    m_rebuildSessionAction->setEnabled(buildSessionState.first);
    m_rebuildSessionForAllConfigsAction->setEnabled(buildSessionState.first);
    m_cleanSessionAction->setEnabled(buildSessionState.first);
    m_cleanSessionForAllConfigsAction->setEnabled(buildSessionState.first);

    m_buildSessionAction->setToolTip(buildSessionState.second);
    m_buildSessionForAllConfigsAction->setToolTip(buildSessionState.second);
    m_rebuildSessionAction->setToolTip(buildSessionState.second);
    m_rebuildSessionForAllConfigsAction->setToolTip(buildSessionState.second);
    m_cleanSessionAction->setToolTip(buildSessionState.second);
    m_cleanSessionForAllConfigsAction->setToolTip(buildSessionState.second);

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

ProjectExplorerPluginPrivate::ProjectExplorerPluginPrivate()
    : m_allProjectDirectoriesFilter("Files in All Project Directories")
{
    m_allProjectDirectoriesFilter.setDisplayName(m_allProjectDirectoriesFilter.id().toString());
    m_allProjectDirectoriesFilter.setShortcutString("a");      // shared with "Files in Any Project"
    m_allProjectDirectoriesFilter.setIncludedByDefault(false); // but not included in default
    m_allProjectDirectoriesFilter.setFilters({});
    m_allProjectDirectoriesFilter.setIsCustomFilter(false);
}

void ProjectExplorerPluginPrivate::runProjectContextMenu()
{
    const Node *node = ProjectTree::currentNode();
    const ProjectNode *projectNode = node ? node->asProjectNode() : nullptr;
    if (projectNode == ProjectTree::currentProject()->rootProjectNode() || !projectNode) {
        m_instance->runProject(ProjectTree::currentProject(), Constants::NORMAL_RUN_MODE);
    } else {
        auto act = qobject_cast<QAction *>(sender());
        if (!act)
            return;
        auto *rc = act->data().value<RunConfiguration *>();
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

static QPair<bool, QString> subprojectEnabledState(const Project *pro)
{
    QPair<bool, QString> result;
    result.first = true;

    const QList<Project *> &projects = SessionManager::projectOrder(pro);
    foreach (Project *project, projects) {
        if (project && project->activeTarget()
            && project->activeTarget()->activeBuildConfiguration()
            && !project->activeTarget()->activeBuildConfiguration()->isEnabled()) {
            result.first = false;
            result.second
                += QCoreApplication::translate("ProjectExplorerPluginPrivate",
                                               "Building \"%1\" is disabled: %2<br>")
                       .arg(project->displayName(),
                            project->activeTarget()->activeBuildConfiguration()->disabledReason());
        }
    }

    return result;
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
        result = subprojectEnabledState(pro);
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
        result = subprojectEnabledState(nullptr);
    }
    return result;
}

bool ProjectExplorerPlugin::coreAboutToClose()
{
    if (!m_instance)
        return true;
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
    return dd->m_outputPane.aboutToClose();
}

void ProjectExplorerPlugin::handleCommandLineArguments(const QStringList &arguments)
{
    CustomWizard::setVerbose(arguments.count(QLatin1String("-customwizard-verbose")));
    JsonWizardFactory::setVerbose(arguments.count(QLatin1String("-customwizard-verbose")));

    const int kitForBinaryOptionIndex = arguments.indexOf("-ensure-kit-for-binary");
    if (kitForBinaryOptionIndex != -1) {
        if (kitForBinaryOptionIndex == arguments.count() - 1) {
            qWarning() << "The \"-ensure-kit-for-binary\" option requires a file path argument.";
        } else {
            const Utils::FilePath binary =
                    Utils::FilePath::fromString(arguments.at(kitForBinaryOptionIndex + 1));
            if (binary.isEmpty() || !binary.exists())
                qWarning() << QString("No such file \"%1\".").arg(binary.toUserOutput());
            else
                KitManager::setBinaryForKit(binary);
        }
    }
}

static bool hasDeploySettings(Project *pro)
{
    return Utils::anyOf(SessionManager::projectOrder(pro), [](Project *project) {
        return project->activeTarget()
                && project->activeTarget()->activeDeployConfiguration();
    });
}

void ProjectExplorerPlugin::runProject(Project *pro, Utils::Id mode, const bool forceSkipDeploy)
{
    if (!pro)
        return;

    if (Target *target = pro->activeTarget())
        if (RunConfiguration *rc = target->activeRunConfiguration())
            runRunConfiguration(rc, mode, forceSkipDeploy);
}

void ProjectExplorerPlugin::runStartupProject(Utils::Id runMode, bool forceSkipDeploy)
{
    runProject(SessionManager::startupProject(), runMode, forceSkipDeploy);
}

void ProjectExplorerPlugin::runRunConfiguration(RunConfiguration *rc,
                                                Utils::Id runMode,
                                                const bool forceSkipDeploy)
{
    if (!rc->isEnabled())
        return;
    const BuildForRunConfigStatus buildStatus = forceSkipDeploy
            ? BuildManager::isBuilding(rc->project())
                ? BuildForRunConfigStatus::Building : BuildForRunConfigStatus::NotBuilding
            : BuildManager::potentiallyBuildForRunConfig(rc);
    switch (buildStatus) {
    case BuildForRunConfigStatus::BuildFailed:
        return;
    case BuildForRunConfigStatus::Building:
        QTC_ASSERT(dd->m_runMode == Constants::NO_RUN_MODE, return);

        // delay running till after our queued steps were processed
        dd->m_runMode = runMode;
        dd->m_delayedRunConfiguration = rc;
        dd->m_shouldHaveRunConfiguration = true;
        break;
    case BuildForRunConfigStatus::NotBuilding:
        dd->executeRunConfiguration(rc, runMode);
        break;
    }

    dd->doUpdateRunActions();
}

QList<QPair<Runnable, Utils::ProcessHandle>> ProjectExplorerPlugin::runningRunControlProcesses()
{
    QList<QPair<Runnable, Utils::ProcessHandle>> processes;
    foreach (RunControl *rc, allRunControls()) {
        if (rc->isRunning())
            processes << qMakePair(rc->runnable(), rc->applicationProcessHandle());
    }
    return processes;
}

QList<RunControl *> ProjectExplorerPlugin::allRunControls()
{
    return dd->m_outputPane.allRunControls();
}

void ProjectExplorerPluginPrivate::projectAdded(Project *pro)
{
    Q_UNUSED(pro)
    m_projectsMode.setEnabled(true);
}

void ProjectExplorerPluginPrivate::projectRemoved(Project *pro)
{
    Q_UNUSED(pro)
    m_projectsMode.setEnabled(SessionManager::hasProjects());
}

void ProjectExplorerPluginPrivate::projectDisplayNameChanged(Project *pro)
{
    addToRecentProjects(pro->projectFilePath().toString(), pro->displayName());
    updateActions();
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

    if (m_projectExplorerSettings.buildBeforeDeploy != BuildBeforeRunMode::Off) {
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
    if (m_projectExplorerSettings.buildBeforeDeploy != BuildBeforeRunMode::Off) {
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

    doUpdateRunActions();
}

bool ProjectExplorerPlugin::canRunStartupProject(Utils::Id runMode, QString *whyNot)
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

    if (dd->m_projectExplorerSettings.buildBeforeDeploy != BuildBeforeRunMode::Off
            && dd->m_projectExplorerSettings.deployBeforeRun
            && !BuildManager::isBuilding(project)
            && hasBuildSettings(project)) {
        QPair<bool, QString> buildState = dd->buildSettingsEnabled(project);
        if (!buildState.first) {
            if (whyNot)
                *whyNot = buildState.second;
            return false;
        }

        if (BuildManager::isBuilding()) {
            if (whyNot)
                *whyNot = tr("A build is still in progress.");
             return false;
        }
    }

    // shouldn't actually be shown to the user...
    if (!RunControl::canRun(runMode,
                            DeviceTypeKitAspect::deviceTypeId(target->kit()),
                            activeRC->id())) {
        if (whyNot)
            *whyNot = tr("Cannot run \"%1\".").arg(activeRC->displayName());
        return false;
    }

    if (dd->m_delayedRunConfiguration && dd->m_delayedRunConfiguration->project() == project) {
        if (whyNot)
            *whyNot = tr("A run action is already scheduled for the active project.");
        return false;
    }

    return true;
}

void ProjectExplorerPluginPrivate::doUpdateRunActions()
{
    QString whyNot;
    const bool state = ProjectExplorerPlugin::canRunStartupProject(Constants::NORMAL_RUN_MODE, &whyNot);
    m_runAction->setEnabled(state);
    m_runAction->setToolTip(whyNot);
    m_runWithoutDeployAction->setEnabled(state);

    emit m_instance->runActionsUpdated();
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
    using StringPairListConstIterator = QList<QPair<QString, QString> >::const_iterator;
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

void ProjectExplorerPluginPrivate::removeFromRecentProjects(const QString &fileName,
                                                            const QString &displayName)
{
    QTC_ASSERT(!fileName.isEmpty() && !displayName.isEmpty(), return);
    QTC_CHECK(m_recentProjects.removeOne(QPair<QString, QString>(fileName, displayName)));
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
    m_addExistingProjectsAction->setEnabled(false);
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
    m_addExistingProjectsAction->setVisible(true);
    m_removeProjectAction->setVisible(true);
    m_removeFileAction->setVisible(true);
    m_duplicateFileAction->setVisible(false);
    m_deleteFileAction->setVisible(true);
    m_runActionContextMenu->setVisible(false);
    m_diffFileAction->setVisible(DiffService::instance());

    m_openTerminalHere->setVisible(true);
    m_openTerminalHereBuildEnv->setVisible(false);
    m_openTerminalHereRunEnv->setVisible(false);

    m_showInGraphicalShell->setVisible(true);
    m_searchOnFileSystem->setVisible(true);

    ActionContainer *runMenu = ActionManager::actionContainer(Constants::RUNMENUCONTEXTMENU);
    runMenu->menu()->clear();
    runMenu->menu()->menuAction()->setVisible(false);

    const Node *currentNode = ProjectTree::currentNode();

    if (currentNode && currentNode->managingProject()) {
        ProjectNode *pn;
        if (const ContainerNode *cn = currentNode->asContainerNode())
            pn = cn->rootProjectNode();
        else
            pn = const_cast<ProjectNode*>(currentNode->asProjectNode());

        Project *project = ProjectTree::currentProject();
        m_openTerminalHereBuildEnv->setVisible(bool(buildEnv(project)));
        m_openTerminalHereRunEnv->setVisible(canOpenTerminalWithRunEnv(project));

        if (pn && project) {
            if (pn == project->rootProjectNode()) {
                m_runActionContextMenu->setVisible(true);
            } else {
                QList<RunConfiguration *> runConfigs;
                if (Target *t = project->activeTarget()) {
                    const QString buildKey = pn->buildKey();
                    for (RunConfiguration *rc : t->runConfigurations()) {
                        if (rc->buildKey() == buildKey)
                            runConfigs.append(rc);
                    }
                }
                if (runConfigs.count() == 1) {
                    m_runActionContextMenu->setVisible(true);
                    m_runActionContextMenu->setData(QVariant::fromValue(runConfigs.first()));
                } else if (runConfigs.count() > 1) {
                    runMenu->menu()->menuAction()->setVisible(true);
                    foreach (RunConfiguration *rc, runConfigs) {
                        auto *act = new QAction(runMenu->menu());
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

        bool canEditProject = true;
        if (project && project->activeTarget()) {
            const BuildSystem * const bs = project->activeTarget()->buildSystem();
            if (bs->isParsing() || bs->isWaitingForParse())
                canEditProject = false;
        }
        if (currentNode->asFolderNode()) {
            // Also handles ProjectNode
            m_addNewFileAction->setEnabled(canEditProject && supports(AddNewFile)
                                              && !ICore::isNewItemDialogRunning());
            m_addNewSubprojectAction->setEnabled(canEditProject && currentNode->isProjectNodeType()
                                                    && supports(AddSubProject)
                                                    && !ICore::isNewItemDialogRunning());
            m_addExistingProjectsAction->setEnabled(canEditProject
                                                    && currentNode->isProjectNodeType()
                                                    && supports(AddExistingProject));
            m_removeProjectAction->setEnabled(canEditProject && currentNode->isProjectNodeType()
                                                    && supports(RemoveSubProject));
            m_addExistingFilesAction->setEnabled(canEditProject && supports(AddExistingFile));
            m_addExistingDirectoryAction->setEnabled(canEditProject
                                                     && supports(AddExistingDirectory));
            m_renameFileAction->setEnabled(canEditProject && supports(Rename));
        } else if (auto fileNode = currentNode->asFileNode()) {
            // Enable and show remove / delete in magic ways:
            // If both are disabled show Remove
            // If both are enabled show both (can't happen atm)
            // If only removeFile is enabled only show it
            // If only deleteFile is enable only show it
            bool isTypeProject = fileNode->fileType() == FileType::Project;
            bool enableRemove = canEditProject && !isTypeProject && supports(RemoveFile);
            m_removeFileAction->setEnabled(enableRemove);
            bool enableDelete = canEditProject && !isTypeProject && supports(EraseFile);
            m_deleteFileAction->setEnabled(enableDelete);
            m_deleteFileAction->setVisible(enableDelete);

            m_removeFileAction->setVisible(!enableDelete || enableRemove);
            m_renameFileAction->setEnabled(canEditProject && !isTypeProject && supports(Rename));
            const bool currentNodeIsTextFile = isTextFile(
                        currentNode->filePath().toString());
            m_diffFileAction->setEnabled(DiffService::instance()
                        && currentNodeIsTextFile && TextEditor::TextDocument::currentTextDocument());

            const bool canDuplicate = canEditProject && supports(AddNewFile)
                    && currentNode->asFileNode()->fileType() != FileType::Project;
            m_duplicateFileAction->setVisible(canDuplicate);
            m_duplicateFileAction->setEnabled(canDuplicate);

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
            m_addExistingProjectsAction->setVisible(false);
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
            = ProjectTree::currentNode() ? ProjectTree::currentNode()->asFolderNode() : nullptr;
    const QVector<FolderNode::LocationInfo> locations = fn ? fn->locationInfo()
                                                           : QVector<FolderNode::LocationInfo>();

    const bool isVisible = !locations.isEmpty();
    projectMenu->menuAction()->setVisible(isVisible);
    folderMenu->menuAction()->setVisible(isVisible);

    if (!isVisible)
        return;

    unsigned int lastPriority = 0;
    for (const FolderNode::LocationInfo &li : locations) {
        if (li.priority != lastPriority) {
            projectMenu->addSeparator();
            folderMenu->addSeparator();
            lastPriority = li.priority;
        }
        const int line = li.line;
        const Utils::FilePath path = li.path;
        QString displayName = fn->filePath() == li.path
                                  ? li.displayName
                                  : tr("%1 in %2").arg(li.displayName).arg(li.path.toUserOutput());
        auto *action = new QAction(displayName, nullptr);
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
    Node *currentNode = ProjectTree::currentNode();
    QTC_ASSERT(currentNode, return);
    QString location = currentNode->directory();

    QVariantMap map;
    // store void pointer to avoid QVariant to use qobject_cast, which might core-dump when trying
    // to access meta data on an object that get deleted in the meantime:
    map.insert(QLatin1String(Constants::PREFERRED_PROJECT_NODE), QVariant::fromValue(static_cast<void *>(currentNode)));
    map.insert(Constants::PREFERRED_PROJECT_NODE_PATH, currentNode->filePath().toString());
    if (Project *p = ProjectTree::currentProject()) {
        const QStringList profileIds = Utils::transform(p->targets(), [](const Target *t) {
            return t->id().toString();
        });
        map.insert(QLatin1String(Constants::PROJECT_KIT_IDS), profileIds);
        map.insert(Constants::PROJECT_POINTER, QVariant::fromValue(static_cast<void *>(p)));
    }
    ICore::showNewItemDialog(ProjectExplorerPlugin::tr("New File", "Title of dialog"),
                             Utils::filtered(IWizardFactory::allWizardFactories(),
                                             [](IWizardFactory *f) {
                                 return f->supportedProjectTypes().isEmpty();
                             }),
                             location, map);
}

void ProjectExplorerPluginPrivate::addNewSubproject()
{
    Node* currentNode = ProjectTree::currentNode();
    QTC_ASSERT(currentNode, return);
    QString location = currentNode->directory();

    if (currentNode->isProjectNodeType()
            && currentNode->supportsAction(AddSubProject, currentNode)) {
        QVariantMap map;
        map.insert(QLatin1String(Constants::PREFERRED_PROJECT_NODE), QVariant::fromValue(currentNode));
        Project *project = ProjectTree::currentProject();
        Utils::Id projectType;
        if (project) {
            const QStringList profileIds = Utils::transform(ProjectTree::currentProject()->targets(),
                                                            [](const Target *t) {
                                                                return t->id().toString();
                                                            });
            map.insert(QLatin1String(Constants::PROJECT_KIT_IDS), profileIds);
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

void ProjectExplorerPluginPrivate::addExistingProjects()
{
    Node * const currentNode = ProjectTree::currentNode();
    if (!currentNode)
        return;
    ProjectNode *projectNode = currentNode->asProjectNode();
    if (!projectNode && currentNode->asContainerNode())
        projectNode = currentNode->asContainerNode()->rootProjectNode();
    QTC_ASSERT(projectNode, return);
    const QString dir = currentNode->directory();
    QStringList subProjectFilePaths = QFileDialog::getOpenFileNames(
                ICore::dialogParent(), tr("Choose Project File"), dir,
                projectNode->subProjectFileNamePatterns().join(";;"));
    if (!ProjectTree::hasNode(projectNode))
        return;
    const QList<Node *> childNodes = projectNode->nodes();
    Utils::erase(subProjectFilePaths, [childNodes](const QString &filePath) {
        return Utils::anyOf(childNodes, [filePath](const Node *n) {
            return n->filePath().toString() == filePath;
        });
    });
    if (subProjectFilePaths.empty())
        return;
    QStringList failedProjects;
    QStringList addedProjects;
    for (const QString &filePath : subProjectFilePaths) {
        if (projectNode->addSubProject(filePath))
            addedProjects << filePath;
        else
            failedProjects << filePath;
    }
    if (!failedProjects.empty()) {
        const QString message = tr("The following subprojects could not be added to project "
                                   "\"%1\":").arg(projectNode->managingProject()->displayName());
        QMessageBox::warning(ICore::dialogParent(), tr("Adding Subproject Failed"),
                             message + "\n  " + failedProjects.join("\n  "));
        return;
    }
    VcsManager::promptToAdd(dir, addedProjects);
}

void ProjectExplorerPluginPrivate::handleAddExistingFiles()
{
    Node *node = ProjectTree::currentNode();
    FolderNode *folderNode = node ? node->asFolderNode() : nullptr;

    QTC_ASSERT(folderNode, return);

    QStringList fileNames = QFileDialog::getOpenFileNames(ICore::dialogParent(),
        tr("Add Existing Files"), node->directory());
    if (fileNames.isEmpty())
        return;

    ProjectExplorerPlugin::addExistingFiles(folderNode, fileNames);
}

void ProjectExplorerPluginPrivate::addExistingDirectory()
{
    Node *node = ProjectTree::currentNode();
    FolderNode *folderNode = node ? node->asFolderNode() : nullptr;

    QTC_ASSERT(folderNode, return);

    SelectableFilesDialogAddDirectory dialog(Utils::FilePath::fromString(node->directory()),
                                             Utils::FilePaths(), ICore::dialogParent());
    dialog.setAddFileFilter({});

    if (dialog.exec() == QDialog::Accepted)
        ProjectExplorerPlugin::addExistingFiles(folderNode, Utils::transform(dialog.selectedFiles(), &Utils::FilePath::toString));
}

void ProjectExplorerPlugin::addExistingFiles(FolderNode *folderNode, const QStringList &filePaths)
{
    // can happen when project is not yet parsed or finished parsing while the dialog was open:
    if (!folderNode || !ProjectTree::hasNode(folderNode))
        return;

    const QString dir = folderNode->directory();
    QStringList fileNames = filePaths;
    QStringList notAdded;
    folderNode->addFiles(fileNames, &notAdded);

    if (!notAdded.isEmpty()) {
        const QString message = tr("Could not add following files to project %1:")
                .arg(folderNode->managingProject()->displayName()) + QLatin1Char('\n');
        const QStringList nativeFiles
                = Utils::transform(notAdded, &QDir::toNativeSeparators);
        QMessageBox::warning(ICore::dialogParent(), tr("Adding Files to Project Failed"),
                             message + nativeFiles.join(QLatin1Char('\n')));
        fileNames = Utils::filtered(fileNames,
                                    [&notAdded](const QString &f) { return !notAdded.contains(f); });
    }

    VcsManager::promptToAdd(dir, fileNames);
}

void ProjectExplorerPluginPrivate::removeProject()
{
    Node *node = ProjectTree::currentNode();
    if (!node)
        return;
    ProjectNode *projectNode = node->managingProject();
    if (projectNode) {
        Utils::RemoveFileDialog removeFileDialog(node->filePath().toString(), ICore::dialogParent());
        removeFileDialog.setDeleteFileVisible(false);
        if (removeFileDialog.exec() == QDialog::Accepted)
            projectNode->removeSubProject(node->filePath().toString());
    }
}

void ProjectExplorerPluginPrivate::openFile()
{
    const Node *currentNode = ProjectTree::currentNode();
    QTC_ASSERT(currentNode, return);
    EditorManager::openEditor(currentNode->filePath().toString());
}

void ProjectExplorerPluginPrivate::searchOnFileSystem()
{
    const Node *currentNode = ProjectTree::currentNode();
    QTC_ASSERT(currentNode, return);
    TextEditor::FindInFiles::findOnFileSystem(currentNode->path());
}

void ProjectExplorerPluginPrivate::showInGraphicalShell()
{
    Node *currentNode = ProjectTree::currentNode();
    QTC_ASSERT(currentNode, return);
    Core::FileUtils::showInGraphicalShell(ICore::dialogParent(), currentNode->path());
}

void ProjectExplorerPluginPrivate::openTerminalHere(const EnvironmentGetter &env)
{
    const Node *currentNode = ProjectTree::currentNode();
    QTC_ASSERT(currentNode, return);

    const auto environment = env(ProjectTree::projectForNode(currentNode));
    if (!environment)
        return;

    Core::FileUtils::openTerminal(currentNode->directory(), environment.value());
}

void ProjectExplorerPluginPrivate::openTerminalHereWithRunEnv()
{
    const Node *currentNode = ProjectTree::currentNode();
    QTC_ASSERT(currentNode, return);

    const Project * const project = ProjectTree::projectForNode(currentNode);
    QTC_ASSERT(project, return);
    const Target * const target = project->activeTarget();
    QTC_ASSERT(target, return);
    const RunConfiguration * const runConfig = target->activeRunConfiguration();
    QTC_ASSERT(runConfig, return);

    const Runnable runnable = runConfig->runnable();
    IDevice::ConstPtr device = runnable.device;
    if (!device)
        device = DeviceKitAspect::device(target->kit());
    QTC_ASSERT(device && device->canOpenTerminal(), return);
    const QString workingDir = device->type() == Constants::DESKTOP_DEVICE_TYPE
            ? currentNode->directory() : runnable.workingDirectory;
    device->openTerminal(runnable.environment, workingDir);
}

void ProjectExplorerPluginPrivate::removeFile()
{
    const Node *currentNode = ProjectTree::currentNode();
    QTC_ASSERT(currentNode && currentNode->asFileNode(), return);

    const Utils::FilePath filePath = currentNode->filePath();
    using NodeAndPath = QPair<const Node *, Utils::FilePath>;
    QList<NodeAndPath> filesToRemove{qMakePair(currentNode, currentNode->filePath())};
    QList<NodeAndPath> siblings;
    for (const Node * const n : ProjectTree::siblingsWithSameBaseName(currentNode))
        siblings << qMakePair(n, n->filePath());

    Utils::RemoveFileDialog removeFileDialog(filePath.toString(), ICore::dialogParent());
    if (removeFileDialog.exec() != QDialog::Accepted)
        return;

    const bool deleteFile = removeFileDialog.isDeleteFileChecked();

    const QMessageBox::StandardButton reply = QMessageBox::question(
                Core::ICore::dialogParent(), tr("Remove More Files?"),
                tr("Remove these files as well?\n    %1")
                .arg(Utils::transform<QStringList>(siblings, [](const NodeAndPath &np) {
        return np.second.toFileInfo().fileName();
    }).join("\n    ")));
    if (reply == QMessageBox::Yes)
        filesToRemove << siblings;

    for (const NodeAndPath &file : filesToRemove) {
        // Nodes can become invalid if the project was re-parsed while the dialog was open
        if (!ProjectTree::hasNode(file.first)) {
            QMessageBox::warning(ICore::dialogParent(), tr("Removing File Failed"),
                                 tr("File \"%1\" was not removed, because the project has changed "
                                    "in the meantime.\nPlease try again.")
                                 .arg(file.second.toUserOutput()));
            return;
        }

        // remove from project
        FolderNode *folderNode = file.first->asFileNode()->parentFolderNode();
        QTC_ASSERT(folderNode, return);

        const RemovedFilesFromProject status
                = folderNode->removeFiles(QStringList(file.second.toString()));
        const bool success = status == RemovedFilesFromProject::Ok
                || (status == RemovedFilesFromProject::Wildcard
                    && removeFileDialog.isDeleteFileChecked());
        if (!success) {
            TaskHub::addTask(BuildSystemTask(Task::Error,
                    tr("Could not remove file \"%1\" from project \"%2\".")
                        .arg(filePath.toUserOutput(), folderNode->managingProject()->displayName()),
                    folderNode->managingProject()->filePath()));
            if (!deleteFile)
                continue;
        }
        FileChangeBlocker changeGuard(filePath.toString());
        Core::FileUtils::removeFile(filePath.toString(), deleteFile);
    }
}

void ProjectExplorerPluginPrivate::duplicateFile()
{
    Node *currentNode = ProjectTree::currentNode();
    QTC_ASSERT(currentNode && currentNode->asFileNode(), return);

    FileNode *fileNode = currentNode->asFileNode();
    QString filePath = currentNode->filePath().toString();
    QFileInfo sourceFileInfo(filePath);
    QString baseName = sourceFileInfo.baseName();

    QString newFileName = sourceFileInfo.fileName();
    int copyTokenIndex = newFileName.lastIndexOf(baseName)+baseName.length();
    newFileName.insert(copyTokenIndex, tr("_copy"));

    bool okPressed;
    newFileName = QInputDialog::getText(ICore::dialogParent(), tr("Choose File Name"),
            tr("New file name:"), QLineEdit::Normal, newFileName, &okPressed);
    if (!okPressed)
        return;
    if (!ProjectTree::hasNode(currentNode))
        return;

    const QString newFilePath = sourceFileInfo.path() + '/' + newFileName;
    FolderNode *folderNode = fileNode->parentFolderNode();
    QTC_ASSERT(folderNode, return);
    QFile sourceFile(filePath);
    if (!sourceFile.copy(newFilePath)) {
        QMessageBox::critical(ICore::dialogParent(), tr("Duplicating File Failed"),
                             tr("Failed to copy file \"%1\" to \"%2\": %3.")
                             .arg(QDir::toNativeSeparators(filePath),
                                  QDir::toNativeSeparators(newFilePath), sourceFile.errorString()));
        return;
    }
    if (!folderNode->addFiles(QStringList(newFilePath))) {
        QMessageBox::critical(ICore::dialogParent(), tr("Duplicating File Failed"),
                              tr("Failed to add new file \"%1\" to the project.")
                              .arg(QDir::toNativeSeparators(newFilePath)));
    }
}

void ProjectExplorerPluginPrivate::deleteFile()
{
    Node *currentNode = ProjectTree::currentNode();
    QTC_ASSERT(currentNode && currentNode->asFileNode(), return);

    FileNode *fileNode = currentNode->asFileNode();

    QString filePath = currentNode->filePath().toString();
    QMessageBox::StandardButton button =
            QMessageBox::question(ICore::dialogParent(),
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
            QMessageBox::warning(ICore::dialogParent(), tr("Deleting File Failed"),
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
            int res = QMessageBox::question(ICore::dialogParent(),
                                            tr("Project Editing Failed"),
                                            tr("The project file %1 cannot be automatically changed.\n\n"
                                               "Rename %2 to %3 anyway?")
                                            .arg(projectFileName)
                                            .arg(QDir::toNativeSeparators(oldFilePath))
                                            .arg(QDir::toNativeSeparators(newFilePath)));
            if (res == QMessageBox::Yes) {
                QTC_CHECK(Core::FileUtils::renameFile(oldFilePath, newFilePath));
            }
        });
        return;
    }

    if (Core::FileUtils::renameFile(oldFilePath, newFilePath)) {
        // Tell the project plugin about rename
        if (!folderNode->renameFile(oldFilePath, newFilePath)) {
            const QString renameFileError
                    = tr("The file %1 was renamed to %2, but the project file %3 could not be automatically changed.")
                    .arg(QDir::toNativeSeparators(oldFilePath))
                    .arg(QDir::toNativeSeparators(newFilePath))
                    .arg(projectFileName);

            QTimer::singleShot(0, [renameFileError]() {
                QMessageBox::warning(ICore::dialogParent(),
                                     tr("Project Editing Failed"),
                                     renameFileError);
            });
        }
    } else {
        const QString renameFileError = tr("The file %1 could not be renamed %2.")
                .arg(QDir::toNativeSeparators(oldFilePath))
                .arg(QDir::toNativeSeparators(newFilePath));

        QTimer::singleShot(0, [renameFileError]() {
            QMessageBox::warning(ICore::dialogParent(), tr("Cannot Rename File"), renameFileError);
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
    auto *ag = new QActionGroup(m_sessionMenu);
    connect(ag, &QActionGroup::triggered, this, &ProjectExplorerPluginPrivate::setSession);
    const QString activeSession = SessionManager::activeSession();

    const QStringList sessions = SessionManager::sessions();
    for (int i = 0; i < sessions.size(); ++i) {
        const QString &session = sessions[i];

        const QString actionText = ActionManager::withNumberAccelerator(Utils::quoteAmpersands(
                                                                            session),
                                                                        i + 1);
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

const ProjectExplorerSettings &ProjectExplorerPlugin::projectExplorerSettings()
{
    return dd->m_projectExplorerSettings;
}

void ProjectExplorerPlugin::setAppOutputSettings(const AppOutputSettings &settings)
{
    dd->m_outputPane.setSettings(settings);
}

const AppOutputSettings &ProjectExplorerPlugin::appOutputSettings()
{
    return dd->m_outputPane.settings();
}

void ProjectExplorerPlugin::setBuildPropertiesSettings(const BuildPropertiesSettings &settings)
{
    dd->m_buildPropertiesSettings = settings;
}

const BuildPropertiesSettings &ProjectExplorerPlugin::buildPropertiesSettings()
{
    return dd->m_buildPropertiesSettings;
}

void ProjectExplorerPlugin::showQtSettings()
{
    dd->m_buildPropertiesSettings.showQtSettings = true;
}

void ProjectExplorerPlugin::setCustomParsers(const QList<CustomParserSettings> &settings)
{
    if (dd->m_customParsers != settings) {
        dd->m_customParsers = settings;
        emit m_instance->customParsersChanged();
    }
}

const QList<CustomParserSettings> ProjectExplorerPlugin::customParsers()
{
    return dd->m_customParsers;
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

bool ProjectExplorerPlugin::isProjectFile(const Utils::FilePath &filePath)
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

/*!
    Returns the current build directory template.

    \sa setBuildDirectoryTemplate
*/
QString ProjectExplorerPlugin::buildDirectoryTemplate()
{
    return dd->m_buildPropertiesSettings.buildDirectoryTemplate;
}

QString ProjectExplorerPlugin::defaultBuildDirectoryTemplate()
{
    return QString(Constants::DEFAULT_BUILD_DIRECTORY_TEMPLATE);
}

void ProjectExplorerPlugin::updateActions()
{
    dd->updateActions();
}

void ProjectExplorerPlugin::activateProjectPanel(Utils::Id panelId)
{
    Core::ModeManager::activateMode(Constants::MODE_SESSION);
    dd->m_proWindow->activateProjectPanel(panelId);
}

void ProjectExplorerPlugin::clearRecentProjects()
{
    dd->clearRecentProjects();
}

void ProjectExplorerPlugin::removeFromRecentProjects(const QString &fileName,
                                                     const QString &displayName)
{
    dd->removeFromRecentProjects(fileName, displayName);
}

void ProjectExplorerPlugin::updateRunActions()
{
    dd->doUpdateRunActions();
}

QList<QPair<QString, QString> > ProjectExplorerPlugin::recentProjects()
{
    return dd->recentProjects();
}

void ProjectManager::registerProjectCreator(const QString &mimeType,
    const std::function<Project *(const Utils::FilePath &)> &creator)
{
    dd->m_projectCreators[mimeType] = creator;
}

Project *ProjectManager::openProject(const Utils::MimeType &mt, const Utils::FilePath &fileName)
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
