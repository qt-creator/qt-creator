// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#pragma once

#include <QStyledItemDelegate>

namespace QmlDesigner {

class EventListDelegate : public QStyledItemDelegate
{
    Q_OBJECT

signals:
    void eventIdChanged(const QString &from, const QString &to) const;
    void shortcutChanged(const QString &from, const QString &to) const;
    void descriptionChanged(const QString &id, const QString &text) const;
    void connectClicked(const QString &id, bool connected) const;

public:
    EventListDelegate(QObject *parent = nullptr);

    QWidget *createEditor(QWidget *parent,
                          const QStyleOptionViewItem &option,
                          const QModelIndex &index) const override;

    void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const override;

    void paint(QPainter *painter,
               const QStyleOptionViewItem &option,
               const QModelIndex &index) const override;

    bool editorEvent(QEvent *event,
                     QAbstractItemModel *model,
                     const QStyleOptionViewItem &option,
                     const QModelIndex &index) override;

protected:
    bool eventFilter(QObject *editor, QEvent *event) override;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;

private:
    void close();
    void commitAndClose();

    static bool hasConnectionColumn(QObject *parent);
    static QRect connectButtonRect(const QStyleOptionViewItem &option);
};

} // namespace QmlDesigner.
