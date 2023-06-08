// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "treeview.h"
#include "structuremodel.h"

#include <QMimeData>
#include <QMouseEvent>

using namespace ScxmlEditor::Common;

TreeView::TreeView(QWidget *parent)
    : QTreeView(parent)
{
    setDragEnabled(true);
    setAcceptDrops(true);
    setDropIndicatorShown(true);
    setDragDropMode(QAbstractItemView::DragDrop);
    setMouseTracking(true);
    setFrameShape(QFrame::NoFrame);
    setHeaderHidden(true);
}

void TreeView::leaveEvent(QEvent *e)
{
    emit mouseExited();
    QTreeView::leaveEvent(e);
}

void TreeView::currentChanged(const QModelIndex &index, const QModelIndex &prev)
{
    QTreeView::currentChanged(index, prev);
    emit currentIndexChanged(index);
}

void TreeView::mousePressEvent(QMouseEvent *event)
{
    QTreeView::mousePressEvent(event);
    if (event->button() == Qt::RightButton)
        emit rightButtonClicked(currentIndex(), event->globalPosition().toPoint());
}
