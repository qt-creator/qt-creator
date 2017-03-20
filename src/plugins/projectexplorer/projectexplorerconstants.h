/****************************************************************************
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

#pragma once

#include <QtGlobal>

namespace ProjectExplorer {
namespace Constants {

// Modes and their priorities
const char MODE_SESSION[]         = "Project";

// Actions
const char BUILD[]                = "ProjectExplorer.Build";
const char STOP[]                 = "ProjectExplorer.Stop";

// Context
const char C_PROJECT_TREE[]       = "ProjectExplorer.ProjectTreeContext";

// Menus
const char M_BUILDPROJECT[]       = "ProjectExplorer.Menu.Build";
const char M_DEBUG[]              = "ProjectExplorer.Menu.Debug";
const char M_DEBUG_STARTDEBUGGING[] = "ProjectExplorer.Menu.Debug.StartDebugging";

// Menu groups
const char G_BUILD_BUILD[]        = "ProjectExplorer.Group.Build";
const char G_BUILD_DEPLOY[]       = "ProjectExplorer.Group.Deploy";
const char G_BUILD_REBUILD[]      = "ProjectExplorer.Group.Rebuild";
const char G_BUILD_CLEAN[]        = "ProjectExplorer.Group.Clean";

// Context menus
const char M_SESSIONCONTEXT[]     = "Project.Menu.Session";
const char M_PROJECTCONTEXT[]     = "Project.Menu.Project";
const char M_SUBPROJECTCONTEXT[]  = "Project.Menu.SubProject";
const char M_FOLDERCONTEXT[]      = "Project.Menu.Folder";
const char M_FILECONTEXT[]        = "Project.Menu.File";
const char M_OPENFILEWITHCONTEXT[] = "Project.Menu.File.OpenWith";

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
const char G_PROJECT_TREE[]       = "Project.Group.Tree";
const char G_PROJECT_LAST[]       = "Project.Group.Last";

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
const char RESOURCE_MIMETYPE[]    = "application/vnd.qt.xml.resource";
const char SCXML_MIMETYPE[]       = "application/scxml+xml";

// Settings page
const char PROJECTEXPLORER_SETTINGS_CATEGORY[]  = "K.ProjectExplorer";
const char PROJECTEXPLORER_SETTINGS_TR_CATEGORY[] = QT_TRANSLATE_NOOP("ProjectExplorer", "Build & Run");
const char PROJECTEXPLORER_SETTINGS_CATEGORY_ICON[]  = ":/projectexplorer/images/category_buildrun.png";
const char PROJECTEXPLORER_SETTINGS_ID[] = "A.ProjectExplorer.ProjectExplorer";
const char TOOLCHAIN_SETTINGS_PAGE_ID[] = "M.ProjectExplorer.ToolChainOptions";
const char DEBUGGER_SETTINGS_PAGE_ID[] = "N.ProjectExplorer.DebuggerOptions";
const char KITS_SETTINGS_PAGE_ID[] = "D.ProjectExplorer.KitsOptions";

// Device settings page
const char DEVICE_SETTINGS_CATEGORY[] = "XW.Devices";
const char DEVICE_SETTINGS_TR_CATEGORY[] = QT_TRANSLATE_NOOP("ProjectExplorer", "Devices");
const char DEVICE_SETTINGS_PAGE_ID[] = "AA.Device Settings";

// Task categories
const char TASK_CATEGORY_COMPILE[] = "Task.Category.Compile";
const char TASK_CATEGORY_BUILDSYSTEM[] = "Task.Category.Buildsystem";
const char TASK_CATEGORY_DEPLOYMENT[] = "Task.Category.Deploy";

// Wizard categories
const char QT_PROJECT_WIZARD_CATEGORY[] = "H.Project";
const char QT_PROJECT_WIZARD_CATEGORY_DISPLAY[] = QT_TRANSLATE_NOOP("ProjectExplorer", "Other Project");

const char QT_APPLICATION_WIZARD_CATEGORY[] = "F.Application";
const char QT_APPLICATION_WIZARD_CATEGORY_DISPLAY[] = QT_TRANSLATE_NOOP("ProjectExplorer", "Application");

const char LIBRARIES_WIZARD_CATEGORY[] = "G.Library";
const char LIBRARIES_WIZARD_CATEGORY_DISPLAY[] = QT_TRANSLATE_NOOP("ProjectExplorer", "Library");

const char IMPORT_WIZARD_CATEGORY[] = "T.Import";
const char IMPORT_WIZARD_CATEGORY_DISPLAY[] = QT_TRANSLATE_NOOP("ProjectExplorer", "Import Project");

// Wizard extra values
const char PREFERRED_PROJECT_NODE[] = "ProjectExplorer.PreferredProjectNode";
const char PROJECT_KIT_IDS[] = "ProjectExplorer.Profile.Ids";

// Build step lists ids:
const char BUILDSTEPS_CLEAN[] = "ProjectExplorer.BuildSteps.Clean";
const char BUILDSTEPS_BUILD[] = "ProjectExplorer.BuildSteps.Build";
const char BUILDSTEPS_DEPLOY[] = "ProjectExplorer.BuildSteps.Deploy";

// Language

// Keep these short: These constants are exposed to the MacroExplorer!
const char C_LANGUAGE_ID[] = "C";
const char CXX_LANGUAGE_ID[] = "Cxx";
const char QMLJS_LANGUAGE_ID[] = "QMLJS";

// ToolChain TypeIds
const char CLANG_TOOLCHAIN_TYPEID[] = "ProjectExplorer.ToolChain.Clang";
const char GCC_TOOLCHAIN_TYPEID[] = "ProjectExplorer.ToolChain.Gcc";
const char LINUXICC_TOOLCHAIN_TYPEID[] = "ProjectExplorer.ToolChain.LinuxIcc";
const char MINGW_TOOLCHAIN_TYPEID[] = "ProjectExplorer.ToolChain.Mingw";
const char MSVC_TOOLCHAIN_TYPEID[] = "ProjectExplorer.ToolChain.Msvc";
const char CLANG_CL_TOOLCHAIN_TYPEID[] = "ProjectExplorer.ToolChain.ClangCl";
const char WINCE_TOOLCHAIN_TYPEID[] = "ProjectExplorer.ToolChain.WinCE";
const char CUSTOM_TOOLCHAIN_TYPEID[] = "ProjectExplorer.ToolChain.Custom";

// Default directory to run custom (build) commands in.
const char DEFAULT_WORKING_DIR[] = "%{buildDir}";
const char DEFAULT_WORKING_DIR_ALTERNATE[] = "%{sourceDir}";

// Desktop Device related ids:
const char DESKTOP_DEVICE_ID[] = "Desktop Device";
const char DESKTOP_DEVICE_TYPE[] = "Desktop";
const int DESKTOP_PORT_START = 30000;
const int DESKTOP_PORT_END = 31000;

// Variable Names:
const char VAR_CURRENTPROJECT_PREFIX[] = "CurrentProject";
const char VAR_CURRENTPROJECT_NAME[] = "CurrentProject:Name";
const char VAR_CURRENTKIT_NAME[] = "CurrentKit:Name";
const char VAR_CURRENTKIT_FILESYSTEMNAME[] = "CurrentKit:FileSystemName";
const char VAR_CURRENTKIT_ID[] = "CurrentKit:Id";
const char VAR_CURRENTBUILD_NAME[] = "CurrentBuild:Name";
const char VAR_CURRENTBUILD_TYPE[] = "CurrentBuild:Type";
const char VAR_CURRENTBUILD_ENV[] = "CurrentBuild:Env";
const char VAR_CURRENTRUN_NAME[] = "CurrentRun:Name";

// JsonWizard:
const char PAGE_ID_PREFIX[] = "PE.Wizard.Page.";
const char GENERATOR_ID_PREFIX[] = "PE.Wizard.Generator.";

// RunMode
const char NO_RUN_MODE[]="RunConfiguration.NoRunMode";
const char NORMAL_RUN_MODE[]="RunConfiguration.NormalRunMode";
const char QML_PROFILER_RUN_MODE[]="RunConfiguration.QmlProfilerRunMode";
const char PERFPROFILER_RUN_MODE[]="PerfProfiler.RunMode";
const char DEBUG_RUN_MODE[]="RunConfiguration.DebugRunMode";
const char DEBUG_RUN_MODE_WITH_BREAK_ON_MAIN[]="RunConfiguration.DebugRunModeWithBreakOnMain";

// Navigation Widget
const char PROJECTTREE_ID[] = "Projects";

// File icon overlays
const char FILEOVERLAY_QT[]=":/projectexplorer/images/fileoverlay_qt.png";
const char FILEOVERLAY_QML[]=":/projectexplorer/images/fileoverlay_qml.png";
const char FILEOVERLAY_UI[]=":/projectexplorer/images/fileoverlay_ui.png";
const char FILEOVERLAY_QRC[]=":/projectexplorer/images/fileoverlay_qrc.png";
const char FILEOVERLAY_CPP[]=":/projectexplorer/images/fileoverlay_cpp.png";
const char FILEOVERLAY_H[]=":/projectexplorer/images/fileoverlay_h.png";
const char FILEOVERLAY_SCXML[]=":/projectexplorer/images/fileoverlay_scxml.png";
const char FILEOVERLAY_UNKNOWN[]=":/projectexplorer/images/fileoverlay_unknown.png";

} // namespace Constants
} // namespace ProjectExplorer
