// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#include "treeview.h"
#include "curveeditormodel.h"
#include "curveitem.h"
#include "selectionmodel.h"
#include "treeitem.h"
#include "treeitemdelegate.h"
#include "treemodel.h"

#include <QHeaderView>
#include <QMouseEvent>

namespace QmlDesigner {

TreeView::TreeView(CurveEditorModel *model, QWidget *parent)
    : QTreeView(parent)
{
    model->setTreeView(this);

    setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
    setUniformRowHeights(true);
    setRootIsDecorated(false);
    setMouseTracking(true);
    setHeaderHidden(true);

    setModel(model);

    auto expandItems = [this]() { expandAll(); };
    connect(model, &QAbstractItemModel::modelReset, expandItems);

    setItemDelegate(new TreeItemDelegate(model->style(), this));

    setSelectionModel(new SelectionModel(model));
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setSelectionMode(QAbstractItemView::ExtendedSelection);

    setStyle(model->style());

    header()->setMinimumSectionSize(20);

    header()->setSectionResizeMode(0, QHeaderView::Stretch);
    header()->setSectionResizeMode(1, QHeaderView::Fixed);
    header()->setSectionResizeMode(2, QHeaderView::Fixed);

    header()->setStretchLastSection(false);
    header()->resizeSection(1, 20);
    header()->resizeSection(2, 20);
}

SelectionModel *TreeView::selectionModel() const
{
    return qobject_cast<SelectionModel *>(QTreeView::selectionModel());
}

void TreeView::setStyle(const CurveEditorStyle &style)
{
    QPalette pal = palette();
    pal.setBrush(QPalette::Base, style.backgroundBrush);
    pal.setBrush(QPalette::Button, style.backgroundAlternateBrush);
    pal.setBrush(QPalette::Text, style.fontColor);

    // Tmp to see what happens on windows/macOS.
    pal.setBrush(backgroundRole(), Qt::white);
    pal.setBrush(foregroundRole(), Qt::white);

    setPalette(pal);

    if (auto *delegate = qobject_cast<TreeItemDelegate *>(itemDelegate()))
        delegate->setStyle(style);
}

void TreeView::changeCurve(unsigned int id, const AnimationCurve &curve)
{
    if (auto *curveModel = qobject_cast<CurveEditorModel *>(model()))
        curveModel->setCurve(id, curve);
}

QSize TreeView::sizeHint() const
{
    return QSize(170, 300);
}

void TreeView::mousePressEvent(QMouseEvent *event)
{
    QModelIndex index = indexAt(event->pos());
    if (index.isValid()) {
        auto *treeItem = static_cast<TreeItem *>(index.internalPointer());
        if (TreeModel::isLockedColumn(index))
            emit treeItemLocked(treeItem, !treeItem->locked());
        else if (TreeModel::isPinnedColumn(index))
            emit treeItemPinned(treeItem, !treeItem->pinned());
    }
    QTreeView::mousePressEvent(event);
}

} // End namespace QmlDesigner.
