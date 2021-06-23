/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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
#include "nodelistdelegate.h"
#include "eventlist.h"
#include "eventlistview.h"
#include "nodelistview.h"
#include "qnamespace.h"
#include "shortcutwidget.h"
#include "eventlistutils.h"

#include <QApplication>
#include <QFocusEvent>
#include <QLineEdit>
#include <QPainter>
#include <QPushButton>
#include <QSortFilterProxyModel>
#include <QStyleOptionButton>

namespace QmlDesigner {

NodeListDelegate::NodeListDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{}

void NodeListDelegate::setModelData(QWidget *editor,
                                    QAbstractItemModel *model,
                                    const QModelIndex &index) const
{
    if (index.column() == NodeListModel::idColumn) {
        if (auto *edit = qobject_cast<QLineEdit *>(editor)) {
            QVariant iidVariant = index.data(NodeListModel::internalIdRole);
            if (iidVariant.isValid()) {
                QString verifiedId = EventList::setNodeId(iidVariant.toInt(), edit->text());
                if (!verifiedId.isNull())
                    edit->setText(verifiedId);
                else
                    edit->setText("");
            }
        }
    }
    QStyledItemDelegate::setModelData(editor, model, index);
}

bool NodeListDelegate::eventFilter(QObject *editor, QEvent *event)
{
    if (event->type() == QEvent::KeyPress) {
        int key = static_cast<QKeyEvent *>(event)->key();
        if (key == Qt::Key_Tab || key == Qt::Key_Backtab)
            return false;
    }
    return QStyledItemDelegate::eventFilter(editor, event);
}

void NodeListDelegate::commitAndClose()
{
    if (auto *editor = qobject_cast<ShortcutWidget *>(sender())) {
        emit commitData(editor);
        emit closeEditor(editor);
    }
}

void NodeListDelegate::close()
{
    if (auto *editor = qobject_cast<ShortcutWidget *>(sender()))
        emit closeEditor(editor);
}

} // namespace QmlDesigner.
