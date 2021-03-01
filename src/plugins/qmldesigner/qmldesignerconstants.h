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

namespace QmlDesigner {
namespace Constants {

const char C_BACKSPACE[] = "QmlDesigner.Backspace";
const char C_DELETE[]    = "QmlDesigner.Delete";

// Context
const char C_QMLDESIGNER[]   = "QmlDesigner::QmlDesignerMain";
const char C_QMLFORMEDITOR[] = "QmlDesigner::FormEditor";
const char C_QMLEDITOR3D[]   = "QmlDesigner::Editor3D";
const char C_QMLNAVIGATOR[]  = "QmlDesigner::Navigator";
const char C_QMLTEXTEDITOR[] = "QmlDesigner::TextEditor";

// Special context for preview menu, shared b/w designer and text editor
const char C_QT_QUICK_TOOLS_MENU[] = "QmlDesigner::ToolsMenu";

// Actions
const char SWITCH_TEXT_DESIGN[]   = "QmlDesigner.SwitchTextDesign";
const char RESTORE_DEFAULT_VIEW[] = "QmlDesigner.RestoreDefaultView";
const char TOGGLE_LEFT_SIDEBAR[] = "QmlDesigner.ToggleLeftSideBar";
const char TOGGLE_RIGHT_SIDEBAR[] = "QmlDesigner.ToggleRightSideBar";
const char TOGGLE_STATES_EDITOR[] = "QmlDesigner.ToggleStatesEditor";
const char GO_INTO_COMPONENT[] = "QmlDesigner.GoIntoComponent";
const char EXPORT_AS_IMAGE[] = "QmlDesigner.ExportAsImage";
const char FORMEDITOR_REFRESH[] = "QmlDesigner.FormEditor.Refresh";
const char FORMEDITOR_SNAPPING[] = "QmlDesigner.FormEditor.Snapping";
const char FORMEDITOR_NO_SNAPPING[] = "QmlDesigner.FormEditor.NoSnapping";
const char FORMEDITOR_NO_SNAPPING_AND_ANCHORING[] = "QmlDesigner.FormEditor.NoSnappingAndAnchoring";
const char FORMEDITOR_NO_SHOW_BOUNDING_RECTANGLE[] = "QmlDesigner.FormEditor.ShowBoundingRectangle";
const char EDIT3D_SELECTION_MODE[] = "QmlDesigner.Editor3D.SelectionModeToggle";
const char EDIT3D_MOVE_TOOL[]      = "QmlDesigner.Editor3D.MoveTool";
const char EDIT3D_ROTATE_TOOL[]    = "QmlDesigner.Editor3D.RotateTool";
const char EDIT3D_SCALE_TOOL[]     = "QmlDesigner.Editor3D.ScaleTool";
const char EDIT3D_FIT_SELECTED[]   = "QmlDesigner.Editor3D.FitSelected";
const char EDIT3D_EDIT_CAMERA[]    = "QmlDesigner.Editor3D.EditCameraToggle";
const char EDIT3D_ORIENTATION[]    = "QmlDesigner.Editor3D.OrientationToggle";
const char EDIT3D_EDIT_LIGHT[]     = "QmlDesigner.Editor3D.EditLightToggle";
const char EDIT3D_EDIT_SHOW_GRID[] = "QmlDesigner.Editor3D.ToggleGrid";
const char EDIT3D_RESET_VIEW[]     = "QmlDesigner.Editor3D.ResetView";

const char QML_DESIGNER_SUBFOLDER[] = "/designer/";
const char QUICK_3D_ASSETS_FOLDER[] = "/Quick3DAssets";
const char QUICK_3D_ASSET_LIBRARY_ICON_SUFFIX[] = "_libicon";
const char QUICK_3D_ASSET_ICON_DIR[] = "_icons";
const char QUICK_3D_ASSET_IMPORT_DATA_NAME[] = "_importdata.json";
const char QUICK_3D_ASSET_IMPORT_DATA_OPTIONS_KEY[] = "import_options";
const char QUICK_3D_ASSET_IMPORT_DATA_SOURCE_KEY[] = "source_scene";
const char DEFAULT_ASSET_IMPORT_FOLDER[] = "/asset_imports";

// Menus
const char M_VIEW_WORKSPACES[] = "QmlDesigner.Menu.View.Workspaces";

const int MODELNODE_PREVIEW_IMAGE_DIMENSIONS = 150;

const char EVENT_TIMELINE_ADDED[] = "Timeline Added";
const char EVENT_TRANSITION_ADDED[] = "Transition Added";
const char EVENT_STATE_ADDED[] = "State Added";
const char EVENT_CONNECTION_ADDED[] = "Connection Added";
const char EVENT_PROPERTY_ADDED[] = "Property Added";
const char EVENT_ANNOTATION_ADDED[] = "Annotation Added";
const char EVENT_RESOURCE_IMPORTED[] = "Resource Imported ";
const char EVENT_ACTION_EXECUTED[] = "Action Executed ";
const char EVENT_IMPORT_ADDED[] = "Import Added ";
const char EVENT_BINDINGEDITOR_OPENED[] = "Binding Editor Opened";
const char EVENT_RICHTEXT_OPENED[] = "Richtext Editor Opened";
const char EVENT_FORMEDITOR_TIME[] = "Form Editor";
const char EVENT_3DEDITOR_TIME[] = "3D Editor";
const char EVENT_TIMELINE_TIME[] = "Timeline";
const char EVENT_TRANSITIONEDITOR_TIME[] = "Transition Editor";
const char EVENT_CURVEDITOR_TIME[] = "Curve Editor";

namespace Internal {
    enum { debug = 0 };
}

} // Constants
} // QmlDesigner
