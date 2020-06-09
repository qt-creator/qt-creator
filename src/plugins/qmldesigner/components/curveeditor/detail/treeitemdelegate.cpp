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
#include "treeitemdelegate.h"
#include "treeitem.h"

#include <QApplication>
#include <QEvent>
#include <QMouseEvent>
#include <QPainter>

namespace DesignTools {

TreeItemDelegate::TreeItemDelegate(const CurveEditorStyle &style, QObject *parent)
    : QStyledItemDelegate(parent)
    , m_style(style)
{}

QSize TreeItemDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    return QStyledItemDelegate::sizeHint(option, index);
}

QRect makeSquare(const QRect &rect)
{
    int size = rect.width() > rect.height() ? rect.height() : rect.width();
    QRect r(QPoint(0, 0), QSize(size, size));
    r.moveCenter(rect.center());
    return r;
}

QPixmap pixmapFromStyle(int column, const CurveEditorStyle &style, const QRect &rect, TreeItem *item, bool underMouse)
{
    QColor color = underMouse ? style.iconHoverColor : style.iconColor;
    if (column == 1) {
        bool locked = item->locked();
        if (underMouse)
            locked = !locked;

        if (locked)
            return pixmapFromIcon(style.treeItemStyle.lockedIcon, rect.size(), color);
        else
            return pixmapFromIcon(style.treeItemStyle.unlockedIcon, rect.size(), color);
    }

    bool pinned = item->pinned();
    if (underMouse)
        pinned = !pinned;

    if (pinned)
        return pixmapFromIcon(style.treeItemStyle.pinnedIcon, rect.size(), color);
    else
        return pixmapFromIcon(style.treeItemStyle.unpinnedIcon, rect.size(), color);
}

void TreeItemDelegate::paint(QPainter *painter,
                             const QStyleOptionViewItem &option,
                             const QModelIndex &index) const
{
    if (index.column() == 1 || index.column() == 2) {
        QStyleOptionViewItem opt = option;
        initStyleOption(&opt, index);

        auto *treeItem = static_cast<TreeItem *>(index.internalPointer());

        QPoint mousePos = QCursor::pos();
        mousePos = option.widget->mapFromGlobal(mousePos);

        QRect iconRect = makeSquare(option.rect);
        bool underMouse = option.rect.contains(m_mousePos)
                          && option.state & QStyle::State_MouseOver;

        QStyle *style = opt.widget ? opt.widget->style() : QApplication::style();
        style->drawPrimitive(QStyle::PE_PanelItemViewItem, &opt, painter, opt.widget);

        QPixmap pixmap = pixmapFromStyle(index.column(), m_style, iconRect, treeItem, underMouse);
        painter->drawPixmap(iconRect, pixmap);

    } else {
        QStyledItemDelegate::paint(painter, option, index);
    }
}

void TreeItemDelegate::setStyle(const CurveEditorStyle &style)
{
    m_style = style;
}

bool TreeItemDelegate::editorEvent(QEvent *event,
                                   QAbstractItemModel *model,
                                   const QStyleOptionViewItem &option,
                                   const QModelIndex &index)
{
    if (event->type() == QEvent::MouseMove)
        m_mousePos = static_cast<QMouseEvent *>(event)->pos();

    return QStyledItemDelegate::editorEvent(event, model, option, index);
}

} // End namespace DesignTools.
