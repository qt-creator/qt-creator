// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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

// TouchBar
const char TOUCH_BAR[]             = "QtCreator.TouchBar";

// Menubar
const char MENU_BAR[]              = "QtCreator.MenuBar";

// Menus
const char M_FILE[]                = "QtCreator.Menu.File";
const char M_FILE_RECENTFILES[]    = "QtCreator.Menu.File.RecentFiles";
const char M_EDIT[]                = "QtCreator.Menu.Edit";
const char M_EDIT_ADVANCED[]       = "QtCreator.Menu.Edit.Advanced";
const char M_VIEW[]                = "QtCreator.Menu.View";
const char M_VIEW_MODESTYLES[]     = "QtCreator.Menu.View.ModeStyles";
const char M_VIEW_VIEWS[]          = "QtCreator.Menu.View.Views";
const char M_VIEW_PANES[]          = "QtCreator.Menu.View.Panes";
const char M_TOOLS[]               = "QtCreator.Menu.Tools";
const char M_TOOLS_EXTERNAL[]      = "QtCreator.Menu.Tools.External";
const char M_TOOLS_DEBUG[]         = "QtCreator.Menu.Tools.Debug";
const char M_WINDOW[]              = "QtCreator.Menu.Window";
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
// Special context that leads to all "more specific" contexts to be ignored.
// If you use Context(mycontextId, C_GLOBAL_CUTOFF) for a widget that has focus,
// mycontextId will be enabled but the contexts for all parent widgets, the manually added
// "additional" contexts, and the global context will be turned off.
const char C_GLOBAL_CUTOFF[]       = "Global Cutoff";

// Default editor kind
const char K_DEFAULT_TEXT_EDITOR_DISPLAY_NAME[] = QT_TRANSLATE_NOOP("QtC::Core", "Plain Text Editor");
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
const char ZOOM_IN[]               = "QtCreator.ZoomIn";
const char ZOOM_OUT[]              = "QtCreator.ZoomOut";
const char ZOOM_RESET[]            = "QtCreator.ZoomReset";

const char NEW[]                   = "QtCreator.New";
const char NEW_FILE[]              = "QtCreator.NewFile";
const char OPEN[]                  = "QtCreator.Open";
const char OPEN_WITH[]             = "QtCreator.OpenWith";
const char OPEN_FROM_DEVICE[]      = "QtCreator.OpenFromDevice";
const char REVERTTOSAVED[]         = "QtCreator.RevertToSaved";
const char SAVE[]                  = "QtCreator.Save";
const char SAVEAS[]                = "QtCreator.SaveAs";
const char SAVEALL[]               = "QtCreator.SaveAll";
const char PRINT[]                 = "QtCreator.Print";
const char EXIT[]                  = "QtCreator.Exit";

const char OPTIONS[]               = "QtCreator.Options";
const char LOGGER[]                = "QtCreator.Logger";
const char TOGGLE_LEFT_SIDEBAR[]   = "QtCreator.ToggleLeftSidebar";
const char TOGGLE_RIGHT_SIDEBAR[]  = "QtCreator.ToggleRightSidebar";
const char TOGGLE_MENUBAR[]        = "QtCreator.ToggleMenubar";
const char CYCLE_MODE_SELECTOR_STYLE[] =
                                     "QtCreator.CycleModeSelectorStyle";
const char TOGGLE_FULLSCREEN[]     = "QtCreator.ToggleFullScreen";
const char THEMEOPTIONS[]          = "QtCreator.ThemeOptions";

const char TR_SHOW_LEFT_SIDEBAR[] = QT_TRANSLATE_NOOP("QtC::Core", "Show Left Sidebar");
const char TR_HIDE_LEFT_SIDEBAR[] = QT_TRANSLATE_NOOP("QtC::Core", "Hide Left Sidebar");

const char TR_SHOW_RIGHT_SIDEBAR[] = QT_TRANSLATE_NOOP("QtC::Core", "Show Right Sidebar");
const char TR_HIDE_RIGHT_SIDEBAR[] = QT_TRANSLATE_NOOP("QtC::Core", "Hide Right Sidebar");

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
const char GOTONEXTINHISTORY[]     = "QtCreator.GotoNextInHistory";
const char GOTOPREVINHISTORY[]     = "QtCreator.GotoPreviousInHistory";
const char GO_BACK[]               = "QtCreator.GoBack";
const char GO_FORWARD[]            = "QtCreator.GoForward";
const char OPEN_PREVIOUS_DOCUMENT[] = "QtCreator.OpenPreviousDocument";
const char OPEN_NEXT_DOCUMENT[] = "QtCreator.OpenNextDocument";
const char GOTOLASTEDIT[]          = "QtCreator.GotoLastEdit";
const char REOPEN_CLOSED_EDITOR[]  = "QtCreator.ReopenClosedEditor";
const char ABOUT_QTCREATOR[]       = "QtCreator.AboutQtCreator";
const char ABOUT_PLUGINS[]         = "QtCreator.AboutPlugins";
const char CHANGE_LOG[]            = "QtCreator.ChangeLog";
const char S_RETURNTOEDITOR[]      = "QtCreator.ReturnToEditor";
const char SHOWINGRAPHICALSHELL[]  = "QtCreator.ShowInGraphicalShell";
const char SHOWINFILESYSTEMVIEW[]  = "QtCreator.ShowInFileSystemView";

const char OUTPUTPANE_CLEAR[] = "Coreplugin.OutputPane.clear";

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
const char G_FILE_RECENT[]        = "QtCreator.Group.File.Recent";
const char G_FILE_SESSION[]        = "QtCreator.Group.File.Session";
const char G_FILE_PROJECT[]        = "QtCreator.Group.File.Project";
const char G_FILE_SAVE[]           = "QtCreator.Group.File.Save";
const char G_FILE_EXPORT[]         = "QtCreator.Group.File.Export";
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

// View menu groups
const char G_VIEW_SIDEBAR[]        = "QtCreator.Group.View.Sidebar";
const char G_VIEW_MODES[]          = "QtCreator.Group.View.Modes";
const char G_VIEW_VIEWS[]          = "QtCreator.Group.View.Views";
const char G_VIEW_PANES[]          = "QtCreator.Group.View.Panes";

// Tools menu groups
const char G_TOOLS_DEBUG[]         = "QtCreator.Group.Tools.Debug";
const char G_EDIT_PREFERENCES[]    = "QtCreator.Group.Edit.Preferences";

// Window menu groups
const char G_WINDOW_SIZE[]         = "QtCreator.Group.Window.Size";
const char G_WINDOW_SPLIT[]        = "QtCreator.Group.Window.Split";
const char G_WINDOW_NAVIGATE[]     = "QtCreator.Group.Window.Navigate";
const char G_WINDOW_LIST[]         = "QtCreator.Group.Window.List";
const char G_WINDOW_OTHER[]        = "QtCreator.Group.Window.Other";

// Help groups (global)
const char G_HELP_HELP[]           = "QtCreator.Group.Help.Help";
const char G_HELP_SUPPORT[]        = "QtCreator.Group.Help.Supprt";
const char G_HELP_ABOUT[]          = "QtCreator.Group.Help.About";
const char G_HELP_UPDATES[]        = "QtCreator.Group.Help.Updates";

// Touchbar groups
const char G_TOUCHBAR_HELP[]       = "QtCreator.Group.TouchBar.Help";
const char G_TOUCHBAR_EDITOR[]     = "QtCreator.Group.TouchBar.Editor";
const char G_TOUCHBAR_NAVIGATION[] = "QtCreator.Group.TouchBar.Navigation";
const char G_TOUCHBAR_OTHER[]      = "QtCreator.Group.TouchBar.Other";

const char WIZARD_CATEGORY_QT[] = "R.Qt";
const char WIZARD_TR_CATEGORY_QT[] = QT_TRANSLATE_NOOP("QtC::Core", "Qt");
const char WIZARD_KIND_UNKNOWN[] = "unknown";
const char WIZARD_KIND_PROJECT[] = "project";
const char WIZARD_KIND_FILE[] = "file";

const char SETTINGS_CATEGORY_CORE[] = "B.Core";
const char SETTINGS_CATEGORY_AI[] = "ZY.AI";
const char SETTINGS_ID_INTERFACE[] = "A.Interface";
const char SETTINGS_ID_SYSTEM[] = "B.Core.System";
const char SETTINGS_ID_SHORTCUTS[] = "C.Keyboard";
const char SETTINGS_ID_TOOLS[] = "D.ExternalTools";
const char SETTINGS_ID_MIMETYPES[] = "E.MimeTypes";

const char SETTINGS_DEFAULTTEXTENCODING[] = "General/DefaultFileEncoding";
const char SETTINGS_DEFAULT_LINE_TERMINATOR[] = "General/DefaultLineTerminator";

const char SETTINGS_THEME[] = "Core/CreatorTheme";

const char TR_CLEAR_MENU[]         = QT_TRANSLATE_NOOP("QtC::Core", "Clear Menu");

const int MODEBAR_ICON_SIZE = 34;
const int MODEBAR_ICONSONLY_BUTTON_SIZE = MODEBAR_ICON_SIZE + 4;
const int DEFAULT_MAX_CHAR_COUNT = 10000000;

const char SETTINGS_MENU_HIDE_TOOLS[] = "Menu/HideTools";

const char HELP_CATEGORY[] = "H.Help";

} // namespace Constants
} // namespace Core
