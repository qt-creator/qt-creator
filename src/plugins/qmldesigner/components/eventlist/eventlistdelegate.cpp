// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#include "eventlistdelegate.h"
#include "eventlist.h"
#include "eventlistview.h"
#include "nodelistview.h"
#include "qnamespace.h"
#include "shortcutwidget.h"
#include "eventlistutils.h"

#include <QApplication>
#include <QLineEdit>
#include <QPainter>
#include <QStyleOptionButton>
#include <QTableView>
#include <QMouseEvent>

namespace QmlDesigner {

EventListDelegate::EventListDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{}

QWidget *EventListDelegate::createEditor(QWidget *parent,
                                         const QStyleOptionViewItem &option,
                                         const QModelIndex &index) const
{
    if (index.column() == EventListModel::shortcutColumn) {
        auto *editor = new ShortcutWidget(parent);
        connect(editor, &ShortcutWidget::done, this, &EventListDelegate::commitAndClose);
        connect(editor, &ShortcutWidget::cancel, this, &EventListDelegate::close);
        return editor;
    } else if (index.column() == EventListModel::connectColumn) {
        return nullptr;
    }
    return QStyledItemDelegate::createEditor(parent, option, index);
}

void EventListDelegate::setModelData(QWidget *editor,
                                     QAbstractItemModel *model,
                                     const QModelIndex &index) const
{
    if (index.column() == EventListModel::idColumn) {
        if (auto *edit = qobject_cast<QLineEdit *>(editor)) {
            QString name = edit->text();
            QString unique = uniqueName(model, name);
            if (name != unique) {
                name = unique;
                edit->setText(unique);
            }
            emit eventIdChanged(model->data(index, Qt::DisplayRole).toString(), name);
        }

    } else if (index.column() == EventListModel::shortcutColumn) {
        if (auto *edit = qobject_cast<ShortcutWidget *>(editor)) {
            auto idIndex = model->index(index.row(), EventListModel::idColumn, index.parent());
            if (idIndex.isValid()) {
                emit shortcutChanged(model->data(idIndex, Qt::DisplayRole).toString(), edit->text());
                model->setData(index, edit->text(), Qt::DisplayRole);
                return;
            }
        }
    } else if (index.column() == EventListModel::descriptionColumn) {
        if (auto *edit = qobject_cast<QLineEdit *>(editor)) {
            auto idIndex = model->index(index.row(), EventListModel::idColumn, index.parent());
            if (idIndex.isValid()) {
                auto id = model->data(idIndex, Qt::DisplayRole).toString();
                emit descriptionChanged(id, edit->text());
            }
        }
    }
    QStyledItemDelegate::setModelData(editor, model, index);
}

bool EventListDelegate::hasConnectionColumn(QObject *parent)
{
    if (auto *table = qobject_cast<QTableView *>(parent))
        return !table->isColumnHidden(EventListModel::connectColumn);

    return false;
}

QRect EventListDelegate::connectButtonRect(const QStyleOptionViewItem &option)
{
    return option.rect.adjusted(3, 3, -3, -3);
}

void EventListDelegate::paint(QPainter *painter,
                              const QStyleOptionViewItem &option,
                              const QModelIndex &index) const
{
    if (hasConnectionColumn(parent())) {
        bool connected = index.data(EventListModel::connectedRole).toBool();
        if (connected) {
            QStyleOptionViewItem opt(option);
            opt.state = QStyle::State_Selected;
            QStyledItemDelegate::paint(painter, opt, index);

            if (index.column() != EventListModel::connectColumn)
                return;
        }

        if (index.column() == EventListModel::connectColumn) {
            QStyleOptionButton button;
            button.rect = connectButtonRect(option);
            button.text = connected ? tr("Release") : tr("Connect");
            button.state = QStyle::State_Enabled;
            QApplication::style()->drawControl(QStyle::CE_PushButton, &button, painter);
            return;
        }
    }
    QStyledItemDelegate::paint(painter, option, index);
}

bool EventListDelegate::editorEvent(QEvent *event,
                                    QAbstractItemModel *model,
                                    const QStyleOptionViewItem &option,
                                    const QModelIndex &index)
{
    if (index.column() == EventListModel::connectColumn) {
        if (event->type() == QEvent::MouseButtonRelease) {
            QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
            if (connectButtonRect(option).contains(mouseEvent->pos())) {
                if (QModelIndex sib = index.siblingAtColumn(EventListModel::idColumn); sib.isValid()) {
                    auto id = sib.data().toString();
                    bool connected = index.data(EventListModel::connectedRole).toBool();
                    for (int c = 0; c < model->columnCount(); ++c) {
                        auto id = model->index(index.row(), c, index.parent());
                        model->setData(id, !connected, EventListModel::connectedRole);
                    }
                    emit connectClicked(id, !connected);
                    return true;
                }
            }
        }
    }
    return QStyledItemDelegate::editorEvent(event, model, option, index);
}

bool EventListDelegate::eventFilter(QObject *editor, QEvent *event)
{
    if (auto *edit = qobject_cast<ShortcutWidget *>(editor)) {
        if (event->type() == QEvent::KeyPress) {
            edit->recordKeysequence(static_cast<QKeyEvent *>(event));
            return true;
        }

        if (event->type() == QEvent::FocusOut) {
            if (!edit->containsFocus())
                edit->reset();
        }
    } else {
        if (event->type() == QEvent::KeyPress) {
            int key = static_cast<QKeyEvent *>(event)->key();
            if (key == Qt::Key_Tab || key == Qt::Key_Backtab)
                return false;
        }
    }

    return QStyledItemDelegate::eventFilter(editor, event);
}

QSize EventListDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    if (index.column() == EventListModel::connectColumn) {
        QSize size = QStyledItemDelegate::sizeHint(option, index);
        size.rwidth() = 20;
        return size;
    }
    return QStyledItemDelegate::sizeHint(option, index);
}

void EventListDelegate::commitAndClose()
{
    if (auto *editor = qobject_cast<ShortcutWidget *>(sender())) {
        emit commitData(editor);
        emit closeEditor(editor);
    }
}

void EventListDelegate::close()
{
    if (auto *editor = qobject_cast<ShortcutWidget *>(sender()))
        emit closeEditor(editor);
}

} // namespace QmlDesigner.
