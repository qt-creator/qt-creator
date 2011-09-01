/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#ifndef QT4PROJECTMANAGERCONSTANTS_H
#define QT4PROJECTMANAGERCONSTANTS_H

#include <QtCore/QtGlobal>

namespace Qt4ProjectManager {
namespace Constants {

//contexts
const char * const C_PROFILEEDITOR  = ".pro File Editor";

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
const char * const RUNQMAKE            = "Qt4Builder.RunQMake";
const char * const RUNQMAKECONTEXTMENU = "Qt4Builder.RunQMakeContextMenu";
const char * const BUILDSUBDIR         = "Qt4Builder.BuildSubDir";
const char * const REBUILDSUBDIR       = "Qt4Builder.RebuildSubDir";
const char * const CLEANSUBDIR         = "Qt4Builder.CleanSubDir";
const char * const ADDLIBRARY          = "Qt4.AddLibrary";
const char * const JUMP_TO_FILE        = "Qt4.JumpToFile";
const char * const SEPARATOR           = "Qt4.Separator";

// C++ wizard categories
const char * const QT_APP_WIZARD_CATEGORY = "F.QtApplicationProjects";
const char * const QT_APP_WIZARD_TR_SCOPE = "Qt4ProjectManager";
const char * const QT_APP_WIZARD_TR_CATEGORY = QT_TRANSLATE_NOOP("Qt4ProjectManager", "Qt Widget Project");

// Html5 wizard categories
const char * const HTML5_WIZARD_CATEGORY = "I.Projects"; // (after Qt)
const char * const HTML5_WIZARD_TR_SCOPE = QT_APP_WIZARD_TR_SCOPE;
const char * const HTML5_WIZARD_TR_CATEGORY = QT_TRANSLATE_NOOP("ProjectExplorer", "Other Project");
const char * const HTML5_WIZARD_ICON = ":/qmlproject/images/qml_wizard.png";

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
const char * const MEEGO_DEVICE_TARGET_ID = "Qt4ProjectManager.Target.MeegoDeviceTarget";
const char * const QT_SIMULATOR_TARGET_ID = "Qt4ProjectManager.Target.QtSimulatorTarget";

// Target Features
const char * const MOBILE_TARGETFEATURE_ID = "Qt4ProjectManager.TargetFeature.Mobile";
const char * const DESKTOP_TARGETFEATURE_ID = "Qt4ProjectManager.TargetFeature.Desktop";
const char * const SHADOWBUILD_TARGETFEATURE_ID = "Qt4ProjectManager.TargetFeature.ShadowBuild";
const char * const QTQUICKCOMPONENTS_SYMBIAN_TARGETFEATURE_ID
                               = "Qt4ProjectManager.TargetFeature.QtQuickComponentsSymbian";
const char * const QTQUICKCOMPONENTS_MEEGO_TARGETFEATURE_ID
                               = "Qt4ProjectManager.TargetFeature.QtQuickComponentsMeego";


// Tool chains:
const char * const GCCE_TOOLCHAIN_ID = "Qt4ProjectManager.ToolChain.GCCE";
const char * const MAEMO_TOOLCHAIN_ID = "Qt4ProjectManager.ToolChain.Maemo";
const char * const RVCT_TOOLCHAIN_ID = "Qt4ProjectManager.ToolChain.RVCT";
const char * const WINSCW_TOOLCHAIN_ID = "Qt4ProjectManager.ToolChain.WINSCW";

// ICONS
const char * const ICON_QT_PROJECT = ":/qt4projectmanager/images/qt_project.png";
const char * const ICON_QTQUICK_APP = ":/wizards/images/qtquickapp.png";
const char * const ICON_HTML5_APP = ":/wizards/images/html5app.png";

// Env variables
const char * const QMAKEVAR_QMLJSDEBUGGER_PATH = "QMLJSDEBUGGER_PATH";
const char * const QMAKEVAR_DECLARATIVE_DEBUG = "CONFIG+=declarative_debug";

} // namespace Constants
} // namespace Qt4ProjectManager

#endif // QT4PROJECTMANAGERCONSTANTS_H

