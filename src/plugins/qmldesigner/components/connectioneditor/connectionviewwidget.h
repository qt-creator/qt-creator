// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QFrame>
#include <QAbstractItemView>

QT_BEGIN_NAMESPACE
class QShortcut;
class QToolButton;
class QTableView;
class QListView;
QT_END_NAMESPACE

namespace QmlDesigner {

namespace Ui { class ConnectionViewWidget; }

class ActionEditor;
class BindingEditor;

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

    explicit ConnectionViewWidget(QWidget *parent = nullptr);
    ~ConnectionViewWidget() override;

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

    void bindingTableViewSelectionChanged(const QModelIndex &current, const QModelIndex &previous);
    void connectionTableViewSelectionChanged(const QModelIndex &current, const QModelIndex &previous);
    void dynamicPropertiesTableViewSelectionChanged(const QModelIndex &current, const QModelIndex &previous);
    void backendTableViewSelectionChanged(const QModelIndex &current, const QModelIndex &previous);

signals:
    void setEnabledAddButton(bool enabled);
    void setEnabledRemoveButton(bool enabled);

protected:
    void contextMenuEvent(QContextMenuEvent *event) override;

private:
    void handleTabChanged(int i);
    void removeButtonClicked();
    void addButtonClicked();

    //methods to prepare editors
    void editorForConnection();
    void editorForBinding();
    void editorForDynamic();

private:
    Ui::ConnectionViewWidget *ui;
    QmlDesigner::ActionEditor *m_connectionEditor; //editor for connections in connection view
    QmlDesigner::BindingEditor *m_bindingEditor; //editor for properties in binding view
    QmlDesigner::BindingEditor *m_dynamicEditor; //editor for properties in dynamic view

    QModelIndex m_bindingIndex;
    QModelIndex m_dynamicIndex;
};

} // namespace Internal

} // namespace QmlDesigner
