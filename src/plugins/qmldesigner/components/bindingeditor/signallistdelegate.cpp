// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "signallistdelegate.h"

#include "signallist.h"

#include <QApplication>
#include <QMouseEvent>
#include <QPainter>
#include <QStyleOptionButton>

namespace QmlDesigner {

SignalListDelegate::SignalListDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{}

QWidget *SignalListDelegate::createEditor(QWidget *parent,
                                          const QStyleOptionViewItem &option,
                                          const QModelIndex &index) const
{
    if (index.column() == SignalListModel::ButtonColumn)
        return nullptr;

    return QStyledItemDelegate::createEditor(parent, option, index);
}

QRect connectButtonRect(const QStyleOptionViewItem &option)
{
    return option.rect.adjusted(3, 3, -3, -3);
}

void SignalListDelegate::paint(QPainter *painter,
                               const QStyleOptionViewItem &option,
                               const QModelIndex &index) const
{
    const bool connected = index.data(SignalListModel::ConnectedRole).toBool();
    if (connected) {
        QStyleOptionViewItem opt(option);
        opt.state = QStyle::State_Selected;
        QStyledItemDelegate::paint(painter, opt, index);
        if (index.column() != SignalListModel::ButtonColumn)
            return;
    }
    if (index.column() == SignalListModel::ButtonColumn) {
        QStyleOptionButton button;
        button.rect = connectButtonRect(option);
        button.text = connected ? tr("Release") : tr("Connect");
        button.state = QStyle::State_Enabled;
        QApplication::style()->drawControl(QStyle::CE_PushButton, &button, painter);
        return;
    }
    QStyledItemDelegate::paint(painter, option, index);
}

bool SignalListDelegate::editorEvent(QEvent *event,
                                     [[maybe_unused]] QAbstractItemModel *model,
                                     const QStyleOptionViewItem &option,
                                     const QModelIndex &index)
{
    if (index.column() == SignalListModel::ButtonColumn
        && event->type() == QEvent::MouseButtonRelease) {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
        if (connectButtonRect(option).contains(mouseEvent->pos()))
            emit connectClicked(index);
    }
    return true;
}

} // QmlDesigner namespace
