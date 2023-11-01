// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "projectexplorer_export.h"

namespace ProjectExplorer {
namespace Constants {

// Modes and their priorities
const char MODE_SESSION[]         = "Project";

// Actions
const char BUILD[]                = "ProjectExplorer.Build";
const char STOP[]                 = "ProjectExplorer.Stop";
const char ADDNEWFILE[]           = "ProjectExplorer.AddNewFile";
const char FILEPROPERTIES[]       = "ProjectExplorer.FileProperties";
const char RENAMEFILE[]           = "ProjectExplorer.RenameFile";
const char REMOVEFILE[]           = "ProjectExplorer.RemoveFile";

// Context
const char C_PROJECTEXPLORER[]    = "Project Explorer";
const char C_PROJECT_TREE[]       = "ProjectExplorer.ProjectTreeContext";

// Menus
const char M_BUILDPROJECT[]       = "ProjectExplorer.Menu.Build";
const char M_DEBUG[]              = "ProjectExplorer.Menu.Debug";
const char M_DEBUG_STARTDEBUGGING[] = "ProjectExplorer.Menu.Debug.StartDebugging";

// Menu groups
const char G_BUILD_BUILD[]        = "ProjectExplorer.Group.Build";
const char G_BUILD_ALLPROJECTS[]  = "ProjectExplorer.Group.AllProjects";
const char G_BUILD_PROJECT[]      = "ProjectExplorer.Group.Project";
const char G_BUILD_PRODUCT[]      = "ProjectExplorer.Group.Product";
const char G_BUILD_SUBPROJECT[]   = "ProjectExplorer.Group.SubProject";
const char G_BUILD_FILE[]         = "ProjectExplorer.Group.File";
const char G_BUILD_ALLPROJECTS_ALLCONFIGURATIONS[] = "ProjectExplorer.Group.AllProjects.AllConfigurations";
const char G_BUILD_PROJECT_ALLCONFIGURATIONS[] = "ProjectExplorer.Group.Project.AllConfigurations";
const char G_BUILD_RUN[]          = "ProjectExplorer.Group.Run";
const char G_BUILD_CANCEL[]       = "ProjectExplorer.Group.BuildCancel";

// Context menus
const char M_SESSIONCONTEXT[]     = "Project.Menu.Session";
const char M_PROJECTCONTEXT[]     = "Project.Menu.Project";
const char M_SUBPROJECTCONTEXT[]  = "Project.Menu.SubProject";
const char M_FOLDERCONTEXT[]      = "Project.Menu.Folder";
const char M_FILECONTEXT[]        = "Project.Menu.File";
const char M_OPENFILEWITHCONTEXT[] = "Project.Menu.File.OpenWith";
const char M_OPENTERMINALCONTEXT[] = "Project.Menu.File.OpenTerminal";

// Context menu groups
const char G_SESSION_BUILD[]      = "Session.Group.Build";
const char G_SESSION_REBUILD[]     = "Session.Group.Rebuild";
const char G_SESSION_FILES[]      = "Session.Group.Files";
const char G_SESSION_OTHER[]      = "Session.Group.Other";

const char G_PROJECT_FIRST[]      = "Project.Group.Open";
const char G_PROJECT_BUILD[]      = "Project.Group.Build";
const char G_PROJECT_REBUILD[]    = "Project.Group.Rebuild";
const char G_PROJECT_RUN[]        = "Project.Group.Run";
const char G_PROJECT_FILES[]      = "Project.Group.Files";
const char G_PROJECT_CLOSE[]      = "Project.Group.Close";
const char G_PROJECT_TREE[]       = "Project.Group.Tree";
const char G_PROJECT_LAST[]       = "Project.Group.Last";

const char G_FOLDER_LOCATIONS[]   = "ProjectFolder.Group.Locations";
const char G_FOLDER_FILES[]       = "ProjectFolder.Group.Files";
const char G_FOLDER_OTHER[]       = "ProjectFolder.Group.Other";
const char G_FOLDER_CONFIG[]      = "ProjectFolder.Group.Config";

const char G_FILE_OPEN[]          = "ProjectFile.Group.Open";
const char G_FILE_OTHER[]         = "ProjectFile.Group.Other";
const char G_FILE_CONFIG[]        = "ProjectFile.Group.Config";

// Mime types
const char C_SOURCE_MIMETYPE[]    = "text/x-csrc";
const char C_HEADER_MIMETYPE[]    = "text/x-chdr";
const char CPP_SOURCE_MIMETYPE[]  = "text/x-c++src";
const char CPP_HEADER_MIMETYPE[]  = "text/x-c++hdr";
const char LINGUIST_MIMETYPE[]    = "text/vnd.trolltech.linguist";
const char FORM_MIMETYPE[]        = "application/x-designer";
const char QML_MIMETYPE[]         = "text/x-qml"; // separate def also in qmljstoolsconstants.h
const char QMLUI_MIMETYPE[]       = "application/x-qt.ui+qml";
const char RESOURCE_MIMETYPE[]    = "application/vnd.qt.xml.resource";
const char SCXML_MIMETYPE[]       = "application/scxml+xml";

// Kits settings category
const char KITS_SETTINGS_CATEGORY[]  = "A.Kits";

// Kits pages
const char KITS_SETTINGS_PAGE_ID[] = "D.ProjectExplorer.KitsOptions";
const char SSH_SETTINGS_PAGE_ID[] = "F.ProjectExplorer.SshOptions";
const char TOOLCHAIN_SETTINGS_PAGE_ID[] = "M.ProjectExplorer.ToolChainOptions";
const char DEBUGGER_SETTINGS_PAGE_ID[] = "N.ProjectExplorer.DebuggerOptions";
const char CUSTOM_PARSERS_SETTINGS_PAGE_ID[] = "X.ProjectExplorer.CustomParsersSettingsPage";

// Build and Run settings category
const char BUILD_AND_RUN_SETTINGS_CATEGORY[]  = "K.BuildAndRun";

// Build and Run page
const char BUILD_AND_RUN_SETTINGS_PAGE_ID[] = "A.ProjectExplorer.BuildAndRunOptions";

// Device settings page
const char DEVICE_SETTINGS_CATEGORY[] = "AM.Devices";
const char DEVICE_SETTINGS_PAGE_ID[] = "AA.Device Settings";

// Task categories
const char TASK_CATEGORY_COMPILE[] = "Task.Category.Compile";
const char TASK_CATEGORY_BUILDSYSTEM[] = "Task.Category.Buildsystem";
const char TASK_CATEGORY_DEPLOYMENT[] = "Task.Category.Deploy";
const char TASK_CATEGORY_AUTOTEST[] = "Task.Category.Autotest";
const char TASK_CATEGORY_SANITIZER[] = "Task.Category.Analyzer";
const char TASK_CATEGORY_TASKLIST_ID[] = "Task.Category.TaskListId";

// Wizard categories
const char QT_PROJECT_WIZARD_CATEGORY[] = "H.Project";
const char QT_PROJECT_WIZARD_CATEGORY_DISPLAY[] = QT_TRANSLATE_NOOP("QtC::ProjectExplorer", "Other Project");

const char IMPORT_WIZARD_CATEGORY[] = "T.Import";
const char IMPORT_WIZARD_CATEGORY_DISPLAY[] = QT_TRANSLATE_NOOP("QtC::ProjectExplorer", "Import Project");

// Wizard extra values
const char PREFERRED_PROJECT_NODE[] = "ProjectExplorer.PreferredProjectNode";
const char PREFERRED_PROJECT_NODE_PATH[] = "ProjectExplorer.PreferredProjectPath";
const char PROJECT_POINTER[] = "ProjectExplorer.Project";
const char PROJECT_KIT_IDS[] = "ProjectExplorer.Profile.Ids";
const char QT_KEYWORDS_ENABLED[] = "ProjectExplorer.QtKeywordsEnabled";

// Build step lists ids:
const char BUILDSTEPS_CLEAN[] = "ProjectExplorer.BuildSteps.Clean";
const char BUILDSTEPS_BUILD[] = "ProjectExplorer.BuildSteps.Build";
const char BUILDSTEPS_DEPLOY[] = "ProjectExplorer.BuildSteps.Deploy";

const char COPY_FILE_STEP[] = "ProjectExplorer.CopyFileStep";
const char COPY_DIRECTORY_STEP[] = "ProjectExplorer.CopyDirectoryStep";
const char DEVICE_CHECK_STEP[] =  "ProjectExplorer.DeviceCheckBuildStep";

// Language

// Keep these short: These constants are exposed to the MacroExplorer!
const char C_LANGUAGE_ID[] = "C";
const char CXX_LANGUAGE_ID[] = "Cxx";
const char QMLJS_LANGUAGE_ID[] = "QMLJS";
const char PYTHON_LANGUAGE_ID[] = "Python";

// ToolChain TypeIds
const char CLANG_TOOLCHAIN_TYPEID[] = "ProjectExplorer.ToolChain.Clang";
const char GCC_TOOLCHAIN_TYPEID[] = "ProjectExplorer.ToolChain.Gcc";
const char LINUXICC_TOOLCHAIN_TYPEID[] = "ProjectExplorer.ToolChain.LinuxIcc";
const char MINGW_TOOLCHAIN_TYPEID[] = "ProjectExplorer.ToolChain.Mingw";
const char MSVC_TOOLCHAIN_TYPEID[] = "ProjectExplorer.ToolChain.Msvc";
const char CLANG_CL_TOOLCHAIN_TYPEID[] = "ProjectExplorer.ToolChain.ClangCl";
const char CUSTOM_TOOLCHAIN_TYPEID[] = "ProjectExplorer.ToolChain.Custom";

// Default directory to run custom (build) commands in.
const char DEFAULT_WORKING_DIR[] = "%{buildDir}";
const char DEFAULT_WORKING_DIR_ALTERNATE[] = "%{sourceDir}";

// Desktop Device related ids:
const char DESKTOP_DEVICE_ID[] = "Desktop Device";
const char DESKTOP_DEVICE_TYPE[] = "Desktop";
const int DESKTOP_PORT_START = 30000;
const int DESKTOP_PORT_END = 31000;

// Android ABIs
const char ANDROID_ABI_ARMEABI[] = "armeabi";
const char ANDROID_ABI_ARMEABI_V7A[] = "armeabi-v7a";
const char ANDROID_ABI_ARM64_V8A[] = "arm64-v8a";
const char ANDROID_ABI_X86[] = "x86";
const char ANDROID_ABI_X86_64[] = "x86_64";

// JsonWizard:
const char PAGE_ID_PREFIX[] = "PE.Wizard.Page.";
const char GENERATOR_ID_PREFIX[] = "PE.Wizard.Generator.";

// RunMode
const char NO_RUN_MODE[]="RunConfiguration.NoRunMode";
const char NORMAL_RUN_MODE[]="RunConfiguration.NormalRunMode";
const char DEBUG_RUN_MODE[]="RunConfiguration.DebugRunMode";
const char DAP_CMAKE_DEBUG_RUN_MODE[]="RunConfiguration.CmakeDebugRunMode";
const char DAP_GDB_DEBUG_RUN_MODE[]="RunConfiguration.DapGdbDebugRunMode";
const char DAP_PY_DEBUG_RUN_MODE[]="RunConfiguration.DapPyDebugRunMode";
const char QML_PROFILER_RUN_MODE[]="RunConfiguration.QmlProfilerRunMode";
const char QML_PROFILER_RUNNER[]="RunConfiguration.QmlProfilerRunner";
const char QML_PREVIEW_RUN_MODE[]="RunConfiguration.QmlPreviewRunMode";
const char QML_PREVIEW_RUNNER[]="RunConfiguration.QmlPreviewRunner";
const char PERFPROFILER_RUN_MODE[]="PerfProfiler.RunMode";
const char PERFPROFILER_RUNNER[]="PerfProfiler.Runner";

// Navigation Widget
const char PROJECTTREE_ID[] = "Projects";

// File icon overlays
const char FILEOVERLAY_QT[]=":/projectexplorer/images/fileoverlay_qt.png";
const char FILEOVERLAY_GROUP[] = ":/projectexplorer/images/fileoverlay_group.png";
const char FILEOVERLAY_PRODUCT[] = ":/projectexplorer/images/fileoverlay_product.png";
const char FILEOVERLAY_MODULES[] = ":/projectexplorer/images/fileoverlay_modules.png";
const char FILEOVERLAY_QML[]=":/projectexplorer/images/fileoverlay_qml.png";
const char FILEOVERLAY_UI[]=":/projectexplorer/images/fileoverlay_ui.png";
const char FILEOVERLAY_QRC[]=":/projectexplorer/images/fileoverlay_qrc.png";
const char FILEOVERLAY_C[]=":/projectexplorer/images/fileoverlay_c.png";
const char FILEOVERLAY_CPP[]=":/projectexplorer/images/fileoverlay_cpp.png";
const char FILEOVERLAY_H[]=":/projectexplorer/images/fileoverlay_h.png";
const char FILEOVERLAY_SCXML[]=":/projectexplorer/images/fileoverlay_scxml.png";
const char FILEOVERLAY_PY[]=":/projectexplorer/images/fileoverlay_py.png";
const char FILEOVERLAY_UNKNOWN[]=":/projectexplorer/images/fileoverlay_unknown.png";

// Settings
const char ADD_FILES_DIALOG_FILTER_HISTORY_KEY[] = "ProjectExplorer.AddFilesFilterKey";
const char PROJECT_ROOT_PATH_KEY[] = "ProjectExplorer.Project.RootPath";
const char SETTINGS_MENU_HIDE_BUILD[] = "Menu/HideBuild";
const char SETTINGS_MENU_HIDE_DEBUG[] = "Menu/HideDebug";
const char SETTINGS_MENU_HIDE_ANALYZE[] = "Menu/HideAnalyze";
const char SESSION_TASKFILE_KEY[] = "TaskList.File";
const char CLEAR_SYSTEM_ENVIRONMENT_KEY[] = "ProjectExplorer.BuildConfiguration.ClearSystemEnvironment";
const char USER_ENVIRONMENT_CHANGES_KEY[] = "ProjectExplorer.BuildConfiguration.UserEnvironmentChanges";

// Called "RemoteLinux." for backwards compatibility
const char SUPPORTS_RSYNC[] = "RemoteLinux.SupportsRSync";
const char SUPPORTS_SFTP[] = "RemoteLinux.SupportsSftp";

// UI texts
PROJECTEXPLORER_EXPORT QString msgAutoDetected();
PROJECTEXPLORER_EXPORT QString msgAutoDetectedToolTip();
PROJECTEXPLORER_EXPORT QString msgManual();

} // namespace Constants
} // namespace ProjectExplorer
