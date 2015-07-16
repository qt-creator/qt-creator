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

#ifndef COMPONENTCORE_CONSTANTS_H
#define COMPONENTCORE_CONSTANTS_H

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

const char selectionCategoryDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Selection");
const char stackCategoryDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Stack (z)");
const char editCategoryDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Edit");
const char anchorsCategoryDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Anchors");
const char positionCategoryDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Position");
const char layoutCategoryDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Layout");

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
const char goToImplementation[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Go to Implementation");

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

const char layoutColumnLayoutDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Layout in ColumnLayout");
const char layoutRowLayoutDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Layout in RowLayout");
const char layoutGridLayoutDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Layout in GridLayout");

const char layoutFillWidthDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Fill Width");
const char layoutFillHeightDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Fill Height");


const int priorityFirst = 220;
const int prioritySelectionCategory = 200;
const int priorityStackCategory = 180;
const int priorityEditCategory = 160;
const int priorityAnchorsCategory = 140;
const int priorityPositionCategory = 130;
const int priorityLayoutCategory = 120;
const int priorityTopLevelSeperator = 100;
const int priorityCustomActions = 80;
const int priorityRefactoring = 60;
const int priorityGoIntoComponent = 40;
const int priorityLast = 60;

} //ComponentCoreConstants

} //QmlDesigner

#endif //COMPONENTCORE_CONSTANTS_H
