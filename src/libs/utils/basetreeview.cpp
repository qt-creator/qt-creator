/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
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

#include "basetreeview.h"

#include <QDebug>
#include <QFontMetrics>
#include <QHeaderView>
#include <QItemDelegate>
#include <QLabel>
#include <QMenu>
#include <QMouseEvent>
#include <QTimer>

namespace Utils {

class BaseTreeViewDelegate : public QItemDelegate
{
public:
    BaseTreeViewDelegate(QObject *parent): QItemDelegate(parent) {}

    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option,
                          const QModelIndex &index) const
    {
        Q_UNUSED(option);
        QLabel *label = new QLabel(parent);
        label->setAutoFillBackground(true);
        label->setTextInteractionFlags(Qt::TextSelectableByMouse
            | Qt::LinksAccessibleByMouse);
        label->setText(index.data().toString());
        return label;
    }
};

BaseTreeView::BaseTreeView(QWidget *parent)
    : TreeView(parent)
{
    setAttribute(Qt::WA_MacShowFocusRect, false);
    setFrameStyle(QFrame::NoFrame);
    setRootIsDecorated(false);
    setIconSize(QSize(10, 10));
    setSelectionMode(QAbstractItemView::ExtendedSelection);
    setUniformRowHeights(true);
    setItemDelegate(new BaseTreeViewDelegate(this));
    header()->setDefaultAlignment(Qt::AlignLeft);
    header()->setClickable(true);

    connect(this, SIGNAL(activated(QModelIndex)),
        SLOT(rowActivatedHelper(QModelIndex)));
    connect(this, SIGNAL(clicked(QModelIndex)),
        SLOT(rowClickedHelper(QModelIndex)));
    connect(header(), SIGNAL(sectionClicked(int)),
        SLOT(toggleColumnWidth(int)));
}

void BaseTreeView::setModel(QAbstractItemModel *m)
{
    const char *sig = "columnAdjustmentRequested()";
    if (model()) {
        int index = model()->metaObject()->indexOfSignal(sig);
        if (index != -1)
            disconnect(model(), SIGNAL(columnAdjustmentRequested()), this, SLOT(resizeColumns()));
    }
    TreeView::setModel(m);
    if (m) {
        int index = m->metaObject()->indexOfSignal(sig);
        if (index != -1)
            connect(m, SIGNAL(columnAdjustmentRequested()), this, SLOT(resizeColumns()));
    }
}

void BaseTreeView::mousePressEvent(QMouseEvent *ev)
{
    TreeView::mousePressEvent(ev);
    const QModelIndex mi = indexAt(ev->pos());
    if (!mi.isValid())
        toggleColumnWidth(columnAt(ev->x()));
}

void BaseTreeView::resizeColumns()
{
    QHeaderView *h = header();
    if (!h)
        return;

    for (int i = 0, n = h->count(); i != n; ++i) {
        int targetSize = suggestedColumnSize(i);
        if (targetSize > 0)
            h->resizeSection(i, targetSize);
    }
}

int BaseTreeView::suggestedColumnSize(int column) const
{
    QHeaderView *h = header();
    if (!h)
        return -1;

    QModelIndex a = indexAt(QPoint(1, 1));
    a = a.sibling(a.row(), column);
    QFontMetrics fm(font());
    int m = fm.width(model()->headerData(column, Qt::Horizontal).toString());
    const int ind = indentation();
    for (int i = 0; i < 100 && a.isValid(); ++i) {
        const QString s = model()->data(a).toString();
        int w = fm.width(s) + 10;
        if (column == 0) {
            for (QModelIndex b = a.parent(); b.isValid(); b = b.parent())
                w += ind;
        }
        if (w > m)
            m = w;
        a = indexBelow(a);
    }
    return m;
}

void BaseTreeView::toggleColumnWidth(int logicalIndex)
{
    QHeaderView *h = header();
    const int currentSize = h->sectionSize(logicalIndex);
    const int suggestedSize = suggestedColumnSize(logicalIndex);
    if (currentSize == suggestedSize) {
        QFontMetrics fm(font());
        int headerSize = fm.width(model()->headerData(logicalIndex, Qt::Horizontal).toString());
        int minSize = 10 * fm.width(QLatin1Char('x'));
        h->resizeSection(logicalIndex, qMax(minSize, headerSize));
    } else {
        h->resizeSection(logicalIndex, suggestedSize);
    }
}

QModelIndexList BaseTreeView::activeRows() const
{
    QItemSelectionModel *selection = selectionModel();
    QModelIndexList indices = selection->selectedRows();
    if (indices.isEmpty()) {
        QModelIndex current = selection->currentIndex();
        if (current.isValid())
            indices.append(current);
    }
    return indices;
}

} // namespace Utils
