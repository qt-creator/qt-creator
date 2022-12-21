// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <bindingeditor/signallistdialog.h>
#include <qmldesignercorelib_global.h>
#include <modelnode.h>
#include <qmlconnections.h>

#include <QtQml>
#include <QObject>
#include <QPointer>
#include <QStandardItemModel>

namespace QmlDesigner {

class SignalListModel : public QStandardItemModel
{
    Q_OBJECT

public:
    enum ColumnRoles : unsigned int {
        TargetColumn = 0,
        SignalColumn = 1,
        ButtonColumn = 2
    };
    enum UserRoles : unsigned int {
        ConnectionsInternalIdRole = Qt::UserRole + 1,
        ConnectedRole
    };

    SignalListModel(QObject *parent = nullptr);

    void setConnected(int row, bool connected);
};

class SignalListFilterModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    SignalListFilterModel(QObject *parent = nullptr);

    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;
};

class SignalList : public QObject
{
    Q_OBJECT

public:
    explicit SignalList(QObject *parent = nullptr);
    ~SignalList();

    static SignalList* showWidget(const ModelNode &modelNode);

    void setModelNode(const ModelNode &modelNode);
    void connectClicked(const QModelIndex &modelIndex);

private:
    void prepareDialog();
    void showWidget();
    void hideWidget();

    void prepareSignals();

    void appendSignalToModel(const QList<QmlConnections> &connections,
                             ModelNode &node,
                             const PropertyName &signal,
                             const PropertyName &property = "");

    void addConnection(const QModelIndex &modelIndex);
    void removeConnection(const QModelIndex &modelIndex);

private:
    QPointer<SignalListDialog> m_dialog;
    SignalListModel *m_model;
    ModelNode m_modelNode;
};

} // QmlDesigner namespace
