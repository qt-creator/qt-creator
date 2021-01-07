/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
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
                                     QAbstractItemModel *model,
                                     const QStyleOptionViewItem &option,
                                     const QModelIndex &index)
{
    Q_UNUSED(model)

    if (index.column() == SignalListModel::ButtonColumn
        && event->type() == QEvent::MouseButtonRelease) {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
        if (connectButtonRect(option).contains(mouseEvent->pos()))
            emit connectClicked(index);
    }
    return true;
}

} // QmlDesigner namespace
