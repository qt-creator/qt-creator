/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator Squish plugin.
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

#include "squishtesttreeview.h"
#include "squishconstants.h"
#include "squishtesttreemodel.h"

#include <coreplugin/icontext.h>
#include <coreplugin/icore.h>

namespace Squish {
namespace Internal {

SquishTestTreeView::SquishTestTreeView(QWidget *parent)
    : Utils::NavigationTreeView(parent)
    , m_context(new Core::IContext(this))
{
    setExpandsOnDoubleClick(false);
    m_context->setWidget(this);
    m_context->setContext(Core::Context(Constants::SQUISH_CONTEXT));
    Core::ICore::addContextObject(m_context);
}

void SquishTestTreeView::resizeEvent(QResizeEvent *event)
{
    // override derived behavior of Utils::NavigationTreeView as we have more than 1 column
    Utils::TreeView::resizeEvent(event);
}

void SquishTestTreeView::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        const QModelIndex index = indexAt(event->pos());
        if (index.isValid() && index.column() > 0 && index.column() < 3) {
            int type = index.data(TypeRole).toInt();
            if (type == SquishTestTreeItem::SquishSuite
                || type == SquishTestTreeItem::SquishTestCase) {
                m_lastMousePressedIndex = index;
            }
        }
    }
    QTreeView::mousePressEvent(event);
}

void SquishTestTreeView::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        const QModelIndex index = indexAt(event->pos());
        if (index.isValid() && index == m_lastMousePressedIndex) {
            int type = index.data(TypeRole).toInt();
            if (type == SquishTestTreeItem::SquishSuite) {
                if (index.column() == 1)
                    emit runTestSuite(index.data(DisplayNameRole).toString());
                else if (index.column() == 2)
                    emit openObjectsMap(index.data(DisplayNameRole).toString());
            } else {
                const QModelIndex &suiteIndex = index.parent();
                if (suiteIndex.isValid()) {
                    if (index.column() == 1) {
                        emit runTestCase(suiteIndex.data(DisplayNameRole).toString(),
                                         index.data(DisplayNameRole).toString());
                    } else if (index.column() == 2) {
                        emit recordTestCase(suiteIndex.data(DisplayNameRole).toString(),
                                            index.data(DisplayNameRole).toString());
                    }
                }
            }
        }
    }
    QTreeView::mouseReleaseEvent(event);
}

/****************************** SquishTestTreeItemDelegate *************************************/

SquishTestTreeItemDelegate::SquishTestTreeItemDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{}

void SquishTestTreeItemDelegate::paint(QPainter *painter,
                                       const QStyleOptionViewItem &option,
                                       const QModelIndex &idx) const
{
    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, idx);

    // elide first column if necessary
    if (idx.column() == 0)
        opt.textElideMode = Qt::ElideMiddle;

    // display disabled items as enabled
    if (idx.flags() == Qt::NoItemFlags)
        opt.palette.setColor(QPalette::Text, opt.palette.color(QPalette::Active, QPalette::Text));

    QStyledItemDelegate::paint(painter, opt, idx);
}

QSize SquishTestTreeItemDelegate::sizeHint(const QStyleOptionViewItem &option,
                                           const QModelIndex &idx) const
{
    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, idx);

    // elide first column if necessary
    if (idx.column() == 0)
        opt.textElideMode = Qt::ElideMiddle;
    return QStyledItemDelegate::sizeHint(opt, idx);
}

} // namespace Internal
} // namespace Squish
