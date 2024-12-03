// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QAbstractListModel>
#include <QFontMetrics>
#include <QList>
#include <QPointer>

QT_FORWARD_DECLARE_CLASS(QModelIndex)

class ListModelWidthCalculator : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QAbstractItemModel *model READ model WRITE setModel NOTIFY modelChanged)
    Q_PROPERTY(QString textRole MEMBER m_textRole WRITE setTextRole NOTIFY textRoleChanged)
    Q_PROPERTY(QFont font MEMBER m_font WRITE setFont NOTIFY fontChanged)
    Q_PROPERTY(int maxWidth READ maxWidth NOTIFY maxWidthChanged)

public:
    explicit ListModelWidthCalculator(QObject *parent = nullptr);

    void setModel(QAbstractItemModel *model);
    void setTextRole(const QString &role);
    void setFont(const QFont &font);

    QAbstractItemModel *model() const;
    int maxWidth() const;

signals:
    void modelChanged();
    void textRoleChanged(QString);
    void fontChanged();
    void maxWidthChanged(int);

private:
    void clearConnections();
    void reset();
    bool updateRole();
    void setMaxWidth(int maxWidth);

    int widthOfText(const QString &str) const;

    void onSourceItemsInserted(const QModelIndex &parent, int first, int last);
    void onSourceItemsRemoved(const QModelIndex &parent, int first, int last);
    void onSourceDataChanged(
        const QModelIndex &topLeft, const QModelIndex &bottomRight, const QList<int> &roles);

    bool isValidRow(int row);

    QPointer<QAbstractItemModel> m_model;
    QList<QMetaObject::Connection> m_connections;

    QString m_textRole;
    int m_role = -1;
    QFont m_font;
    QFontMetrics m_fontMetrics;

    int m_maxWidth = 0;
    QList<int> m_data;
};
