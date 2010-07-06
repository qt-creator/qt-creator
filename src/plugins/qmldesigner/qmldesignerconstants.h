/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef QMLDESIGNERPLUGIN_CONSTANTS_H
#define QMLDESIGNERPLUGIN_CONSTANTS_H

namespace QmlDesigner {
namespace Constants {

const char * const DELETE               = "QmlDesigner.Delete";

// context
const char * const C_DESIGN_MODE        = "QmlDesigner::DesignMode";
const char * const C_FORMEDITOR         = "QmlDesigner::QmlFormEditor";

// special context for preview menu, shared b/w designer and text editor
const char * const C_QT_QUICK_TOOLS_MENU = "QmlDesigner::ToolsMenu";

// actions
const char * const SWITCH_TEXT_DESIGN   = "QmlDesigner.SwitchTextDesign";
const char * const RESTORE_DEFAULT_VIEW = "QmlDesigner.RestoreDefaultView";
const char * const TOGGLE_LEFT_SIDEBAR = "QmlDesigner.ToggleLeftSideBar";
const char * const TOGGLE_RIGHT_SIDEBAR = "QmlDesigner.ToggleRightSideBar";

// mode
const char * const DESIGN_MODE_NAME     = "Design";

// Wizard type
const char * const FORM_MIMETYPE        = "application/x-qmldesigner";


// This setting is also accessed by the QMlJsEditor.
const char * const QML_SETTINGS_GROUP = "QML";
const char * const QML_DESIGNER_SETTINGS_GROUP = "Designer";
const char * const QML_OPENDESIGNMODE_SETTINGS_KEY = "OpenDesignMode";
const char * const QML_ITEMSPACING_KEY = "ItemSpacing";
const char * const QML_SNAPMARGIN_KEY = "SnapMargin";
const char * const QML_CONTEXTPANE_KEY = "ContextPaneEnabled";
enum { QML_OPENDESIGNMODE_DEFAULT = 0 }; // 0 for text mode, 1 for design mode

const char * const SETTINGS_CATEGORY_QML_ICON = ":/core/images/category_qml.png";

namespace Internal {
    enum { debug = 0 };
}

} // Constants
} // QmlDesigner

#endif //QMLDESIGNERPLUGIN_CONSTANTS_H
