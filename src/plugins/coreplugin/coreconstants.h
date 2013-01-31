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

#ifndef CORECONSTANTS_H
#define CORECONSTANTS_H

#include <QtGlobal>

namespace Core {
namespace Constants {

// Modes
const char MODE_WELCOME[]          = "Welcome";
const char MODE_WELCOME_TYPE[]     = "Type.Welcome";
const char MODE_EDIT[]             = "Edit";
const char MODE_EDIT_TYPE[]        = "Type.Edit";
const char MODE_DESIGN[]           = "Design";
const char MODE_DESIGN_TYPE[]      = "Type.Design";
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
const char TOGGLE_SIDEBAR[]        = "QtCreator.ToggleSidebar";
const char TOGGLE_FULLSCREEN[]     = "QtCreator.ToggleFullScreen";

const char MINIMIZE_WINDOW[]       = "QtCreator.MinimizeWindow";
const char ZOOM_WINDOW[]           = "QtCreator.ZoomWindow";

const char SPLIT[]                 = "QtCreator.Split";
const char SPLIT_SIDE_BY_SIDE[]    = "QtCreator.SplitSideBySide";
const char REMOVE_CURRENT_SPLIT[]  = "QtCreator.RemoveCurrentSplit";
const char REMOVE_ALL_SPLITS[]     = "QtCreator.RemoveAllSplits";
const char GOTO_OTHER_SPLIT[]      = "QtCreator.GotoOtherSplit";
const char CLOSE[]                 = "QtCreator.Close";
#ifdef Q_OS_WIN
const char CLOSE_ALTERNATIVE[]     = "QtCreator.Close_Alternative"; // temporary, see QTCREATORBUG-72
#endif
const char CLOSEALL[]              = "QtCreator.CloseAll";
const char CLOSEOTHERS[]           = "QtCreator.CloseOthers";
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
const char G_WINDOW_OTHER[]        = "QtCreator.Group.Window.Other";

// Help groups (global)
const char G_HELP_HELP[]           = "QtCreator.Group.Help.Help";
const char G_HELP_ABOUT[]          = "QtCreator.Group.Help.About";

const char ICON_MINUS[]              = ":/core/images/minus.png";
const char ICON_PLUS[]               = ":/core/images/plus.png";
const char ICON_NEWFILE[]            = ":/core/images/filenew.png";
const char ICON_OPENFILE[]           = ":/core/images/fileopen.png";
const char ICON_SAVEFILE[]           = ":/core/images/filesave.png";
const char ICON_UNDO[]               = ":/core/images/undo.png";
const char ICON_REDO[]               = ":/core/images/redo.png";
const char ICON_COPY[]               = ":/core/images/editcopy.png";
const char ICON_PASTE[]              = ":/core/images/editpaste.png";
const char ICON_CUT[]                = ":/core/images/editcut.png";
const char ICON_NEXT[]               = ":/core/images/next.png";
const char ICON_PREV[]               = ":/core/images/prev.png";
const char ICON_DIR[]                = ":/core/images/dir.png";
const char ICON_CLEAN_PANE[]         = ":/core/images/clean_pane_small.png";
const char ICON_CLEAR[]              = ":/core/images/clear.png";
const char ICON_RESET[]              = ":/core/images/reset.png";
const char ICON_MAGNIFIER[]          = ":/core/images/magnifier.png";
const char ICON_TOGGLE_SIDEBAR[]     = ":/core/images/sidebaricon.png";
const char ICON_CLOSE_DOCUMENT[]     = ":/core/images/button_close.png";
const char ICON_CLOSE[]              = ":/core/images/closebutton.png";
const char ICON_CLOSE_DARK[]         = ":/core/images/darkclosebutton.png";
const char ICON_SPLIT_HORIZONTAL[]   = ":/core/images/splitbutton_horizontal.png";
const char ICON_SPLIT_VERTICAL[]     = ":/core/images/splitbutton_vertical.png";
const char ICON_CLOSE_SPLIT_TOP[]    = ":/core/images/splitbutton_closetop.png";
const char ICON_CLOSE_SPLIT_BOTTOM[] = ":/core/images/splitbutton_closebottom.png";
const char ICON_CLOSE_SPLIT_LEFT[]   = ":/core/images/splitbutton_closeleft.png";
const char ICON_CLOSE_SPLIT_RIGHT[]  = ":/core/images/splitbutton_closeright.png";
const char ICON_FILTER[]             = ":/core/images/filtericon.png";
const char ICON_LINK[]               = ":/core/images/linkicon.png";
const char ICON_QTLOGO_32[]          = ":/core/images/logo/32/QtProject-qtcreator.png";
const char ICON_QTLOGO_128[]         = ":/core/images/logo/128/QtProject-qtcreator.png";

const char WIZARD_CATEGORY_QT[] = "R.Qt";
const char WIZARD_TR_CATEGORY_QT[] = QT_TRANSLATE_NOOP("Core", "Qt");

const char SETTINGS_CATEGORY_CORE[] = "A.Core";
const char SETTINGS_CATEGORY_CORE_ICON[] = ":/core/images/category_core.png";
const char SETTINGS_TR_CATEGORY_CORE[] = QT_TRANSLATE_NOOP("Core", "Environment");
const char SETTINGS_ID_ENVIRONMENT[] = "A.General";
const char SETTINGS_ID_SHORTCUTS[] = "B.Keyboard";
const char SETTINGS_ID_TOOLS[] = "C.ExternalTools";
const char SETTINGS_ID_MIMETYPES[] = "D.MimeTypes";

const char SETTINGS_DEFAULTTEXTENCODING[] = "General/DefaultFileEncoding";

const char ALL_FILES_FILTER[]      = QT_TRANSLATE_NOOP("Core", "All Files (*)");

const char VARIABLE_SUPPORT_PROPERTY[] = "QtCreator.VariableSupport";

const char TR_CLEAR_MENU[]         = QT_TRANSLATE_NOOP("Core", "Clear Menu");

const char DEFAULT_BUILD_DIRECTORY[] = "../build-%{CurrentProject:Name}-%{CurrentKit:FileSystemName}-%{CurrentBuild:Name}";

const int TARGET_ICON_SIZE = 32;

} // namespace Constants
} // namespace Core

#endif // CORECONSTANTS_H
