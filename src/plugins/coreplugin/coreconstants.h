// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QtGlobal>

namespace Core::Constants {

// Modes
inline constexpr char MODE_WELCOME[]          = "Welcome";
inline constexpr char MODE_EDIT[]             = "Edit";
inline constexpr char MODE_DESIGN[]           = "Design";
inline constexpr char MODE_SETTINGS[]         = "Settings";
inline constexpr int  P_MODE_WELCOME          = 100;
inline constexpr int  P_MODE_EDIT             = 90;
inline constexpr int  P_MODE_DESIGN           = 89;
inline constexpr int  P_MODE_SETIINGS         = 20;

// TouchBar
inline constexpr char TOUCH_BAR[]             = "QtCreator.TouchBar";

// Menubar
inline constexpr char MENU_BAR[]              = "QtCreator.MenuBar";

// Menus
inline constexpr char M_FILE[]                = "QtCreator.Menu.File";
inline constexpr char M_FILE_RECENTFILES[]    = "QtCreator.Menu.File.RecentFiles";
inline constexpr char M_EDIT[]                = "QtCreator.Menu.Edit";
inline constexpr char M_EDIT_ADVANCED[]       = "QtCreator.Menu.Edit.Advanced";
inline constexpr char M_VIEW[]                = "QtCreator.Menu.View";
inline constexpr char M_VIEW_MODESTYLES[]     = "QtCreator.Menu.View.ModeStyles";
inline constexpr char M_VIEW_VIEWS[]          = "QtCreator.Menu.View.Views";
inline constexpr char M_VIEW_PANES[]          = "QtCreator.Menu.View.Panes";
inline constexpr char M_TOOLS[]               = "QtCreator.Menu.Tools";
inline constexpr char M_TOOLS_EXTERNAL[]      = "QtCreator.Menu.Tools.External";
inline constexpr char M_TOOLS_DEBUG[]         = "QtCreator.Menu.Tools.Debug";
inline constexpr char M_WINDOW[]              = "QtCreator.Menu.Window";
inline constexpr char M_HELP[]                = "QtCreator.Menu.Help";

// Contexts
inline constexpr char C_GLOBAL[]              = "Global Context";
inline constexpr char C_WELCOME_MODE[]        = "Core.WelcomeMode";
inline constexpr char C_EDIT_MODE[]           = "Core.EditMode";
inline constexpr char C_DESIGN_MODE[]         = "Core.DesignMode";
inline constexpr char C_DEBUG_MODE[]          = "Debugger.DebugMode";
inline constexpr char C_EDITORMANAGER[]       = "Core.EditorManager";
inline constexpr char C_NAVIGATION_PANE[]     = "Core.NavigationPane";
inline constexpr char C_SETTINGS_MODE[]       = "Core.SettingsMode";
inline constexpr char C_PROBLEM_PANE[]        = "Core.ProblemPane";
inline constexpr char C_GENERAL_OUTPUT_PANE[] = "Core.GeneralOutputPane";
// Special context that leads to all "more specific" contexts to be ignored.
// If you use Context(mycontextId, C_GLOBAL_CUTOFF) for a widget that has focus,
// mycontextId will be enabled but the contexts for all parent widgets, the manually added
// "additional" contexts, and the global context will be turned off.
inline constexpr char C_GLOBAL_CUTOFF[]       = "Global Cutoff";

// Default editor kind
inline constexpr char K_DEFAULT_TEXT_EDITOR_ID[] = "Core.PlainTextEditor";
inline constexpr char K_DEFAULT_BINARY_EDITOR_ID[] = "Core.BinaryEditor";

//actions
inline constexpr char UNDO[]                  = "QtCreator.Undo";
inline constexpr char REDO[]                  = "QtCreator.Redo";
inline constexpr char COPY[]                  = "QtCreator.Copy";
inline constexpr char PASTE[]                 = "QtCreator.Paste";
inline constexpr char CUT[]                   = "QtCreator.Cut";
inline constexpr char SELECTALL[]             = "QtCreator.SelectAll";

inline constexpr char GOTO[]                  = "QtCreator.Goto";
inline constexpr char ZOOM_IN[]               = "QtCreator.ZoomIn";
inline constexpr char ZOOM_OUT[]              = "QtCreator.ZoomOut";
inline constexpr char ZOOM_RESET[]            = "QtCreator.ZoomReset";

inline constexpr char NEW[]                   = "QtCreator.New";
inline constexpr char NEW_FILE[]              = "QtCreator.NewFile";
inline constexpr char OPEN[]                  = "QtCreator.Open";
inline constexpr char OPEN_PROJECT[]          = "ProjectExplorer.Load";
inline constexpr char OPEN_FILE[]             = "QtCreator.OpenFile";
inline constexpr char OPEN_WITH[]             = "QtCreator.OpenWith";
inline constexpr char OPEN_FROM_DEVICE[]      = "QtCreator.OpenFromDevice";
inline constexpr char REVERTTOSAVED[]         = "QtCreator.RevertToSaved";
inline constexpr char SAVE[]                  = "QtCreator.Save";
inline constexpr char SAVEAS[]                = "QtCreator.SaveAs";
inline constexpr char SAVE_WITHOUT_FORMATTING[] = "QtCreator.SaveWithoutFormatting";
inline constexpr char SAVEALL[]               = "QtCreator.SaveAll";
inline constexpr char PRINT[]                 = "QtCreator.Print";
inline constexpr char EXIT[]                  = "QtCreator.Exit";

inline constexpr char OPTIONS[]               = "QtCreator.Options";
inline constexpr char LOGGER[]                = "QtCreator.Logger";
inline constexpr char TOGGLE_LEFT_SIDEBAR[]   = "QtCreator.ToggleLeftSidebar";
inline constexpr char TOGGLE_RIGHT_SIDEBAR[]  = "QtCreator.ToggleRightSidebar";
inline constexpr char TOGGLE_MENUBAR[]        = "QtCreator.ToggleMenubar";
inline constexpr char CYCLE_MODE_SELECTOR_STYLE[] =
                                     "QtCreator.CycleModeSelectorStyle";
inline constexpr char TOGGLE_FULLSCREEN[]     = "QtCreator.ToggleFullScreen";
inline constexpr char THEMEOPTIONS[]          = "QtCreator.ThemeOptions";

inline constexpr char TR_CYCLE_NEXT_EDITOR[] = QT_TRANSLATE_NOOP("QtC::Core", "Cycle to Next Editor");

inline constexpr char MINIMIZE_WINDOW[]       = "QtCreator.MinimizeWindow";
inline constexpr char ZOOM_WINDOW[]           = "QtCreator.ZoomWindow";
inline constexpr char CLOSE_WINDOW[]           = "QtCreator.CloseWindow";

inline constexpr char SPLIT[]                 = "QtCreator.Split";
inline constexpr char SPLIT_SIDE_BY_SIDE[]    = "QtCreator.SplitSideBySide";
inline constexpr char SPLIT_NEW_WINDOW[]      = "QtCreator.SplitNewWindow";
inline constexpr char REMOVE_CURRENT_SPLIT[]  = "QtCreator.RemoveCurrentSplit";
inline constexpr char REMOVE_ALL_SPLITS[]     = "QtCreator.RemoveAllSplits";
inline constexpr char GOTO_PREV_SPLIT[]       = "QtCreator.GoToPreviousSplit";
inline constexpr char GOTO_NEXT_SPLIT[]       = "QtCreator.GoToNextSplit";
inline constexpr char CLOSE[]                 = "QtCreator.Close";
inline constexpr char CLOSE_ALTERNATIVE[]     = "QtCreator.Close_Alternative"; // temporary, see QTCREATORBUG-72
inline constexpr char CLOSEALL[]              = "QtCreator.CloseAll";
inline constexpr char CLOSEOTHERS[]           = "QtCreator.CloseOthers";
inline constexpr char CLOSEALLEXCEPTVISIBLE[] = "QtCreator.CloseAllExceptVisible";
inline constexpr char GOTONEXTINHISTORY[]     = "QtCreator.GotoNextInHistory";
inline constexpr char GOTOPREVINHISTORY[]     = "QtCreator.GotoPreviousInHistory";
inline constexpr char GOTONEXTTAB[]           = "QtCreator.GotoNextTab";
inline constexpr char GOTOPREVTAB[]           = "QtCreator.GotoPreviousTab";
inline constexpr char CLOSEALLTABS[]          = "QtCreator.CloseAllTabs";
inline constexpr char CLOSEOTHERTABS[]        = "QtCreator.CloseOtherTabs";
inline constexpr char GO_BACK[]               = "QtCreator.GoBack";
inline constexpr char GO_FORWARD[]            = "QtCreator.GoForward";
inline constexpr char OPEN_PREVIOUS_DOCUMENT[] = "QtCreator.OpenPreviousDocument";
inline constexpr char OPEN_NEXT_DOCUMENT[] = "QtCreator.OpenNextDocument";
inline constexpr char GOTOLASTEDIT[]          = "QtCreator.GotoLastEdit";
inline constexpr char REOPEN_CLOSED_EDITOR[]  = "QtCreator.ReopenClosedEditor";
inline constexpr char ABOUT_QTCREATOR[]       = "QtCreator.AboutQtCreator";
inline constexpr char ABOUT_PLUGINS[]         = "QtCreator.AboutPlugins";
inline constexpr char CHANGE_LOG[]            = "QtCreator.ChangeLog";
inline constexpr char S_RETURNTOEDITOR[]      = "QtCreator.ReturnToEditor";
inline constexpr char SHOWINGRAPHICALSHELL[]  = "QtCreator.ShowInGraphicalShell";
inline constexpr char SHOWINFILESYSTEMVIEW[]  = "QtCreator.ShowInFileSystemView";
inline constexpr char CYCLE_NEXT_EDITOR[]     = "QtCreator.CycleToNextEditor";

inline constexpr char OUTPUTPANE_CLEAR[] = "Coreplugin.OutputPane.clear";
inline constexpr char OUTPUTPANE_CLOSE[] = "Coreplugin.OutputPane.close";

// Default groups
inline constexpr char G_DEFAULT_ONE[]         = "QtCreator.Group.Default.One";
inline constexpr char G_DEFAULT_TWO[]         = "QtCreator.Group.Default.Two";
inline constexpr char G_DEFAULT_THREE[]       = "QtCreator.Group.Default.Three";

// Main menu bar groups
inline constexpr char G_FILE[]                = "QtCreator.Group.File";
inline constexpr char G_EDIT[]                = "QtCreator.Group.Edit";
inline constexpr char G_VIEW[]                = "QtCreator.Group.View";
inline constexpr char G_TOOLS[]               = "QtCreator.Group.Tools";
inline constexpr char G_WINDOW[]              = "QtCreator.Group.Window";
inline constexpr char G_HELP[]                = "QtCreator.Group.Help";

// File menu groups
inline constexpr char G_FILE_NEW[]            = "QtCreator.Group.File.New";
inline constexpr char G_FILE_OPEN[]           = "QtCreator.Group.File.Open";
inline constexpr char G_FILE_RECENT[]        = "QtCreator.Group.File.Recent";
inline constexpr char G_FILE_SESSION[]        = "QtCreator.Group.File.Session";
inline constexpr char G_FILE_PROJECT[]        = "QtCreator.Group.File.Project";
inline constexpr char G_FILE_SAVE[]           = "QtCreator.Group.File.Save";
inline constexpr char G_FILE_EXPORT[]         = "QtCreator.Group.File.Export";
inline constexpr char G_FILE_CLOSE[]          = "QtCreator.Group.File.Close";
inline constexpr char G_FILE_PRINT[]          = "QtCreator.Group.File.Print";
inline constexpr char G_FILE_OTHER[]          = "QtCreator.Group.File.Other";

// Edit menu groups
inline constexpr char G_EDIT_UNDOREDO[]       = "QtCreator.Group.Edit.UndoRedo";
inline constexpr char G_EDIT_COPYPASTE[]      = "QtCreator.Group.Edit.CopyPaste";
inline constexpr char G_EDIT_SELECTALL[]      = "QtCreator.Group.Edit.SelectAll";
inline constexpr char G_EDIT_ADVANCED[]       = "QtCreator.Group.Edit.Advanced";

inline constexpr char G_EDIT_FIND[]           = "QtCreator.Group.Edit.Find";
inline constexpr char G_EDIT_OTHER[]          = "QtCreator.Group.Edit.Other";

// Advanced edit menu groups
inline constexpr char G_EDIT_FORMAT[]         = "QtCreator.Group.Edit.Format";
inline constexpr char G_EDIT_COLLAPSING[]     = "QtCreator.Group.Edit.Collapsing";
inline constexpr char G_EDIT_TEXT[]           = "QtCreator.Group.Edit.Text";
inline constexpr char G_EDIT_BLOCKS[]         = "QtCreator.Group.Edit.Blocks";
inline constexpr char G_EDIT_FONT[]           = "QtCreator.Group.Edit.Font";
inline constexpr char G_EDIT_EDITOR[]         = "QtCreator.Group.Edit.Editor";

// View menu groups
inline constexpr char G_VIEW_SIDEBAR[]        = "QtCreator.Group.View.Sidebar";
inline constexpr char G_VIEW_MODES[]          = "QtCreator.Group.View.Modes";
inline constexpr char G_VIEW_VIEWS[]          = "QtCreator.Group.View.Views";
inline constexpr char G_VIEW_PANES[]          = "QtCreator.Group.View.Panes";

// Tools menu groups
inline constexpr char G_TOOLS_DEBUG[]         = "QtCreator.Group.Tools.Debug";
inline constexpr char G_EDIT_PREFERENCES[]    = "QtCreator.Group.Edit.Preferences";

// Window menu groups
inline constexpr char G_WINDOW_SIZE[]         = "QtCreator.Group.Window.Size";
inline constexpr char G_WINDOW_SPLIT[]        = "QtCreator.Group.Window.Split";
inline constexpr char G_WINDOW_NAVIGATE[]     = "QtCreator.Group.Window.Navigate";
inline constexpr char G_WINDOW_LIST[]         = "QtCreator.Group.Window.List";
inline constexpr char G_WINDOW_OTHER[]        = "QtCreator.Group.Window.Other";

// Help groups (global)
inline constexpr char G_HELP_HELP[]           = "QtCreator.Group.Help.Help";
inline constexpr char G_HELP_SUPPORT[]        = "QtCreator.Group.Help.Supprt";
inline constexpr char G_HELP_ABOUT[]          = "QtCreator.Group.Help.About";
inline constexpr char G_HELP_UPDATES[]        = "QtCreator.Group.Help.Updates";

// Touchbar groups
inline constexpr char G_TOUCHBAR_HELP[]       = "QtCreator.Group.TouchBar.Help";
inline constexpr char G_TOUCHBAR_EDITOR[]     = "QtCreator.Group.TouchBar.Editor";
inline constexpr char G_TOUCHBAR_NAVIGATION[] = "QtCreator.Group.TouchBar.Navigation";
inline constexpr char G_TOUCHBAR_OTHER[]      = "QtCreator.Group.TouchBar.Other";

inline constexpr char WIZARD_CATEGORY_QT[] = "R.Qt";
inline constexpr char WIZARD_KIND_UNKNOWN[] = "unknown";
inline constexpr char WIZARD_KIND_PROJECT[] = "project";
inline constexpr char WIZARD_KIND_FILE[] = "file";

inline constexpr char SETTINGS_CATEGORY_CORE[] = "B.Core";
inline constexpr char SETTINGS_CATEGORY_AI[] = "ZY.AI";
inline constexpr char SETTINGS_ID_INTERFACE[] = "A.Interface";
inline constexpr char SETTINGS_ID_SYSTEM[] = "B.Core.System";
inline constexpr char SETTINGS_ID_SHORTCUTS[] = "C.Keyboard";
inline constexpr char SETTINGS_ID_TOOLS[] = "D.ExternalTools";
inline constexpr char SETTINGS_ID_MIMETYPES[] = "E.MimeTypes";

inline constexpr char SETTINGS_DEFAULTTEXTENCODING[] = "General/DefaultFileEncoding";
inline constexpr char SETTINGS_DEFAULT_LINE_TERMINATOR[] = "General/DefaultLineTerminator";

inline constexpr char SETTINGS_THEME[] = "Core/CreatorTheme";

inline constexpr int MODEBAR_ICON_SIZE = 34;
inline constexpr int MODEBAR_ICONSONLY_BUTTON_SIZE = MODEBAR_ICON_SIZE + 4;
inline constexpr int DEFAULT_MAX_CHAR_COUNT = 10000000;

inline constexpr char SETTINGS_MENU_HIDE_TOOLS[] = "Menu/HideTools";

inline constexpr char HELP_CATEGORY[] = "H.Help";

inline constexpr char QT_JIRA_URL[] = "https://qt-project.atlassian.net";

inline constexpr char RELOAD_INFOBAR[]          = "QtCreator.ReloadInfoBar";

// Analyzer menu and groups
inline constexpr char M_DEBUG_ANALYZER[]        = "Analyzer.Menu.StartAnalyzer";
inline constexpr char G_ANALYZER_CONTROL[]      = "Menu.Group.Analyzer.Control";
inline constexpr char G_ANALYZER_TOOLS[]        = "Menu.Group.Analyzer.Tools";
inline constexpr char G_ANALYZER_REMOTE_TOOLS[] = "Menu.Group.Analyzer.RemoteTools";
inline constexpr char G_ANALYZER_OPTIONS[]      = "Menu.Group.Analyzer.Options";
inline constexpr char ANALYZERTASK_ID[]         = "Analyzer.TaskId";

} // namespace Core::Constants
