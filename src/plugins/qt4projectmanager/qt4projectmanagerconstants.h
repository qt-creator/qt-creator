/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef QT4PROJECTMANAGERCONSTANTS_H
#define QT4PROJECTMANAGERCONSTANTS_H

#include <QtCore/QtGlobal>

namespace Qt4ProjectManager {
namespace Constants {

//contexts
const char * const C_PROFILEEDITOR  = ".pro File Editor";
const char * const C_PROFILEEDITOR_PANEL  = ".pro File Editor (embedded)";

// menus
const char * const M_CONTEXT = "ProFileEditor.ContextMenu";

// kinds
const char * const PROJECT_ID      = "Qt4.Qt4Project";
const char * const PROFILE_EDITOR_ID = "Qt4.proFileEditor";
const char * const PROFILE_EDITOR_DISPLAY_NAME = QT_TRANSLATE_NOOP("OpenWith::Editors", ".pro File Editor");
const char * const PROFILE_MIMETYPE  = "application/vnd.nokia.qt.qmakeprofile";
const char * const PROINCLUDEFILE_MIMETYPE  = "application/vnd.nokia.qt.qmakeproincludefile";
const char * const PROFEATUREFILE_MIMETYPE  = "application/vnd.nokia.qt.qmakeprofeaturefile";
const char * const CPP_SOURCE_MIMETYPE = "text/x-c++src";
const char * const CPP_HEADER_MIMETYPE = "text/x-c++hdr";
const char * const FORM_MIMETYPE = "application/x-designer";
const char * const LINGUIST_MIMETYPE = "application/x-linguist";

//actions
const char * const NEWMENU             = "Qt4.NewMenu";
const char * const PROJECT_NEWMENU_SEPARATOR      = "Qt4.NewMenuSeparator";
const char * const SUBPROJECT_NEWMENU_SEPARATOR   = "Qt4.SubProjectNewMenuSeparator";
const char * const ADDTOPROJECT        = "Qt4.AddToProject";
const char * const RUNQMAKE            = "Qt4Builder.RunQMake";
const char * const RUNQMAKECONTEXTMENU = "Qt4Builder.RunQMakeContextMenu";
const char * const BUILDSUBDIR         = "Qt4Builder.BuildSubDir";
const char * const REBUILDSUBDIR       = "Qt4Builder.RebuildSubDir";
const char * const CLEANSUBDIR         = "Qt4Builder.CleanSubDir";
const char * const RUNSUBDIR           = "Qt4Builder.RunSubDir";
const char * const RUNMENUSUBDIR       = "Qt4Builder.RunMenuSubDir";
const char * const ADDLIBRARY          = "Qt4.AddLibrary";
const char * const JUMP_TO_FILE        = "Qt4.JumpToFile";
const char * const SEPARATOR           = "Qt4.Separator";

//configurations
const char * const CONFIG_DEBUG     = "debug";
const char * const CONFIG_RELEASE   = "release";

//global configurations
const char * const GC_BUILDCONFIG   = "Qt4.BuildConfig";
const char * const GC_QTVERSION     = "Qt4.QtVersion";
const char * const GC_COMPILER      = "Qt4.Compiler";

// qmakestep
const char * const QMAKESTEP  = "trolltech.qt4projectmanager.qmake";
const char * const MAKESTEP   = "trolltech.qt4projectmanager.make";
const char * const QT4RUNSTEP = "trolltech.qt4projectmanager.qt4runstep";
const char * const DEPLOYHELPERRUNSTEP = "trolltech.qt4projectmanager.deployhelperrunstep";

//Qt4 settings pages
const char * const QT_SETTINGS_CATEGORY       = "L.Qt4";
const char * const QT_SETTINGS_CATEGORY_ICON  = ":/core/images/category_qt.png";
const char * const QT_SETTINGS_TR_CATEGORY    = QT_TRANSLATE_NOOP("Qt4ProjectManager", "Qt4");
const char * const QTVERSION_SETTINGS_PAGE_ID = "Qt Versions";
const char * const QTVERSION_SETTINGS_PAGE_NAME = QT_TRANSLATE_NOOP("Qt4ProjectManager", "Qt Versions");

// C++ wizard categories
const char * const QT_APP_WIZARD_CATEGORY = "F.QtApplicationProjects";
const char * const QT_APP_WIZARD_TR_SCOPE = "Qt4ProjectManager";
const char * const QT_APP_WIZARD_TR_CATEGORY = QT_TRANSLATE_NOOP("Qt4ProjectManager", "Qt QWidget Project");

// QML wizard categories
const char * const QML_WIZARD_CATEGORY = "C.Projects"; // (before Qt)
const char * const QML_WIZARD_TR_SCOPE = "QmlProjectManager";
const char * const QML_WIZARD_TR_CATEGORY = QT_TRANSLATE_NOOP("QmlProjectManager", "Qt Quick Project");
const char * const QML_WIZARD_ICON = ":/qmlproject/images/qml_wizard.png";

// Tasks
const char * const PROFILE_EVALUATE = "Qt4ProjectManager.ProFileEvaluate";

// Project
const char * const QT4PROJECT_ID("Qt4ProjectManager.Qt4Project");

// Targets
const char * const DESKTOP_TARGET_ID      = "Qt4ProjectManager.Target.DesktopTarget";
const char * const S60_EMULATOR_TARGET_ID = "Qt4ProjectManager.Target.S60EmulatorTarget";
const char * const S60_DEVICE_TARGET_ID   = "Qt4ProjectManager.Target.S60DeviceTarget";
const char * const MAEMO5_DEVICE_TARGET_ID = "Qt4ProjectManager.Target.MaemoDeviceTarget";
const char * const HARMATTAN_DEVICE_TARGET_ID = "Qt4ProjectManager.Target.HarmattanDeviceTarget";
const char * const QT_SIMULATOR_TARGET_ID = "Qt4ProjectManager.Target.QtSimulatorTarget";

// ICONS
const char * const ICON_QT_PROJECT = ":/qt4projectmanager/images/qt_project.png";
const char * const ICON_WINDOW = ":/qt4projectmanager/images/window.png";
const char * const ICON_QML_STANDALONE = ":/wizards/images/qml_standalone.png";

// Env variables
const char * const QMAKEVAR_QMLJSDEBUGGER_PATH = "QMLJSDEBUGGER_PATH";

} // namespace Constants
} // namespace Qt4ProjectManager

#endif // QT4PROJECTMANAGERCONSTANTS_H

