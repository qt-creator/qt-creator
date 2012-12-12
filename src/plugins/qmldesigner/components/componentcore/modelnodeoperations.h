/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
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

#ifndef MODELNODEOPERATIONS_H
#define MODELNODEOPERATIONS_H

#include "selectioncontext.h"

namespace QmlDesigner {

class ModelNodeOperations
{
public:
    static void goIntoComponent(const ModelNode &modelNode);

    static void select(const SelectionContext &selectionState);
    static void deSelect(const SelectionContext &selectionState);
    static void cut(const SelectionContext &selectionState);
    static void copy(const SelectionContext &selectionState);
    static void deleteSelection(const SelectionContext &selectionState);
    static void toFront(const SelectionContext &selectionState);
    static void toBack(const SelectionContext &selectionState);
    static void raise(const SelectionContext &selectionState);
    static void lower(const SelectionContext &selectionState);
    static void paste(const SelectionContext &selectionState);
    static void undo(const SelectionContext &selectionState);
    static void redo(const SelectionContext &selectionState);
    static void setVisible(const SelectionContext &selectionState);
    static void resetSize(const SelectionContext &selectionState);
    static void resetPosition(const SelectionContext &selectionState);
    static void goIntoComponent(const SelectionContext &selectionState);
    static void setId(const SelectionContext &selectionState);
    static void resetZ(const SelectionContext &selectionState);
    static void anchorsFill(const SelectionContext &selectionState);
    static void anchorsReset(const SelectionContext &selectionState);
    static void layoutRow(const SelectionContext &selectionState);
    static void layoutColumn(const SelectionContext &selectionState);
    static void layoutGrid(const SelectionContext &selectionState);
    static void layoutFlow(const SelectionContext &selectionState);
};

} //QmlDesigner

#endif //MODELNODEOPERATIONS_H
