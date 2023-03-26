// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
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
