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

#ifndef MODELNODEOPERATIONS_H
#define MODELNODEOPERATIONS_H

#include "selectioncontext.h"

namespace QmlDesigner {
namespace ModelNodeOperations {

void goIntoComponent(const ModelNode &modelNode);

typedef void (*SelectionAction)(const SelectionContext &);

void select(const SelectionContext &selectionState);
void deSelect(const SelectionContext &selectionState);
void cut(const SelectionContext &selectionState);
void copy(const SelectionContext &selectionState);
void deleteSelection(const SelectionContext &selectionState);
void toFront(const SelectionContext &selectionState);
void toBack(const SelectionContext &selectionState);
void raise(const SelectionContext &selectionState);
void lower(const SelectionContext &selectionState);
void paste(const SelectionContext &selectionState);
void undo(const SelectionContext &selectionState);
void redo(const SelectionContext &selectionState);
void setVisible(const SelectionContext &selectionState);
void setFillWidth(const SelectionContext &selectionState);
void setFillHeight(const SelectionContext &selectionState);
void resetSize(const SelectionContext &selectionState);
void resetPosition(const SelectionContext &selectionState);
void goIntoComponent(const SelectionContext &selectionState);
void setId(const SelectionContext &selectionState);
void resetZ(const SelectionContext &selectionState);
void anchorsFill(const SelectionContext &selectionState);
void anchorsReset(const SelectionContext &selectionState);
void layoutRowPositioner(const SelectionContext &selectionState);
void layoutColumnPositioner(const SelectionContext &selectionState);
void layoutGridPositioner(const SelectionContext &selectionState);
void layoutFlowPositioner(const SelectionContext &selectionState);
void layoutRowLayout(const SelectionContext &selectionState);
void layoutColumnLayout(const SelectionContext &selectionState);
void layoutGridLayout(const SelectionContext &selectionState);
void gotoImplementation(const SelectionContext &selectionState);
void removeLayout(const SelectionContext &selectionContext);
void removePositioner(const SelectionContext &selectionContext);


} // namespace ModelNodeOperationso
} //QmlDesigner

#endif //MODELNODEOPERATIONS_H
