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

#include <QtGlobal>

namespace QmlDesigner {

namespace ComponentCoreConstants {

const char rootCategory[] = "";

const char selectionCategory[] = "Selection";
const char stackCategory[] = "Stack (z)";
const char editCategory[] = "Edit";
const char anchorsCategory[] = "Anchors";
const char positionCategory[] = "Position";
const char layoutCategory[] = "Layout";
const char stackedContainerCategory[] = "StackedContainer";
const char genericToolBarCategory[] = "GenericToolBar";

const char toFrontCommandId[] = "ToFront";
const char toBackCommandId[] = "ToBack";
const char raiseCommandId[] = "Raise";
const char lowerCommandId[] = "Lower";
const char resetZCommandId[] = "ResetZ";
const char resetSizeCommandId[] = "ResetSize";
const char resetPositionCommandId[] = "ResetPosition";
const char visiblityCommandId[] = "ToggleVisiblity";
const char anchorsFillCommandId[] = "AnchorsFill";
const char anchorsResetCommandId[] = "AnchorsReset";
const char removePositionerCommandId[] = "RemovePositioner";
const char layoutRowPositionerCommandId[] = "LayoutRowPositioner";
const char layoutColumnPositionerCommandId[] = "LayoutColumnPositioner";
const char layoutGridPositionerCommandId[] = "LayoutGridPositioner";
const char layoutFlowPositionerCommandId[] = "LayoutFlowPositioner";
const char removeLayoutCommandId[] = "RemoveLayout";
const char layoutRowLayoutCommandId[] = "LayoutRowLayout";
const char layoutColumnLayoutCommandId[] = "LayoutColumnLayout";
const char layoutGridLayoutCommandId[] = "LayoutGridLayout";
const char layoutFillWidthCommandId[] = "LayoutFillWidth";
const char layoutFillHeightCommandId[] = "LayoutFillHeight";
const char goIntoComponentCommandId[] = "GoIntoComponent";
const char goToImplementationCommandId[] = "GoToImplementation";
const char addSignalHandlerCommandId[] = "AddSignalHandler";
const char moveToComponentCommandId[] = "MoveToComponent";
const char addItemToStackedContainerCommandId[] = "AddItemToStackedContainer";
const char addTabBarToStackedContainerCommandId[] = "AddTabBarToStackedContainer";
const char increaseIndexOfStackedContainerCommandId[] = "IncreaseIndexOfStackedContainer";
const char decreaseIndexOfStackedContainerCommandId[] = "decreaseIndexOfStackedContainer";

const char selectionCategoryDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Selection");
const char stackCategoryDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Stacked Container");
const char editCategoryDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Edit");
const char anchorsCategoryDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Anchors");
const char positionCategoryDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Position");
const char layoutCategoryDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Layout");
const char stackedContainerCategoryDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Stacked Container");

const char selectParentDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Select Parent: %1");
const char selectDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Select: %1");
const char deSelectDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Deselect: ");

const char cutSelectionDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Cut");
const char copySelectionDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Copy");
const char pasteSelectionDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Paste");
const char deleteSelectionDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Delete Selection");

const char toFrontDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "To Front");
const char toBackDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "To Back");

const char raiseDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Raise");
const char lowerDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Lower");

const char undoDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Undo");
const char redoDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Redo");

const char visibilityDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Visibility");

const char resetSizeDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Reset Size");
const char resetPositionDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Reset Position");

const char goIntoComponentDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Go into Component");
const char goToImplementationDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Go to Implementation");
const char addSignalHandlerDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Add New Signal Handler");
const char moveToComponentDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Move Component into Separate File");

const char setIdDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Set Id");

const char resetZDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Reset z Property");

const char anchorsFillDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Fill");
const char anchorsResetDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Reset");

const char layoutColumnPositionerDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Position in Column");
const char layoutRowPositionerDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Position in Row");
const char layoutGridPositionerDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Position in Grid");
const char layoutFlowPositionerDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Position in Flow");
const char removePositionerDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Remove Positioner");
const char removeLayoutDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Remove Layout");

const char addItemToStackedContainerDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Add Item");
const char addTabBarToStackedContainerDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Add Tab Bar");
const char increaseIndexToStackedContainerDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Increase Index");
const char decreaseIndexToStackedContainerDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Decrease Index");

const char layoutColumnLayoutDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Layout in Column Layout");
const char layoutRowLayoutDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Layout in Row Layout");
const char layoutGridLayoutDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Layout in Grid Layout");

const char layoutFillWidthDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Fill Width");
const char layoutFillHeightDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Fill Height");

const char raiseToolTip[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Raise selected item.");
const char lowerToolTip[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Lower selected item.");

const char resetSizeToolTip[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Reset size and use implicit size.");
const char resetPositionTooltip[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Reset position and use implicit position.");

const char anchorsFillToolTip[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Fill selected item to parent. ");
const char anchorsResetToolTip[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Reset anchors for selected item.");

const char layoutColumnLayoutToolTip[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Layout selected items in column layout.");
const char layoutRowLayoutToolTip[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Layout selected items in row layout.");
const char layoutGridLayoutToolTip[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Layout selected items in grid layout.");

const char increaseIndexOfStackedContainerToolTip[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Increase index of stacked container.");
const char decreaseIndexOfStackedContainerToolTip[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Decrease index of stacked container.");
const char addItemToStackedContainerToolTip[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Add item to stacked container.");

const int priorityFirst = 220;
const int prioritySelectionCategory = 200;
const int priorityStackCategory = 180;
const int priorityEditCategory = 160;
const int priorityAnchorsCategory = 140;
const int priorityPositionCategory = 130;
const int priorityLayoutCategory = 120;
const int priorityStackedContainerCategory = priorityLayoutCategory;
const int priorityTopLevelSeperator = 100;
const int priorityCustomActions = 80;
const int priorityRefactoring = 60;
const int priorityGoIntoComponent = 40;
const int priorityGenericToolBar = 50;
const int priorityLast = 60;

} //ComponentCoreConstants

} //QmlDesigner
