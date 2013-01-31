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

#ifndef PROJECTEXPLORERCONSTANTS_H
#define PROJECTEXPLORERCONSTANTS_H

#include <QtGlobal>

namespace ProjectExplorer {
namespace Constants {

// Modes and their priorities
const char MODE_SESSION[]         = "Project";
const int  P_MODE_SESSION         = 85;

// Actions
const char NEWSESSION[]           = "ProjectExplorer.NewSession";
const char NEWPROJECT[]           = "ProjectExplorer.NewProject";
const char LOAD[]                 = "ProjectExplorer.Load";
const char UNLOAD[]               = "ProjectExplorer.Unload";
const char CLEARSESSION[]         = "ProjectExplorer.ClearSession";
const char BUILDPROJECTONLY[]     = "ProjectExplorer.BuildProjectOnly";
const char BUILD[]                = "ProjectExplorer.Build";
const char BUILDCM[]              = "ProjectExplorer.BuildCM";
const char BUILDSESSION[]         = "ProjectExplorer.BuildSession";
const char REBUILDPROJECTONLY[]   = "ProjectExplorer.RebuildProjectOnly";
const char REBUILD[]              = "ProjectExplorer.Rebuild";
const char REBUILDCM[]            = "ProjectExplorer.RebuildCM";
const char REBUILDSESSION[]       = "ProjectExplorer.RebuildSession";
const char DEPLOYPROJECTONLY[]    = "ProjectExplorer.DeployProjectOnly";
const char DEPLOY[]               = "ProjectExplorer.Deploy";
const char DEPLOYCM[]             = "ProjectExplorer.DeployCM";
const char DEPLOYSESSION[]        = "ProjectExplorer.DeploySession";
const char PUBLISH[]              = "ProjectExplorer.Publish";
const char CLEANPROJECTONLY[]     = "ProjectExplorer.CleanProjectOnly";
const char CLEAN[]                = "ProjectExplorer.Clean";
const char CLEANCM[]              = "ProjectExplorer.CleanCM";
const char CLEANSESSION[]         = "ProjectExplorer.CleanSession";
const char CANCELBUILD[]          = "ProjectExplorer.CancelBuild";
const char RUN[]                  = "ProjectExplorer.Run";
const char RUNWITHOUTDEPLOY[]     = "ProjectExplorer.RunWithoutDeploy";
const char RUNCONTEXTMENU[]       = "ProjectExplorer.RunContextMenu";
const char STOP[]                 = "ProjectExplorer.Stop";
const char ADDNEWFILE[]           = "ProjectExplorer.AddNewFile";
const char ADDEXISTINGFILES[]     = "ProjectExplorer.AddExistingFiles";
const char ADDNEWSUBPROJECT[]     = "ProjectExplorer.AddNewSubproject";
const char REMOVEPROJECT[]        = "ProjectExplorer.RemoveProject";
const char OPENFILE[]             = "ProjectExplorer.OpenFile";
const char SEARCHONFILESYSTEM[]   = "ProjectExplorer.SearchOnFileSystem";
const char SHOWINGRAPHICALSHELL[] = "ProjectExplorer.ShowInGraphicalShell";
const char OPENTERMIANLHERE[]     = "ProjectExplorer.OpenTerminalHere";
const char REMOVEFILE[]           = "ProjectExplorer.RemoveFile";
const char DELETEFILE[]           = "ProjectExplorer.DeleteFile";
const char RENAMEFILE[]           = "ProjectExplorer.RenameFile";
const char SETSTARTUP[]           = "ProjectExplorer.SetStartup";
const char PROJECTTREE_COLLAPSE_ALL[] = "ProjectExplorer.CollapseAll";

const char SELECTTARGET[]         = "ProjectExplorer.SelectTarget";
const char SELECTTARGETQUICK[]    = "ProjectExplorer.SelectTargetQuick";

// Action priorities
const int  P_ACTION_RUN            = 100;
const int  P_ACTION_BUILDPROJECT   = 80;

// Context
const char C_PROJECTEXPLORER[]    = "Project Explorer";
const char C_PROJECT_TREE[]       = "ProjectExplorer.ProjectTreeContext";
const char C_APP_OUTPUT[]         = "ProjectExplorer.ApplicationOutput";
const char C_COMPILE_OUTPUT[]     = "ProjectExplorer.CompileOutput";

// Languages
const char LANG_CXX[]             = "CXX";
const char LANG_QMLJS[]           = "QMLJS";

// Menus
const char M_RECENTPROJECTS[]     = "ProjectExplorer.Menu.Recent";
const char M_BUILDPROJECT[]       = "ProjectExplorer.Menu.Build";
const char M_DEBUG[]              = "ProjectExplorer.Menu.Debug";
const char M_DEBUG_STARTDEBUGGING[] = "ProjectExplorer.Menu.Debug.StartDebugging";
const char M_SESSION[]            = "ProjectExplorer.Menu.Session";

// Menu groups
const char G_BUILD_BUILD[]        = "ProjectExplorer.Group.Build";
const char G_BUILD_DEPLOY[]       = "ProjectExplorer.Group.Deploy";
const char G_BUILD_REBUILD[]      = "ProjectExplorer.Group.Rebuild";
const char G_BUILD_CLEAN[]        = "ProjectExplorer.Group.Clean";
const char G_BUILD_RUN[]          = "ProjectExplorer.Group.Run";
const char G_BUILD_CANCEL[]       = "ProjectExplorer.Group.BuildCancel";

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

const char RUNMENUCONTEXTMENU[]   = "Project.RunMenu";

// File factory
const char FILE_FACTORY_ID[]      = "ProjectExplorer.FileFactoryId";

// Icons
const char ICON_BUILD[]           = ":/projectexplorer/images/build.png";
const char ICON_BUILD_SMALL[]     = ":/projectexplorer/images/build_small.png";
const char ICON_CLEAN[]           = ":/projectexplorer/images/clean.png";
const char ICON_CLEAN_SMALL[]     = ":/projectexplorer/images/clean_small.png";
const char ICON_REBUILD[]         = ":/projectexplorer/images/rebuild.png";
const char ICON_REBUILD_SMALL[]   = ":/projectexplorer/images/rebuild_small.png";
const char ICON_RUN[]             = ":/projectexplorer/images/run.png";
const char ICON_RUN_SMALL[]       = ":/projectexplorer/images/run_small.png";
const char ICON_DEBUG_SMALL[]     = ":/projectexplorer/images/debugger_start_small.png";
const char ICON_STOP[]            = ":/projectexplorer/images/stop.png";
const char ICON_STOP_SMALL[]      = ":/projectexplorer/images/stop_small.png";
const char ICON_WINDOW[]          = ":/projectexplorer/images/window.png";

const char TASK_BUILD[]           = "ProjectExplorer.Task.Build";

// Mime types
const char C_SOURCE_MIMETYPE[]    = "text/x-csrc";
const char C_HEADER_MIMETYPE[]    = "text/x-chdr";
const char CPP_SOURCE_MIMETYPE[]  = "text/x-c++src";
const char CPP_HEADER_MIMETYPE[]  = "text/x-c++hdr";
const char FORM_MIMETYPE[]        = "application/x-designer";
const char QML_MIMETYPE[]         = "application/x-qml"; // separate def also in qmljstoolsconstants.h
const char RESOURCE_MIMETYPE[]    = "application/vnd.nokia.xml.qt.resource";

// Settings page
const char PROJECTEXPLORER_SETTINGS_CATEGORY[]  = "K.ProjectExplorer";
const char PROJECTEXPLORER_SETTINGS_TR_CATEGORY[] = QT_TRANSLATE_NOOP("ProjectExplorer", "Build & Run");
const char PROJECTEXPLORER_SETTINGS_CATEGORY_ICON[]  = ":/core/images/category_buildrun.png";
const char PROJECTEXPLORER_SETTINGS_ID[] = "A.ProjectExplorer.ProjectExplorer";
const char TOOLCHAIN_SETTINGS_PAGE_ID[] = "M.ProjectExplorer.ToolChainOptions";
const char KITS_SETTINGS_PAGE_ID[] = "D.ProjectExplorer.KitsOptions";

// Device settings page
const char DEVICE_SETTINGS_CATEGORY[] = "XW.Devices";
const char DEVICE_SETTINGS_PAGE_ID[] = "AA.Device Settings";

// Task categories
const char TASK_CATEGORY_COMPILE[] = "Task.Category.Compile";
const char TASK_CATEGORY_BUILDSYSTEM[] = "Task.Category.Buildsystem";

// Wizard categories
const char QT_PROJECT_WIZARD_CATEGORY[] = "H.QtProjects";
const char QT_PROJECT_WIZARD_CATEGORY_DISPLAY[] = QT_TRANSLATE_NOOP("ProjectExplorer", "Other Project");

const char QT_APPLICATION_WIZARD_CATEGORY[] = "F.QtApplications";
const char QT_APPLICATION_WIZARD_CATEGORY_DISPLAY[] = QT_TRANSLATE_NOOP("ProjectExplorer", "Applications");

const char LIBRARIES_WIZARD_CATEGORY[] = "G.Libraries";
const char LIBRARIES_WIZARD_CATEGORY_DISPLAY[] = QT_TRANSLATE_NOOP("ProjectExplorer", "Libraries");

const char PROJECT_WIZARD_CATEGORY[] = "I.Projects"; // (after Qt)
const char PROJECT_WIZARD_CATEGORY_DISPLAY[] = QT_TRANSLATE_NOOP("ProjectExplorer", "Non-Qt Project");

const char IMPORT_WIZARD_CATEGORY[] = "T.Import";
const char IMPORT_WIZARD_CATEGORY_DISPLAY[] = QT_TRANSLATE_NOOP("ProjectExplorer", "Import Project");

// Wizard extra values
const char PREFERED_PROJECT_NODE[] = "ProjectExplorer.PreferedProjectNode";
const char PROJECT_KIT_IDS[] = "ProjectExplorer.Profile.Ids";

// Build step lists ids:
const char BUILDSTEPS_CLEAN[] = "ProjectExplorer.BuildSteps.Clean";
const char BUILDSTEPS_BUILD[] = "ProjectExplorer.BuildSteps.Build";
const char BUILDSTEPS_DEPLOY[] = "ProjectExplorer.BuildSteps.Deploy";

// Deploy Configuration id:
const char DEFAULT_DEPLOYCONFIGURATION_ID[] = "ProjectExplorer.DefaultDeployConfiguration";

// ToolChain Ids
const char CLANG_TOOLCHAIN_ID[] = "ProjectExplorer.ToolChain.Clang";
const char GCC_TOOLCHAIN_ID[] = "ProjectExplorer.ToolChain.Gcc";
const char LINUXICC_TOOLCHAIN_ID[] = "ProjectExplorer.ToolChain.LinuxIcc";
const char MINGW_TOOLCHAIN_ID[] = "ProjectExplorer.ToolChain.Mingw";
const char MSVC_TOOLCHAIN_ID[] = "ProjectExplorer.ToolChain.Msvc";
const char WINCE_TOOLCHAIN_ID[] = "ProjectExplorer.ToolChain.WinCE";
const char CUSTOM_TOOLCHAIN_ID[] = "ProjectExplorer.ToolChain.Custom";

// Run Configuration defaults:
const int QML_DEFAULT_DEBUG_SERVER_PORT = 3768;

// Default directory to run custom (build) commands in.
const char DEFAULT_WORKING_DIR[] = "%{buildDir}";

// Desktop Device related ids:
const char DESKTOP_DEVICE_ID[] = "Desktop Device";
const char DESKTOP_DEVICE_TYPE[] = "Desktop";

// Variable Names:
const char VAR_CURRENTPROJECT_PATH[] = "CurrentProject:Path";
const char VAR_CURRENTPROJECT_FILEPATH[] = "CurrentProject:FilePath";
const char VAR_CURRENTPROJECT_BUILDPATH[] = "CurrentProject:BuildPath";
const char VAR_CURRENTPROJECT_NAME[] = "CurrentProject:Name";
const char VAR_CURRENTKIT_NAME[] = "CurrentKit:Name";
const char VAR_CURRENTKIT_FILESYSTEMNAME[] = "CurrentKit:FileSystemName";
const char VAR_CURRENTKIT_ID[] = "CurrentKit:Id";
const char VAR_CURRENTBUILD_NAME[] = "CurrentBuild:Name";
const char VAR_CURRENTBUILD_TYPE[] = "CurrentBuild:Type";

} // namespace Constants

// Run modes
enum RunMode {
    NoRunMode,
    NormalRunMode,
    DebugRunMode,
    DebugRunModeWithBreakOnMain,
    QmlProfilerRunMode,
    CallgrindRunMode,
    MemcheckRunMode
};

} // namespace ProjectExplorer

#endif // PROJECTEXPLORERCONSTANTS_H
