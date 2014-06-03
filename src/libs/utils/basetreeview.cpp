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
    : Utils::TreeView(parent)
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

    m_alwaysAdjustColumns = false;

    m_layoutTimer.setSingleShot(true);
    m_layoutTimer.setInterval(20);
    connect(&m_layoutTimer, SIGNAL(timeout()), this, SLOT(resizeColumnsFinish()));
}

void BaseTreeView::setModel(QAbstractItemModel *model)
{
    disconnectColumnAdjustment();
    Utils::TreeView::setModel(model);
    connectColumnAdjustment();
}

void BaseTreeView::connectColumnAdjustment()
{
    if (m_alwaysAdjustColumns && model()) {
        connect(model(), SIGNAL(layoutChanged()), this, SLOT(resizeColumns()));
        connect(model(), SIGNAL(dataChanged(QModelIndex,QModelIndex)), this, SLOT(resizeColumns()));
        connect(this, SIGNAL(expanded(QModelIndex)), this, SLOT(resizeColumns()));
        connect(this, SIGNAL(collapsed(QModelIndex)), this, SLOT(resizeColumns()));
    }
}

void BaseTreeView::disconnectColumnAdjustment()
{
    if (m_alwaysAdjustColumns && model()) {
        disconnect(model(), SIGNAL(layoutChanged()), this, SLOT(resizeColumns()));
        disconnect(model(), SIGNAL(dataChanged(QModelIndex,QModelIndex)), this, SLOT(resizeColumns()));
        disconnect(this, SIGNAL(expanded(QModelIndex)), this, SLOT(resizeColumns()));
        disconnect(this, SIGNAL(collapsed(QModelIndex)), this, SLOT(resizeColumns()));
    }
}

void BaseTreeView::mousePressEvent(QMouseEvent *ev)
{
    Utils::TreeView::mousePressEvent(ev);
    const QModelIndex mi = indexAt(ev->pos());
    if (!mi.isValid())
        toggleColumnWidth(columnAt(ev->x()));
}

void BaseTreeView::resizeColumns()
{
    QHeaderView *h = header();
    if (!h)
        return;
    const int n = h->count();
    if (n) {
        for (int i = 0; i != n; ++i)
            h->setResizeMode(i, QHeaderView::ResizeToContents);
        m_layoutTimer.start();
    }
}

void BaseTreeView::resizeColumnsFinish()
{
    QHeaderView *h = header();
    if (!h)
        return;

    QFontMetrics fm(font());
    for (int i = 0, n = h->count(); i != n; ++i) {
        int headerSize = fm.width(model()->headerData(i, Qt::Horizontal).toString());
        int targetSize = qMax(sizeHintForColumn(i), headerSize);
        if (targetSize > 0) {
            h->setResizeMode(i, QHeaderView::Interactive);
            h->resizeSection(i, targetSize);
        }
    }
}

void BaseTreeView::toggleColumnWidth(int logicalIndex)
{
    QHeaderView *h = header();
    const int currentSize = h->sectionSize(logicalIndex);
    if (currentSize == sizeHintForColumn(logicalIndex)) {
        QFontMetrics fm(font());
        int headerSize = fm.width(model()->headerData(logicalIndex, Qt::Horizontal).toString());
        int minSize = 10 * fm.width(QLatin1Char('x'));
        h->resizeSection(logicalIndex, qMax(minSize, headerSize));
    } else {
        resizeColumnToContents(logicalIndex);
    }
}

void BaseTreeView::reset()
{
    Utils::TreeView::reset();
    if (m_alwaysAdjustColumns)
        resizeColumns();
}

void BaseTreeView::setAlwaysAdjustColumns(bool on)
{
    if (on == m_alwaysAdjustColumns)
        return;
    disconnectColumnAdjustment();
    m_alwaysAdjustColumns = on;
    connectColumnAdjustment();
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
