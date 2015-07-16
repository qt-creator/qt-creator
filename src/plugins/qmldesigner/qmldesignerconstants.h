/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef QMLDESIGNERCONSTANTS_H
#define QMLDESIGNERCONSTANTS_H

namespace QmlDesigner {
namespace Constants {

const char C_BACKSPACE[]            = "QmlDesigner.Backspace";
const char C_DELETE[]               = "QmlDesigner.Delete";

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
const char QML_ITEMSPACING_KEY[] = "ItemSpacing";
const char QML_CONTAINERPADDING_KEY[] = "ContainerPadding";
const char QML_CANVASWIDTH_KEY[] = "CanvasWidth";
const char QML_CANVASHEIGHT_KEY[] = "CanvasHeight";
const char QML_WARNIN_FOR_FEATURES_IN_DESIGNER_KEY[] = "WarnAboutQtQuickFeaturesInDesigner";
const char QML_WARNIN_FOR_DESIGNER_FEATURES_IN_EDITOR_KEY[] = "WarnAboutQtQuickDesignerFeaturesInCodeEditor";
const char QML_SHOW_DEBUGVIEW[] = "ShowQtQuickDesignerDebugView";
const char QML_ENABLE_DEBUGVIEW[] = "EnableQtQuickDesignerDebugView";
const char QML_ALWAYS_SAFE_IN_CRUMBLEBAR[] = "AlwaysSafeInCrumbleBar";
const char QML_USE_ONLY_FALLBACK_PUPPET[] = "UseOnlyFallbackPuppet";
const char QML_PUPPET_TOPLEVEL_BUILD_DIRECTORY[] = "PuppetToplevelBuildDirectory";
const char QML_PUPPET_FALLBACK_DIRECTORY[] = "PuppetFallbackDirectory";
const char QML_CONTROLS_STYLE[] = "ControlsStyle";

const char QML_DESIGNER_SUBFOLDER[] = "/designer/";

namespace Internal {
    enum { debug = 0 };
}

} // Constants
} // QmlDesigner

#endif //QMLDESIGNERCONSTANTS_H
