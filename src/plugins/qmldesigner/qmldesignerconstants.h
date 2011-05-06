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

#ifndef QMLDESIGNERPLUGIN_CONSTANTS_H
#define QMLDESIGNERPLUGIN_CONSTANTS_H

namespace QmlDesigner {
namespace Constants {

const char * const DELETE               = "QmlDesigner.Delete";

// context
const char * const C_QMLDESIGNER         = "QmlDesigner::QmlDesignerMain";
const char * const C_QMLFORMEDITOR       = "QmlDesigner::FormEditor";
const char * const C_QMLNAVIGATOR        = "QmlDesigner::Navigator";

// special context for preview menu, shared b/w designer and text editor
const char * const C_QT_QUICK_TOOLS_MENU = "QmlDesigner::ToolsMenu";

// actions
const char * const SWITCH_TEXT_DESIGN   = "QmlDesigner.SwitchTextDesign";
const char * const RESTORE_DEFAULT_VIEW = "QmlDesigner.RestoreDefaultView";
const char * const TOGGLE_LEFT_SIDEBAR = "QmlDesigner.ToggleLeftSideBar";
const char * const TOGGLE_RIGHT_SIDEBAR = "QmlDesigner.ToggleRightSideBar";

// This setting is also accessed by the QMlJsEditor.
const char * const QML_SETTINGS_GROUP = "QML";
const char * const QML_DESIGNER_SETTINGS_GROUP = "Designer";
const char * const QML_OPENDESIGNMODE_SETTINGS_KEY = "OpenDesignMode";
const char * const QML_ITEMSPACING_KEY = "ItemSpacing";
const char * const QML_SNAPMARGIN_KEY = "SnapMargin";
const char * const QML_CANVASWIDTH_KEY = "CanvasWidth";
const char * const QML_CANVASHEIGHT_KEY = "CanvasHeight";
const char * const QML_CONTEXTPANE_KEY = "ContextPaneEnabled";
const char * const QML_CONTEXTPANEPIN_KEY = "ContextPanePinned";
enum { QML_OPENDESIGNMODE_DEFAULT = 0 }; // 0 for text mode, 1 for design mode

const char * const SETTINGS_CATEGORY_QML_ICON = ":/core/images/category_qml.png";

namespace Internal {
    enum { debug = 0 };
}

} // Constants
} // QmlDesigner

#endif //QMLDESIGNERPLUGIN_CONSTANTS_H
