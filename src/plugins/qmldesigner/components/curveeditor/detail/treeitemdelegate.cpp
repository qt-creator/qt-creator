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

void TreeItemDelegate::paint(QPainter *painter,
                             const QStyleOptionViewItem &option,
                             const QModelIndex &index) const
{
    if (index.column() == 1 || index.column() == 2) {

        int height = option.rect.size().height();
        QRect iconRect(QPoint(0, 0), QSize(height, height));
        iconRect.moveCenter(option.rect.center());

        auto *treeItem = static_cast<TreeItem *>(index.internalPointer());
        if (option.state & QStyle::State_MouseOver && iconRect.contains(m_mousePos)) {

            painter->fillRect(option.rect, option.backgroundBrush);

            if (index.column() == 1) {

                if (treeItem->locked()) {

                    QPixmap pixmap = pixmapFromIcon(
                        m_style.treeItemStyle.unlockedIcon,
                        iconRect.size(),
                        m_style.fontColor);

                    painter->drawPixmap(iconRect, pixmap);

                } else {

                    QPixmap pixmap = pixmapFromIcon(
                        m_style.treeItemStyle.lockedIcon,
                        iconRect.size(),
                        m_style.fontColor);

                    painter->drawPixmap(iconRect, pixmap);
                }

            } else if (index.column() == 2) {

                if (treeItem->pinned()) {

                    QPixmap pixmap = pixmapFromIcon(
                        m_style.treeItemStyle.unpinnedIcon,
                        iconRect.size(),
                        m_style.fontColor);

                    painter->drawPixmap(iconRect, pixmap);

                } else {

                    QPixmap pixmap = pixmapFromIcon(
                        m_style.treeItemStyle.pinnedIcon,
                        iconRect.size(),
                        m_style.fontColor);

                    painter->drawPixmap(iconRect, pixmap);

                }
            }

        } else {

            if (treeItem->locked() && index.column() == 1) {

                QPixmap pixmap = pixmapFromIcon(
                    m_style.treeItemStyle.lockedIcon,
                    iconRect.size(),
                    m_style.fontColor);

                painter->drawPixmap(iconRect, pixmap);

            } else if (treeItem->pinned() && index.column() == 2) {

                QPixmap pixmap = pixmapFromIcon(
                    m_style.treeItemStyle.pinnedIcon,
                    iconRect.size(),
                    m_style.fontColor);

                painter->drawPixmap(iconRect, pixmap);

            }
        }
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
