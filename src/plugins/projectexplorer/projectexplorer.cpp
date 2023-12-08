// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "projectexplorer.h"

#include "appoutputpane.h"
#include "buildpropertiessettings.h"
#include "buildsystem.h"
#include "compileoutputwindow.h"
#include "configtaskhandler.h"
#include "copystep.h"
#include "customexecutablerunconfiguration.h"
#include "customparserssettingspage.h"
#include "customwizard/customwizard.h"
#include "deployablefile.h"
#include "deployconfiguration.h"
#include "desktoprunconfiguration.h"
#include "environmentwidget.h"
#include "extraabi.h"
#include "gcctoolchain.h"
#ifdef WITH_JOURNALD
#include "journaldwatcher.h"
#endif
#include "allprojectsfilter.h"
#include "allprojectsfind.h"
#include "appoutputpane.h"
#include "buildconfiguration.h"
#include "buildmanager.h"
#include "codestylesettingspropertiespage.h"
#include "copytaskhandler.h"
#include "currentprojectfilter.h"
#include "currentprojectfind.h"
#include "customtoolchain.h"
#include "customwizard/customwizard.h"
#include "dependenciespanel.h"
#include "devicesupport/desktopdevice.h"
#include "devicesupport/desktopdevicefactory.h"
#include "devicesupport/devicecheckbuildstep.h"
#include "devicesupport/devicemanager.h"
#include "devicesupport/devicesettingspage.h"
#include "devicesupport/sshsettings.h"
#include "devicesupport/sshsettingspage.h"
#include "editorsettingspropertiespage.h"
#include "environmentaspect.h"
#include "filesinallprojectsfind.h"
#include "jsonwizard/jsonwizardfactory.h"
#include "jsonwizard/jsonwizardgeneratorfactory.h"
#include "jsonwizard/jsonwizardpagefactory_p.h"
#include "kitfeatureprovider.h"
#include "kitaspects.h"
#include "kitmanager.h"
#include "miniprojecttargetselector.h"
#include "namedwidget.h"
#include "parseissuesdialog.h"
#include "processstep.h"
#include "project.h"
#include "projectcommentssettings.h"
#include "projectexplorerconstants.h"
#include "projectexplorericons.h"
#include "projectexplorersettings.h"
#include "projectexplorertr.h"
#include "projectfilewizardextension.h"
#include "projectmanager.h"
#include "projectnodes.h"
#include "projectpanelfactory.h"
#include "projecttreewidget.h"
#include "projectwindow.h"
#include "removetaskhandler.h"
#include "sanitizerparser.h"
#include "selectablefilesmodel.h"
#include "showineditortaskhandler.h"
#include "simpleprojectwizard.h"
#include "target.h"
#include "taskfile.h"
#include "taskhub.h"
#include "toolchainmanager.h"
#include "toolchainoptionspage.h"
#include "vcsannotatetaskhandler.h"

#ifdef Q_OS_WIN
#include "windebuginterface.h"
#include "msvctoolchain.h"
#endif

#include "customparser.h"
#include "projecttree.h"
#include "projectwelcomepage.h"

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/coreplugintr.h>
#include <coreplugin/diffservice.h>
#include <coreplugin/documentmanager.h>
#include <coreplugin/editormanager/documentmodel.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/fileutils.h>
#include <coreplugin/findplaceholder.h>
#include <coreplugin/foldernavigationwidget.h>
#include <coreplugin/icore.h>
#include <coreplugin/idocument.h>
#include <coreplugin/idocumentfactory.h>
#include <coreplugin/imode.h>
#include <coreplugin/iversioncontrol.h>
#include <coreplugin/locator/directoryfilter.h>
#include <coreplugin/messagemanager.h>
#include <coreplugin/minisplitter.h>
#include <coreplugin/modemanager.h>
#include <coreplugin/navigationwidget.h>
#include <coreplugin/outputpane.h>
#include <coreplugin/session.h>
#include <coreplugin/vcsmanager.h>

#include <extensionsystem/pluginmanager.h>
#include <extensionsystem/pluginspec.h>

#include <texteditor/findinfiles.h>
#include <texteditor/textdocument.h>
#include <texteditor/texteditorconstants.h>
#include <texteditor/texteditorsettings.h>

#include <utils/algorithm.h>
#include <utils/fileutils.h>
#include <utils/macroexpander.h>
#include <utils/mimeutils.h>
#include <utils/parameteraction.h>
#include <utils/processhandle.h>
#include <utils/processinterface.h>
#include <utils/proxyaction.h>
#include <utils/qtcassert.h>
#include <utils/removefiledialog.h>
#include <utils/stringutils.h>
#include <utils/terminalhooks.h>
#include <utils/tooltip/tooltip.h>
#include <utils/utilsicons.h>

#include <nanotrace/nanotrace.h>

#include <QAction>
#include <QActionGroup>
#include <QApplication>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QInputDialog>
#include <QJsonObject>
#include <QMenu>
#include <QMessageBox>
#include <QPair>
#include <QPushButton>
#include <QThreadPool>
#include <QTimer>
#include <QVBoxLayout>

#include <algorithm>
#include <functional>
#include <iterator>
#include <memory>
#include <vector>

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
using namespace ExtensionSystem;
using namespace ProjectExplorer::Internal;
using namespace Utils;

namespace ProjectExplorer {

namespace Constants {
const int  P_MODE_SESSION         = 85;

// Actions
const char LOAD[]                 = "ProjectExplorer.Load";
const char UNLOAD[]               = "ProjectExplorer.Unload";
const char UNLOADCM[]             = "ProjectExplorer.UnloadCM";
const char UNLOADOTHERSCM[]       = "ProjectExplorer.UnloadOthersCM";
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
const char OPENTERMINALHERE[]     = "ProjectExplorer.OpenTerminalHere";
const char SHOWINFILESYSTEMVIEW[] = "ProjectExplorer.OpenFileSystemView";
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
const char M_GENERATORS[]         = "ProjectExplorer.Menu.Generators";

const char RUNMENUCONTEXTMENU[]   = "Project.RunMenu";
const char FOLDER_OPEN_LOCATIONS_CONTEXT_MENU[]  = "Project.F.OpenLocation.CtxMenu";
const char PROJECT_OPEN_LOCATIONS_CONTEXT_MENU[]  = "Project.P.OpenLocation.CtxMenu";


const char RECENTPROJECTS_FILE_NAMES_KEY[] = "ProjectExplorer/RecentProjects/FileNames";
const char RECENTPROJECTS_DISPLAY_NAMES_KEY[] = "ProjectExplorer/RecentProjects/DisplayNames";
const char BUILD_BEFORE_DEPLOY_SETTINGS_KEY[] = "ProjectExplorer/Settings/BuildBeforeDeploy";
const char DEPLOY_BEFORE_RUN_SETTINGS_KEY[] = "ProjectExplorer/Settings/DeployBeforeRun";
const char SAVE_BEFORE_BUILD_SETTINGS_KEY[] = "ProjectExplorer/Settings/SaveBeforeBuild";
const char USE_JOM_SETTINGS_KEY[] = "ProjectExplorer/Settings/UseJom";
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

const char CUSTOM_PARSER_COUNT_KEY[] = "ProjectExplorer/Settings/CustomParserCount";
const char CUSTOM_PARSER_PREFIX_KEY[] = "ProjectExplorer/Settings/CustomParser";

} // namespace Constants


static std::optional<Environment> sysEnv(const Project *)
{
    return Environment::systemEnvironment();
}

static std::optional<Environment> buildEnv(const Project *project)
{
    if (!project || !project->activeTarget() || !project->activeTarget()->activeBuildConfiguration())
        return {};
    return project->activeTarget()->activeBuildConfiguration()->environment();
}

static const RunConfiguration *runConfigForNode(const Target *target, const ProjectNode *node)
{
    if (node && node->productType() == ProductType::App) {
        const QString buildKey = node->buildKey();
        for (const RunConfiguration * const rc : target->runConfigurations()) {
            if (rc->buildKey() == buildKey)
                return rc;
        }
    }
    return target->activeRunConfiguration();
}

static bool hideBuildMenu()
{
    return ICore::settings()->value(Constants::SETTINGS_MENU_HIDE_BUILD, false).toBool();
}

static bool hideDebugMenu()
{
    return ICore::settings()->value(Constants::SETTINGS_MENU_HIDE_DEBUG, false).toBool();
}

static bool canOpenTerminalWithRunEnv(const Project *project, const ProjectNode *node)
{
    if (!project)
        return false;
    const Target * const target = project->activeTarget();
    if (!target)
        return false;
    const RunConfiguration * const runConfig = runConfigForNode(target, node);
    if (!runConfig)
        return false;
    IDevice::ConstPtr device
        = DeviceManager::deviceForPath(runConfig->runnable().command.executable());
    if (!device)
        device = DeviceKitAspect::device(target->kit());
    return device && device->canOpenTerminal();
}

static BuildConfiguration *currentBuildConfiguration()
{
    const Project * const project = ProjectTree::currentProject();
    const Target * const target = project ? project->activeTarget() : nullptr;
    return target ? target->activeBuildConfiguration() : nullptr;
}

static Target *activeTarget()
{
    const Project * const project = ProjectManager::startupProject();
    return project ? project->activeTarget() : nullptr;
}

static BuildConfiguration *activeBuildConfiguration()
{
    const Target * const target = activeTarget();
    return target ? target->activeBuildConfiguration() : nullptr;
}

static RunConfiguration *activeRunConfiguration()
{
    const Target * const target = activeTarget();
    return target ? target->activeRunConfiguration() : nullptr;
}

static bool isTextFile(const FilePath &filePath)
{
    return Utils::mimeTypeForFile(filePath).inherits(
                TextEditor::Constants::C_TEXTEDITOR_MIMETYPE_TEXT);
}

class ProjectsMode : public IMode
{
public:
    ProjectsMode()
    {
        setContext(Context(Constants::C_PROJECTEXPLORER));
        setDisplayName(Tr::tr("Projects"));
        setIcon(Icon::modeIcon(Icons::MODE_PROJECT_CLASSIC,
                               Icons::MODE_PROJECT_FLAT, Icons::MODE_PROJECT_FLAT_ACTIVE));
        setPriority(Constants::P_MODE_SESSION);
        setId(Constants::MODE_SESSION);
        setContextHelp("Managing Projects");
    }
};

class ProjectEnvironmentWidget : public NamedWidget
{
public:
    explicit ProjectEnvironmentWidget(Project *project) : NamedWidget(Tr::tr("Project Environment"))
    {
        setUseGlobalSettingsCheckBoxVisible(false);
        setUseGlobalSettingsLabelVisible(false);
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

class AllProjectFilesFilter : public DirectoryFilter
{
public:
    AllProjectFilesFilter();

protected:
    void saveState(QJsonObject &object) const override;
    void restoreState(const QJsonObject &object) override;
};

class RunConfigurationStartFilter final : public ILocatorFilter
{
public:
    RunConfigurationStartFilter();

private:
    Core::LocatorMatcherTasks matchers() final;
};

class RunConfigurationSwitchFilter final : public ILocatorFilter
{
public:
    RunConfigurationSwitchFilter();

private:
    Core::LocatorMatcherTasks matchers() final;
};

class DefaultDeployConfigurationFactory final : public DeployConfigurationFactory
{
public:
    DefaultDeployConfigurationFactory()
    {
        setConfigBaseId("ProjectExplorer.DefaultDeployConfiguration");
        addSupportedTargetDeviceType(Constants::DESKTOP_DEVICE_TYPE);
        //: Display name of the default deploy configuration
        setDefaultDisplayName(Tr::tr("Deploy Configuration"));
    }
};

class ProjectExplorerPluginPrivate : public QObject
{
public:
    ProjectExplorerPluginPrivate();

    void updateContextMenuActions(Node *currentNode);
    void updateLocationSubMenus();
    void executeRunConfiguration(RunConfiguration *, Id mode);
    QPair<bool, QString> buildSettingsEnabledForSession();
    QPair<bool, QString> buildSettingsEnabled(const Project *pro);

    void addToRecentProjects(const FilePath &filePath, const QString &displayName);
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
    void unloadOtherProjectsContextMenu();
    void closeAllProjects();

    void runProjectContextMenu(RunConfiguration *rc);
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
    void showInFileSystemPane();
    void removeFile();
    void duplicateFile();
    void deleteFile();
    void handleRenameFile();
    void handleSetStartupProject();
    void setStartupProject(Project *project);
    bool closeAllFilesInProject(const Project *project);

    void updateRecentProjectMenu();
    void clearRecentProjects();
    void openRecentProject(const FilePath &filePath);
    void removeFromRecentProjects(const FilePath &filePath);
    void updateUnloadProjectMenu();
    using EnvironmentGetter = std::function<std::optional<Environment>(const Project *project)>;
    void openTerminalHere(const EnvironmentGetter &env);
    void openTerminalHereWithRunEnv();

    void invalidateProject(Project *project);

    void projectAdded(Project *pro);
    void projectRemoved(Project *pro);
    void projectDisplayNameChanged(Project *pro);

    void doUpdateRunActions();

    void currentModeChanged(Id mode, Id oldMode);

    void updateWelcomePage();
    void loadSesssionTasks();

    void checkForShutdown();
    void timerEvent(QTimerEvent *) override;

    RecentProjectsEntries recentProjects() const;

    void extendFolderNavigationWidgetFactory();

    QString projectFilterString() const;

public:
    QMenu *m_openWithMenu;
    QMenu *m_openTerminalMenu;

    QMultiMap<int, QObject*> m_actionMap;
    QAction *m_newAction;
    QAction *m_loadAction;
    ParameterAction *m_unloadAction;
    ParameterAction *m_unloadActionContextMenu;
    ParameterAction *m_unloadOthersActionContextMenu;
    QAction *m_closeAllProjects;
    QAction *m_buildProjectOnlyAction;
    ParameterAction *m_buildProjectForAllConfigsAction;
    ParameterAction *m_buildAction;
    ParameterAction *m_buildForRunConfigAction;
    ProxyAction *m_modeBarBuildAction;
    QAction *m_buildActionContextMenu;
    QAction *m_buildDependenciesActionContextMenu;
    QAction *m_buildSessionAction;
    QAction *m_buildSessionForAllConfigsAction;
    QAction *m_rebuildProjectOnlyAction;
    QAction *m_rebuildAction;
    QAction *m_rebuildProjectForAllConfigsAction;
    QAction *m_rebuildActionContextMenu;
    QAction *m_rebuildDependenciesActionContextMenu;
    QAction *m_rebuildSessionAction;
    QAction *m_rebuildSessionForAllConfigsAction;
    QAction *m_cleanProjectOnlyAction;
    QAction *m_deployProjectOnlyAction;
    QAction *m_deployAction;
    QAction *m_deployActionContextMenu;
    QAction *m_deploySessionAction;
    QAction *m_cleanAction;
    QAction *m_cleanProjectForAllConfigsAction;
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
    ParameterAction *m_closeProjectFilesActionFileMenu;
    ParameterAction *m_closeProjectFilesActionContextMenu;
    QAction *m_searchOnFileSystem;
    QAction *m_showInGraphicalShell;
    QAction *m_showFileSystemPane;
    QAction *m_openTerminalHere;
    QAction *m_openTerminalHereBuildEnv;
    QAction *m_openTerminalHereRunEnv;
    ParameterAction *m_setStartupProjectAction;
    QAction *m_projectSelectorAction;
    QAction *m_projectSelectorActionMenu;
    QAction *m_projectSelectorActionQuick;
    QAction *m_runSubProject;

    ProjectWindow *m_proWindow = nullptr;

    QStringList m_profileMimeTypes;
    int m_activeRunControlCount = 0;
    int m_shutdownWatchDogId = -1;

    QHash<QString, std::function<Project *(const FilePath &)>> m_projectCreators;
    RecentProjectsEntries m_recentProjects; // pair of filename, displayname
    static const int m_maxRecentProjects = 25;

    FilePath m_lastOpenDirectory;
    QPointer<RunConfiguration> m_defaultRunConfiguration;
    QPointer<RunConfiguration> m_delayedRunConfiguration;
    MiniProjectTargetSelector * m_targetSelector;
    ProjectExplorerSettings m_projectExplorerSettings;
    QList<CustomParserSettings> m_customParsers;
    bool m_shouldHaveRunConfiguration = false;
    Id m_runMode = Constants::NO_RUN_MODE;

    ToolChainManager *m_toolChainManager = nullptr;

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
    GccToolChainFactory m_linuxToolChainFactory{GccToolChain::LinuxIcc};
#endif

#ifndef Q_OS_MACOS
    // Mingw offers cross-compiling to windows
    GccToolChainFactory m_mingwToolChainFactory{GccToolChain::MinGW};
#endif

    GccToolChainFactory m_gccToolChainFactory{GccToolChain::RealGcc};
    GccToolChainFactory m_clangToolChainFactory{GccToolChain::Clang};
    CustomToolChainFactory m_customToolChainFactory;

    DesktopDeviceFactory m_desktopDeviceFactory;

    ToolChainOptionsPage m_toolChainOptionsPage;

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

    ProjectManager m_sessionManager;
    AppOutputPane m_outputPane;

    ProjectTree m_projectTree;

    AllProjectsFilter m_allProjectsFilter;
    CurrentProjectFilter m_currentProjectFilter;
    AllProjectFilesFilter m_allProjectDirectoriesFilter;
    RunConfigurationStartFilter m_runConfigurationStartFilter;
    RunConfigurationSwitchFilter m_runConfigurationSwitchFilter;

    CopyFileStepFactory m_copyFileStepFactory;
    CopyDirectoryStepFactory m_copyDirectoryFactory;
    ProcessStepFactory m_processStepFactory;

    AllProjectsFind m_allProjectsFind;
    CurrentProjectFind m_curretProjectFind;
    FilesInAllProjectsFind m_filesInAllProjectsFind;

    CustomExecutableRunConfigurationFactory m_customExecutableRunConfigFactory;
    CustomExecutableRunWorkerFactory m_customExecutableRunWorkerFactory;

    ProjectFileWizardExtension m_projectFileWizardExtension;

    // Settings pages
    ProjectExplorerSettingsPage m_projectExplorerSettingsPage;
    AppOutputSettingsPage m_appOutputSettingsPage;
    DeviceSettingsPage m_deviceSettingsPage;
    SshSettingsPage m_sshSettingsPage;
    CustomParsersSettingsPage m_customParsersSettingsPage;

    ProjectTreeWidgetFactory m_projectTreeFactory;
    DefaultDeployConfigurationFactory m_defaultDeployConfigFactory;

    IDocumentFactory m_documentFactory;
    IDocumentFactory m_taskFileFactory;
    StopMonitoringHandler closeTaskFile;

    DesktopQmakeRunConfigurationFactory qmakeRunConfigFactory;
    QbsRunConfigurationFactory qbsRunConfigFactory;
    CMakeRunConfigurationFactory cmakeRunConfigFactory;
    SimpleTargetRunnerFactory desktopRunWorkerFactory{{
        qmakeRunConfigFactory.runConfigurationId(),
        qbsRunConfigFactory.runConfigurationId(),
        cmakeRunConfigFactory.runConfigurationId()
    }};

    DeviceCheckBuildStepFactory deviceCheckBuildStepFactory;
    SanitizerOutputFormatterFactory sanitizerFormatterFactory;

    // JsonWizard related
    FieldPageFactory fieldPageFactory;
    FilePageFactory filePageFactory;
    KitsPageFactory kitsPageFactory;
    ProjectPageFactory projectPageFactory;
    SummaryPageFactory summaryPageFactory;
    FileGeneratorFactory fileGeneratorFactory;
    ScannerGeneratorFactory scannerGeneratorFactory;
};

static ProjectExplorerPlugin *m_instance = nullptr;
static ProjectExplorerPluginPrivate *dd = nullptr;

static FilePaths projectFilesInDirectory(const FilePath &path)
{
    return path.dirEntries({ProjectExplorerPlugin::projectFileGlobs(), QDir::Files});
}

static FilePaths projectsInDirectory(const FilePath &filePath)
{
    if (!filePath.isReadableDir())
        return {};
    return projectFilesInDirectory(filePath);
}

static void openProjectsInDirectory(const FilePath &filePath)
{
    const FilePaths projectFiles = projectsInDirectory(filePath);
    if (!projectFiles.isEmpty())
        ICore::openFiles(projectFiles);
}

static QStringList projectNames(const QVector<FolderNode *> &folders)
{
    const QStringList names = Utils::transform<QList>(folders, [](FolderNode *n) {
        return n->managingProject()->filePath().fileName();
    });
    return Utils::filteredUnique(names);
}

static QVector<FolderNode *> renamableFolderNodes(const FilePath &before, const FilePath &after)
{
    QVector<FolderNode *> folderNodes;
    ProjectTree::forEachNode([&](Node *node) {
        if (node->asFileNode() && node->filePath() == before && node->parentFolderNode()
            && node->parentFolderNode()->canRenameFile(before, after)) {
            folderNodes.append(node->parentFolderNode());
        }
    });
    return folderNodes;
}

static QVector<FolderNode *> removableFolderNodes(const FilePath &filePath)
{
    QVector<FolderNode *> folderNodes;
    ProjectTree::forEachNode([&](Node *node) {
        if (node->asFileNode() && node->filePath() == filePath && node->parentFolderNode()
            && node->parentFolderNode()->supportsAction(RemoveFile, node)) {
            folderNodes.append(node->parentFolderNode());
        }
    });
    return folderNodes;
}

ProjectExplorerPlugin::ProjectExplorerPlugin()
{
    m_instance = this;
}

ProjectExplorerPlugin::~ProjectExplorerPlugin()
{
    QTC_ASSERT(dd, return);

    delete dd->m_proWindow; // Needs access to the kit manager.

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

    dd->extendFolderNavigationWidgetFactory();

    qRegisterMetaType<ProjectExplorer::BuildSystem *>();
    qRegisterMetaType<ProjectExplorer::RunControl *>();
    qRegisterMetaType<ProjectExplorer::DeployableFile>("ProjectExplorer::DeployableFile");

    handleCommandLineArguments(arguments);

    dd->m_toolChainManager = new ToolChainManager;

    // Register languages
    ToolChainManager::registerLanguage(Constants::C_LANGUAGE_ID, Tr::tr("C"));
    ToolChainManager::registerLanguage(Constants::CXX_LANGUAGE_ID, Tr::tr("C++"));

    IWizardFactory::registerFeatureProvider(new KitFeatureProvider);
    IWizardFactory::registerFactoryCreator([] { return new SimpleProjectWizard; });

    ProjectManager *sessionManager = &dd->m_sessionManager;
    connect(sessionManager, &ProjectManager::projectAdded,
            this, &ProjectExplorerPlugin::fileListChanged);
    connect(sessionManager, &ProjectManager::aboutToRemoveProject,
            dd, &ProjectExplorerPluginPrivate::invalidateProject);
    connect(sessionManager, &ProjectManager::projectRemoved,
            this, &ProjectExplorerPlugin::fileListChanged);
    connect(sessionManager, &ProjectManager::projectAdded,
            dd, &ProjectExplorerPluginPrivate::projectAdded);
    connect(sessionManager, &ProjectManager::projectRemoved,
            dd, &ProjectExplorerPluginPrivate::projectRemoved);
    connect(sessionManager, &ProjectManager::projectDisplayNameChanged,
            dd, &ProjectExplorerPluginPrivate::projectDisplayNameChanged);
    connect(sessionManager, &ProjectManager::dependencyChanged,
            dd, &ProjectExplorerPluginPrivate::updateActions);
    connect(SessionManager::instance(), &SessionManager::sessionLoaded,
            dd, &ProjectExplorerPluginPrivate::updateActions);
    connect(SessionManager::instance(), &SessionManager::sessionLoaded,
            dd, &ProjectExplorerPluginPrivate::updateWelcomePage);
    connect(SessionManager::instance(), &SessionManager::sessionLoaded,
            dd, &ProjectExplorerPluginPrivate::loadSesssionTasks);
    connect(SessionManager::instance(), &SessionManager::sessionCreated,
            dd, &ProjectExplorerPluginPrivate::updateWelcomePage);
    connect(SessionManager::instance(), &SessionManager::sessionRenamed,
            dd, &ProjectExplorerPluginPrivate::updateWelcomePage);
    connect(SessionManager::instance(), &SessionManager::sessionRemoved,
            dd, &ProjectExplorerPluginPrivate::updateWelcomePage);

    ProjectTree *tree = &dd->m_projectTree;
    connect(tree, &ProjectTree::currentProjectChanged, dd, [] {
        dd->updateContextMenuActions(ProjectTree::currentNode());
    });
    connect(tree, &ProjectTree::nodeActionsChanged, dd, [] {
        dd->updateContextMenuActions(ProjectTree::currentNode());
    });
    connect(tree, &ProjectTree::currentNodeChanged,
            dd, &ProjectExplorerPluginPrivate::updateContextMenuActions);
    connect(tree, &ProjectTree::currentProjectChanged,
            dd, &ProjectExplorerPluginPrivate::updateActions);
    connect(tree, &ProjectTree::currentProjectChanged, this, [](Project *project) {
        TextEditor::FindInFiles::instance()->setBaseDirectory(project ? project->projectDirectory()
                                                                      : FilePath());
    });

    dd->m_proWindow = new ProjectWindow;

    Context projectTreeContext(Constants::C_PROJECT_TREE);

    auto splitter = new MiniSplitter(Qt::Vertical);
    splitter->addWidget(dd->m_proWindow);
    splitter->addWidget(new OutputPanePlaceHolder(Constants::MODE_SESSION, splitter));
    dd->m_projectsMode.setWidget(splitter);
    dd->m_projectsMode.setEnabled(false);

    ICore::addPreCloseListener([]() -> bool { return coreAboutToClose(); });

    connect(ProjectManager::instance(), &ProjectManager::projectRemoved,
            &dd->m_outputPane, &AppOutputPane::projectRemoved);

    // ProjectPanelFactories
    auto panelFactory = new ProjectPanelFactory;
    panelFactory->setPriority(30);
    panelFactory->setDisplayName(Tr::tr("Editor"));
    panelFactory->setCreateWidgetFunction([](Project *project) { return new EditorSettingsWidget(project); });
    ProjectPanelFactory::registerFactory(panelFactory);

    panelFactory = new ProjectPanelFactory;
    panelFactory->setPriority(40);
    panelFactory->setDisplayName(Tr::tr("Code Style"));
    panelFactory->setCreateWidgetFunction([](Project *project) { return new CodeStyleSettingsWidget(project); });
    ProjectPanelFactory::registerFactory(panelFactory);

    panelFactory = new ProjectExplorer::ProjectPanelFactory;
    panelFactory->setPriority(45);
    panelFactory->setDisplayName(Tr::tr("Documentation Comments"));
    panelFactory->setCreateWidgetFunction([](ProjectExplorer::Project *project) {
        return new ProjectCommentsSettingsWidget(project);
    });
    ProjectExplorer::ProjectPanelFactory::registerFactory(panelFactory);
    TextEditor::TextEditorSettings::setCommentsSettingsRetriever([](const FilePath &filePath) {
        return ProjectCommentsSettings(ProjectManager::projectForFile(filePath)).settings();
    });

    panelFactory = new ProjectPanelFactory;
    panelFactory->setPriority(50);
    panelFactory->setDisplayName(Tr::tr("Dependencies"));
    panelFactory->setCreateWidgetFunction([](Project *project) { return new DependenciesWidget(project); });
    ProjectPanelFactory::registerFactory(panelFactory);

    panelFactory = new ProjectPanelFactory;
    panelFactory->setPriority(60);
    panelFactory->setDisplayName(Tr::tr("Environment"));
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
    folderOpenLocationCtxMenu->menu()->setTitle(Tr::tr("Open..."));
    folderOpenLocationCtxMenu->setOnAllDisabledBehavior(ActionContainer::Hide);

    ActionContainer *projectOpenLocationCtxMenu =
            ActionManager::createMenu(Constants::PROJECT_OPEN_LOCATIONS_CONTEXT_MENU);
    projectOpenLocationCtxMenu->menu()->setTitle(Tr::tr("Open..."));
    projectOpenLocationCtxMenu->setOnAllDisabledBehavior(ActionContainer::Hide);

    // build menu
    ActionContainer *mbuild =
        ActionManager::createMenu(Constants::M_BUILDPROJECT);

    mbuild->menu()->setTitle(Tr::tr("&Build"));
    if (!hideBuildMenu())
        menubar->addMenu(mbuild, Core::Constants::G_VIEW);

    // debug menu
    ActionContainer *mdebug =
        ActionManager::createMenu(Constants::M_DEBUG);
    mdebug->menu()->setTitle(Tr::tr("&Debug"));
    if (!hideDebugMenu())
        menubar->addMenu(mdebug, Core::Constants::G_VIEW);

    ActionContainer *mstartdebugging =
        ActionManager::createMenu(Constants::M_DEBUG_STARTDEBUGGING);
    mstartdebugging->menu()->setTitle(Tr::tr("&Start Debugging"));
    mdebug->addMenu(mstartdebugging, Core::Constants::G_DEFAULT_ONE);

    //
    // Groups
    //

    mbuild->appendGroup(Constants::G_BUILD_ALLPROJECTS);
    mbuild->appendGroup(Constants::G_BUILD_PROJECT);
    mbuild->appendGroup(Constants::G_BUILD_PRODUCT);
    mbuild->appendGroup(Constants::G_BUILD_SUBPROJECT);
    mbuild->appendGroup(Constants::G_BUILD_FILE);
    mbuild->appendGroup(Constants::G_BUILD_ALLPROJECTS_ALLCONFIGURATIONS);
    mbuild->appendGroup(Constants::G_BUILD_PROJECT_ALLCONFIGURATIONS);
    mbuild->appendGroup(Constants::G_BUILD_CANCEL);
    mbuild->appendGroup(Constants::G_BUILD_BUILD);
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
    mprojectContextMenu->appendGroup(Constants::G_PROJECT_CLOSE);
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
    const QIcon runSideBarIcon = Icon::sideBarIcon(Icons::RUN, Icons::RUN_FLAT);
    const QIcon runIcon = Icon::combinedIcon({Utils::Icons::RUN_SMALL.icon(), runSideBarIcon});

    runMenu->menu()->setIcon(runIcon);
    runMenu->menu()->setTitle(Tr::tr("Run"));
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
    dd->m_openWithMenu->setTitle(Tr::tr("Open With"));

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
    mprojectContextMenu->addSeparator(projectTreeContext, Constants::G_PROJECT_CLOSE);
    mprojectContextMenu->addSeparator(projectTreeContext, Constants::G_PROJECT_LAST);
    msubProjectContextMenu->addSeparator(projectTreeContext, Constants::G_PROJECT_FILES);
    mfile->addSeparator(Core::Constants::G_FILE_PROJECT);
    mbuild->addSeparator(Constants::G_BUILD_ALLPROJECTS);
    mbuild->addSeparator(Constants::G_BUILD_PROJECT);
    mbuild->addSeparator(Constants::G_BUILD_PRODUCT);
    mbuild->addSeparator(Constants::G_BUILD_SUBPROJECT);
    mbuild->addSeparator(Constants::G_BUILD_FILE);
    mbuild->addSeparator(Constants::G_BUILD_ALLPROJECTS_ALLCONFIGURATIONS);
    mbuild->addSeparator(Constants::G_BUILD_PROJECT_ALLCONFIGURATIONS);
    msessionContextMenu->addSeparator(Constants::G_SESSION_OTHER);
    mbuild->addSeparator(Constants::G_BUILD_CANCEL);
    mbuild->addSeparator(Constants::G_BUILD_BUILD);
    mbuild->addSeparator(Constants::G_BUILD_RUN);
    mprojectContextMenu->addSeparator(Constants::G_PROJECT_REBUILD);

    //
    // Actions
    //

    // new action
    dd->m_newAction = new QAction(Tr::tr("New Project..."), this);
    cmd = ActionManager::registerAction(dd->m_newAction, Core::Constants::NEW, projectTreeContext);
    msessionContextMenu->addAction(cmd, Constants::G_SESSION_FILES);

    // open action
    dd->m_loadAction = new QAction(Tr::tr("Load Project..."), this);
    cmd = ActionManager::registerAction(dd->m_loadAction, Constants::LOAD);
    if (!HostOsInfo::isMacHost())
        cmd->setDefaultKeySequence(QKeySequence(Tr::tr("Ctrl+Shift+O")));
    msessionContextMenu->addAction(cmd, Constants::G_SESSION_FILES);

    // Default open action
    dd->m_openFileAction = new QAction(Tr::tr("Open File"), this);
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
    cmd = ActionManager::registerAction(dd->m_showInGraphicalShell,
                                        Core::Constants::SHOWINGRAPHICALSHELL,
                                        projectTreeContext);
    mfileContextMenu->addAction(cmd, Constants::G_FILE_OPEN);
    mfolderContextMenu->addAction(cmd, Constants::G_FOLDER_FILES);

    // Show in File System View
    dd->m_showFileSystemPane = new QAction(Core::FileUtils::msgFileSystemAction(), this);
    cmd = ActionManager::registerAction(dd->m_showFileSystemPane,
                                        Constants::SHOWINFILESYSTEMVIEW,
                                        projectTreeContext);
    mfileContextMenu->addAction(cmd, Constants::G_FILE_OPEN);
    mfolderContextMenu->addAction(cmd, Constants::G_FOLDER_FILES);
    msubProjectContextMenu->addAction(cmd, Constants::G_PROJECT_LAST);
    mprojectContextMenu->addAction(cmd, Constants::G_PROJECT_LAST);

    // Open Terminal Here menu
    dd->m_openTerminalHere = new QAction(Core::FileUtils::msgTerminalHereAction(), this);
    cmd = ActionManager::registerAction(dd->m_openTerminalHere, Constants::OPENTERMINALHERE,
                                        projectTreeContext);

    mfileContextMenu->addAction(cmd, Constants::G_FILE_OPEN);
    mfolderContextMenu->addAction(cmd, Constants::G_FOLDER_FILES);
    msubProjectContextMenu->addAction(cmd, Constants::G_PROJECT_LAST);
    mprojectContextMenu->addAction(cmd, Constants::G_PROJECT_LAST);

    mfileContextMenu->addMenu(openTerminal, Constants::G_FILE_OPEN);
    mfolderContextMenu->addMenu(openTerminal, Constants::G_FOLDER_FILES);
    msubProjectContextMenu->addMenu(openTerminal, Constants::G_PROJECT_LAST);
    mprojectContextMenu->addMenu(openTerminal, Constants::G_PROJECT_LAST);


    dd->m_openTerminalHereBuildEnv = new QAction(Tr::tr("Build Environment"), this);
    dd->m_openTerminalHereRunEnv = new QAction(Tr::tr("Run Environment"), this);
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
    mrecent->menu()->setTitle(Tr::tr("Recent P&rojects"));
    mrecent->setOnAllDisabledBehavior(ActionContainer::Show);
    mfile->addMenu(mrecent, Core::Constants::G_FILE_OPEN);
    connect(mfile->menu(), &QMenu::aboutToShow,
            dd, &ProjectExplorerPluginPrivate::updateRecentProjectMenu);

    // unload action
    dd->m_unloadAction = new ParameterAction(Tr::tr("Close Project"), Tr::tr("Close Pro&ject \"%1\""),
                                             ParameterAction::AlwaysEnabled, this);
    cmd = ActionManager::registerAction(dd->m_unloadAction, Constants::UNLOAD);
    cmd->setAttribute(Command::CA_UpdateText);
    cmd->setDescription(dd->m_unloadAction->text());
    mfile->addAction(cmd, Core::Constants::G_FILE_PROJECT);

    dd->m_closeProjectFilesActionFileMenu = new ParameterAction(
                Tr::tr("Close All Files in Project"), Tr::tr("Close All Files in Project \"%1\""),
                ParameterAction::AlwaysEnabled, this);
    cmd = ActionManager::registerAction(dd->m_closeProjectFilesActionFileMenu,
                                        "ProjectExplorer.CloseProjectFilesFileMenu");
    cmd->setAttribute(Command::CA_UpdateText);
    cmd->setDescription(dd->m_closeProjectFilesActionFileMenu->text());
    mfile->addAction(cmd, Core::Constants::G_FILE_PROJECT);

    ActionContainer *munload =
        ActionManager::createMenu(Constants::M_UNLOADPROJECTS);
    munload->menu()->setTitle(Tr::tr("Close Pro&ject"));
    munload->setOnAllDisabledBehavior(ActionContainer::Show);
    mfile->addMenu(munload, Core::Constants::G_FILE_PROJECT);
    connect(mfile->menu(), &QMenu::aboutToShow,
            dd, &ProjectExplorerPluginPrivate::updateUnloadProjectMenu);

    // unload session action
    dd->m_closeAllProjects = new QAction(Tr::tr("Close All Projects and Editors"), this);
    cmd = ActionManager::registerAction(dd->m_closeAllProjects, Constants::CLEARSESSION);
    mfile->addAction(cmd, Core::Constants::G_FILE_PROJECT);
    msessionContextMenu->addAction(cmd, Constants::G_SESSION_FILES);

    // build session action
    const QIcon sideBarIcon = Icon::sideBarIcon(Icons::BUILD, Icons::BUILD_FLAT);
    const QIcon buildIcon = Icon::combinedIcon({Icons::BUILD_SMALL.icon(), sideBarIcon});
    dd->m_buildSessionAction = new QAction(buildIcon, Tr::tr("Build All Projects"), this);
    cmd = ActionManager::registerAction(dd->m_buildSessionAction, Constants::BUILDSESSION);
    cmd->setDefaultKeySequence(QKeySequence(Tr::tr("Ctrl+Shift+B")));
    mbuild->addAction(cmd, Constants::G_BUILD_ALLPROJECTS);
    msessionContextMenu->addAction(cmd, Constants::G_SESSION_BUILD);

    dd->m_buildSessionForAllConfigsAction
            = new QAction(buildIcon, Tr::tr("Build All Projects for All Configurations"), this);
    cmd = ActionManager::registerAction(dd->m_buildSessionForAllConfigsAction,
                                        Constants::BUILDSESSIONALLCONFIGS);
    mbuild->addAction(cmd, Constants::G_BUILD_ALLPROJECTS_ALLCONFIGURATIONS);
    msessionContextMenu->addAction(cmd, Constants::G_SESSION_BUILD);

    // deploy session
    dd->m_deploySessionAction = new QAction(Tr::tr("Deploy"), this);
    dd->m_deploySessionAction->setWhatsThis(Tr::tr("Deploy All Projects"));
    cmd = ActionManager::registerAction(dd->m_deploySessionAction, Constants::DEPLOYSESSION);
    cmd->setDescription(dd->m_deploySessionAction->whatsThis());
    mbuild->addAction(cmd, Constants::G_BUILD_ALLPROJECTS);
    msessionContextMenu->addAction(cmd, Constants::G_SESSION_BUILD);

    // rebuild session action
    dd->m_rebuildSessionAction = new QAction(Icons::REBUILD.icon(), Tr::tr("Rebuild"),
                                             this);
    dd->m_rebuildSessionAction->setWhatsThis(Tr::tr("Rebuild All Projects"));
    cmd = ActionManager::registerAction(dd->m_rebuildSessionAction, Constants::REBUILDSESSION);
    cmd->setDescription(dd->m_rebuildSessionAction->whatsThis());
    mbuild->addAction(cmd, Constants::G_BUILD_ALLPROJECTS);
    msessionContextMenu->addAction(cmd, Constants::G_SESSION_REBUILD);

    dd->m_rebuildSessionForAllConfigsAction
            = new QAction(Icons::REBUILD.icon(), Tr::tr("Rebuild"),
                          this);
    dd->m_rebuildSessionForAllConfigsAction->setWhatsThis(
        Tr::tr("Rebuild All Projects for All Configurations"));
    cmd = ActionManager::registerAction(dd->m_rebuildSessionForAllConfigsAction,
                                        Constants::REBUILDSESSIONALLCONFIGS);
    cmd->setDescription(dd->m_rebuildSessionForAllConfigsAction->whatsThis());
    mbuild->addAction(cmd, Constants::G_BUILD_ALLPROJECTS_ALLCONFIGURATIONS);
    msessionContextMenu->addAction(cmd, Constants::G_SESSION_REBUILD);

    // clean session
    dd->m_cleanSessionAction = new QAction(Utils::Icons::CLEAN.icon(), Tr::tr("Clean"),
                                           this);
    dd->m_cleanSessionAction->setWhatsThis(Tr::tr("Clean All Projects"));
    cmd = ActionManager::registerAction(dd->m_cleanSessionAction, Constants::CLEANSESSION);
    cmd->setDescription(dd->m_cleanSessionAction->whatsThis());
    mbuild->addAction(cmd, Constants::G_BUILD_ALLPROJECTS);
    msessionContextMenu->addAction(cmd, Constants::G_SESSION_REBUILD);

    dd->m_cleanSessionForAllConfigsAction = new QAction(Utils::Icons::CLEAN.icon(),
            Tr::tr("Clean"), this);
    dd->m_cleanSessionForAllConfigsAction->setWhatsThis(
        Tr::tr("Clean All Projects for All Configurations"));
    cmd = ActionManager::registerAction(dd->m_cleanSessionForAllConfigsAction,
                                        Constants::CLEANSESSIONALLCONFIGS);
    cmd->setDescription(dd->m_cleanSessionForAllConfigsAction->whatsThis());
    mbuild->addAction(cmd, Constants::G_BUILD_ALLPROJECTS_ALLCONFIGURATIONS);
    msessionContextMenu->addAction(cmd, Constants::G_SESSION_REBUILD);

    // build action
    dd->m_buildAction = new ParameterAction(Tr::tr("Build Project"), Tr::tr("Build Project \"%1\""),
                                            ParameterAction::AlwaysEnabled, this);
    dd->m_buildAction->setIcon(buildIcon);
    cmd = ActionManager::registerAction(dd->m_buildAction, Constants::BUILD);
    cmd->setAttribute(Command::CA_UpdateText);
    cmd->setDescription(dd->m_buildAction->text());
    cmd->setDefaultKeySequence(QKeySequence(Tr::tr("Ctrl+B")));
    mbuild->addAction(cmd, Constants::G_BUILD_PROJECT);

    dd->m_buildProjectForAllConfigsAction
            = new ParameterAction(Tr::tr("Build Project for All Configurations"),
                                  Tr::tr("Build Project \"%1\" for All Configurations"),
                                  ParameterAction::AlwaysEnabled, this);
    dd->m_buildProjectForAllConfigsAction->setIcon(buildIcon);
    cmd = ActionManager::registerAction(dd->m_buildProjectForAllConfigsAction,
                                        Constants::BUILDALLCONFIGS);
    cmd->setAttribute(Command::CA_UpdateText);
    cmd->setDescription(dd->m_buildProjectForAllConfigsAction->text());
    mbuild->addAction(cmd, Constants::G_BUILD_PROJECT_ALLCONFIGURATIONS);

    // Add to mode bar
    dd->m_modeBarBuildAction = new ProxyAction(this);
    dd->m_modeBarBuildAction->setObjectName("Build"); // used for UI introduction
    dd->m_modeBarBuildAction->initialize(cmd->action());
    dd->m_modeBarBuildAction->setAttribute(ProxyAction::UpdateText);
    dd->m_modeBarBuildAction->setAction(cmd->action());
    if (!hideBuildMenu())
        ModeManager::addAction(dd->m_modeBarBuildAction, Constants::P_ACTION_BUILDPROJECT);

    // build for run config
    dd->m_buildForRunConfigAction = new ParameterAction(
                Tr::tr("Build for &Run Configuration"), Tr::tr("Build for &Run Configuration \"%1\""),
                ParameterAction::EnabledWithParameter, this);
    dd->m_buildForRunConfigAction->setIcon(buildIcon);
    cmd = ActionManager::registerAction(dd->m_buildForRunConfigAction,
                                        "ProjectExplorer.BuildForRunConfig");
    cmd->setAttribute(Command::CA_UpdateText);
    cmd->setDescription(dd->m_buildForRunConfigAction->text());
    mbuild->addAction(cmd, Constants::G_BUILD_BUILD);

    // Generators
    ActionContainer * const generatorContainer
            = ActionManager::createMenu(Id(Constants::M_GENERATORS));
    generatorContainer->setOnAllDisabledBehavior(ActionContainer::Show);
    generatorContainer->menu()->setTitle(Tr::tr("Run Generator"));
    mbuild->addMenu(generatorContainer, Constants::G_BUILD_BUILD);

    // FIXME: This menu will never become visible if the user tried to open it once
    //        without a project loaded.
    connect(generatorContainer->menu(), &QMenu::aboutToShow,
            this, [menu = generatorContainer->menu()] {
        menu->clear();
        if (Project * const project = ProjectManager::startupProject()) {
            for (const auto &generator : project->allGenerators()) {
                menu->addAction(generator.second, [project, id = generator.first] {
                    project->runGenerator(id);
                });
            }
        }
    });

    // deploy action
    dd->m_deployAction = new QAction(Tr::tr("Deploy"), this);
    dd->m_deployAction->setWhatsThis(Tr::tr("Deploy Project"));
    cmd = ActionManager::registerAction(dd->m_deployAction, Constants::DEPLOY);
    cmd->setAttribute(Command::CA_UpdateText);
    cmd->setDescription(dd->m_deployAction->whatsThis());
    mbuild->addAction(cmd, Constants::G_BUILD_PROJECT);

    // rebuild action
    dd->m_rebuildAction = new QAction(Icons::REBUILD.icon(), Tr::tr("Rebuild"), this);
    dd->m_rebuildAction->setWhatsThis(Tr::tr("Rebuild Project"));
    cmd = ActionManager::registerAction(dd->m_rebuildAction, Constants::REBUILD);
    cmd->setAttribute(Command::CA_UpdateText);
    cmd->setDescription(dd->m_rebuildAction->whatsThis());
    mbuild->addAction(cmd, Constants::G_BUILD_PROJECT);

    dd->m_rebuildProjectForAllConfigsAction
            = new QAction(Icons::REBUILD.icon(), Tr::tr("Rebuild"), this);
    dd->m_rebuildProjectForAllConfigsAction->setWhatsThis(
        Tr::tr("Rebuild Project for All Configurations"));
    cmd = ActionManager::registerAction(dd->m_rebuildProjectForAllConfigsAction,
                                        Constants::REBUILDALLCONFIGS);
    cmd->setAttribute(Command::CA_UpdateText);
    cmd->setDescription(dd->m_rebuildProjectForAllConfigsAction->whatsThis());
    mbuild->addAction(cmd, Constants::G_BUILD_PROJECT_ALLCONFIGURATIONS);

    // clean action
    dd->m_cleanAction = new QAction(Utils::Icons::CLEAN.icon(), Tr::tr("Clean"), this);
    dd->m_cleanAction->setWhatsThis(Tr::tr("Clean Project"));
    cmd = ActionManager::registerAction(dd->m_cleanAction, Constants::CLEAN);
    cmd->setAttribute(Command::CA_UpdateText);
    cmd->setDescription(dd->m_cleanAction->whatsThis());
    mbuild->addAction(cmd, Constants::G_BUILD_PROJECT);

    dd->m_cleanProjectForAllConfigsAction
            = new QAction(Utils::Icons::CLEAN.icon(), Tr::tr("Clean"), this);
    dd->m_cleanProjectForAllConfigsAction->setWhatsThis(Tr::tr("Clean Project for All Configurations"));
    cmd = ActionManager::registerAction(dd->m_cleanProjectForAllConfigsAction,
                                        Constants::CLEANALLCONFIGS);
    cmd->setAttribute(Command::CA_UpdateText);
    cmd->setDescription(dd->m_cleanProjectForAllConfigsAction->whatsThis());
    mbuild->addAction(cmd, Constants::G_BUILD_PROJECT_ALLCONFIGURATIONS);

    // cancel build action
    dd->m_cancelBuildAction = new QAction(Utils::Icons::STOP_SMALL.icon(), Tr::tr("Cancel Build"), this);
    cmd = ActionManager::registerAction(dd->m_cancelBuildAction, Constants::CANCELBUILD);
    cmd->setDefaultKeySequence(QKeySequence(useMacShortcuts ? Tr::tr("Meta+Backspace") : Tr::tr("Alt+Backspace")));
    mbuild->addAction(cmd, Constants::G_BUILD_CANCEL);

    // run action
    dd->m_runAction = new QAction(runIcon, Tr::tr("Run"), this);
    cmd = ActionManager::registerAction(dd->m_runAction, Constants::RUN);
    cmd->setAttribute(Command::CA_UpdateText);

    cmd->setDefaultKeySequence(QKeySequence(Tr::tr("Ctrl+R")));
    mbuild->addAction(cmd, Constants::G_BUILD_RUN);

    cmd->action()->setObjectName("Run"); // used for UI introduction
    ModeManager::addAction(cmd->action(), Constants::P_ACTION_RUN);

    // Run without deployment action
    dd->m_runWithoutDeployAction = new QAction(Tr::tr("Run Without Deployment"), this);
    cmd = ActionManager::registerAction(dd->m_runWithoutDeployAction, Constants::RUNWITHOUTDEPLOY);
    mbuild->addAction(cmd, Constants::G_BUILD_RUN);

    // build action with dependencies (context menu)
    dd->m_buildDependenciesActionContextMenu = new QAction(Tr::tr("Build"), this);
    cmd = ActionManager::registerAction(dd->m_buildDependenciesActionContextMenu, Constants::BUILDDEPENDCM, projectTreeContext);
    cmd->setAttribute(Command::CA_UpdateText);
    mprojectContextMenu->addAction(cmd, Constants::G_PROJECT_BUILD);

    // build action (context menu)
    dd->m_buildActionContextMenu = new QAction(Tr::tr("Build Without Dependencies"), this);
    cmd = ActionManager::registerAction(dd->m_buildActionContextMenu, Constants::BUILDCM, projectTreeContext);
    cmd->setAttribute(Command::CA_UpdateText);
    mprojectContextMenu->addAction(cmd, Constants::G_PROJECT_BUILD);

    // rebuild action with dependencies (context menu)
    dd->m_rebuildDependenciesActionContextMenu = new QAction(Tr::tr("Rebuild"), this);
    cmd = ActionManager::registerAction(dd->m_rebuildDependenciesActionContextMenu, Constants::REBUILDDEPENDCM, projectTreeContext);
    cmd->setAttribute(Command::CA_UpdateText);
    mprojectContextMenu->addAction(cmd, Constants::G_PROJECT_REBUILD);

    // rebuild action (context menu)
    dd->m_rebuildActionContextMenu = new QAction(Tr::tr("Rebuild Without Dependencies"), this);
    cmd = ActionManager::registerAction(dd->m_rebuildActionContextMenu, Constants::REBUILDCM, projectTreeContext);
    cmd->setAttribute(Command::CA_UpdateText);
    mprojectContextMenu->addAction(cmd, Constants::G_PROJECT_REBUILD);

    // clean action with dependencies (context menu)
    dd->m_cleanDependenciesActionContextMenu = new QAction(Tr::tr("Clean"), this);
    cmd = ActionManager::registerAction(dd->m_cleanDependenciesActionContextMenu, Constants::CLEANDEPENDCM, projectTreeContext);
    cmd->setAttribute(Command::CA_UpdateText);
    mprojectContextMenu->addAction(cmd, Constants::G_PROJECT_REBUILD);

    // clean action (context menu)
    dd->m_cleanActionContextMenu = new QAction(Tr::tr("Clean Without Dependencies"), this);
    cmd = ActionManager::registerAction(dd->m_cleanActionContextMenu, Constants::CLEANCM, projectTreeContext);
    cmd->setAttribute(Command::CA_UpdateText);
    mprojectContextMenu->addAction(cmd, Constants::G_PROJECT_REBUILD);

    // build without dependencies action
    dd->m_buildProjectOnlyAction = new QAction(Tr::tr("Build Without Dependencies"), this);
    ActionManager::registerAction(dd->m_buildProjectOnlyAction, Constants::BUILDPROJECTONLY);

    // rebuild without dependencies action
    dd->m_rebuildProjectOnlyAction = new QAction(Tr::tr("Rebuild Without Dependencies"), this);
    ActionManager::registerAction(dd->m_rebuildProjectOnlyAction, Constants::REBUILDPROJECTONLY);

    // deploy without dependencies action
    dd->m_deployProjectOnlyAction = new QAction(Tr::tr("Deploy Without Dependencies"), this);
    ActionManager::registerAction(dd->m_deployProjectOnlyAction, Constants::DEPLOYPROJECTONLY);

    // clean without dependencies action
    dd->m_cleanProjectOnlyAction = new QAction(Tr::tr("Clean Without Dependencies"), this);
    ActionManager::registerAction(dd->m_cleanProjectOnlyAction, Constants::CLEANPROJECTONLY);

    // deploy action (context menu)
    dd->m_deployActionContextMenu = new QAction(Tr::tr("Deploy"), this);
    cmd = ActionManager::registerAction(dd->m_deployActionContextMenu, Constants::DEPLOYCM, projectTreeContext);
    mprojectContextMenu->addAction(cmd, Constants::G_PROJECT_RUN);

    dd->m_runActionContextMenu = new QAction(runIcon, Tr::tr("Run"), this);
    cmd = ActionManager::registerAction(dd->m_runActionContextMenu, Constants::RUNCONTEXTMENU, projectTreeContext);
    mprojectContextMenu->addAction(cmd, Constants::G_PROJECT_RUN);
    msubProjectContextMenu->addAction(cmd, Constants::G_PROJECT_RUN);

    // add new file action
    dd->m_addNewFileAction = new QAction(Tr::tr("Add New..."), this);
    cmd = ActionManager::registerAction(dd->m_addNewFileAction, Constants::ADDNEWFILE,
                       projectTreeContext);
    mprojectContextMenu->addAction(cmd, Constants::G_PROJECT_FILES);
    msubProjectContextMenu->addAction(cmd, Constants::G_PROJECT_FILES);
    mfolderContextMenu->addAction(cmd, Constants::G_FOLDER_FILES);

    // add existing file action
    dd->m_addExistingFilesAction = new QAction(Tr::tr("Add Existing Files..."), this);
    cmd = ActionManager::registerAction(dd->m_addExistingFilesAction, Constants::ADDEXISTINGFILES,
                       projectTreeContext);
    mprojectContextMenu->addAction(cmd, Constants::G_PROJECT_FILES);
    msubProjectContextMenu->addAction(cmd, Constants::G_PROJECT_FILES);
    mfolderContextMenu->addAction(cmd, Constants::G_FOLDER_FILES);

    // add existing projects action
    dd->m_addExistingProjectsAction = new QAction(Tr::tr("Add Existing Projects..."), this);
    cmd = ActionManager::registerAction(dd->m_addExistingProjectsAction,
                                        "ProjectExplorer.AddExistingProjects", projectTreeContext);
    mprojectContextMenu->addAction(cmd, Constants::G_PROJECT_FILES);
    msubProjectContextMenu->addAction(cmd, Constants::G_PROJECT_FILES);

    // add existing directory action
    dd->m_addExistingDirectoryAction = new QAction(Tr::tr("Add Existing Directory..."), this);
    cmd = ActionManager::registerAction(dd->m_addExistingDirectoryAction,
                                              Constants::ADDEXISTINGDIRECTORY,
                                              projectTreeContext);
    mprojectContextMenu->addAction(cmd, Constants::G_PROJECT_FILES);
    msubProjectContextMenu->addAction(cmd, Constants::G_PROJECT_FILES);
    mfolderContextMenu->addAction(cmd, Constants::G_FOLDER_FILES);

    // new subproject action
    dd->m_addNewSubprojectAction = new QAction(Tr::tr("New Subproject..."), this);
    cmd = ActionManager::registerAction(dd->m_addNewSubprojectAction, Constants::ADDNEWSUBPROJECT,
                       projectTreeContext);
    mprojectContextMenu->addAction(cmd, Constants::G_PROJECT_FILES);
    msubProjectContextMenu->addAction(cmd, Constants::G_PROJECT_FILES);

    dd->m_closeProjectFilesActionContextMenu = new ParameterAction(
                Tr::tr("Close All Files"), Tr::tr("Close All Files in Project \"%1\""),
                ParameterAction::EnabledWithParameter, this);
    cmd = ActionManager::registerAction(dd->m_closeProjectFilesActionContextMenu,
                                        "ProjectExplorer.CloseAllFilesInProjectContextMenu");
    cmd->setAttribute(Command::CA_UpdateText);
    cmd->setDescription(dd->m_closeProjectFilesActionContextMenu->text());
    mprojectContextMenu->addAction(cmd, Constants::G_PROJECT_CLOSE);

    // unload project again, in right position
    dd->m_unloadActionContextMenu = new ParameterAction(Tr::tr("Close Project"), Tr::tr("Close Project \"%1\""),
                                                              ParameterAction::EnabledWithParameter, this);
    cmd = ActionManager::registerAction(dd->m_unloadActionContextMenu, Constants::UNLOADCM);
    cmd->setAttribute(Command::CA_UpdateText);
    cmd->setDescription(dd->m_unloadActionContextMenu->text());
    mprojectContextMenu->addAction(cmd, Constants::G_PROJECT_CLOSE);

    dd->m_unloadOthersActionContextMenu = new ParameterAction(Tr::tr("Close Other Projects"), Tr::tr("Close All Projects Except \"%1\""),
                                                              ParameterAction::EnabledWithParameter, this);
    cmd = ActionManager::registerAction(dd->m_unloadOthersActionContextMenu, Constants::UNLOADOTHERSCM);
    cmd->setAttribute(Command::CA_UpdateText);
    cmd->setDescription(dd->m_unloadOthersActionContextMenu->text());
    mprojectContextMenu->addAction(cmd, Constants::G_PROJECT_CLOSE);

    // file properties action
    dd->m_filePropertiesAction = new QAction(Tr::tr("Properties..."), this);
    cmd = ActionManager::registerAction(dd->m_filePropertiesAction, Constants::FILEPROPERTIES,
                       projectTreeContext);
    mfileContextMenu->addAction(cmd, Constants::G_FILE_OTHER);

    // remove file action
    dd->m_removeFileAction = new QAction(Tr::tr("Remove..."), this);
    cmd = ActionManager::registerAction(dd->m_removeFileAction, Constants::REMOVEFILE,
                       projectTreeContext);
    cmd->setDefaultKeySequences({QKeySequence::Delete, QKeySequence::Backspace});
    mfileContextMenu->addAction(cmd, Constants::G_FILE_OTHER);

    // duplicate file action
    dd->m_duplicateFileAction = new QAction(Tr::tr("Duplicate File..."), this);
    cmd = ActionManager::registerAction(dd->m_duplicateFileAction, Constants::DUPLICATEFILE,
                       projectTreeContext);
    mfileContextMenu->addAction(cmd, Constants::G_FILE_OTHER);

    //: Remove project from parent profile (Project explorer view); will not physically delete any files.
    dd->m_removeProjectAction = new QAction(Tr::tr("Remove Project..."), this);
    cmd = ActionManager::registerAction(dd->m_removeProjectAction, Constants::REMOVEPROJECT,
                       projectTreeContext);
    msubProjectContextMenu->addAction(cmd, Constants::G_PROJECT_FILES);

    // delete file action
    dd->m_deleteFileAction = new QAction(Tr::tr("Delete File..."), this);
    cmd = ActionManager::registerAction(dd->m_deleteFileAction, Constants::DELETEFILE,
                             projectTreeContext);
    cmd->setDefaultKeySequences({QKeySequence::Delete, QKeySequence::Backspace});
    mfileContextMenu->addAction(cmd, Constants::G_FILE_OTHER);

    // renamefile action
    dd->m_renameFileAction = new QAction(Tr::tr("Rename..."), this);
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
    dd->m_setStartupProjectAction = new ParameterAction(Tr::tr("Set as Active Project"),
                                                        Tr::tr("Set \"%1\" as Active Project"),
                                                        ParameterAction::AlwaysEnabled, this);
    cmd = ActionManager::registerAction(dd->m_setStartupProjectAction, Constants::SETSTARTUP,
                             projectTreeContext);
    cmd->setAttribute(Command::CA_UpdateText);
    cmd->setDescription(dd->m_setStartupProjectAction->text());
    mprojectContextMenu->addAction(cmd, Constants::G_PROJECT_FIRST);

    // Collapse & Expand.
    const Id treeGroup = Constants::G_PROJECT_TREE;

    dd->m_projectTreeExpandNodeAction = new QAction(Tr::tr("Expand"), this);
    connect(dd->m_projectTreeExpandNodeAction, &QAction::triggered,
            ProjectTree::instance(), &ProjectTree::expandCurrentNodeRecursively);
    Command * const expandNodeCmd = ActionManager::registerAction(
                dd->m_projectTreeExpandNodeAction, "ProjectExplorer.ExpandNode",
                projectTreeContext);
    dd->m_projectTreeCollapseAllAction = new QAction(Tr::tr("Collapse All"), this);
    Command * const collapseCmd = ActionManager::registerAction(
                dd->m_projectTreeCollapseAllAction, Constants::PROJECTTREE_COLLAPSE_ALL,
                projectTreeContext);
    dd->m_projectTreeExpandAllAction = new QAction(Tr::tr("Expand All"), this);
    Command * const expandCmd = ActionManager::registerAction(
                dd->m_projectTreeExpandAllAction, Constants::PROJECTTREE_EXPAND_ALL,
                projectTreeContext);
    for (ActionContainer * const ac : {mfileContextMenu, msubProjectContextMenu,
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
    dd->m_projectSelectorActionMenu->setText(Tr::tr("Open Build and Run Kit Selector..."));
    connect(dd->m_projectSelectorActionMenu, &QAction::triggered,
            dd->m_targetSelector,
            &MiniProjectTargetSelector::toggleVisible);
    cmd = ActionManager::registerAction(dd->m_projectSelectorActionMenu, Constants::SELECTTARGET);
    mbuild->addAction(cmd, Constants::G_BUILD_RUN);

    dd->m_projectSelectorActionQuick = new QAction(this);
    dd->m_projectSelectorActionQuick->setEnabled(false);
    dd->m_projectSelectorActionQuick->setText(Tr::tr("Quick Switch Kit Selector"));
    connect(dd->m_projectSelectorActionQuick, &QAction::triggered,
            dd->m_targetSelector, &MiniProjectTargetSelector::nextOrShow);
    cmd = ActionManager::registerAction(dd->m_projectSelectorActionQuick, Constants::SELECTTARGETQUICK);
    cmd->setDefaultKeySequence(QKeySequence(Tr::tr("Ctrl+T")));

    connect(ICore::instance(), &ICore::saveSettingsRequested,
            dd, &ProjectExplorerPluginPrivate::savePersistentSettings);
    connect(qApp, &QApplication::applicationStateChanged, this, [](Qt::ApplicationState state) {
        if (!PluginManager::isShuttingDown() && state == Qt::ApplicationActive)
            dd->updateWelcomePage();
    });

    QtcSettings *s = ICore::settings();
    const QStringList fileNames = s->value(Constants::RECENTPROJECTS_FILE_NAMES_KEY).toStringList();
    const QStringList displayNames = s->value(Constants::RECENTPROJECTS_DISPLAY_NAMES_KEY)
                                         .toStringList();
    if (fileNames.size() == displayNames.size()) {
        for (int i = 0; i < fileNames.size(); ++i) {
            dd->m_recentProjects.append({FilePath::fromUserInput(fileNames.at(i)), displayNames.at(i)});
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

    static const ProjectExplorerSettings defaultSettings;

    dd->m_projectExplorerSettings.deployBeforeRun
        = s->value(Constants::DEPLOY_BEFORE_RUN_SETTINGS_KEY, defaultSettings.deployBeforeRun)
              .toBool();
    dd->m_projectExplorerSettings.saveBeforeBuild
        = s->value(Constants::SAVE_BEFORE_BUILD_SETTINGS_KEY, defaultSettings.saveBeforeBuild)
              .toBool();
    dd->m_projectExplorerSettings.useJom
        = s->value(Constants::USE_JOM_SETTINGS_KEY, defaultSettings.useJom).toBool();
    dd->m_projectExplorerSettings.addLibraryPathsToRunEnv
        = s->value(Constants::ADD_LIBRARY_PATHS_TO_RUN_ENV_SETTINGS_KEY,
                   defaultSettings.addLibraryPathsToRunEnv)
              .toBool();
    dd->m_projectExplorerSettings.prompToStopRunControl
        = s->value(Constants::PROMPT_TO_STOP_RUN_CONTROL_SETTINGS_KEY,
                   defaultSettings.prompToStopRunControl)
              .toBool();
    dd->m_projectExplorerSettings.automaticallyCreateRunConfigurations
        = s->value(Constants::AUTO_CREATE_RUN_CONFIGS_SETTINGS_KEY,
                   defaultSettings.automaticallyCreateRunConfigurations)
              .toBool();
    dd->m_projectExplorerSettings.environmentId =
            QUuid(s->value(Constants::ENVIRONMENT_ID_SETTINGS_KEY).toByteArray());
    if (dd->m_projectExplorerSettings.environmentId.isNull())
        dd->m_projectExplorerSettings.environmentId = QUuid::createUuid();
    int tmp = s->value(Constants::STOP_BEFORE_BUILD_SETTINGS_KEY,
                       int(defaultSettings.stopBeforeBuild))
                  .toInt();
    if (tmp < 0 || tmp > int(StopBeforeBuild::SameApp))
        tmp = int(defaultSettings.stopBeforeBuild);
    dd->m_projectExplorerSettings.stopBeforeBuild = StopBeforeBuild(tmp);
    dd->m_projectExplorerSettings.terminalMode = static_cast<TerminalMode>(
        s->value(Constants::TERMINAL_MODE_SETTINGS_KEY, int(defaultSettings.terminalMode)).toInt());
    dd->m_projectExplorerSettings.closeSourceFilesWithProject
        = s->value(Constants::CLOSE_FILES_WITH_PROJECT_SETTINGS_KEY,
                   defaultSettings.closeSourceFilesWithProject)
              .toBool();
    dd->m_projectExplorerSettings.clearIssuesOnRebuild
        = s->value(Constants::CLEAR_ISSUES_ON_REBUILD_SETTINGS_KEY,
                   defaultSettings.clearIssuesOnRebuild)
              .toBool();
    dd->m_projectExplorerSettings.abortBuildAllOnError
        = s->value(Constants::ABORT_BUILD_ALL_ON_ERROR_SETTINGS_KEY,
                   defaultSettings.abortBuildAllOnError)
              .toBool();
    dd->m_projectExplorerSettings.lowBuildPriority
        = s->value(Constants::LOW_BUILD_PRIORITY_SETTINGS_KEY, defaultSettings.lowBuildPriority)
              .toBool();

    const int customParserCount = s->value(Constants::CUSTOM_PARSER_COUNT_KEY).toInt();
    for (int i = 0; i < customParserCount; ++i) {
        CustomParserSettings settings;
        settings.fromMap(storeFromVariant(
            s->value(numberedKey(Constants::CUSTOM_PARSER_PREFIX_KEY, i))));
        dd->m_customParsers << settings;
    }

    auto buildManager = new BuildManager(this, dd->m_cancelBuildAction);
    connect(buildManager, &BuildManager::buildStateChanged,
            dd, &ProjectExplorerPluginPrivate::updateActions);
    connect(buildManager, &BuildManager::buildQueueFinished,
            dd, &ProjectExplorerPluginPrivate::buildQueueFinished, Qt::QueuedConnection);

    connect(dd->m_newAction, &QAction::triggered,
            dd, &ProjectExplorerPlugin::openNewProjectDialog);
    connect(dd->m_loadAction, &QAction::triggered,
            dd, &ProjectExplorerPluginPrivate::loadAction);
    connect(dd->m_buildProjectOnlyAction, &QAction::triggered, dd, [] {
        BuildManager::buildProjectWithoutDependencies(ProjectManager::startupProject());
    });
    connect(dd->m_buildAction, &QAction::triggered, dd, [] {
        BuildManager::buildProjectWithDependencies(ProjectManager::startupProject());
    });
    connect(dd->m_buildProjectForAllConfigsAction, &QAction::triggered, dd, [] {
        BuildManager::buildProjectWithDependencies(ProjectManager::startupProject(),
                                                   ConfigSelection::All);
    });
    connect(dd->m_buildActionContextMenu, &QAction::triggered, dd, [] {
        BuildManager::buildProjectWithoutDependencies(ProjectTree::currentProject());
    });
    connect(dd->m_buildForRunConfigAction, &QAction::triggered, dd, [] {
        const Project * const project = ProjectManager::startupProject();
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
        BuildManager::buildProjects(ProjectManager::projectOrder(), ConfigSelection::Active);
    });
    connect(dd->m_buildSessionForAllConfigsAction, &QAction::triggered, dd, [] {
        BuildManager::buildProjects(ProjectManager::projectOrder(), ConfigSelection::All);
    });
    connect(dd->m_rebuildProjectOnlyAction, &QAction::triggered, dd, [] {
        BuildManager::rebuildProjectWithoutDependencies(ProjectManager::startupProject());
    });
    connect(dd->m_rebuildAction, &QAction::triggered, dd, [] {
        BuildManager::rebuildProjectWithDependencies(ProjectManager::startupProject(),
                                                     ConfigSelection::Active);
    });
    connect(dd->m_rebuildProjectForAllConfigsAction, &QAction::triggered, dd, [] {
        BuildManager::rebuildProjectWithDependencies(ProjectManager::startupProject(),
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
        BuildManager::rebuildProjects(ProjectManager::projectOrder(), ConfigSelection::Active);
    });
    connect(dd->m_rebuildSessionForAllConfigsAction, &QAction::triggered, dd, [] {
        BuildManager::rebuildProjects(ProjectManager::projectOrder(), ConfigSelection::All);
    });
    connect(dd->m_deployProjectOnlyAction, &QAction::triggered, dd, [] {
        BuildManager::deployProjects({ProjectManager::startupProject()});
    });
    connect(dd->m_deployAction, &QAction::triggered, dd, [] {
        BuildManager::deployProjects(ProjectManager::projectOrder(ProjectManager::startupProject()));
    });
    connect(dd->m_deployActionContextMenu, &QAction::triggered, dd, [] {
        BuildManager::deployProjects({ProjectTree::currentProject()});
    });
    connect(dd->m_deploySessionAction, &QAction::triggered, dd, [] {
        BuildManager::deployProjects(ProjectManager::projectOrder());
    });
    connect(dd->m_cleanProjectOnlyAction, &QAction::triggered, dd, [] {
        BuildManager::cleanProjectWithoutDependencies(ProjectManager::startupProject());
    });
    connect(dd->m_cleanAction, &QAction::triggered, dd, [] {
        BuildManager::cleanProjectWithDependencies(ProjectManager::startupProject(),
                                                   ConfigSelection::Active);
    });
    connect(dd->m_cleanProjectForAllConfigsAction, &QAction::triggered, dd, [] {
        BuildManager::cleanProjectWithDependencies(ProjectManager::startupProject(),
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
        BuildManager::cleanProjects(ProjectManager::projectOrder(), ConfigSelection::Active);
    });
    connect(dd->m_cleanSessionForAllConfigsAction, &QAction::triggered, dd, [] {
        BuildManager::cleanProjects(ProjectManager::projectOrder(), ConfigSelection::All);
    });
    connect(dd->m_runAction, &QAction::triggered,
            dd, [] { runStartupProject(Constants::NORMAL_RUN_MODE); });
    connect(dd->m_runActionContextMenu, &QAction::triggered,
            dd, [] { dd->runProjectContextMenu(dd->m_defaultRunConfiguration); });
    connect(dd->m_runWithoutDeployAction, &QAction::triggered,
            dd, [] { runStartupProject(Constants::NORMAL_RUN_MODE, true); });
    connect(dd->m_cancelBuildAction, &QAction::triggered,
            BuildManager::instance(), &BuildManager::cancel);
    connect(dd->m_unloadAction, &QAction::triggered,
            dd, &ProjectExplorerPluginPrivate::handleUnloadProject);
    connect(dd->m_unloadActionContextMenu, &QAction::triggered,
            dd, &ProjectExplorerPluginPrivate::unloadProjectContextMenu);
    connect(dd->m_unloadOthersActionContextMenu, &QAction::triggered,
            dd, &ProjectExplorerPluginPrivate::unloadOtherProjectsContextMenu);
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
    // the following can delete the projects view that triggered the action, so make sure we
    // are out of the context menu before actually doing it by queuing the action
    connect(dd->m_showFileSystemPane,
            &QAction::triggered,
            dd,
            &ProjectExplorerPluginPrivate::showInFileSystemPane,
            Qt::QueuedConnection);

    connect(dd->m_openTerminalHere, &QAction::triggered, dd, [] { dd->openTerminalHere(sysEnv); });
    connect(dd->m_openTerminalHereBuildEnv, &QAction::triggered, dd, [] { dd->openTerminalHere(buildEnv); });
    connect(dd->m_openTerminalHereRunEnv, &QAction::triggered, dd, [] { dd->openTerminalHereWithRunEnv(); });

    connect(dd->m_filePropertiesAction, &QAction::triggered, this, [] {
                const Node *currentNode = ProjectTree::currentNode();
                QTC_ASSERT(currentNode && currentNode->asFileNode(), return);
                ProjectTree::CurrentNodeKeeper nodeKeeper;
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
            dd, [] { dd->closeAllFilesInProject(ProjectManager::projects().first()); });
    connect(dd->m_closeProjectFilesActionContextMenu, &QAction::triggered,
            dd, [] { dd->closeAllFilesInProject(ProjectTree::currentProject()); });
    connect(dd->m_projectTreeCollapseAllAction, &QAction::triggered,
            ProjectTree::instance(), &ProjectTree::collapseAll);
    connect(dd->m_projectTreeExpandAllAction, &QAction::triggered,
            ProjectTree::instance(), &ProjectTree::expandAll);

    connect(this, &ProjectExplorerPlugin::settingsChanged,
            dd, &ProjectExplorerPluginPrivate::updateRunWithoutDeployMenu);

    connect(ICore::instance(), &ICore::newItemDialogStateChanged, dd, [] {
        dd->updateContextMenuActions(ProjectTree::currentNode());
    });

    connect(ModeManager::instance(),
            &ModeManager::currentModeChanged,
            dd,
            &ProjectExplorerPluginPrivate::currentModeChanged);
    connect(&dd->m_welcomePage,
            &ProjectWelcomePage::requestProject,
            m_instance,
            &ProjectExplorerPlugin::openProjectWelcomePage);
    connect(SessionManager::instance(),
            &SessionManager::startupSessionRestored,
            m_instance,
            &ProjectExplorerPlugin::finishedInitialization);
    dd->updateWelcomePage();

    MacroExpander *expander = Utils::globalMacroExpander();
    // Global variables for the project that the current document belongs to.
    Project::addVariablesToMacroExpander("CurrentDocument:Project:",
                                         "Project of current document",
                                         expander,
                                         &ProjectTree::currentProject);
    EnvironmentProvider::addProvider(
        {"CurrentDocument:Project:BuildConfig:Env", Tr::tr("Current Build Environment"), [] {
             if (BuildConfiguration *bc = currentBuildConfiguration())
                 return bc->environment();
             return Environment::systemEnvironment();
         }});
    EnvironmentProvider::addProvider(
        {"CurrentDocument:Project:RunConfig:Env", Tr::tr("Current Run Environment"), [] {
             const Project *const project = ProjectTree::currentProject();
             const Target *const target = project ? project->activeTarget() : nullptr;
             const RunConfiguration *const rc = target ? target->activeRunConfiguration() : nullptr;
             if (rc) {
                 if (auto envAspect = rc->aspect<EnvironmentAspect>())
                     return envAspect->environment();
             }
             return Environment::systemEnvironment();
         }});

    // Global variables for the active project.
    Project::addVariablesToMacroExpander("ActiveProject:",
                                         "Active project",
                                         expander,
                                         &ProjectManager::startupProject);
    EnvironmentProvider::addProvider(
        {"ActiveProject:BuildConfig:Env", Tr::tr("Active build environment of the active project."), [] {
             if (const BuildConfiguration * const bc = activeBuildConfiguration())
                 return bc->environment();
             return Environment::systemEnvironment();
         }});
    EnvironmentProvider::addProvider(
        {"ActiveProject:RunConfig:Env", Tr::tr("Active run environment of the active project."), [] {
             if (const RunConfiguration *const rc = activeRunConfiguration()) {
                 if (auto envAspect = rc->aspect<EnvironmentAspect>())
                     return envAspect->environment();
             }
             return Environment::systemEnvironment();
         }});

    DeviceManager::instance()->addDevice(IDevice::Ptr(new DesktopDevice));

    if (auto sanitizerTester = SanitizerParser::testCreator())
        addTestCreator(sanitizerTester.value());

    return true;
}

void ProjectExplorerPluginPrivate::loadAction()
{
    FilePath dir = dd->m_lastOpenDirectory;

    // for your special convenience, we preselect a pro file if it is
    // the current file
    if (const IDocument *document = EditorManager::currentDocument()) {
        const FilePath fn = document->filePath();
        const bool isProject = dd->m_profileMimeTypes.contains(document->mimeType());
        dir = isProject ? fn : fn.absolutePath();
    }

    FilePath filePath = Utils::FileUtils::getOpenFilePath(ICore::dialogParent(),
                                                          Tr::tr("Load Project"),
                                                          dir,
                                                          dd->projectFilterString());
    if (filePath.isEmpty())
        return;

    ProjectExplorerPlugin::OpenProjectResult result = ProjectExplorerPlugin::openProject(filePath);
    if (!result)
        ProjectExplorerPlugin::showOpenProjectError(result);

    updateActions();
}

void ProjectExplorerPluginPrivate::unloadProjectContextMenu()
{
    if (Project *p = ProjectTree::currentProject())
        ProjectExplorerPlugin::unloadProject(p);
}

void ProjectExplorerPluginPrivate::unloadOtherProjectsContextMenu()
{
    if (Project *currentProject = ProjectTree::currentProject()) {
        const QList<Project *> projects = ProjectManager::projects();
        QTC_ASSERT(!projects.isEmpty(), return);

        for (Project *p : projects) {
            if (p == currentProject)
                continue;
            ProjectExplorerPlugin::unloadProject(p);
        }
    }
}

void ProjectExplorerPluginPrivate::handleUnloadProject()
{
    QList<Project *> projects = ProjectManager::projects();
    QTC_ASSERT(!projects.isEmpty(), return);

    ProjectExplorerPlugin::unloadProject(projects.first());
}

void ProjectExplorerPlugin::unloadProject(Project *project)
{
    if (BuildManager::isBuilding(project)) {
        QMessageBox box;
        QPushButton *closeAnyway = box.addButton(Tr::tr("Cancel Build && Unload"), QMessageBox::AcceptRole);
        QPushButton *cancelClose = box.addButton(Tr::tr("Do Not Unload"), QMessageBox::RejectRole);
        box.setDefaultButton(cancelClose);
        box.setWindowTitle(Tr::tr("Unload Project %1?").arg(project->displayName()));
        box.setText(Tr::tr("The project %1 is currently being built.").arg(project->displayName()));
        box.setInformativeText(Tr::tr("Do you want to cancel the build process and unload the project anyway?"));
        box.exec();
        if (box.clickedButton() != closeAnyway)
            return;
        BuildManager::cancel();
    }

    if (projectExplorerSettings().closeSourceFilesWithProject && !dd->closeAllFilesInProject(project))
        return;

    dd->addToRecentProjects(project->projectFilePath(), project->displayName());

    ProjectManager::removeProject(project);
    dd->updateActions();
}

void ProjectExplorerPluginPrivate::closeAllProjects()
{
    if (!EditorManager::closeAllDocuments())
        return; // Action has been cancelled

    ProjectManager::closeAllProjects();
    updateActions();

    ModeManager::activateMode(Core::Constants::MODE_WELCOME);
}

void ProjectExplorerPlugin::extensionsInitialized()
{
    CustomWizard::createWizards();
    JsonWizardFactory::createWizardFactories();

    // Register factories for all project managers

    dd->m_documentFactory.setOpener([](FilePath filePath) {
        if (filePath.isDir()) {
            const FilePaths files = projectFilesInDirectory(filePath.absoluteFilePath());
            if (!files.isEmpty())
                filePath = files.front();
        }

        OpenProjectResult result = ProjectExplorerPlugin::openProject(filePath);
        if (!result)
            showOpenProjectError(result);
        return nullptr;
    });

    dd->m_documentFactory.addMimeType(QStringLiteral("inode/directory"));
    for (auto it = dd->m_projectCreators.cbegin(); it != dd->m_projectCreators.cend(); ++it) {
        const QString &mimeType = it.key();
        dd->m_documentFactory.addMimeType(mimeType);
        dd->m_profileMimeTypes += mimeType;
    }

    dd->m_taskFileFactory.addMimeType("text/x-tasklist");
    dd->m_taskFileFactory.setOpener([](const FilePath &filePath) {
        return TaskFile::openTasks(filePath);
    });

    BuildManager::extensionsInitialized();
    TaskHub::addCategory({Constants::TASK_CATEGORY_SANITIZER,
                          Tr::tr("Sanitizer", "Category for sanitizer issues listed under 'Issues'"),
                          Tr::tr("Memory handling issues that the address sanitizer found.")});
    TaskHub::addCategory({Constants::TASK_CATEGORY_TASKLIST_ID,
                          Tr::tr("My Tasks"),
                          Tr::tr("Issues from a task list file (.tasks).")});

    SshSettings::loadSettings(ICore::settings());
    const auto searchPathRetriever = [] {
        FilePaths searchPaths = {ICore::libexecPath()};
        if (HostOsInfo::isWindowsHost()) {
            const QString gitBinary = ICore::settings()->value("Git/BinaryPath", "git")
                    .toString();
            const QStringList rawGitSearchPaths = ICore::settings()->value("Git/Path")
                    .toString().split(':', Qt::SkipEmptyParts);
            const FilePaths gitSearchPaths = Utils::transform(rawGitSearchPaths,
                    [](const QString &rawPath) { return FilePath::fromUserInput(rawPath); });
            const FilePath fullGitPath = Environment::systemEnvironment()
                    .searchInPath(gitBinary, gitSearchPaths);
            if (!fullGitPath.isEmpty()) {
                searchPaths << fullGitPath.parentDir()
                            << fullGitPath.parentDir().parentDir().pathAppended("usr/bin");
            }
        }
        return searchPaths;
    };
    SshSettings::setExtraSearchPathRetriever(searchPathRetriever);

    const auto parseIssuesAction = new QAction(Tr::tr("Parse Build Output..."), this);
    ActionContainer *mtools = ActionManager::actionContainer(Core::Constants::M_TOOLS);
    Command * const cmd = ActionManager::registerAction(parseIssuesAction,
                                                        "ProjectExplorer.ParseIssuesAction");
    connect(parseIssuesAction, &QAction::triggered, this, [] {
        ParseIssuesDialog dlg(ICore::dialogParent());
        dlg.exec();
    });
    mtools->addAction(cmd);

    // Load devices immediately, as other plugins might want to use them
    DeviceManager::instance()->load();
}

bool ProjectExplorerPlugin::delayedInitialize()
{
    NANOTRACE_SCOPE("ProjectExplorer", "ProjectExplorerPlugin::restoreKits");
    ExtraAbi::load(); // Load this before Toolchains!
    ToolChainManager::restoreToolChains();
    KitManager::restoreKits();
    return true;
}

void ProjectExplorerPluginPrivate::updateRunWithoutDeployMenu()
{
    m_runWithoutDeployAction->setVisible(m_projectExplorerSettings.deployBeforeRun);
}

IPlugin::ShutdownFlag ProjectExplorerPlugin::aboutToShutdown()
{
    disconnect(ModeManager::instance(), &ModeManager::currentModeChanged,
               dd, &ProjectExplorerPluginPrivate::currentModeChanged);
    ProjectTree::aboutToShutDown();
    ToolChainManager::aboutToShutdown();
    ProjectManager::closeAllProjects();

    // Attempt to synchronously shutdown all run controls.
    // If that fails, fall back to asynchronous shutdown (Debugger run controls
    // might shutdown asynchronously).
    if (dd->m_activeRunControlCount == 0)
        return SynchronousShutdown;

    dd->m_outputPane.closeTabs(AppOutputPane::CloseTabNoPrompt /* No prompt any more */);
    dd->m_shutdownWatchDogId = dd->startTimer(10 * 1000); // Make sure we shutdown *somehow*
    return AsynchronousShutdown;
}

void ProjectExplorerPlugin::openNewProjectDialog()
{
    if (!ICore::isNewItemDialogRunning()) {
        ICore::showNewItemDialog(Tr::tr("New Project", "Title of dialog"),
                                 Utils::filtered(IWizardFactory::allWizardFactories(),
                                 [](IWizardFactory *f) { return !f->supportedProjectTypes().isEmpty(); }));
    } else {
        ICore::raiseWindow(ICore::newItemDialog());
    }
}

void ProjectExplorerPluginPrivate::setStartupProject(Project *project)
{
    if (!project)
        return;
    ProjectManager::setStartupProject(project);
    updateActions();
}

bool ProjectExplorerPluginPrivate::closeAllFilesInProject(const Project *project)
{
    QTC_ASSERT(project, return false);
    QList<DocumentModel::Entry *> openFiles = DocumentModel::entries();
    Utils::erase(openFiles, [project](const DocumentModel::Entry *entry) {
        return entry->pinned || !project->isKnownFile(entry->filePath());
    });
    for (const Project * const otherProject : ProjectManager::projects()) {
        if (otherProject == project)
            continue;
        Utils::erase(openFiles, [otherProject](const DocumentModel::Entry *entry) {
            return otherProject->isKnownFile(entry->filePath());
        });
    }
    return EditorManager::closeDocuments(openFiles);
}

void ProjectExplorerPluginPrivate::savePersistentSettings()
{
    if (PluginManager::isShuttingDown())
        return;

    if (!SessionManager::isLoadingSession()) {
        for (Project *pro : ProjectManager::projects())
            pro->saveSettings();
    }

    QtcSettings *s = ICore::settings();
    s->remove("ProjectExplorer/RecentProjects/Files");

    QStringList fileNames;
    QStringList displayNames;
    RecentProjectsEntries::const_iterator it, end;
    end = dd->m_recentProjects.constEnd();
    for (it = dd->m_recentProjects.constBegin(); it != end; ++it) {
        fileNames << (*it).first.toUserOutput();
        displayNames << (*it).second;
    }

    s->setValueWithDefault(Constants::RECENTPROJECTS_FILE_NAMES_KEY, fileNames);
    s->setValueWithDefault(Constants::RECENTPROJECTS_DISPLAY_NAMES_KEY, displayNames);

    static const ProjectExplorerSettings defaultSettings;

    s->setValueWithDefault(Constants::BUILD_BEFORE_DEPLOY_SETTINGS_KEY,
                           int(dd->m_projectExplorerSettings.buildBeforeDeploy),
                           int(defaultSettings.buildBeforeDeploy));
    s->setValueWithDefault(Constants::DEPLOY_BEFORE_RUN_SETTINGS_KEY,
                           dd->m_projectExplorerSettings.deployBeforeRun,
                           defaultSettings.deployBeforeRun);
    s->setValueWithDefault(Constants::SAVE_BEFORE_BUILD_SETTINGS_KEY,
                           dd->m_projectExplorerSettings.saveBeforeBuild,
                           defaultSettings.saveBeforeBuild);
    s->setValueWithDefault(Constants::USE_JOM_SETTINGS_KEY,
                           dd->m_projectExplorerSettings.useJom,
                           defaultSettings.useJom);
    s->setValueWithDefault(Constants::ADD_LIBRARY_PATHS_TO_RUN_ENV_SETTINGS_KEY,
                           dd->m_projectExplorerSettings.addLibraryPathsToRunEnv,
                           defaultSettings.addLibraryPathsToRunEnv);
    s->setValueWithDefault(Constants::PROMPT_TO_STOP_RUN_CONTROL_SETTINGS_KEY,
                           dd->m_projectExplorerSettings.prompToStopRunControl,
                           defaultSettings.prompToStopRunControl);
    s->setValueWithDefault(Constants::TERMINAL_MODE_SETTINGS_KEY,
                           int(dd->m_projectExplorerSettings.terminalMode),
                           int(defaultSettings.terminalMode));
    s->setValueWithDefault(Constants::CLOSE_FILES_WITH_PROJECT_SETTINGS_KEY,
                           dd->m_projectExplorerSettings.closeSourceFilesWithProject,
                           defaultSettings.closeSourceFilesWithProject);
    s->setValueWithDefault(Constants::CLEAR_ISSUES_ON_REBUILD_SETTINGS_KEY,
                           dd->m_projectExplorerSettings.clearIssuesOnRebuild,
                           defaultSettings.clearIssuesOnRebuild);
    s->setValueWithDefault(Constants::ABORT_BUILD_ALL_ON_ERROR_SETTINGS_KEY,
                           dd->m_projectExplorerSettings.abortBuildAllOnError,
                           defaultSettings.abortBuildAllOnError);
    s->setValueWithDefault(Constants::LOW_BUILD_PRIORITY_SETTINGS_KEY,
                           dd->m_projectExplorerSettings.lowBuildPriority,
                           defaultSettings.lowBuildPriority);
    s->setValueWithDefault(Constants::AUTO_CREATE_RUN_CONFIGS_SETTINGS_KEY,
                           dd->m_projectExplorerSettings.automaticallyCreateRunConfigurations,
                           defaultSettings.automaticallyCreateRunConfigurations);
    s->setValueWithDefault(Constants::ENVIRONMENT_ID_SETTINGS_KEY,
                           dd->m_projectExplorerSettings.environmentId.toByteArray());
    s->setValueWithDefault(Constants::STOP_BEFORE_BUILD_SETTINGS_KEY,
                           int(dd->m_projectExplorerSettings.stopBeforeBuild),
                           int(defaultSettings.stopBeforeBuild));

    buildPropertiesSettings().writeSettings(); // FIXME: Should not be needed.

    s->setValueWithDefault(Constants::CUSTOM_PARSER_COUNT_KEY, int(dd->m_customParsers.count()), 0);
    for (int i = 0; i < dd->m_customParsers.count(); ++i) {
        s->setValue(numberedKey(Constants::CUSTOM_PARSER_PREFIX_KEY, i),
                    variantFromStore(dd->m_customParsers.at(i).toMap()));
    }
}

void ProjectExplorerPlugin::openProjectWelcomePage(const FilePath &filePath)
{
    OpenProjectResult result = openProject(filePath);
    if (!result)
        showOpenProjectError(result);
}

ProjectExplorerPlugin::OpenProjectResult ProjectExplorerPlugin::openProject(const FilePath &filePath)
{
    OpenProjectResult result = openProjects({filePath});
    Project *project = result.project();
    if (!project)
        return result;
    dd->addToRecentProjects(filePath, project->displayName());
    ProjectManager::setStartupProject(project);
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
        QMessageBox::critical(ICore::dialogParent(), Tr::tr("Failed to Open Project"), errorMessage);
    } else {
        // ignore multiple alreadyOpen
        Project *alreadyOpen = result.alreadyOpen().constFirst();
        ProjectTree::highlightProject(alreadyOpen,
                                      Tr::tr("<h3>Project already open</h3>"));
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

ProjectExplorerPlugin::OpenProjectResult ProjectExplorerPlugin::openProjects(const FilePaths &filePaths)
{
    QList<Project*> openedPro;
    QList<Project *> alreadyOpen;
    QString errorString;
    for (const FilePath &fileName : filePaths) {
        QTC_ASSERT(!fileName.isEmpty(), continue);
        const FilePath filePath = fileName.absoluteFilePath();

        Project *found = Utils::findOrDefault(ProjectManager::projects(),
                                              Utils::equal(&Project::projectFilePath, filePath));
        if (found) {
            alreadyOpen.append(found);
            SessionManager::sessionLoadingProgress();
            continue;
        }

        MimeType mt = Utils::mimeTypeForFile(filePath);
        if (ProjectManager::canOpenProjectForMimeType(mt)) {
            if (!filePath.isFile()) {
                appendError(errorString,
                            Tr::tr("Failed opening project \"%1\": Project is not a file.").arg(filePath.toUserOutput()));
            } else if (Project *pro = ProjectManager::openProject(mt, filePath)) {
                QString restoreError;
                Project::RestoreResult restoreResult = pro->restoreSettings(&restoreError);
                if (restoreResult == Project::RestoreResult::Ok) {
                    connect(pro, &Project::fileListChanged,
                            m_instance, &ProjectExplorerPlugin::fileListChanged);
                    ProjectManager::addProject(pro);
                    openedPro += pro;
                } else {
                    if (restoreResult == Project::RestoreResult::Error)
                        appendError(errorString, restoreError);
                    delete pro;
                }
            }
        } else {
            appendError(errorString, Tr::tr("Failed opening project \"%1\": No plugin can open project type \"%2\".")
                        .arg(filePath.toUserOutput())
                        .arg(mt.name()));
        }
        if (filePaths.size() > 1)
            SessionManager::sessionLoadingProgress();
    }
    dd->updateActions();

    const bool switchToProjectsMode = Utils::anyOf(openedPro, &Project::needsConfiguration);
    const bool switchToEditMode = Utils::allOf(openedPro, &Project::isEditModePreferred);
    if (!openedPro.isEmpty()) {
        if (switchToProjectsMode)
            ModeManager::activateMode(Constants::MODE_SESSION);
        else if (switchToEditMode)
            ModeManager::activateMode(Core::Constants::MODE_EDIT);
        ModeManager::setFocusToCurrentMode();
    }

    return OpenProjectResult(openedPro, alreadyOpen, errorString);
}

void ProjectExplorerPluginPrivate::updateWelcomePage()
{
    m_welcomePage.reloadWelcomeScreenData();
}

void ProjectExplorerPluginPrivate::loadSesssionTasks()
{
    const FilePath filePath = FilePath::fromSettings(
        SessionManager::value(Constants::SESSION_TASKFILE_KEY));
    if (!filePath.isEmpty())
        TaskFile::openTasks(filePath);
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

// Return a list of glob patterns for project files ("*.pro", etc), use first, main pattern only.
QStringList ProjectExplorerPlugin::projectFileGlobs()
{
    QStringList result;
    for (auto it = dd->m_projectCreators.cbegin(); it != dd->m_projectCreators.cend(); ++it) {
        MimeType mimeType = Utils::mimeTypeForName(it.key());
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

void ProjectExplorerPluginPrivate::executeRunConfiguration(RunConfiguration *runConfiguration, Id runMode)
{
    const Tasks runConfigIssues = runMode == Constants::DAP_CMAKE_DEBUG_RUN_MODE
                                      ? Tasks()
                                      : runConfiguration->checkForIssues();
    if (!runConfigIssues.isEmpty()) {
        for (const Task &t : runConfigIssues)
            TaskHub::addTask(t);
        // TODO: Insert an extra task with a "link" to the run settings page?
        TaskHub::requestPopup();
        return;
    }

    auto runControl = new RunControl(runMode);
    runControl->copyDataFromRunConfiguration(runConfiguration);

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
    Id runMode = runControl->runMode();
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
    if (PluginManager::isShuttingDown() && m_activeRunControlCount == 0)
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
                                             Tr::tr("Ignore All Errors?"),
                                             Tr::tr("Found some build errors in current task.\n"
                                                "Do you want to ignore them?"),
                                             QMessageBox::Yes | QMessageBox::No,
                                             QMessageBox::No) == QMessageBox::Yes;
    }
    if (m_delayedRunConfiguration.isNull() && m_shouldHaveRunConfiguration) {
        QMessageBox::warning(ICore::dialogParent(),
                             Tr::tr("Run Configuration Removed"),
                             Tr::tr("The configuration that was supposed to run is no longer "
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

RecentProjectsEntries ProjectExplorerPluginPrivate::recentProjects() const
{
    return Utils::filtered(dd->m_recentProjects, [](const RecentProjectsEntry &p) {
        // check if project is available, but avoid querying devices
        return p.first.needsDevice() || p.first.isFile();
    });
}

void ProjectExplorerPluginPrivate::updateActions()
{
    const Project *const project = ProjectManager::startupProject();
    const Project *const currentProject = ProjectTree::currentProject(); // for context menu actions

    const QPair<bool, QString> buildActionState = buildSettingsEnabled(project);
    const QPair<bool, QString> buildActionContextState = buildSettingsEnabled(currentProject);
    const QPair<bool, QString> buildSessionState = buildSettingsEnabledForSession();
    const bool isBuilding = BuildManager::isBuilding(project);

    const QString projectName = project ? project->displayName() : QString();
    const QString projectNameContextMenu = currentProject ? currentProject->displayName() : QString();

    m_unloadAction->setParameter(ProjectManager::projects().size() == 1 ? projectName : QString());
    m_unloadActionContextMenu->setParameter(projectNameContextMenu);
    m_unloadOthersActionContextMenu->setParameter(projectNameContextMenu);
    m_closeProjectFilesActionFileMenu->setParameter(ProjectManager::projects().size() == 1
                                                    ? projectName : QString());
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

    const bool hasDependencies = ProjectManager::projectOrder(currentProject).size() > 1;
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
    m_closeAllProjects->setEnabled(ProjectManager::hasProjects());
    m_unloadAction->setEnabled(ProjectManager::projects().size() <= 1);
    m_unloadAction->setEnabled(ProjectManager::projects().size() == 1);
    m_unloadActionContextMenu->setEnabled(ProjectManager::hasProjects());
    m_unloadOthersActionContextMenu->setEnabled(ProjectManager::projects().size() >= 2);
    m_closeProjectFilesActionFileMenu->setEnabled(ProjectManager::projects().size() == 1);
    m_closeProjectFilesActionContextMenu->setEnabled(ProjectManager::hasProjects());

    ActionContainer *aci =
        ActionManager::actionContainer(Constants::M_UNLOADPROJECTS);
    aci->menu()->menuAction()->setEnabled(ProjectManager::hasProjects());

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

    const bool hasProjects = ProjectManager::hasProjects();
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
                                                        Tr::tr("Always save files before build"), &alwaysSave)) {
                if (cancelled)
                    return false;
            }

            if (alwaysSave)
                dd->m_projectExplorerSettings.saveBeforeBuild = true;
        }
    }
    return true;
}

ProjectExplorerPluginPrivate::ProjectExplorerPluginPrivate() {}

void ProjectExplorerPluginPrivate::extendFolderNavigationWidgetFactory()
{
    auto folderNavigationWidgetFactory = FolderNavigationWidgetFactory::instance();
    connect(folderNavigationWidgetFactory,
            &FolderNavigationWidgetFactory::aboutToShowContextMenu,
            this,
            [this](QMenu *menu, const FilePath &filePath, bool isDir) {
                if (isDir) {
                    QAction *actionOpenProjects = menu->addAction(
                        Tr::tr("Open Project in \"%1\"")
                            .arg(filePath.toUserOutput()));
                    connect(actionOpenProjects, &QAction::triggered, this, [filePath] {
                        openProjectsInDirectory(filePath);
                    });
                    if (projectsInDirectory(filePath).isEmpty())
                        actionOpenProjects->setEnabled(false);
                } else if (ProjectExplorerPlugin::isProjectFile(filePath)) {
                    QAction *actionOpenAsProject = menu->addAction(
                        Tr::tr("Open Project \"%1\"").arg(filePath.toUserOutput()));
                    connect(actionOpenAsProject, &QAction::triggered, this, [filePath] {
                        ProjectExplorerPlugin::openProject(filePath);
                    });
                }
            });
    connect(folderNavigationWidgetFactory,
            &FolderNavigationWidgetFactory::fileRenamed,
            this,
            [](const FilePath &before, const FilePath &after) {
                const QVector<FolderNode *> folderNodes = renamableFolderNodes(before, after);
                QVector<FolderNode *> failedNodes;
                for (FolderNode *folder : folderNodes) {
                    if (!folder->renameFile(before, after))
                        failedNodes.append(folder);
                }
                if (!failedNodes.isEmpty()) {
                    const QString projects = projectNames(failedNodes).join(", ");
                    const QString errorMessage
                        = Tr::tr(
                              "The file \"%1\" was renamed to \"%2\", "
                              "but the following projects could not be automatically changed: %3")
                              .arg(before.toUserOutput(), after.toUserOutput(), projects);
                    QTimer::singleShot(0, ICore::instance(), [errorMessage] {
                        QMessageBox::warning(ICore::dialogParent(),
                                             Tr::tr("Project Editing Failed"), errorMessage);
                    });
                }
            });
    connect(folderNavigationWidgetFactory,
            &FolderNavigationWidgetFactory::aboutToRemoveFile,
            this,
            [](const FilePath &filePath) {
                const QVector<FolderNode *> folderNodes = removableFolderNodes(filePath);
                const QVector<FolderNode *> failedNodes
                    = Utils::filtered(folderNodes, [filePath](FolderNode *folder) {
                          return folder->removeFiles({filePath}) != RemovedFilesFromProject::Ok;
                      });
                if (!failedNodes.isEmpty()) {
                    const QString projects = projectNames(failedNodes).join(", ");
                    const QString errorMessage
                        = Tr::tr("The following projects failed to automatically remove the file: %1")
                              .arg(projects);
                    QTimer::singleShot(0, ICore::instance(), [errorMessage] {
                        QMessageBox::warning(ICore::dialogParent(),
                                             Tr::tr("Project Editing Failed"),
                                             errorMessage);
                    });
                }
            });
}

QString ProjectExplorerPluginPrivate::projectFilterString() const
{
    const QString filterSeparator = QLatin1String(";;");
    QStringList filterStrings;
    QStringList allGlobPatterns;
    for (auto it = m_projectCreators.cbegin(); it != m_projectCreators.cend(); ++it) {
        const QString &mimeType = it.key();
        MimeType mime = Utils::mimeTypeForName(mimeType);
        allGlobPatterns.append(mime.globPatterns());
        filterStrings.append(mime.filterString());
    }
    QString allProjectsFilter = Tr::tr("All Projects");
    allProjectsFilter += QLatin1String(" (") + allGlobPatterns.join(QLatin1Char(' '))
                         + QLatin1Char(')');
    filterStrings.prepend(allProjectsFilter);
    return filterStrings.join(filterSeparator);
}

void ProjectExplorerPluginPrivate::runProjectContextMenu(RunConfiguration *rc)
{
    const Node *node = ProjectTree::currentNode();
    const ProjectNode *projectNode = node ? node->asProjectNode() : nullptr;
    if (projectNode == ProjectTree::currentProject()->rootProjectNode() || !projectNode) {
        ProjectExplorerPlugin::runProject(ProjectTree::currentProject(),
                                          Constants::NORMAL_RUN_MODE);
    } else if (rc) {
        ProjectExplorerPlugin::runRunConfiguration(rc, Constants::NORMAL_RUN_MODE);
    }
}

static bool hasBuildSettings(const Project *pro)
{
    return Utils::anyOf(ProjectManager::projectOrder(pro), [](const Project *project) {
        return project
                && project->activeTarget()
                && project->activeTarget()->activeBuildConfiguration();
    });
}

static QPair<bool, QString> subprojectEnabledState(const Project *pro)
{
    QPair<bool, QString> result;
    result.first = true;

    const QList<Project *> &projects = ProjectManager::projectOrder(pro);
    for (const Project *project : projects) {
        if (project && project->activeTarget()
            && project->activeTarget()->activeBuildConfiguration()
            && !project->activeTarget()->activeBuildConfiguration()->isEnabled()) {
            result.first = false;
            result.second
                += Tr::tr("Building \"%1\" is disabled: %2<br>")
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
        result.second = Tr::tr("No project loaded.");
    } else if (BuildManager::isBuilding(pro)) {
        result.first = false;
        result.second = Tr::tr("Currently building the active project.");
    } else if (pro->needsConfiguration()) {
        result.first = false;
        result.second = Tr::tr("The project %1 is not configured.").arg(pro->displayName());
    } else if (!hasBuildSettings(pro)) {
        result.first = false;
        result.second = Tr::tr("Project has no build settings.");
    } else {
        result = subprojectEnabledState(pro);
    }
    return result;
}

QPair<bool, QString> ProjectExplorerPluginPrivate::buildSettingsEnabledForSession()
{
    QPair<bool, QString> result;
    result.first = true;
    if (!ProjectManager::hasProjects()) {
        result.first = false;
        result.second = Tr::tr("No project loaded.");
    } else if (BuildManager::isBuilding()) {
        result.first = false;
        result.second = Tr::tr("A build is in progress.");
    } else if (!hasBuildSettings(nullptr)) {
        result.first = false;
        result.second = Tr::tr("Project has no build settings.");
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
        QPushButton *closeAnyway = box.addButton(Tr::tr("Cancel Build && Close"), QMessageBox::AcceptRole);
        QPushButton *cancelClose = box.addButton(Tr::tr("Do Not Close"), QMessageBox::RejectRole);
        box.setDefaultButton(cancelClose);
        box.setWindowTitle(Tr::tr("Close %1?").arg(QGuiApplication::applicationDisplayName()));
        box.setText(Tr::tr("A project is currently being built."));
        box.setInformativeText(
            Tr::tr("Do you want to cancel the build process and close %1 anyway?")
                .arg(QGuiApplication::applicationDisplayName()));
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
            const FilePath binary =
                    FilePath::fromString(arguments.at(kitForBinaryOptionIndex + 1));
            if (binary.isEmpty() || !binary.exists())
                qWarning() << QString("No such file \"%1\".").arg(binary.toUserOutput());
            else
                KitManager::setBinaryForKit(binary);
        }
    }
}

static bool hasDeploySettings(Project *pro)
{
    return Utils::anyOf(ProjectManager::projectOrder(pro), [](Project *project) {
        return project->activeTarget()
                && project->activeTarget()->activeDeployConfiguration();
    });
}

void ProjectExplorerPlugin::runProject(Project *pro, Id mode, const bool forceSkipDeploy)
{
    if (!pro)
        return;

    if (Target *target = pro->activeTarget())
        if (RunConfiguration *rc = target->activeRunConfiguration())
            runRunConfiguration(rc, mode, forceSkipDeploy);
}

void ProjectExplorerPlugin::runStartupProject(Id runMode, bool forceSkipDeploy)
{
    runProject(ProjectManager::startupProject(), runMode, forceSkipDeploy);
}

void ProjectExplorerPlugin::runRunConfiguration(RunConfiguration *rc,
                                                Id runMode,
                                                const bool forceSkipDeploy)
{
    if (!rc->isEnabled())
        return;
    const auto delay = [rc, runMode] {
        dd->m_runMode = runMode;
        dd->m_delayedRunConfiguration = rc;
        dd->m_shouldHaveRunConfiguration = true;
    };

    BuildForRunConfigStatus buildStatus = forceSkipDeploy
            ? BuildManager::isBuilding(rc->project())
                ? BuildForRunConfigStatus::Building : BuildForRunConfigStatus::NotBuilding
            : BuildManager::potentiallyBuildForRunConfig(rc);

    if (dd->m_runMode == Constants::DAP_CMAKE_DEBUG_RUN_MODE)
        buildStatus = BuildForRunConfigStatus::NotBuilding;

    switch (buildStatus) {
    case BuildForRunConfigStatus::BuildFailed:
        return;
    case BuildForRunConfigStatus::Building:
        QTC_ASSERT(dd->m_runMode == Constants::NO_RUN_MODE, return);
        delay();
        break;
    case BuildForRunConfigStatus::NotBuilding:
        if (rc->isEnabled())
            dd->executeRunConfiguration(rc, runMode);
        else
            delay();
        break;
    }

    dd->doUpdateRunActions();
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
    m_projectsMode.setEnabled(ProjectManager::hasProjects());
}

void ProjectExplorerPluginPrivate::projectDisplayNameChanged(Project *pro)
{
    addToRecentProjects(pro->projectFilePath(), pro->displayName());
    updateActions();
}

void ProjectExplorerPluginPrivate::updateDeployActions()
{
    Project *project = ProjectManager::startupProject();

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

    bool hasProjects = ProjectManager::hasProjects();

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

        if (Utils::anyOf(ProjectManager::projectOrder(nullptr), hasDisabledBuildConfiguration))
            enableDeploySessionAction = false;
    }
    if (!hasProjects || !hasDeploySettings(nullptr) || BuildManager::isBuilding())
        enableDeploySessionAction = false;
    m_deploySessionAction->setEnabled(enableDeploySessionAction);

    doUpdateRunActions();
}

expected_str<void> ProjectExplorerPlugin::canRunStartupProject(Utils::Id runMode)
{
    Project *project = ProjectManager::startupProject();
    if (!project)
        return make_unexpected(Tr::tr("No active project."));

    if (project->needsConfiguration()) {
        return make_unexpected(Tr::tr("The project \"%1\" is not configured.")
                                   .arg(project->displayName()));
    }

    Target *target = project->activeTarget();
    if (!target) {
        return make_unexpected(Tr::tr("The project \"%1\" has no active kit.")
                                   .arg(project->displayName()));
    }

    RunConfiguration *activeRC = target->activeRunConfiguration();
    if (!activeRC) {
        return make_unexpected(
            Tr::tr("The kit \"%1\" for the project \"%2\" has no active run configuration.")
                .arg(target->displayName(), project->displayName()));
    }

    if (!activeRC->isEnabled())
        return make_unexpected(activeRC->disabledReason());

    if (dd->m_projectExplorerSettings.buildBeforeDeploy != BuildBeforeRunMode::Off
            && dd->m_projectExplorerSettings.deployBeforeRun
            && !BuildManager::isBuilding(project)
            && hasBuildSettings(project)) {
        QPair<bool, QString> buildState = dd->buildSettingsEnabled(project);
        if (!buildState.first)
            return make_unexpected(buildState.second);

        if (BuildManager::isBuilding())
            return make_unexpected(Tr::tr("A build is still in progress."));
    }

    // shouldn't actually be shown to the user...
    if (!RunControl::canRun(runMode, DeviceTypeKitAspect::deviceTypeId(target->kit()),
                            activeRC->id())) {
        return make_unexpected(Tr::tr("Cannot run \"%1\".").arg(activeRC->displayName()));
    }

    if (dd->m_delayedRunConfiguration && dd->m_delayedRunConfiguration->project() == project)
        return make_unexpected(Tr::tr("A run action is already scheduled for the active project."));

    return {};
}

void ProjectExplorerPluginPrivate::doUpdateRunActions()
{
    const auto canRun = ProjectExplorerPlugin::canRunStartupProject(Constants::NORMAL_RUN_MODE);
    m_runAction->setEnabled(bool(canRun));
    m_runAction->setToolTip(canRun ? QString() : canRun.error());
    m_runWithoutDeployAction->setEnabled(bool(canRun));

    emit m_instance->runActionsUpdated();
}

void ProjectExplorerPluginPrivate::addToRecentProjects(const FilePath &filePath, const QString &displayName)
{
    if (filePath.isEmpty())
        return;

    RecentProjectsEntries::iterator it;
    for (it = m_recentProjects.begin(); it != m_recentProjects.end();)
        if ((*it).first == filePath)
            it = m_recentProjects.erase(it);
        else
            ++it;

    if (m_recentProjects.count() > m_maxRecentProjects)
        m_recentProjects.removeLast();
    m_recentProjects.push_front({filePath, displayName});
    m_lastOpenDirectory = filePath.absolutePath();
    emit m_instance->recentProjectsChanged();
}

void ProjectExplorerPluginPrivate::updateUnloadProjectMenu()
{
    ActionContainer *aci = ActionManager::actionContainer(Constants::M_UNLOADPROJECTS);
    QMenu *menu = aci->menu();
    menu->clear();
    for (Project *project : ProjectManager::projects()) {
        QAction *action = menu->addAction(Tr::tr("Close Project \"%1\"").arg(project->displayName()));
        connect(action, &QAction::triggered, this,
                [project] { ProjectExplorerPlugin::unloadProject(project); });
    }
}

void ProjectExplorerPluginPrivate::updateRecentProjectMenu()
{
    ActionContainer *aci = ActionManager::actionContainer(Constants::M_RECENTPROJECTS);
    QMenu *menu = aci->menu();
    menu->clear();

    int acceleratorKey = 1;
    const RecentProjectsEntries projects = recentProjects();
    //projects (ignore sessions, they used to be in this list)
    for (const RecentProjectsEntry &item : projects) {
        const FilePath &filePath = item.first;
        if (filePath.endsWith(QLatin1String(".qws")))
            continue;

        const QString displayPath =
            filePath.osType() == OsTypeWindows ? filePath.displayName()
                                               : filePath.withTildeHomePath();
        const QString actionText =
            ActionManager::withNumberAccelerator(displayPath, acceleratorKey);
        QAction *action = menu->addAction(actionText);
        connect(action, &QAction::triggered, this, [this, filePath] {
            openRecentProject(filePath);
        });
        ++acceleratorKey;
    }
    const bool hasRecentProjects = !projects.empty();
    menu->setEnabled(hasRecentProjects);

    // add the Clear Menu item
    if (hasRecentProjects) {
        menu->addSeparator();
        QAction *action = menu->addAction(::Core::Tr::tr(Core::Constants::TR_CLEAR_MENU));
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

void ProjectExplorerPluginPrivate::openRecentProject(const FilePath &filePath)
{
    if (!filePath.isEmpty()) {
        ProjectExplorerPlugin::OpenProjectResult result
                = ProjectExplorerPlugin::openProject(filePath);
        if (!result)
            ProjectExplorerPlugin::showOpenProjectError(result);
    }
}

void ProjectExplorerPluginPrivate::removeFromRecentProjects(const FilePath &filePath)
{
    QTC_ASSERT(!filePath.isEmpty(), return);
    QTC_CHECK(Utils::eraseOne(m_recentProjects, [filePath](const RecentProjectsEntry &entry) {
        return entry.first == filePath;
    }));
}

void ProjectExplorerPluginPrivate::invalidateProject(Project *project)
{
    disconnect(project, &Project::fileListChanged,
               m_instance, &ProjectExplorerPlugin::fileListChanged);
    updateActions();
}

void ProjectExplorerPluginPrivate::updateContextMenuActions(Node *currentNode)
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
    m_runActionContextMenu->setEnabled(false);
    m_defaultRunConfiguration.clear();
    m_diffFileAction->setVisible(DiffService::instance());

    m_openTerminalHere->setVisible(true);
    m_openTerminalHereBuildEnv->setVisible(false);
    m_openTerminalHereRunEnv->setVisible(false);

    m_showInGraphicalShell->setVisible(true);
    m_showFileSystemPane->setVisible(true);
    m_searchOnFileSystem->setVisible(true);

    ActionContainer *runMenu = ActionManager::actionContainer(Constants::RUNMENUCONTEXTMENU);
    runMenu->menu()->clear();
    runMenu->menu()->menuAction()->setVisible(false);

    if (currentNode && currentNode->managingProject()) {
        ProjectNode *pn;
        if (const ContainerNode *cn = currentNode->asContainerNode())
            pn = cn->rootProjectNode();
        else
            pn = const_cast<ProjectNode*>(currentNode->asProjectNode());

        Project *project = ProjectTree::currentProject();
        m_openTerminalHereBuildEnv->setVisible(bool(buildEnv(project)));
        m_openTerminalHereRunEnv->setVisible(canOpenTerminalWithRunEnv(project, pn));

        if (pn && project) {
            if (pn == project->rootProjectNode()) {
                m_runActionContextMenu->setEnabled(true);
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
                    m_runActionContextMenu->setEnabled(true);
                    m_defaultRunConfiguration = runConfigs.first();
                } else if (runConfigs.count() > 1) {
                    runMenu->menu()->menuAction()->setVisible(true);
                    for (RunConfiguration *rc : std::as_const(runConfigs)) {
                        auto *act = new QAction(runMenu->menu());
                        act->setText(Tr::tr("Run %1").arg(rc->displayName()));
                        runMenu->menu()->addAction(act);
                        connect(act, &QAction::triggered,
                                this, [this, rc] { runProjectContextMenu(rc); });
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
            const bool currentNodeIsTextFile = isTextFile(currentNode->filePath());
            m_diffFileAction->setEnabled(DiffService::instance()
                        && currentNodeIsTextFile && TextEditor::TextDocument::currentTextDocument());

            const bool canDuplicate = canEditProject && supports(AddNewFile)
                    && currentNode->asFileNode()->fileType() != FileType::Project;
            m_duplicateFileAction->setVisible(canDuplicate);
            m_duplicateFileAction->setEnabled(canDuplicate);

            EditorManager::populateOpenWithMenu(m_openWithMenu, currentNode->filePath());
        }

        if (supports(HidePathActions)) {
            m_openTerminalHere->setVisible(false);
            m_showInGraphicalShell->setVisible(false);
            m_showFileSystemPane->setVisible(false);
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
        const FilePath path = li.path;
        QString displayName = fn->filePath() == li.path
                                  ? li.displayName
                                  : Tr::tr("%1 in %2").arg(li.displayName).arg(li.path.toUserOutput());
        auto *action = new QAction(displayName, nullptr);
        connect(action, &QAction::triggered, this, [line, path] {
            EditorManager::openEditorAt(Link(path, line), {}, EditorManager::AllowExternalEditor);
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
    FilePath location = currentNode->directory();

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
    ICore::showNewItemDialog(Tr::tr("New File", "Title of dialog"),
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
    FilePath location = currentNode->directory();

    if (currentNode->isProjectNodeType()
            && currentNode->supportsAction(AddSubProject, currentNode)) {
        QVariantMap map;
        map.insert(QLatin1String(Constants::PREFERRED_PROJECT_NODE), QVariant::fromValue(currentNode));
        Project *project = ProjectTree::currentProject();
        Id projectType;
        if (project) {
            const QStringList profileIds = Utils::transform(ProjectTree::currentProject()->targets(),
                                                            [](const Target *t) {
                                                                return t->id().toString();
                                                            });
            map.insert(QLatin1String(Constants::PROJECT_KIT_IDS), profileIds);
            projectType = project->id();
        }

        ICore::showNewItemDialog(Tr::tr("New Subproject", "Title of dialog"),
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
    const FilePath dir = currentNode->directory();
    FilePaths subProjectFilePaths = Utils::FileUtils::getOpenFilePaths(
                nullptr, Tr::tr("Choose Project File"), dir,
                projectNode->subProjectFileNamePatterns().join(";;"));
    if (!ProjectTree::hasNode(projectNode))
        return;
    const QList<Node *> childNodes = projectNode->nodes();
    Utils::erase(subProjectFilePaths, [childNodes](const FilePath &filePath) {
        return Utils::anyOf(childNodes, [filePath](const Node *n) {
            return n->filePath() == filePath;
        });
    });
    if (subProjectFilePaths.empty())
        return;
    FilePaths failedProjects;
    FilePaths addedProjects;
    for (const FilePath &filePath : std::as_const(subProjectFilePaths)) {
        if (projectNode->addSubProject(filePath))
            addedProjects << filePath;
        else
            failedProjects << filePath;
    }
    if (!failedProjects.empty()) {
        const QString message = Tr::tr("The following subprojects could not be added to project "
                                   "\"%1\":").arg(projectNode->managingProject()->displayName());
        QMessageBox::warning(ICore::dialogParent(), Tr::tr("Adding Subproject Failed"),
                             message + "\n  " + FilePath::formatFilePaths(failedProjects, "\n  "));
        return;
    }
    VcsManager::promptToAdd(dir, addedProjects);
}

void ProjectExplorerPluginPrivate::handleAddExistingFiles()
{
    Node *node = ProjectTree::currentNode();
    FolderNode *folderNode = node ? node->asFolderNode() : nullptr;

    QTC_ASSERT(folderNode, return);

    const FilePaths filePaths =
            Utils::FileUtils::getOpenFilePaths(nullptr, Tr::tr("Add Existing Files"), node->directory());
    if (filePaths.isEmpty())
        return;

    ProjectExplorerPlugin::addExistingFiles(folderNode, filePaths);
}

void ProjectExplorerPluginPrivate::addExistingDirectory()
{
    Node *node = ProjectTree::currentNode();
    FolderNode *folderNode = node ? node->asFolderNode() : nullptr;

    QTC_ASSERT(folderNode, return);

    SelectableFilesDialogAddDirectory dialog(node->directory(), FilePaths(), ICore::dialogParent());
    dialog.setAddFileFilter({});

    if (dialog.exec() == QDialog::Accepted)
        ProjectExplorerPlugin::addExistingFiles(folderNode, dialog.selectedFiles());
}

void ProjectExplorerPlugin::addExistingFiles(FolderNode *folderNode, const FilePaths &filePaths)
{
    // can happen when project is not yet parsed or finished parsing while the dialog was open:
    if (!folderNode || !ProjectTree::hasNode(folderNode))
        return;

    const FilePath dir = folderNode->directory();
    FilePaths fileNames = filePaths;
    FilePaths notAdded;
    folderNode->addFiles(fileNames, &notAdded);

    if (!notAdded.isEmpty()) {
        const QString message = Tr::tr("Could not add following files to project %1:")
                .arg(folderNode->managingProject()->displayName()) + QLatin1Char('\n');
        QMessageBox::warning(ICore::dialogParent(), Tr::tr("Adding Files to Project Failed"),
                             message + FilePath::formatFilePaths(notAdded, "\n"));
        fileNames = Utils::filtered(fileNames,
                                    [&notAdded](const FilePath &f) { return !notAdded.contains(f); });
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
        RemoveFileDialog removeFileDialog(node->filePath(), ICore::dialogParent());
        removeFileDialog.setDeleteFileVisible(false);
        if (removeFileDialog.exec() == QDialog::Accepted)
            projectNode->removeSubProject(node->filePath());
    }
}

void ProjectExplorerPluginPrivate::openFile()
{
    const Node *currentNode = ProjectTree::currentNode();
    QTC_ASSERT(currentNode, return);
    EditorManager::openEditor(currentNode->filePath());
}

void ProjectExplorerPluginPrivate::searchOnFileSystem()
{
    const Node *currentNode = ProjectTree::currentNode();
    QTC_ASSERT(currentNode, return);
    TextEditor::FindInFiles::findOnFileSystem(currentNode->path().toString());
}

void ProjectExplorerPluginPrivate::showInGraphicalShell()
{
    Node *currentNode = ProjectTree::currentNode();
    QTC_ASSERT(currentNode, return);
    Core::FileUtils::showInGraphicalShell(ICore::dialogParent(), currentNode->path());
}

void ProjectExplorerPluginPrivate::showInFileSystemPane()
{
    Node *currentNode = ProjectTree::currentNode();
    QTC_ASSERT(currentNode, return );
    Core::FileUtils::showInFileSystemView(currentNode->filePath());
}

static BuildConfiguration *activeBuildConfiguration(Project *project)
{
    if (!project || !project->activeTarget() || !project->activeTarget()->activeBuildConfiguration())
        return {};
    return project->activeTarget()->activeBuildConfiguration();
}

void ProjectExplorerPluginPrivate::openTerminalHere(const EnvironmentGetter &env)
{
    const Node *currentNode = ProjectTree::currentNode();
    QTC_ASSERT(currentNode, return);

    const auto environment = env(ProjectTree::projectForNode(currentNode));
    if (!environment)
        return;

    BuildConfiguration *bc = activeBuildConfiguration(ProjectTree::projectForNode(currentNode));
    if (!bc) {
        Terminal::Hooks::instance().openTerminal({currentNode->directory(), environment});
        return;
    }

    IDeviceConstPtr buildDevice = BuildDeviceKitAspect::device(bc->target()->kit());

    if (!buildDevice)
        return;

    FilePath workingDir = currentNode->directory();
    if (!buildDevice->filePath(workingDir.path()).exists()
        && !buildDevice->ensureReachable(workingDir))
        workingDir.clear();

    const expected_str<FilePath> shell = Terminal::defaultShellForDevice(buildDevice->rootPath());

    if (!shell) {
        Core::MessageManager::writeDisrupting(
            Tr::tr("Failed opening terminal.\n%1").arg(shell.error()));
        return;
    }

    if (buildDevice->rootPath().needsDevice())
        Terminal::Hooks::instance().openTerminal({CommandLine{*shell, {}}, workingDir, environment});
    else
        Terminal::Hooks::instance().openTerminal({workingDir, environment});
}

void ProjectExplorerPluginPrivate::openTerminalHereWithRunEnv()
{
    const Node *currentNode = ProjectTree::currentNode();
    QTC_ASSERT(currentNode, return);

    const Project * const project = ProjectTree::projectForNode(currentNode);
    QTC_ASSERT(project, return);
    const Target * const target = project->activeTarget();
    QTC_ASSERT(target, return);
    const RunConfiguration * const runConfig = runConfigForNode(target,
                                                                currentNode->asProjectNode());
    QTC_ASSERT(runConfig, return);

    const ProcessRunData runnable = runConfig->runnable();
    IDevice::ConstPtr device = DeviceManager::deviceForPath(runnable.command.executable());
    if (!device)
        device = DeviceKitAspect::device(target->kit());
    QTC_ASSERT(device && device->canOpenTerminal(), return);

    FilePath workingDir = device->type() == Constants::DESKTOP_DEVICE_TYPE
                              ? currentNode->directory()
                              : runnable.workingDirectory;

    if (!device->filePath(workingDir.path()).exists() && !device->ensureReachable(workingDir))
        workingDir.clear();

    const expected_str<FilePath> shell = Terminal::defaultShellForDevice(device->rootPath());

    if (!shell) {
        Core::MessageManager::writeDisrupting(
            Tr::tr("Failed opening terminal.\n%1").arg(shell.error()));
        return;
    }

    if (device->rootPath().needsDevice()) {
        Terminal::Hooks::instance().openTerminal(
            {CommandLine{*shell, {}}, workingDir, runnable.environment});
    } else {
        Terminal::Hooks::instance().openTerminal({workingDir, runnable.environment});
    }
}

void ProjectExplorerPluginPrivate::removeFile()
{
    const Node *currentNode = ProjectTree::currentNode();
    QTC_ASSERT(currentNode && currentNode->asFileNode(), return);

    ProjectTree::CurrentNodeKeeper nodeKeeper;

    const FilePath filePath = currentNode->filePath();
    using NodeAndPath = QPair<const Node *, FilePath>;
    QList<NodeAndPath> filesToRemove{{currentNode, currentNode->filePath()}};
    QList<NodeAndPath> siblings;
    for (const Node * const n : ProjectTree::siblingsWithSameBaseName(currentNode))
        siblings.push_back({n, n->filePath()});

    RemoveFileDialog removeFileDialog(filePath, ICore::dialogParent());
    if (removeFileDialog.exec() != QDialog::Accepted)
        return;

    const bool deleteFile = removeFileDialog.isDeleteFileChecked();

    if (!siblings.isEmpty()) {
        const QMessageBox::StandardButton reply = QMessageBox::question(
                    ICore::dialogParent(), Tr::tr("Remove More Files?"),
                    Tr::tr("Remove these files as well?\n    %1")
                    .arg(Utils::transform<QStringList>(siblings, [](const NodeAndPath &np) {
            return np.second.fileName();
        }).join("\n    ")));
        if (reply == QMessageBox::Yes)
            filesToRemove << siblings;
    }

    for (const NodeAndPath &file : std::as_const(filesToRemove)) {
        // Nodes can become invalid if the project was re-parsed while the dialog was open
        if (!ProjectTree::hasNode(file.first)) {
            QMessageBox::warning(ICore::dialogParent(), Tr::tr("Removing File Failed"),
                                 Tr::tr("File \"%1\" was not removed, because the project has changed "
                                    "in the meantime.\nPlease try again.")
                                 .arg(file.second.toUserOutput()));
            return;
        }

        // remove from project
        FolderNode *folderNode = file.first->asFileNode()->parentFolderNode();
        QTC_ASSERT(folderNode, return);

        const FilePath &currentFilePath = file.second;
        const RemovedFilesFromProject status = folderNode->removeFiles({currentFilePath});
        const bool success = status == RemovedFilesFromProject::Ok
                || (status == RemovedFilesFromProject::Wildcard
                    && removeFileDialog.isDeleteFileChecked());
        if (!success) {
            TaskHub::addTask(BuildSystemTask(Task::Error,
                    Tr::tr("Could not remove file \"%1\" from project \"%2\".")
                        .arg(currentFilePath.toUserOutput(), folderNode->managingProject()->displayName()),
                    folderNode->managingProject()->filePath()));
        }
    }

    std::vector<std::unique_ptr<FileChangeBlocker>> changeGuards;
    FilePaths pathList;
    for (const NodeAndPath &file : std::as_const(filesToRemove)) {
        pathList << file.second;
        changeGuards.emplace_back(std::make_unique<FileChangeBlocker>(file.second));
    }

    Core::FileUtils::removeFiles(pathList, deleteFile);
}

static HandleIncludeGuards canTryToRenameIncludeGuards(const Node *node)
{
    return node->asFileNode() && node->asFileNode()->fileType() == FileType::Header
            ? HandleIncludeGuards::Yes : HandleIncludeGuards::No;
}

void ProjectExplorerPluginPrivate::duplicateFile()
{
    Node *currentNode = ProjectTree::currentNode();
    QTC_ASSERT(currentNode && currentNode->asFileNode(), return);

    ProjectTree::CurrentNodeKeeper nodeKeeper;

    FileNode *fileNode = currentNode->asFileNode();
    QString filePath = currentNode->filePath().toString();
    QFileInfo sourceFileInfo(filePath);
    QString baseName = sourceFileInfo.baseName();

    QString newFileName = sourceFileInfo.fileName();
    int copyTokenIndex = newFileName.lastIndexOf(baseName)+baseName.length();
    newFileName.insert(copyTokenIndex, Tr::tr("_copy"));

    bool okPressed;
    newFileName = QInputDialog::getText(ICore::dialogParent(), Tr::tr("Choose File Name"),
            Tr::tr("New file name:"), QLineEdit::Normal, newFileName, &okPressed);
    if (!okPressed)
        return;
    if (!ProjectTree::hasNode(currentNode))
        return;

    const QString newFilePath = sourceFileInfo.path() + '/' + newFileName;
    FolderNode *folderNode = fileNode->parentFolderNode();
    QTC_ASSERT(folderNode, return);
    QFile sourceFile(filePath);
    if (!sourceFile.copy(newFilePath)) {
        QMessageBox::critical(ICore::dialogParent(), Tr::tr("Duplicating File Failed"),
                             Tr::tr("Failed to copy file \"%1\" to \"%2\": %3.")
                             .arg(QDir::toNativeSeparators(filePath),
                                  QDir::toNativeSeparators(newFilePath), sourceFile.errorString()));
        return;
    }
    Core::FileUtils::updateHeaderFileGuardIfApplicable(currentNode->filePath(),
                                                       FilePath::fromString(newFilePath),
                                                       canTryToRenameIncludeGuards(currentNode));
    if (!folderNode->addFiles({FilePath::fromString(newFilePath)})) {
        QMessageBox::critical(ICore::dialogParent(), Tr::tr("Duplicating File Failed"),
                              Tr::tr("Failed to add new file \"%1\" to the project.")
                              .arg(QDir::toNativeSeparators(newFilePath)));
    }
}

void ProjectExplorerPluginPrivate::deleteFile()
{
    Node *currentNode = ProjectTree::currentNode();
    QTC_ASSERT(currentNode && currentNode->asFileNode(), return);

    ProjectTree::CurrentNodeKeeper nodeKeeper;

    FileNode *fileNode = currentNode->asFileNode();

    FilePath filePath = currentNode->filePath();
    QMessageBox::StandardButton button =
            QMessageBox::question(ICore::dialogParent(),
                                  Tr::tr("Delete File"),
                                  Tr::tr("Delete %1 from file system?")
                                  .arg(filePath.toUserOutput()),
                                  QMessageBox::Yes | QMessageBox::No);
    if (button != QMessageBox::Yes)
        return;

    FolderNode *folderNode = fileNode->parentFolderNode();
    QTC_ASSERT(folderNode, return);

    folderNode->deleteFiles({filePath});

    FileChangeBlocker changeGuard(currentNode->filePath());
    if (IVersionControl *vc = VcsManager::findVersionControlForDirectory(filePath.absolutePath()))
        vc->vcsDelete(filePath);

    if (filePath.exists()) {
        if (!filePath.removeFile())
            QMessageBox::warning(ICore::dialogParent(), Tr::tr("Deleting File Failed"),
                                 Tr::tr("Could not delete file %1.")
                                 .arg(filePath.toUserOutput()));
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

void ProjectExplorerPlugin::renameFile(Node *node, const QString &newFileName)
{
    const FilePath oldFilePath = node->filePath().absoluteFilePath();
    FolderNode *folderNode = node->parentFolderNode();
    QTC_ASSERT(folderNode, return);
    const QString projectFileName = folderNode->managingProject()->filePath().fileName();

    const FilePath newFilePath = FilePath::fromString(newFileName);

    if (oldFilePath == newFilePath)
        return;

    const HandleIncludeGuards handleGuards = canTryToRenameIncludeGuards(node);
    if (!folderNode->canRenameFile(oldFilePath, newFilePath)) {
        QTimer::singleShot(0, m_instance,
                           [oldFilePath, newFilePath, projectFileName, handleGuards] {
            int res = QMessageBox::question(ICore::dialogParent(),
                                            Tr::tr("Project Editing Failed"),
                                            Tr::tr("The project file %1 cannot be automatically changed.\n\n"
                                               "Rename %2 to %3 anyway?")
                                            .arg(projectFileName)
                                            .arg(oldFilePath.toUserOutput())
                                            .arg(newFilePath.toUserOutput()));
            if (res == QMessageBox::Yes) {
                QTC_CHECK(Core::FileUtils::renameFile(oldFilePath, newFilePath, handleGuards));
            }
        });
        return;
    }

    if (Core::FileUtils::renameFile(oldFilePath, newFilePath, handleGuards)) {
        // Tell the project plugin about rename
        if (!folderNode->renameFile(oldFilePath, newFilePath)) {
            const QString renameFileError = Tr::tr("The file %1 was renamed to %2, but the project "
                                               "file %3 could not be automatically changed.")
                                                .arg(oldFilePath.toUserOutput())
                                                .arg(newFilePath.toUserOutput())
                                                .arg(projectFileName);

            QTimer::singleShot(0, m_instance, [renameFileError] {
                QMessageBox::warning(ICore::dialogParent(),
                                     Tr::tr("Project Editing Failed"),
                                     renameFileError);
            });
        }
    } else {
        const QString renameFileError = Tr::tr("The file %1 could not be renamed %2.")
                                            .arg(oldFilePath.toUserOutput())
                                            .arg(newFilePath.toUserOutput());

        QTimer::singleShot(0, m_instance, [renameFileError] {
            QMessageBox::warning(ICore::dialogParent(), Tr::tr("Cannot Rename File"), renameFileError);
        });
    }
}

void ProjectExplorerPluginPrivate::handleSetStartupProject()
{
    setStartupProject(ProjectTree::currentProject());
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

void ProjectExplorerPlugin::setCustomParsers(const QList<CustomParserSettings> &settings)
{
    if (dd->m_customParsers != settings) {
        dd->m_customParsers = settings;
        emit m_instance->customParsersChanged();
    }
}

void ProjectExplorerPlugin::addCustomParser(const CustomParserSettings &settings)
{
    QTC_ASSERT(settings.id.isValid(), return);
    QTC_ASSERT(!contains(dd->m_customParsers, [&settings](const CustomParserSettings &s) {
        return s.id == settings.id;
    }), return);

    dd->m_customParsers << settings;
    emit m_instance->customParsersChanged();
}

void ProjectExplorerPlugin::removeCustomParser(Id id)
{
    Utils::erase(dd->m_customParsers, [id](const CustomParserSettings &s) {
        return s.id == id;
    });
    emit m_instance->customParsersChanged();
}

const QList<CustomParserSettings> ProjectExplorerPlugin::customParsers()
{
    return dd->m_customParsers;
}

QStringList ProjectExplorerPlugin::projectFilePatterns()
{
    QStringList patterns;
    for (auto it = dd->m_projectCreators.cbegin(); it != dd->m_projectCreators.cend(); ++it) {
        MimeType mt = Utils::mimeTypeForName(it.key());
        if (mt.isValid())
            patterns.append(mt.globPatterns());
    }
    return patterns;
}

bool ProjectExplorerPlugin::isProjectFile(const FilePath &filePath)
{
    MimeType mt = Utils::mimeTypeForFile(filePath);
    for (auto it = dd->m_projectCreators.cbegin(); it != dd->m_projectCreators.cend(); ++it) {
        if (mt.inherits(it.key()))
            return true;
    }
    return false;
}

void ProjectExplorerPlugin::openOpenProjectDialog()
{
    const FilePath path = DocumentManager::useProjectsDirectory()
                             ? DocumentManager::projectsDirectory()
                             : FilePath();
    const FilePaths files = DocumentManager::getOpenFileNames(dd->projectFilterString(), path);
    if (!files.isEmpty())
        ICore::openFiles(files, ICore::SwitchMode);
}

void ProjectExplorerPlugin::updateActions()
{
    dd->updateActions();
}

void ProjectExplorerPlugin::activateProjectPanel(Id panelId)
{
    ModeManager::activateMode(Constants::MODE_SESSION);
    dd->m_proWindow->activateProjectPanel(panelId);
}

void ProjectExplorerPlugin::clearRecentProjects()
{
    dd->clearRecentProjects();
}

void ProjectExplorerPlugin::removeFromRecentProjects(const FilePath &filePath)
{
    dd->removeFromRecentProjects(filePath);
}

void ProjectExplorerPlugin::updateRunActions()
{
    dd->doUpdateRunActions();
}

OutputWindow *ProjectExplorerPlugin::buildSystemOutput()
{
    return dd->m_proWindow->buildSystemOutput();
}

RecentProjectsEntries ProjectExplorerPlugin::recentProjects()
{
    return dd->recentProjects();
}

void ProjectExplorerPlugin::renameFilesForSymbol(const QString &oldSymbolName,
        const QString &newSymbolName, const FilePaths &files, bool preferLowerCaseFileNames)
{
    static const auto isAllLowerCase = [](const QString &text) { return text.toLower() == text; };

    for (const FilePath &file : files) {
        Node * const node = ProjectTree::nodeForFile(file);
        if (!node)
            continue;
        const QString oldBaseName = file.baseName();
        QString newBaseName = newSymbolName;

        // 1) new symbol lowercase: new base name lowercase
        if (isAllLowerCase(newSymbolName)) {
            newBaseName = newSymbolName;

        // 2) old base name mixed case: new base name is verbatim symbol name
        } else if (!isAllLowerCase(oldBaseName)) {
            newBaseName = newSymbolName;

        // 3) old base name lowercase, old symbol mixed case: new base name lowercase
        } else if (!isAllLowerCase(oldSymbolName)) {
            newBaseName = newSymbolName.toLower();

        // 4) old base name lowercase, old symbol lowercase, new symbol mixed case:
        //    use the preferences setting for new base name case
        } else if (preferLowerCaseFileNames) {
            newBaseName = newSymbolName.toLower();
        }

        if (newBaseName == oldBaseName)
            continue;

        const QString newFilePath = file.absolutePath().toString() + '/' + newBaseName + '.'
                + file.completeSuffix();
        renameFile(node, newFilePath);
    }
}

void ProjectManager::registerProjectCreator(const QString &mimeType,
    const std::function<Project *(const FilePath &)> &creator)
{
    dd->m_projectCreators[mimeType] = creator;
}

Project *ProjectManager::openProject(const MimeType &mt, const FilePath &fileName)
{
    if (mt.isValid()) {
        for (auto it = dd->m_projectCreators.cbegin(); it != dd->m_projectCreators.cend(); ++it) {
            if (mt.matchesName(it.key()))
                return it.value()(fileName);
        }
    }
    return nullptr;
}

bool ProjectManager::canOpenProjectForMimeType(const MimeType &mt)
{
    if (mt.isValid()) {
        for (auto it = dd->m_projectCreators.cbegin(); it != dd->m_projectCreators.cend(); ++it) {
            if (mt.matchesName(it.key()))
                return true;
        }
    }
    return false;
}

AllProjectFilesFilter::AllProjectFilesFilter()
    : DirectoryFilter("Files in All Project Directories")
{
    setDisplayName(id().toString());
    // shared with "Files in Any Project":
    setDefaultShortcutString("a");
    setDefaultIncludedByDefault(false); // but not included in default
    setFilters({});
    setIsCustomFilter(false);
    setDescription(Tr::tr("Locates files from all project directories. Append \"+<number>\" or "
                          "\":<number>\" to jump to the given line number. Append another "
                          "\"+<number>\" or \":<number>\" to jump to the column number as well."));

    ProjectManager *projectManager = ProjectManager::instance();
    QTC_ASSERT(projectManager, return);
    connect(projectManager, &ProjectManager::projectAdded, this, [this](Project *project) {
        addDirectory(project->projectDirectory());
    });
    connect(projectManager, &ProjectManager::projectRemoved, this, [this](Project *project) {
        removeDirectory(project->projectDirectory());
    });
}

const char kDirectoriesKey[] = "directories";
const char kFilesKey[] = "files";

void AllProjectFilesFilter::saveState(QJsonObject &object) const
{
    DirectoryFilter::saveState(object);
    // do not save the directories, they are automatically managed
    object.remove(kDirectoriesKey);
    object.remove(kFilesKey);
}

void AllProjectFilesFilter::restoreState(const QJsonObject &object)
{
    // do not restore the directories (from saved settings from Qt Creator <= 5,
    // they are automatically managed
    QJsonObject withoutDirectories = object;
    withoutDirectories.remove(kDirectoriesKey);
    withoutDirectories.remove(kFilesKey);
    DirectoryFilter::restoreState(withoutDirectories);
}

static void setupFilter(ILocatorFilter *filter)
{
    QObject::connect(ProjectManager::instance(), &ProjectManager::startupProjectChanged,
                     filter, [filter] { filter->setEnabled(ProjectManager::startupProject()); });
    filter->setEnabled(ProjectManager::startupProject());
}

using RunAcceptor = std::function<void(RunConfiguration *)>;

static RunConfiguration *runConfigurationForDisplayName(const QString &displayName)
{
    const Target *target = ProjectManager::startupTarget();
    if (!target)
        return nullptr;
    const QList<RunConfiguration *> runconfigs = target->runConfigurations();
    return Utils::findOrDefault(runconfigs, [displayName](RunConfiguration *rc) {
        return rc->displayName() == displayName;
    });
}

static LocatorMatcherTasks runConfigurationMatchers(const RunAcceptor &acceptor)
{
    using namespace Tasking;

    TreeStorage<LocatorStorage> storage;

    const auto onSetup = [storage, acceptor] {
        const QString input = storage->input();
        const Target *target = ProjectManager::startupTarget();
        if (!target)
            return;

        LocatorFilterEntries entries;
        for (auto rc : target->runConfigurations()) {
            if (rc->displayName().contains(input, Qt::CaseInsensitive)) {
                LocatorFilterEntry entry;
                entry.displayName = rc->displayName();
                entry.acceptor = [name = entry.displayName, acceptor = acceptor] {
                    RunConfiguration *config = runConfigurationForDisplayName(name);
                    if (!config)
                        return AcceptResult();
                    acceptor(config);
                    return AcceptResult();
                };
                entries.append(entry);
            }
        }
        storage->reportOutput(entries);
    };
    return {{Sync(onSetup), storage}};
}

static void runAcceptor(RunConfiguration *config)
{
    if (!BuildManager::isBuilding(config->project()))
        ProjectExplorerPlugin::runRunConfiguration(config, Constants::NORMAL_RUN_MODE, true);
}

RunConfigurationStartFilter::RunConfigurationStartFilter()
{
    setId("Run run configuration");
    setDisplayName(Tr::tr("Run Run Configuration"));
    setDescription(Tr::tr("Runs a run configuration of the active project."));
    setDefaultShortcutString("rr");
    setPriority(Medium);
    setupFilter(this);
}

LocatorMatcherTasks RunConfigurationStartFilter::matchers()
{
    return runConfigurationMatchers(&runAcceptor);
}

static void switchAcceptor(RunConfiguration *config)
{
    ProjectManager::startupTarget()->setActiveRunConfiguration(config);
    QTimer::singleShot(200, ICore::mainWindow(), [name = config->displayName()] {
        if (auto ks = ICore::mainWindow()->findChild<QWidget *>("KitSelector.Button")) {
            ToolTip::show(ks->mapToGlobal(QPoint{25, 25}),
                          Tr::tr("Switched run configuration to\n%1").arg(name),
                          ICore::dialogParent());
        }
    });
}

RunConfigurationSwitchFilter::RunConfigurationSwitchFilter()
{
    setId("Switch run configuration");
    setDisplayName(Tr::tr("Switch Run Configuration"));
    setDescription(Tr::tr("Switches the active run configuration of the active project."));
    setDefaultShortcutString("sr");
    setPriority(Medium);
    setupFilter(this);
}

LocatorMatcherTasks RunConfigurationSwitchFilter::matchers()
{
    return runConfigurationMatchers(&switchAcceptor);
}

} // namespace ProjectExplorer
