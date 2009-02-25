/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#ifndef PROJECTEXPLORERCONSTANTS_H
#define PROJECTEXPLORERCONSTANTS_H

namespace ProjectExplorer {
namespace Constants {

// modes and their priorities
const char * const MODE_SESSION         = "Project";
const int          P_MODE_SESSION       = 85;

// actions
const char * const NEWSESSION           = "ProjectExplorer.NewSession";
const char * const NEWPROJECT           = "ProjectExplorer.NewProject";
const char * const LOAD                 = "ProjectExplorer.Load";
const char * const UNLOAD               = "ProjectExplorer.Unload";
const char * const CLEARSESSION         = "ProjectExplorer.ClearSession";
const char * const BUILD                = "ProjectExplorer.Build";
const char * const BUILDSESSION         = "ProjectExplorer.BuildSession";
const char * const REBUILD              = "ProjectExplorer.Rebuild";
const char * const REBUILDSESSION       = "ProjectExplorer.RebuildSession";
const char * const CLEAN                = "ProjectExplorer.Clean";
const char * const CLEANSESSION         = "ProjectExplorer.CleanSession";
const char * const BUILDCONFIGURATIONMENU = "ProjectExplorer.BuildConfigurationMenu";
const char * const CANCELBUILD          = "ProjectExplorer.CancelBuild";
const char * const RUNCONFIGURATIONMENU = "ProjectExplorer.RunConfigurationMenu";
const char * const RUN                  = "ProjectExplorer.Run";
const char * const RUNCONTEXTMENU       = "ProjectExplorer.RunContextMenu";
const char * const STOP                 = "ProjectExplorer.Stop";
const char * const DEBUG                = "ProjectExplorer.Debug";
const char * const DEPENDENCIES         = "ProjectExplorer.Dependencies";
const char * const FINDINALLPROJECTS    = "ProjectExplorer.FindInAllProjects";
const char * const GOTOTASKWINDOW       = "ProjectExplorer.GoToTaskWindow";
const char * const SHOWPROPERTIES       = "ProjectExplorer.ShowProperties";
const char * const ADDNEWFILE           = "ProjectExplorer.AddNewFile";
const char * const ADDEXISTINGFILES     = "ProjectExplorer.AddExistingFiles";
const char * const OPENFILE             = "ProjectExplorer.OpenFile";
const char * const REMOVEFILE           = "ProjectExplorer.RemoveFile";
const char * const RENAMEFILE           = "ProjectExplorer.RenameFile";

// Run modes
const char * const RUNMODE              = "ProjectExplorer.RunMode";
const char * const DEBUGMODE            = "ProjectExplorer.DebugMode";

// action priorities
const int          P_ACTION_RUN            = 100;
const int          P_ACTION_DEBUG          = 90;
const int          P_ACTION_BUILDSESSION   = 80;

// context
const char * const C_PROJECTEXPLORER    = "Project Explorer";

// languages
const char * const LANG_CXX             = "CXX";

// menus
const char * const M_RECENTPROJECTS     = "ProjectExplorer.Menu.Recent";
const char * const M_BUILDPROJECT       = "ProjectExplorer.Menu.Build";
const char * const M_DEBUG              = "ProjectExplorer.Menu.Debug";
const char * const M_SESSION            = "ProjectExplorer.Menu.Session";

// toolbars
const char * const T_BUILDPROJECT       = "ProjectExplorer.ToolBar.Build";

// menu groups
const char * const G_BUILD_SESSION      = "ProjectExplorer.Group.BuildSession";
const char * const G_BUILD_PROJECT      = "ProjectExplorer.Group.Build";
const char * const G_BUILD_OTHER        = "ProjectExplorer.Group.Other";
const char * const G_BUILD_RUN          = "ProjectExplorer.Group.Run";
const char * const G_BUILD_TASK         = "ProjectExplorer.Group.BuildTask";
const char * const G_BUILD_CANCEL       = "ProjectExplorer.Group.BuildCancel";

// toolbar groups
const char * const G_TOOLBAR_CUSTOM     = "ProjectExplorer.ToolBarGroup.Custom";
const char * const G_TOOLBAR_BUILD      = "ProjectExplorer.ToolBarGroup.Build";
const char * const G_TOOLBAR_RUN        = "ProjectExplorer.ToolBarGroup.Run";
const char * const G_TOOLBAR_OTHER      = "ProjectExplorer.ToolBarGroup.Other";

// context menus
const char * const M_SESSIONCONTEXT     = "Project.Menu.Session";
const char * const M_PROJECTCONTEXT     = "Project.Menu.Project";
const char * const M_SUBPROJECTCONTEXT  = "Project.Menu.SubProject";
const char * const M_FOLDERCONTEXT      = "Project.Menu.Folder";
const char * const M_FILECONTEXT        = "Project.Menu.File";
const char * const M_OPENFILEWITHCONTEXT = "Project.Menu.File.OpenWith";

// context menu groups
const char * const G_SESSION_BUILD      = "Session.Group.Build";
const char * const G_SESSION_FILES      = "Session.Group.Files";
const char * const G_SESSION_OTHER      = "Session.Group.Other";
const char * const G_SESSION_CONFIG     = "Session.Group.Config";

const char * const G_PROJECT_OPEN       = "Project.Group.Open";
const char * const G_PROJECT_NEW        = "Project.Group.New";
const char * const G_PROJECT_FILES      = "Project.Group.Files";
const char * const G_PROJECT_BUILD      = "Project.Group.Build";
const char * const G_PROJECT_OTHER      = "Project.Group.Other";
const char * const G_PROJECT_RUN        = "Project.Group.Run";
const char * const G_PROJECT_CONFIG     = "Project.Group.Config";

const char * const G_FOLDER_FILES       = "ProjectFolder.Group.Files";
const char * const G_FOLDER_OTHER       = "ProjectFolder.Group.Other";
const char * const G_FOLDER_CONFIG      = "ProjectFolder.Group.Config";

const char * const G_FILE_OPEN          = "ProjectFile.Group.Open";
const char * const G_FILE_OTHER         = "ProjectFile.Group.Other";
const char * const G_FILE_CONFIG        = "ProjectFile.Group.Config";

// file kind
const char * const FILE_FACTORY_KIND    = "ProjectExplorer.FileFactoryKind";

// wizard kind
const char * const WIZARD_TYPE_PROJECT  = "ProjectExplorer.WizardType.Project";

// icons
const char * const ICON_BUILD           = ":/projectexplorer/images/build.png";
const char * const ICON_BUILD_SMALL     = ":/projectexplorer/images/build_small.png";
const char * const ICON_CLEAN           = ":/projectexplorer/images/clean.png";
const char * const ICON_CLEAN_SMALL     = ":/projectexplorer/images/clean_small.png";
const char * const ICON_REBUILD         = ":/projectexplorer/images/rebuild.png";
const char * const ICON_REBUILD_SMALL   = ":/projectexplorer/images/rebuild_small.png";
const char * const ICON_RUN             = ":/projectexplorer/images/run.png";
const char * const ICON_RUN_SMALL       = ":/projectexplorer/images/run_small.png";
const char * const ICON_SESSION         = ":/projectexplorer/images/session.png";
const char * const ICON_INSERT_LINE     = ":/projectexplorer/images/insert_line_small.png";
const char * const ICON_DEBUG           = ":/projectexplorer/images/debugger_start.png";
const char * const ICON_DEBUG_SMALL     = ":/projectexplorer/images/debugger_start_small.png";
const char * const ICON_CLOSETAB        = ":/projectexplorer/images/closetab.png";
const char * const ICON_STOP            = ":/projectexplorer/images/stop.png";

// find filters
const char * const FIND_CUR_PROJECT     = "ProjectExplorer.FindFilter.CurrentProject";
const char * const FIND_ALL_PROJECTS    = "ProjectExplorer.FindFilter.AllProjects";

const char * const TASK_BUILD           = "ProjectExplorer.Task.Build";
const char * const SESSIONFILE_MIMETYPE = "application/vnd.nokia.xml.qt.creator.session";


const char * const PROFILE_MIMETYPE  = "application/vnd.nokia.qt.qmakeprofile";
const char * const C_SOURCE_MIMETYPE = "text/x-csrc";
const char * const C_HEADER_MIMETYPE = "text/x-chdr";
const char * const CPP_SOURCE_MIMETYPE = "text/x-c++src";
const char * const CPP_HEADER_MIMETYPE = "text/x-c++hdr";
const char * const FORM_MIMETYPE = "application/x-designer";
const char * const RESOURCE_MIMETYPE = "application/vnd.nokia.xml.qt.resource";

} // namespace Constants
} // namespace ProjectExplorer

#endif // PROJECTEXPLORERCONSTANTS_H
