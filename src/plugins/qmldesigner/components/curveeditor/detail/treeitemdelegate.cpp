// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#include "treeitemdelegate.h"
#include "treeitem.h"
#include "treemodel.h"

#include <QApplication>
#include <QEvent>
#include <QMouseEvent>
#include <QPainter>

namespace QmlDesigner {

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

void TreeItemDelegate::paint(QPainter *painter,
                             const QStyleOptionViewItem &option,
                             const QModelIndex &index) const
{
    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);

    QColor high = Theme::getColor(Theme::Color::QmlDesigner_HighlightColor);
    opt.palette.setColor(QPalette::Active, QPalette::Highlight, high);
    opt.palette.setColor(QPalette::Inactive, QPalette::Highlight, high);

    QStyle *style = opt.widget ? opt.widget->style() : QApplication::style();
    style->drawPrimitive(QStyle::PE_PanelItemViewItem, &opt, painter, opt.widget);

    auto *treeItem = static_cast<TreeItem *>(index.internalPointer());

    bool textColumn = TreeModel::isTextColumn(index);
    bool lockedColumn = TreeModel::isLockedColumn(index);
    bool pinnedColumn = TreeModel::isPinnedColumn(index);

    QPixmap pixmap;
    QRect iconRect = makeSquare(option.rect);

    if (lockedColumn) {
        if (treeItem->locked()) {
            pixmap = m_style.treeItemStyle.lockedIcon.pixmap(iconRect.size());
        } else if (treeItem->asNodeItem() != nullptr && treeItem->implicitlyLocked()) {
            pixmap = m_style.treeItemStyle.implicitlyLockedIcon.pixmap(iconRect.size());
        } else if (option.state.testFlag(QStyle::State_MouseOver)) {
            if (treeItem->implicitlyLocked()) {
                pixmap = m_style.treeItemStyle.implicitlyLockedIcon.pixmap(iconRect.size());
            } else {
                pixmap = m_style.treeItemStyle.unlockedIcon.pixmap(iconRect.size());
            }
        }

    } else if (pinnedColumn) {
        if (treeItem->pinned()) {
            pixmap = m_style.treeItemStyle.pinnedIcon.pixmap(iconRect.size());
        } else if (treeItem->asNodeItem() != nullptr && treeItem->implicitlyPinned()) {
            pixmap = m_style.treeItemStyle.implicitlyPinnedIcon.pixmap(iconRect.size());
        } else if (option.state.testFlag(QStyle::State_MouseOver)) {
            if (treeItem->implicitlyPinned()) {
                pixmap = m_style.treeItemStyle.implicitlyPinnedIcon.pixmap(iconRect.size());
            } else {
                pixmap = m_style.treeItemStyle.unpinnedIcon.pixmap(iconRect.size());
            }
        }

    } else {
        if (textColumn && (treeItem->locked() || treeItem->implicitlyLocked())) {
            QColor col = opt.palette.color(QPalette::Disabled, QPalette::Text).darker();
            opt.palette.setColor(QPalette::Active, QPalette::Text, col);
            opt.palette.setColor(QPalette::Inactive, QPalette::Text, col);
        }
        QStyledItemDelegate::paint(painter, opt, index);
    }

    if (!pixmap.isNull())
        painter->drawPixmap(iconRect, pixmap);
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

} // End namespace QmlDesigner.
