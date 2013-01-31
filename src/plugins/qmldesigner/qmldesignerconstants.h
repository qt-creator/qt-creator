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

#ifndef QMLDESIGNERPLUGIN_CONSTANTS_H
#define QMLDESIGNERPLUGIN_CONSTANTS_H

namespace QmlDesigner {
namespace Constants {

const char DELETE[]               = "QmlDesigner.Delete";

// Context
const char C_QMLDESIGNER[]         = "QmlDesigner::QmlDesignerMain";
const char C_QMLFORMEDITOR[]       = "QmlDesigner::FormEditor";
const char C_QMLNAVIGATOR[]        = "QmlDesigner::Navigator";

// Special context for preview menu, shared b/w designer and text editor
const char C_QT_QUICK_TOOLS_MENU[] = "QmlDesigner::ToolsMenu";

// Actions
const char SWITCH_TEXT_DESIGN[]   = "QmlDesigner.SwitchTextDesign";
const char RESTORE_DEFAULT_VIEW[] = "QmlDesigner.RestoreDefaultView";
const char TOGGLE_LEFT_SIDEBAR[] = "QmlDesigner.ToggleLeftSideBar";
const char TOGGLE_RIGHT_SIDEBAR[] = "QmlDesigner.ToggleRightSideBar";
const char GO_INTO_COMPONENT[] = "QmlDesigner.GoIntoComponent";

// This setting is also accessed by the QMlJsEditor.
const char QML_SETTINGS_GROUP[] = "QML";
const char QML_DESIGNER_SETTINGS_GROUP[] = "Designer";
const char QML_OPENDESIGNMODE_SETTINGS_KEY[] = "OpenDesignMode";
const char QML_ITEMSPACING_KEY[] = "ItemSpacing";
const char QML_SNAPMARGIN_KEY[] = "SnapMargin";
const char QML_CANVASWIDTH_KEY[] = "CanvasWidth";
const char QML_CANVASHEIGHT_KEY[] = "CanvasHeight";
const char QML_CONTEXTPANE_KEY[] = "ContextPaneEnabled";
const char QML_CONTEXTPANEPIN_KEY[] = "ContextPanePinned";
const char QML_WARNIN_FOR_FEATURES_IN_DESIGNER_KEY[] = "WarnAboutQtQuickFeaturesInDesigner";
const char QML_WARNIN_FOR_DESIGNER_FEATURES_IN_EDITOR_KEY[] = "WarnAboutQtQuickDesignerFeaturesInCodeEditor";
enum { QML_OPENDESIGNMODE_DEFAULT = 0 }; // 0 for text mode, 1 for design mode

const char SETTINGS_CATEGORY_QML_ICON[] = ":/core/images/category_qml.png";

const char QML_DESIGNER_SUBFOLDER[] = "/designer/";

namespace Internal {
    enum { debug = 0 };
}

} // Constants
} // QmlDesigner

#endif //QMLDESIGNERPLUGIN_CONSTANTS_H
