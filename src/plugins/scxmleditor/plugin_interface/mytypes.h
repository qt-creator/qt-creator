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
