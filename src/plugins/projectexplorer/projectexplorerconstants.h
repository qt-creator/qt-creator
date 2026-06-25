// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "projectexplorer_export.h"

namespace ProjectExplorer::Constants {

// Modes and their priorities
inline constexpr char MODE_SESSION[]         = "Project";

// Actions
inline constexpr char BUILD[]                = "ProjectExplorer.Build";
inline constexpr char BUILD_SUBPROJECT[]     = "ProjectExplorer.BuildSubProject";
inline constexpr char CLEAN_SUBPROJECT[]     = "ProjectExplorer.CleanSubProject";
inline constexpr char REBUILD_SUBPROJECT[]   = "ProjectExplorer.RebuildSubProject";
inline constexpr char BUILD_FILE[]           = "ProjectExplorer.BuildFile";
inline constexpr char BUILD_FILE_CTX[]       = "ProjectExplorer.BuildFileCtx";
inline constexpr char BUILD_SUBPROJECT_CTX[] = "ProjectExplorer.BuildSubProjectCtx";
inline constexpr char CLEAN_SUBPROJECT_CTX[] = "ProjectExplorer.CleanSubProjectCtx";
inline constexpr char REBUILD_SUBPROJECT_CTX[] = "ProjectExplorer.RebuildSubProjectCtx";
inline constexpr char CLEAN[]                = "ProjectExplorer.Clean";
inline constexpr char STOP[]                 = "ProjectExplorer.Stop";
inline constexpr char ADDNEWFILE[]           = "ProjectExplorer.AddNewFile";
inline constexpr char FILEPROPERTIES[]       = "ProjectExplorer.FileProperties";
inline constexpr char RENAMEFILE[]           = "ProjectExplorer.RenameFile";
inline constexpr char REMOVEFILE[]           = "ProjectExplorer.RemoveFile";
inline constexpr char RUN[]                  = "ProjectExplorer.Run";

// Context
inline constexpr char C_PROJECTEXPLORER[]    = "Project Explorer";
inline constexpr char C_PROJECT_TREE[]       = "ProjectExplorer.ProjectTreeContext";

// Menus
inline constexpr char M_BUILDPROJECT[]       = "ProjectExplorer.Menu.Build";
inline constexpr char M_DEBUG[]              = "ProjectExplorer.Menu.Debug";
inline constexpr char M_DEBUG_STARTDEBUGGING[] = "ProjectExplorer.Menu.Debug.StartDebugging";

// Menu groups
inline constexpr char G_BUILD_BUILD[]        = "ProjectExplorer.Group.Build";
inline constexpr char G_BUILD_ALLPROJECTS[]  = "ProjectExplorer.Group.AllProjects";
inline constexpr char G_BUILD_PROJECT[]      = "ProjectExplorer.Group.Project";
inline constexpr char G_BUILD_SUBPROJECT[]   = "ProjectExplorer.Group.SubProject";
inline constexpr char G_BUILD_FILE[]         = "ProjectExplorer.Group.File";
inline constexpr char G_BUILD_ALLPROJECTS_ALLCONFIGURATIONS[] = "ProjectExplorer.Group.AllProjects.AllConfigurations";
inline constexpr char G_BUILD_PROJECT_ALLCONFIGURATIONS[] = "ProjectExplorer.Group.Project.AllConfigurations";
inline constexpr char G_BUILD_RUN[]          = "ProjectExplorer.Group.Run";
inline constexpr char G_BUILD_CANCEL[]       = "ProjectExplorer.Group.BuildCancel";

// Context menus
inline constexpr char M_SESSIONCONTEXT[]     = "Project.Menu.Session";
inline constexpr char M_PROJECTCONTEXT[]     = "Project.Menu.Project";
inline constexpr char M_SUBPROJECTCONTEXT[]  = "Project.Menu.SubProject";
inline constexpr char M_FOLDERCONTEXT[]      = "Project.Menu.Folder";
inline constexpr char M_FILECONTEXT[]        = "Project.Menu.File";
inline constexpr char M_OPENFILEWITHCONTEXT[] = "Project.Menu.File.OpenWith";
inline constexpr char M_OPENTERMINALCONTEXT[] = "Project.Menu.File.OpenTerminal";
inline constexpr char M_VCSFILECONTEXT[]      = "Project.Menu.File.Vcs";

// Context menu groups
inline constexpr char G_SESSION_BUILD[]      = "Session.Group.Build";
inline constexpr char G_SESSION_REBUILD[]     = "Session.Group.Rebuild";
inline constexpr char G_SESSION_FILES[]      = "Session.Group.Files";
inline constexpr char G_SESSION_OTHER[]      = "Session.Group.Other";

inline constexpr char G_PROJECT_FIRST[]      = "Project.Group.Open";
inline constexpr char G_PROJECT_BUILD[]      = "Project.Group.Build";
inline constexpr char G_PROJECT_REBUILD[]    = "Project.Group.Rebuild";
inline constexpr char G_PROJECT_RUN[]        = "Project.Group.Run";
inline constexpr char G_PROJECT_FILES[]      = "Project.Group.Files";
inline constexpr char G_PROJECT_CLOSE[]      = "Project.Group.Close";
inline constexpr char G_PROJECT_TREE[]       = "Project.Group.Tree";
inline constexpr char G_PROJECT_LAST[]       = "Project.Group.Last";

inline constexpr char G_FOLDER_LOCATIONS[]   = "ProjectFolder.Group.Locations";
inline constexpr char G_FOLDER_FILES[]       = "ProjectFolder.Group.Files";
inline constexpr char G_FOLDER_OTHER[]       = "ProjectFolder.Group.Other";
inline constexpr char G_FOLDER_CONFIG[]      = "ProjectFolder.Group.Config";

inline constexpr char G_FILE_OPEN[]          = "ProjectFile.Group.Open";
inline constexpr char G_FILE_OTHER[]         = "ProjectFile.Group.Other";
inline constexpr char G_FILE_CONFIG[]        = "ProjectFile.Group.Config";

// Kits settings category
inline constexpr char KITS_SETTINGS_CATEGORY[]  = "A.Kits";

// Kits pages
inline constexpr char KITS_SETTINGS_PAGE_ID[] = "D.ProjectExplorer.KitsOptions";
inline constexpr char SSH_SETTINGS_PAGE_ID[] = "F.ProjectExplorer.SshOptions";
inline constexpr char TOOLCHAIN_SETTINGS_PAGE_ID[] = "M.ProjectExplorer.ToolChainOptions";
inline constexpr char DEBUGGER_SETTINGS_PAGE_ID[] = "N.ProjectExplorer.DebuggerOptions";
inline constexpr char CUSTOM_PARSERS_SETTINGS_PAGE_ID[] = "X.ProjectExplorer.CustomParsersSettingsPage";

// Build and Run settings category
inline constexpr char BUILD_AND_RUN_SETTINGS_CATEGORY[]  = "K.BuildAndRun";

// Build and Run page
inline constexpr char BUILD_AND_RUN_SETTINGS_PAGE_ID[] = "A.ProjectExplorer.BuildAndRunOptions";

// Device settings page
inline constexpr char DEVICE_SETTINGS_CATEGORY[] = "AM.Devices";
inline constexpr char DEVICE_SETTINGS_PAGE_ID[] = "AA.Device Settings";

// Task categories
inline constexpr char TASK_CATEGORY_COMPILE[] = "Task.Category.Compile";
inline constexpr char TASK_CATEGORY_BUILDSYSTEM[] = "Task.Category.Buildsystem";
inline constexpr char TASK_CATEGORY_DEPLOYMENT[] = "Task.Category.Deploy";
inline constexpr char TASK_CATEGORY_AUTOTEST[] = "Task.Category.Autotest";
inline constexpr char TASK_CATEGORY_OTHER[] = "Task.Category.Other";
inline constexpr char TASK_CATEGORY_TASKLIST_ID[] = "Task.Category.TaskListId";

// Wizard categories
inline constexpr char QT_PROJECT_WIZARD_CATEGORY[] = "H.Project";

inline constexpr char IMPORT_WIZARD_CATEGORY[] = "T.Import";
inline constexpr char IMPORT_WIZARD_CATEGORY_DISPLAY[] = QT_TRANSLATE_NOOP("QtC::ProjectExplorer", "Import Project");

// Wizard extra values
inline constexpr char PREFERRED_PROJECT_NODE[] = "ProjectExplorer.PreferredProjectNode";
inline constexpr char PREFERRED_PROJECT_NODE_PATH[] = "ProjectExplorer.PreferredProjectPath";
inline constexpr char PROJECT_POINTER[] = "ProjectExplorer.Project";
inline constexpr char PROJECT_KIT_IDS[] = "ProjectExplorer.Profile.Ids";
inline constexpr char QT_KEYWORDS_ENABLED[] = "ProjectExplorer.QtKeywordsEnabled";
inline constexpr char PROJECT_ISSUBPROJECT[] = "IsSubproject"; // Used inside wizard, no prefix!
inline constexpr char PROJECT_ENABLESUBPROJECT[] = "ProjectExplorer.EnableSubproject";

// Build step lists ids:
inline constexpr char BUILDSTEPS_CLEAN[] = "ProjectExplorer.BuildSteps.Clean";
inline constexpr char BUILDSTEPS_BUILD[] = "ProjectExplorer.BuildSteps.Build";
inline constexpr char BUILDSTEPS_DEPLOY[] = "ProjectExplorer.BuildSteps.Deploy";

inline constexpr char COPY_FILE_STEP[] = "ProjectExplorer.CopyFileStep";
inline constexpr char COPY_DIRECTORY_STEP[] = "ProjectExplorer.CopyDirectoryStep";
inline constexpr char DEVICE_CHECK_STEP[] =  "ProjectExplorer.DeviceCheckBuildStep";
inline constexpr char CUSTOM_PROCESS_STEP[] =  "ProjectExplorer.ProcessStep";

// Project Configuration
inline constexpr char CONFIGURATION_ID_KEY[] = "ProjectExplorer.ProjectConfiguration.Id";
inline constexpr char DISPLAY_NAME_KEY[] = "ProjectExplorer.ProjectConfiguration.DisplayName";

// Language

// Keep these short: These constants are exposed to the MacroExplorer!
inline constexpr char C_LANGUAGE_ID[] = "C";
inline constexpr char CXX_LANGUAGE_ID[] = "Cxx";
inline constexpr char QMLJS_LANGUAGE_ID[] = "QMLJS";
inline constexpr char PYTHON_LANGUAGE_ID[] = "Python";

// ToolChain TypeIds
inline constexpr char CLANG_TOOLCHAIN_TYPEID[] = "ProjectExplorer.ToolChain.Clang";
inline constexpr char GCC_TOOLCHAIN_TYPEID[] = "ProjectExplorer.ToolChain.Gcc";
inline constexpr char LINUXICC_TOOLCHAIN_TYPEID[] = "ProjectExplorer.ToolChain.LinuxIcc";
inline constexpr char MINGW_TOOLCHAIN_TYPEID[] = "ProjectExplorer.ToolChain.Mingw";
inline constexpr char MSVC_TOOLCHAIN_TYPEID[] = "ProjectExplorer.ToolChain.Msvc";
inline constexpr char CLANG_CL_TOOLCHAIN_TYPEID[] = "ProjectExplorer.ToolChain.ClangCl";
inline constexpr char CUSTOM_TOOLCHAIN_TYPEID[] = "ProjectExplorer.ToolChain.Custom";

// Default directory to run custom (build) commands in.
inline constexpr char DEFAULT_WORKING_DIR[] = "%{buildDir}";
inline constexpr char DEFAULT_WORKING_DIR_ALTERNATE[] = "%{sourceDir}";

inline constexpr char QTC_DEFAULT_BUILD_DIRECTORY_TEMPLATE[] = "QTC_DEFAULT_BUILD_DIRECTORY_TEMPLATE";
inline constexpr char QTC_DEFAULT_WORKING_DIRECTORY_TEMPLATE[] = "QTC_DEFAULT_WORKING_DIRECTORY_TEMPLATE";

// Desktop Device related ids:
inline constexpr char DESKTOP_DEVICE_ID[] = "Desktop Device";
inline constexpr int DESKTOP_PORT_START = 30000;
inline constexpr int DESKTOP_PORT_END = 31000;

// Android ABIs
inline constexpr char ANDROID_ABI_ARMEABI[] = "armeabi";
inline constexpr char ANDROID_ABI_ARMEABI_V7A[] = "armeabi-v7a";
inline constexpr char ANDROID_ABI_ARM64_V8A[] = "arm64-v8a";
inline constexpr char ANDROID_ABI_X86[] = "x86";
inline constexpr char ANDROID_ABI_X86_64[] = "x86_64";

// JsonWizard:
inline constexpr char PAGE_ID_PREFIX[] = "PE.Wizard.Page.";
inline constexpr char GENERATOR_ID_PREFIX[] = "PE.Wizard.Generator.";

// Device types
inline constexpr char DESKTOP_DEVICE_TYPE[] = "Desktop";
inline constexpr char BOOT2QT_DEVICE_TYPE[] = "QdbLinuxOsType";

// RunMode
inline constexpr char NO_RUN_MODE[]="RunConfiguration.NoRunMode";
inline constexpr char NORMAL_RUN_MODE[]="RunConfiguration.NormalRunMode";
inline constexpr char DEBUG_RUN_MODE[]="RunConfiguration.DebugRunMode";
inline constexpr char DAP_CMAKE_DEBUG_RUN_MODE[]="RunConfiguration.CmakeDebugRunMode";
inline constexpr char DAP_GDB_DEBUG_RUN_MODE[]="RunConfiguration.DapGdbDebugRunMode";
inline constexpr char DAP_LLDB_DEBUG_RUN_MODE[]="RunConfiguration.DapLldbDebugRunMode";
inline constexpr char DAP_PY_DEBUG_RUN_MODE[]="RunConfiguration.DapPyDebugRunMode";
inline constexpr char QML_PROFILER_RUN_MODE[]="RunConfiguration.QmlProfilerRunMode";
inline constexpr char QML_PROFILER_RUNNER[]="RunConfiguration.QmlProfilerRunner";
inline constexpr char QML_PREVIEW_RUN_MODE[]="RunConfiguration.QmlPreviewRunMode";
inline constexpr char QML_PREVIEW_RUNNER[]="RunConfiguration.QmlPreviewRunner";
inline constexpr char PERFPROFILER_RUN_MODE[]="PerfProfiler.RunMode";
inline constexpr char PERFPROFILER_RUNNER[]="PerfProfiler.Runner";

// RunWorkerFactory
inline constexpr char QML_PROFILER_RUN_FACTORY[] = "LocalQmlProfilerRunWorkerFactory";
inline constexpr char QML_PREVIEW_RUN_FACTORY[] = "LocalQmlPreviewSupportFactory";

// RunConfig
inline constexpr char QMAKE_RUNCONFIG_ID[] = "Qt4ProjectManager.Qt4RunConfiguration:";
inline constexpr char QBS_RUNCONFIG_ID[]   = "Qbs.RunConfiguration:";
inline constexpr char CMAKE_RUNCONFIG_ID[] = "CMakeProjectManager.CMakeRunConfiguration.";
inline constexpr char GN_RUNCONFIG_ID[] = "GNProjectManager.GNRunConfiguration.";
inline constexpr char CUSTOM_EXECUTABLE_RUNCONFIG_ID[] = "ProjectExplorer.CustomExecutableRunConfiguration";
inline constexpr char WORKSPACE_RUNCONFIG_ID[] = "WorkspaceProject.RunConfiguration:";

inline constexpr char STDPROCESS_EXECUTION_TYPE_ID[] = "StdProcessExecutionType";

// Navigation Widget
inline constexpr char PROJECTTREE_ID[] = "Projects";

// File icon overlays
inline constexpr char FILEOVERLAY_QT[]=":/projectexplorer/images/fileoverlay_qt.png";
inline constexpr char FILEOVERLAY_GROUP[] = ":/projectexplorer/images/fileoverlay_group.png";
inline constexpr char FILEOVERLAY_PRODUCT[] = ":/projectexplorer/images/fileoverlay_product.png";
inline constexpr char FILEOVERLAY_MODULES[] = ":/projectexplorer/images/fileoverlay_modules.png";
inline constexpr char FILEOVERLAY_QML[]=":/projectexplorer/images/fileoverlay_qml.png";
inline constexpr char FILEOVERLAY_UI[]=":/projectexplorer/images/fileoverlay_ui.png";
inline constexpr char FILEOVERLAY_QRC[]=":/projectexplorer/images/fileoverlay_qrc.png";
inline constexpr char FILEOVERLAY_C[]=":/projectexplorer/images/fileoverlay_c.png";
inline constexpr char FILEOVERLAY_CPP[]=":/projectexplorer/images/fileoverlay_cpp.png";
inline constexpr char FILEOVERLAY_H[]=":/projectexplorer/images/fileoverlay_h.png";
inline constexpr char FILEOVERLAY_SCXML[]=":/projectexplorer/images/fileoverlay_scxml.png";
inline constexpr char FILEOVERLAY_PY[]=":/projectexplorer/images/fileoverlay_py.png";
inline constexpr char FILEOVERLAY_UNKNOWN[]=":/projectexplorer/images/fileoverlay_unknown.png";

// Settings
inline constexpr char ADD_FILES_DIALOG_FILTER_HISTORY_KEY[] = "ProjectExplorer.AddFilesFilterKey";
inline constexpr char PROJECT_ROOT_PATH_KEY[] = "ProjectExplorer.Project.RootPath";
inline constexpr char SETTINGS_MENU_HIDE_BUILD[] = "Menu/HideBuild";
inline constexpr char SETTINGS_MENU_HIDE_DEBUG[] = "Menu/HideDebug";
inline constexpr char SETTINGS_MENU_HIDE_ANALYZE[] = "Menu/HideAnalyze";
inline constexpr char SESSION_TASKFILE_KEY[] = "TaskList.File";
inline constexpr char CLEAR_SYSTEM_ENVIRONMENT_KEY[] = "ProjectExplorer.BuildConfiguration.ClearSystemEnvironment";
inline constexpr char USER_ENVIRONMENT_CHANGES_KEY[] = "ProjectExplorer.BuildConfiguration.UserEnvironmentChanges";

// File Transfer
// Called "RemoteLinux." for backwards compatibility
inline constexpr char SUPPORTS_RSYNC[] = "RemoteLinux.SupportsRSync";
inline constexpr char SUPPORTS_SFTP[] = "RemoteLinux.SupportsSftp";
// Tool implementation is actually in RemoteLinux
inline constexpr char RSYNC_TOOL_ID[] = "RsyncExecutable";
inline constexpr char SSH_TOOL_ID[] = "SshExecutable";

// Ninja tool
inline constexpr char TOOL_TYPE_NINJA[] = "ninja";

// SDKs related ids:
inline constexpr char SDK_SETTINGS_CATEGORY[] = "AN.SDKs";
inline constexpr char WINDOWS_SETTINGS_ID[] = "Windows Configurations";
inline constexpr char WINDOWS_WINAPPSDK_ROOT_ENV_KEY[] = "WIN_APP_SDK_ROOT";

// Welcome page
inline constexpr char PROJECT_WELCOMEPAGE_ID[] = "Develop";

// URL handler
inline constexpr char URL_HANDLER_SCHEME[] = "QCProjectExplorer";
inline constexpr char ACTIVE_RUN_CONFIG_PATH[] = "activeRunConfiguration";

// Project-local settings directory
inline constexpr char PROJECT_QTC_DIR[] = ".qtcreator";

// UI texts
PROJECTEXPLORER_EXPORT QString msgAutoDetected();
PROJECTEXPLORER_EXPORT QString msgAutoDetectedToolTip();
PROJECTEXPLORER_EXPORT QString msgManual();

} // namespace ProjectExplorer::Constants
