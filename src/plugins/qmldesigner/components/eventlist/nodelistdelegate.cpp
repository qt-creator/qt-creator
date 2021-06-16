/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd
** All rights reserved.
** For any questions to The Qt Company, please use contact form at http://www.qt.io/contact-us
**
** This file is part of the Qt Event List module.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** If you have questions regarding the use of this file, please use
** contact form at http://www.qt.io/contact-us
**
******************************************************************************/
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
