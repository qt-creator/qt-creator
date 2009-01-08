/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.3, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/

#ifndef CORECONSTANTS_H
#define CORECONSTANTS_H

#include <extensionsystem/ExtensionSystemInterfaces>

namespace Core {
namespace Constants {

#define IDE_VERSION_MAJOR 0
#define IDE_VERSION_MINOR 9
#define IDE_VERSION_RELEASE 2

#define STRINGIFY_INTERNAL(x) #x
#define STRINGIFY(x) STRINGIFY_INTERNAL(x)

#define IDE_VERSION STRINGIFY(IDE_VERSION_MAJOR) \
    "." STRINGIFY(IDE_VERSION_MINOR) \
    "." STRINGIFY(IDE_VERSION_RELEASE)

const char * const IDE_VERSION_LONG      = IDE_VERSION;
const char * const IDE_AUTHOR            = "Nokia Corporation";
const char * const IDE_YEAR              = "2009";

#ifdef IDE_REVISION
const char * const IDE_REVISION_STR      = STRINGIFY(IDE_REVISION);
#else
const char * const IDE_REVISION_STR      = "";
#endif

#undef IDE_VERSION
#undef STRINGIFY
#undef STRINGIFY_INTERNAL

//modes
const char * const MODE_WELCOME          = "QtCreator.Mode.Welcome";
const char * const MODE_EDIT             = "QtCreator.Mode.Edit";
const char * const MODE_OUTPUT           = "QtCreator.Mode.Output";
const int          P_MODE_WELCOME        = 100;
const int          P_MODE_EDIT           = 90;
const int          P_MODE_OUTPUT         = 10;

//menubar
const char * const MENU_BAR              = "QtCreator.MenuBar";

//menus
const char * const M_FILE                = "QtCreator.Menu.File";
const char * const M_FILE_OPEN           = "QtCreator.Menu.File.Open";
const char * const M_FILE_NEW            = "QtCreator.Menu.File.New";
const char * const M_FILE_RECENTFILES    = "QtCreator.Menu.File.RecentFiles";
const char * const M_EDIT                = "QtCreator.Menu.Edit";
const char * const M_EDIT_ADVANCED       = "QtCreator.Menu.Edit.Advanced";
const char * const M_TOOLS               = "QtCreator.Menu.Tools";
const char * const M_WINDOW              = "QtCreator.Menu.Window";
const char * const M_WINDOW_PANES        = "QtCreator.Menu.Window.Panes";
const char * const M_HELP                = "QtCreator.Menu.Help";

//contexts
const char * const C_GLOBAL              = "Global Context";
const int          C_GLOBAL_ID           = 0;
const char * const C_WELCOME_MODE        = "Core.WelcomeMode";
const char * const C_EDIT_MODE           = "Core.EditMode";
const char * const C_EDITORMANAGER       = "Core.EditorManager";
const char * const C_NAVIGATION_PANE     = "Core.NavigationPane";
const char * const C_PROBLEM_PANE        = "Core.ProblemPane";

//default editor kind
const char * const K_DEFAULT_TEXT_EDITOR = "Plain Text Editor";
const char * const K_DEFAULT_BINARY_EDITOR = "Binary Editor";

//actions
const char * const UNDO                  = "QtCreator.Undo";
const char * const REDO                  = "QtCreator.Redo";
const char * const COPY                  = "QtCreator.Copy";
const char * const PASTE                 = "QtCreator.Paste";
const char * const CUT                   = "QtCreator.Cut";
const char * const SELECTALL             = "QtCreator.SelectAll";

const char * const GOTO                  = "QtCreator.Goto";

const char * const NEW                   = "QtCreator.New";
const char * const OPEN                  = "QtCreator.Open";
const char * const OPEN_WITH             = "QtCreator.OpenWith";
const char * const REVERTTOSAVED         = "QtCreator.RevertToSaved";
const char * const SAVE                  = "QtCreator.Save";
const char * const SAVEAS                = "QtCreator.SaveAs";
const char * const SAVEALL               = "QtCreator.SaveAll";
const char * const PRINT                 = "QtCreator.Print";
const char * const EXIT                  = "QtCreator.Exit";

const char * const OPTIONS               = "QtCreator.Options";
const char * const TOGGLE_SIDEBAR        = "QtCreator.ToggleSidebar";

const char * const MINIMIZE_WINDOW       = "QtCreator.MinimizeWindow";
const char * const ZOOM_WINDOW           = "QtCreator.ZoomWindow";

const char * const HORIZONTAL            = "QtCreator.Horizontal";
const char * const VERTICAL              = "QtCreator.Vertical";
const char * const REMOVE                = "QtCreator.Remove";
const char * const SAVEASDEFAULT         = "QtCreator.SaveAsDefaultLayout";
const char * const RESTOREDEFAULT        = "QtCreator.RestoreDefaultLayout";
const char * const CLOSE                 = "QtCreator.Close";
const char * const DUPLICATEDOCUMENT     = "QtCreator.DuplicateDocument";
const char * const CLOSEALL              = "QtCreator.CloseAll";
const char * const GOTONEXT              = "QtCreator.GotoNext";
const char * const GOTOPREV              = "QtCreator.GotoPrevious";
const char * const GOTONEXTINHISTORY     = "QtCreator.GotoNextInHistory";
const char * const GOTOPREVINHISTORY     = "QtCreator.GotoPreviousInHistory";
const char * const GO_BACK               = "QtCreator.GoBack";
const char * const GO_FORWARD            = "QtCreator.GoForward";
const char * const GOTOPREVIOUSGROUP     = "QtCreator.GotoPreviousTabGroup";
const char * const GOTONEXTGROUP         = "QtCreator.GotoNextTabGroup";
const char * const WINDOWSLIST           = "QtCreator.WindowsList";
const char * const ABOUT_WORKBENCH       = "QtCreator.AboutWorkbench";
const char * const ABOUT_PLUGINS         = "QtCreator.AboutPlugins";
const char * const ABOUT_QT              = "QtCreator.AboutQt";
const char * const S_RETURNTOEDITOR      = "QtCreator.ReturnToEditor";
const char * const OPEN_IN_EXTERNAL_EDITOR = "QtCreattor.OpenInExternalEditor";

// default groups
const char * const G_DEFAULT_ONE         = "QtCreator.Group.Default.One";
const char * const G_DEFAULT_TWO         = "QtCreator.Group.Default.Two";
const char * const G_DEFAULT_THREE       = "QtCreator.Group.Default.Three";

// main menu bar groups
const char * const G_FILE                = "QtCreator.Group.File";
const char * const G_EDIT                = "QtCreator.Group.Edit";
const char * const G_VIEW                = "QtCreator.Group.View";
const char * const G_TOOLS               = "QtCreator.Group.Tools";
const char * const G_WINDOW              = "QtCreator.Group.Window";
const char * const G_HELP                = "QtCreator.Group.Help";

// file menu groups
const char * const G_FILE_NEW            = "QtCreator.Group.File.New";
const char * const G_FILE_OPEN           = "QtCreator.Group.File.Open";
const char * const G_FILE_PROJECT        = "QtCreator.Group.File.Project";
const char * const G_FILE_SAVE           = "QtCreator.Group.File.Save";
const char * const G_FILE_CLOSE          = "QtCreator.Group.File.Close";
const char * const G_FILE_PRINT          = "QtCreator.Group.File.Print";
const char * const G_FILE_OTHER          = "QtCreator.Group.File.Other";

// edit menu groups
const char * const G_EDIT_UNDOREDO       = "QtCreator.Group.Edit.UndoRedo";
const char * const G_EDIT_COPYPASTE      = "QtCreator.Group.Edit.CopyPaste";
const char * const G_EDIT_SELECTALL      = "QtCreator.Group.Edit.SelectAll";
const char * const G_EDIT_FORMAT         = "QtCreator.Group.Edit.Format";

const char * const G_EDIT_FIND           = "QtCreator.Group.Edit.Find";
const char * const G_EDIT_OTHER          = "QtCreator.Group.Edit.Other";

// window menu groups
const char * const G_WINDOW_SIZE         = "QtCreator.Group.Window.Size";
const char * const G_WINDOW_PANES        = "QtCreator.Group.Window.Panes";
const char * const G_WINDOW_SPLIT        = "QtCreator.Group.Window.Split";
const char * const G_WINDOW_CLOSE        = "QtCreator.Group.Window.Close";
const char * const G_WINDOW_NAVIGATE     = "QtCreator.Group.Window.Navigate";
const char * const G_WINDOW_NAVIGATE_GROUPS = "QtCreator.Group.Window.Navigate.Groups";
const char * const G_WINDOW_OTHER        = "QtCreator.Group.Window.Other";
const char * const G_WINDOW_LIST         = "QtCreator.Group.Window.List";

// help groups (global)
const char * const G_HELP_HELP           = "QtCreator.Group.Help.Help";
const char * const G_HELP_ABOUT          = "QtCreator.Group.Help.About";

const char * const ICON_MINUS            = ":/qworkbench/images/minus.png";
const char * const ICON_PLUS             = ":/qworkbench/images/plus.png";
const char * const ICON_NEWFILE          = ":/qworkbench/images/filenew.png";
const char * const ICON_OPENFILE         = ":/qworkbench/images/fileopen.png";
const char * const ICON_SAVEFILE         = ":/qworkbench/images/filesave.png";
const char * const ICON_UNDO             = ":/qworkbench/images/undo.png";
const char * const ICON_REDO             = ":/qworkbench/images/redo.png";
const char * const ICON_COPY             = ":/qworkbench/images/editcopy.png";
const char * const ICON_PASTE            = ":/qworkbench/images/editpaste.png";
const char * const ICON_CUT              = ":/qworkbench/images/editcut.png";
const char * const ICON_NEXT             = ":/qworkbench/images/next.png";
const char * const ICON_PREV             = ":/qworkbench/images/prev.png";
const char * const ICON_DIR              = ":/qworkbench/images/dir.png";
const char * const ICON_CLEAN_PANE       = ":/qworkbench/images/clean_pane_small.png";
const char * const ICON_CLEAR            = ":/qworkbench/images/clear.png";
const char * const ICON_FIND             = ":/qworkbench/images/find.png";
const char * const ICON_FINDNEXT         = ":/qworkbench/images/findnext.png";
const char * const ICON_REPLACE          = ":/qworkbench/images/replace.png";
const char * const ICON_RESET            = ":/qworkbench/images/reset.png";
const char * const ICON_MAGNIFIER        = ":/qworkbench/images/magnifier.png";
const char * const ICON_TOGGLE_SIDEBAR   = ":/qworkbench/images/sidebaricon.png";

// wizard kind
const char * const WIZARD_TYPE_FILE      = "QtCreator::WizardType::File";
const char * const WIZARD_TYPE_CLASS     = "QtCreator::WizardType::Class";

} // namespace Constants
} // namespace Core

#endif // CORECONSTANTS_H
