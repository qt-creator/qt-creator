// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QGraphicsItem>

namespace ScxmlEditor {

namespace PluginInterface {

/**
 * @brief The type of the graphics-items
 */
enum ItemType {
    UnknownType = QGraphicsItem::UserType + 1,

    // Warning items ->
    HighlightType,
    LayoutType,
    IdWarningType,
    StateWarningType,
    TransitionWarningType,
    InitialWarningType,

    // Helper items ->
    CornerGrabberType,
    QuickTransitionType,
    TextType,
    TagTextType,
    TagWrapperType,
    TransitionType,

    // Connectable-items ->
    InitialStateType,
    FinalStateType,
    HistoryType,
    StateType,
    ParallelType
};

enum ActionType {
    ActionZoomIn = 0,
    ActionZoomOut,
    ActionFitToView,
    ActionPan,
    ActionMagnifier,
    ActionNavigator,
    ActionCopy,
    ActionCut,
    ActionPaste,
    ActionScreenshot,
    ActionExportToImage,
    ActionFullNamespace,
    ActionAlignLeft,
    ActionAlignRight,
    ActionAlignTop,
    ActionAlignBottom,
    ActionAlignHorizontal,
    ActionAlignVertical,
    ActionAdjustWidth,
    ActionAdjustHeight,
    ActionAdjustSize,
    ActionStatistics,
    ActionLast,

    ActionColorTheme
};

enum ToolButtonType {
    ToolButtonStateColor,
    ToolButtonFontColor,
    ToolButtonAlignment,
    ToolButtonAdjustment,
    ToolButtonLast,

    ToolButtonColorTheme
};

enum AttributeRole {
    DataTypeRole = Qt::UserRole + 1,
    DataRole
};

enum DocumentChangeType {
    BeginSave = 0,
    AfterSave,
    AfterLoad,
    NewDocument
};

} // namespace PluginInterface
} // namespace ScxmlEditor
