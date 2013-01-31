/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

namespace Utils {

class BaseTreeViewDelegate : public QItemDelegate
{
public:
    BaseTreeViewDelegate() {}

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
    : QTreeView(parent)
{
    setAttribute(Qt::WA_MacShowFocusRect, false);
    setFrameStyle(QFrame::NoFrame);
    setRootIsDecorated(false);
    setIconSize(QSize(10, 10));
    setSelectionMode(QAbstractItemView::ExtendedSelection);
    setUniformRowHeights(true);
    setItemDelegate(new BaseTreeViewDelegate);
    header()->setDefaultAlignment(Qt::AlignLeft);
    header()->setClickable(true);

    connect(this, SIGNAL(activated(QModelIndex)),
        SLOT(rowActivatedHelper(QModelIndex)));
    connect(this, SIGNAL(clicked(QModelIndex)),
        SLOT(rowClickedHelper(QModelIndex)));
    connect(header(), SIGNAL(sectionClicked(int)),
        SLOT(headerSectionClicked(int)));

    m_adjustColumnsAction = new QAction(tr("Adjust Column Widths to Contents"), this);
    m_alwaysAdjustColumnsAction = 0;
}

void BaseTreeView::setAlwaysAdjustColumnsAction(QAction *action)
{
    m_alwaysAdjustColumnsAction = action;
    connect(action, SIGNAL(toggled(bool)),
        SLOT(setAlwaysResizeColumnsToContents(bool)));
}

void BaseTreeView::addBaseContextActions(QMenu *menu)
{
    menu->addSeparator();
    if (m_alwaysAdjustColumnsAction)
        menu->addAction(m_alwaysAdjustColumnsAction);
    menu->addAction(m_adjustColumnsAction);
    menu->addSeparator();
}

bool BaseTreeView::handleBaseContextAction(QAction *act)
{
    if (act == 0)
        return true;
    if (act == m_adjustColumnsAction) {
        resizeColumnsToContents();
        return true;
    }
    if (act == m_alwaysAdjustColumnsAction) {
        if (act->isChecked())
            resizeColumnsToContents();
        // Action triggered automatically.
        return true;
    }
    return false;
}

void BaseTreeView::setModel(QAbstractItemModel *model)
{
    QTreeView::setModel(model);
    if (header() && m_alwaysAdjustColumnsAction)
        setAlwaysResizeColumnsToContents(m_alwaysAdjustColumnsAction->isChecked());
}

void BaseTreeView::mousePressEvent(QMouseEvent *ev)
{
    QTreeView::mousePressEvent(ev);
    if (!indexAt(ev->pos()).isValid())
        resizeColumnsToContents();
}

void BaseTreeView::resizeColumnsToContents()
{
    const int columnCount = model()->columnCount();
    for (int c = 0 ; c != columnCount; ++c)
        resizeColumnToContents(c);
}

void BaseTreeView::setAlwaysResizeColumnsToContents(bool on)
{
    QHeaderView::ResizeMode mode = on
        ? QHeaderView::ResizeToContents : QHeaderView::Interactive;
    header()->setResizeMode(0, mode);
}

void BaseTreeView::headerSectionClicked(int logicalIndex)
{
    resizeColumnToContents(logicalIndex);
}

void BaseTreeView::reset()
{
    QTreeView::reset();
    if (header() && m_alwaysAdjustColumnsAction
            && m_alwaysAdjustColumnsAction->isChecked())
        resizeColumnsToContents();
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
