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

namespace Core {
namespace Constants {

// Modes
const char MODE_WELCOME[]          = "Welcome";
const char MODE_EDIT[]             = "Edit";
const char MODE_DESIGN[]           = "Design";
const int  P_MODE_WELCOME          = 100;
const int  P_MODE_EDIT             = 90;
const int  P_MODE_DESIGN           = 89;

// Menubar
const char MENU_BAR[]              = "QtCreator.MenuBar";

// Menus
const char M_FILE[]                = "QtCreator.Menu.File";
const char M_FILE_RECENTFILES[]    = "QtCreator.Menu.File.RecentFiles";
const char M_EDIT[]                = "QtCreator.Menu.Edit";
const char M_EDIT_ADVANCED[]       = "QtCreator.Menu.Edit.Advanced";
const char M_TOOLS[]               = "QtCreator.Menu.Tools";
const char M_TOOLS_EXTERNAL[]      = "QtCreator.Menu.Tools.External";
const char M_WINDOW[]              = "QtCreator.Menu.Window";
const char M_WINDOW_PANES[]        = "QtCreator.Menu.Window.Panes";
const char M_WINDOW_VIEWS[]        = "QtCreator.Menu.Window.Views";
const char M_HELP[]                = "QtCreator.Menu.Help";

// Contexts
const char C_GLOBAL[]              = "Global Context";
const char C_WELCOME_MODE[]        = "Core.WelcomeMode";
const char C_EDIT_MODE[]           = "Core.EditMode";
const char C_DESIGN_MODE[]         = "Core.DesignMode";
const char C_EDITORMANAGER[]       = "Core.EditorManager";
const char C_NAVIGATION_PANE[]     = "Core.NavigationPane";
const char C_PROBLEM_PANE[]        = "Core.ProblemPane";
const char C_GENERAL_OUTPUT_PANE[] = "Core.GeneralOutputPane";

// Default editor kind
const char K_DEFAULT_TEXT_EDITOR_DISPLAY_NAME[] = QT_TRANSLATE_NOOP("OpenWith::Editors", "Plain Text Editor");
const char K_DEFAULT_TEXT_EDITOR_ID[] = "Core.PlainTextEditor";
const char K_DEFAULT_BINARY_EDITOR_ID[] = "Core.BinaryEditor";

//actions
const char UNDO[]                  = "QtCreator.Undo";
const char REDO[]                  = "QtCreator.Redo";
const char COPY[]                  = "QtCreator.Copy";
const char PASTE[]                 = "QtCreator.Paste";
const char CUT[]                   = "QtCreator.Cut";
const char SELECTALL[]             = "QtCreator.SelectAll";

const char GOTO[]                  = "QtCreator.Goto";

const char NEW[]                   = "QtCreator.New";
const char OPEN[]                  = "QtCreator.Open";
const char OPEN_WITH[]             = "QtCreator.OpenWith";
const char REVERTTOSAVED[]         = "QtCreator.RevertToSaved";
const char SAVE[]                  = "QtCreator.Save";
const char SAVEAS[]                = "QtCreator.SaveAs";
const char SAVEALL[]               = "QtCreator.SaveAll";
const char PRINT[]                 = "QtCreator.Print";
const char EXIT[]                  = "QtCreator.Exit";

const char OPTIONS[]               = "QtCreator.Options";
const char TOGGLE_LEFT_SIDEBAR[]   = "QtCreator.ToggleLeftSidebar";
const char TOGGLE_RIGHT_SIDEBAR[]  = "QtCreator.ToggleRightSidebar";
const char TOGGLE_MODE_SELECTOR[]  = "QtCreator.ToggleModeSelector";
const char TOGGLE_FULLSCREEN[]     = "QtCreator.ToggleFullScreen";
const char THEMEOPTIONS[]          = "QtCreator.ThemeOptions";

const char TR_SHOW_LEFT_SIDEBAR[]  = QT_TRANSLATE_NOOP("Core", "Show Left Sidebar");
const char TR_HIDE_LEFT_SIDEBAR[]  = QT_TRANSLATE_NOOP("Core", "Hide Left Sidebar");

const char TR_SHOW_RIGHT_SIDEBAR[] = QT_TRANSLATE_NOOP("Core", "Show Right Sidebar");
const char TR_HIDE_RIGHT_SIDEBAR[] = QT_TRANSLATE_NOOP("Core", "Hide Right Sidebar");

const char MINIMIZE_WINDOW[]       = "QtCreator.MinimizeWindow";
const char ZOOM_WINDOW[]           = "QtCreator.ZoomWindow";
const char CLOSE_WINDOW[]           = "QtCreator.CloseWindow";

const char SPLIT[]                 = "QtCreator.Split";
const char SPLIT_SIDE_BY_SIDE[]    = "QtCreator.SplitSideBySide";
const char SPLIT_NEW_WINDOW[]      = "QtCreator.SplitNewWindow";
const char REMOVE_CURRENT_SPLIT[]  = "QtCreator.RemoveCurrentSplit";
const char REMOVE_ALL_SPLITS[]     = "QtCreator.RemoveAllSplits";
const char GOTO_PREV_SPLIT[]       = "QtCreator.GoToPreviousSplit";
const char GOTO_NEXT_SPLIT[]       = "QtCreator.GoToNextSplit";
const char CLOSE[]                 = "QtCreator.Close";
const char CLOSE_ALTERNATIVE[]     = "QtCreator.Close_Alternative"; // temporary, see QTCREATORBUG-72
const char CLOSEALL[]              = "QtCreator.CloseAll";
const char CLOSEOTHERS[]           = "QtCreator.CloseOthers";
const char CLOSEALLEXCEPTVISIBLE[] = "QtCreator.CloseAllExceptVisible";
const char GOTONEXT[]              = "QtCreator.GotoNext";
const char GOTOPREV[]              = "QtCreator.GotoPrevious";
const char GOTONEXTINHISTORY[]     = "QtCreator.GotoNextInHistory";
const char GOTOPREVINHISTORY[]     = "QtCreator.GotoPreviousInHistory";
const char GO_BACK[]               = "QtCreator.GoBack";
const char GO_FORWARD[]            = "QtCreator.GoForward";
const char ABOUT_QTCREATOR[]       = "QtCreator.AboutQtCreator";
const char ABOUT_PLUGINS[]         = "QtCreator.AboutPlugins";
const char S_RETURNTOEDITOR[]      = "QtCreator.ReturnToEditor";

// Default groups
const char G_DEFAULT_ONE[]         = "QtCreator.Group.Default.One";
const char G_DEFAULT_TWO[]         = "QtCreator.Group.Default.Two";
const char G_DEFAULT_THREE[]       = "QtCreator.Group.Default.Three";

// Main menu bar groups
const char G_FILE[]                = "QtCreator.Group.File";
const char G_EDIT[]                = "QtCreator.Group.Edit";
const char G_VIEW[]                = "QtCreator.Group.View";
const char G_TOOLS[]               = "QtCreator.Group.Tools";
const char G_WINDOW[]              = "QtCreator.Group.Window";
const char G_HELP[]                = "QtCreator.Group.Help";

// File menu groups
const char G_FILE_NEW[]            = "QtCreator.Group.File.New";
const char G_FILE_OPEN[]           = "QtCreator.Group.File.Open";
const char G_FILE_PROJECT[]        = "QtCreator.Group.File.Project";
const char G_FILE_SAVE[]           = "QtCreator.Group.File.Save";
const char G_FILE_CLOSE[]          = "QtCreator.Group.File.Close";
const char G_FILE_PRINT[]          = "QtCreator.Group.File.Print";
const char G_FILE_OTHER[]          = "QtCreator.Group.File.Other";

// Edit menu groups
const char G_EDIT_UNDOREDO[]       = "QtCreator.Group.Edit.UndoRedo";
const char G_EDIT_COPYPASTE[]      = "QtCreator.Group.Edit.CopyPaste";
const char G_EDIT_SELECTALL[]      = "QtCreator.Group.Edit.SelectAll";
const char G_EDIT_ADVANCED[]       = "QtCreator.Group.Edit.Advanced";

const char G_EDIT_FIND[]           = "QtCreator.Group.Edit.Find";
const char G_EDIT_OTHER[]          = "QtCreator.Group.Edit.Other";

// Advanced edit menu groups
const char G_EDIT_FORMAT[]         = "QtCreator.Group.Edit.Format";
const char G_EDIT_COLLAPSING[]     = "QtCreator.Group.Edit.Collapsing";
const char G_EDIT_TEXT[]           = "QtCreator.Group.Edit.Text";
const char G_EDIT_BLOCKS[]         = "QtCreator.Group.Edit.Blocks";
const char G_EDIT_FONT[]           = "QtCreator.Group.Edit.Font";
const char G_EDIT_EDITOR[]         = "QtCreator.Group.Edit.Editor";

const char G_TOOLS_OPTIONS[]       = "QtCreator.Group.Tools.Options";

// Window menu groups
const char G_WINDOW_SIZE[]         = "QtCreator.Group.Window.Size";
const char G_WINDOW_PANES[]        = "QtCreator.Group.Window.Panes";
const char G_WINDOW_VIEWS[]        = "QtCreator.Group.Window.Views";
const char G_WINDOW_SPLIT[]        = "QtCreator.Group.Window.Split";
const char G_WINDOW_NAVIGATE[]     = "QtCreator.Group.Window.Navigate";
const char G_WINDOW_LIST[]         = "QtCreator.Group.Window.List";
const char G_WINDOW_OTHER[]        = "QtCreator.Group.Window.Other";

// Help groups (global)
const char G_HELP_HELP[]           = "QtCreator.Group.Help.Help";
const char G_HELP_SUPPORT[]        = "QtCreator.Group.Help.Supprt";
const char G_HELP_ABOUT[]          = "QtCreator.Group.Help.About";
const char G_HELP_UPDATES[]        = "QtCreator.Group.Help.Updates";

const char WIZARD_CATEGORY_QT[] = "R.Qt";
const char WIZARD_TR_CATEGORY_QT[] = QT_TRANSLATE_NOOP("Core", "Qt");
const char WIZARD_KIND_UNKNOWN[] = "unknown";
const char WIZARD_KIND_PROJECT[] = "project";
const char WIZARD_KIND_FILE[] = "file";

const char SETTINGS_CATEGORY_CORE[] = "A.Core";
const char SETTINGS_CATEGORY_CORE_ICON[] = ":/core/images/category_core.png";
const char SETTINGS_TR_CATEGORY_CORE[] = QT_TRANSLATE_NOOP("Core", "Environment");
const char SETTINGS_ID_INTERFACE[] = "A.Interface";
const char SETTINGS_ID_SYSTEM[] = "B.Core.System";
const char SETTINGS_ID_SHORTCUTS[] = "C.Keyboard";
const char SETTINGS_ID_TOOLS[] = "D.ExternalTools";
const char SETTINGS_ID_MIMETYPES[] = "E.MimeTypes";

const char SETTINGS_DEFAULTTEXTENCODING[] = "General/DefaultFileEncoding";

const char SETTINGS_THEME[] = "Core/CreatorTheme";
const char DEFAULT_THEME[] = "flat";

const char TR_CLEAR_MENU[]         = QT_TRANSLATE_NOOP("Core", "Clear Menu");

const char DEFAULT_BUILD_DIRECTORY[] = "../%{JS: Util.asciify(\"build-%{CurrentProject:Name}-%{CurrentKit:FileSystemName}-%{CurrentBuild:Name}\")}";

const int MODEBAR_ICON_SIZE = 34;
const int DEFAULT_MAX_LINE_COUNT = 100000;

} // namespace Constants
} // namespace Core
