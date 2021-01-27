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
