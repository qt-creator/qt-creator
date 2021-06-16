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
