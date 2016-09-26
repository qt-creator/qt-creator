/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include <QFrame>
#include <QAbstractItemView>

QT_BEGIN_NAMESPACE
class QToolButton;
class QTableView;
class QListView;
QT_END_NAMESPACE

namespace QmlDesigner {

namespace Ui { class ConnectionViewWidget; }

namespace Internal {

class BindingModel;
class ConnectionModel;
class DynamicPropertiesModel;
class BackendModel;

class ConnectionViewWidget : public QFrame
{
    Q_OBJECT

public:

    enum TabStatus {
        ConnectionTab,
        BindingTab,
        DynamicPropertiesTab,
        BackendTab,
        InvalidTab
    };

    explicit ConnectionViewWidget(QWidget *parent = 0);
    ~ConnectionViewWidget();

    void setBindingModel(BindingModel *model);
    void setConnectionModel(ConnectionModel *model);
    void setDynamicPropertiesModel(DynamicPropertiesModel *model);
    void setBackendModel(BackendModel *model);

    QList<QToolButton*> createToolBarWidgets();

    TabStatus currentTab() const;

    void resetItemViews();
    void invalidateButtonStatus();

    QTableView *connectionTableView() const;
    QTableView *bindingTableView() const;
    QTableView *dynamicPropertiesTableView() const;
    QTableView *backendView() const;

public slots:
    void bindingTableViewSelectionChanged(const QModelIndex &current, const QModelIndex &previous);
    void connectionTableViewSelectionChanged(const QModelIndex &current, const QModelIndex &previous);
    void dynamicPropertiesTableViewSelectionChanged(const QModelIndex &current, const QModelIndex &previous);
    void backendTableViewSelectionChanged(const QModelIndex &current, const QModelIndex &previous);

signals:
    void setEnabledAddButton(bool enabled);
    void setEnabledRemoveButton(bool enabled);

private slots:
    void handleTabChanged(int i);
    void removeButtonClicked();
    void addButtonClicked();

private:
    Ui::ConnectionViewWidget *ui;
};

} // namespace Internal

} // namespace QmlDesigner
