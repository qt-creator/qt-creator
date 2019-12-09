/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Design Tooling
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

#include "curveeditormodel.h"
#include "treeitem.h"
#include "detail/graphicsview.h"
#include "detail/selectionmodel.h"

namespace DesignTools {

CurveEditorModel::CurveEditorModel(QObject *parent)
    : TreeModel(parent)
{}

CurveEditorModel::~CurveEditorModel() {}

void CurveEditorModel::setCurrentFrame(int frame)
{
    if (graphicsView())
        graphicsView()->setCurrentFrame(frame);
}

void CurveEditorModel::setCurve(unsigned int id, const AnimationCurve &curve)
{
    if (TreeItem *item = find(id)) {
        if (PropertyTreeItem *propertyItem = item->asPropertyItem()) {
            propertyItem->setCurve(curve);
            emit curveChanged(propertyItem);
        }
    }
}

void CurveEditorModel::reset(const std::vector<TreeItem *> &items)
{
    std::vector<TreeItem::Path> sel;
    if (SelectionModel *sm = selectionModel())
        sel = sm->selectedPaths();

    beginResetModel();

    initialize();

    unsigned int counter = 0;
    for (auto *item : items) {
        item->setId(++counter);
        root()->addChild(item);
    }

    endResetModel();

    if (SelectionModel *sm = selectionModel())
        sm->select(sel);
}

} // End namespace DesignTools.
