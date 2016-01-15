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

#include "connectionviewwidget.h"
#include "connectionview.h"
#include "ui_connectionviewwidget.h"

#include "bindingmodel.h"
#include "connectionmodel.h"
#include "dynamicpropertiesmodel.h"

#include <coreplugin/coreconstants.h>
#include <utils/fileutils.h>

#include <QToolButton>

namespace QmlDesigner {

namespace Internal {

ConnectionViewWidget::ConnectionViewWidget(QWidget *parent) :
    QFrame(parent),
    ui(new Ui::ConnectionViewWidget)
{

    setWindowTitle(tr("Connections", "Title of connection view"));
    ui->setupUi(this);

    setStyleSheet(QLatin1String(Utils::FileReader::fetchQrc(QLatin1String(":/connectionview/stylesheet.css"))));

    //ui->tabWidget->tabBar()->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    ui->tabBar->addTab(tr("Connections", "Title of connection view"));
    ui->tabBar->addTab(tr("Bindings", "Title of connection view"));
    ui->tabBar->addTab(tr("Dynamic Properties", "Title of dynamic properties view"));
    ui->tabBar->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    ui->connectionView->setStyleSheet(
            QLatin1String(Utils::FileReader::fetchQrc(QLatin1String(":/qmldesigner/scrollbar.css"))));

    ui->bindingView->setStyleSheet(
            QLatin1String(Utils::FileReader::fetchQrc(QLatin1String(":/qmldesigner/scrollbar.css"))));

    connect(ui->tabBar, SIGNAL(currentChanged(int)),
            ui->stackedWidget, SLOT(setCurrentIndex(int)));

    connect(ui->tabBar, SIGNAL(currentChanged(int)),
            this, SLOT(handleTabChanged(int)));

    ui->stackedWidget->setCurrentIndex(0);
}

ConnectionViewWidget::~ConnectionViewWidget()
{
    delete ui;
}

void ConnectionViewWidget::setBindingModel(BindingModel *model)
{
    ui->bindingView->setModel(model);
    ui->bindingView->verticalHeader()->hide();
    ui->bindingView->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->bindingView->setItemDelegate(new BindingDelegate);
    connect(ui->bindingView->selectionModel(), SIGNAL(currentRowChanged(QModelIndex,QModelIndex)),
            this, SLOT(bindingTableViewSelectionChanged(QModelIndex,QModelIndex)));
}

void ConnectionViewWidget::setConnectionModel(ConnectionModel *model)
{
    ui->connectionView->setModel(model);
    ui->connectionView->verticalHeader()->hide();
    ui->connectionView->horizontalHeader()->setDefaultSectionSize(160);
    ui->connectionView->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->connectionView->setItemDelegate(new ConnectionDelegate);
    connect(ui->connectionView->selectionModel(), SIGNAL(currentRowChanged(QModelIndex,QModelIndex)),
            this, SLOT(connectionTableViewSelectionChanged(QModelIndex,QModelIndex)));

}

void ConnectionViewWidget::setDynamicPropertiesModelModel(DynamicPropertiesModel *model)
{
    ui->dynamicPropertiesView->setModel(model);
    ui->dynamicPropertiesView->verticalHeader()->hide();
    ui->dynamicPropertiesView->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->dynamicPropertiesView->setItemDelegate(new DynamicPropertiesDelegate);
    connect(ui->dynamicPropertiesView->selectionModel(), SIGNAL(currentRowChanged(QModelIndex,QModelIndex)),
            this, SLOT(dynamicPropertiesTableViewSelectionChanged(QModelIndex,QModelIndex)));
}

QList<QToolButton *> ConnectionViewWidget::createToolBarWidgets()
{
    QList<QToolButton *> buttons;

    buttons << new QToolButton();
    buttons.last()->setIcon(QIcon(QStringLiteral(":/connectionview/plus.png")));
    buttons.last()->setToolTip(tr("Add binding or connection."));
    connect(buttons.last(), SIGNAL(clicked()), this, SLOT(addButtonClicked()));
    connect(this, SIGNAL(setEnabledAddButtonChanged(bool)), buttons.last(), SLOT(setEnabled(bool)));

    buttons << new QToolButton();
    buttons.last()->setIcon(QIcon(QStringLiteral(":/connectionview/minus.png")));
    buttons.last()->setToolTip(tr("Remove selected binding or connection."));
    buttons.last()->setShortcut(QKeySequence(Qt::Key_Delete));
    connect(buttons.last(), SIGNAL(clicked()), this, SLOT(removeButtonClicked()));
    connect(this, SIGNAL(setEnabledRemoveButtonChanged(bool)), buttons.last(), SLOT(setEnabled(bool)));

    return buttons;
}

void ConnectionViewWidget::setEnabledAddButton(bool enabled)
{
    emit setEnabledAddButtonChanged(enabled);
}

void ConnectionViewWidget::setEnabledRemoveButton(bool enabled)
{
    emit setEnabledRemoveButtonChanged(enabled);
}

ConnectionViewWidget::TabStatus ConnectionViewWidget::currentTab() const
{
    switch (ui->stackedWidget->currentIndex()) {
    case 0: return ConnectionTab;
    case 1: return BindingTab;
    case 2: return DynamicPropertiesTab;
    default: return InvalidTab;
    }
}

void ConnectionViewWidget::resetItemViews()
{
    if (currentTab() == ConnectionTab) {
        ui->connectionView->selectionModel()->clear();

    } else if (currentTab() == BindingTab) {
        ui->bindingView->selectionModel()->clear();

    } else if (currentTab() == DynamicPropertiesTab) {
        ui->dynamicPropertiesView->selectionModel()->clear();
    }
    invalidateButtonStatus();
}

void ConnectionViewWidget::invalidateButtonStatus()
{
    if (currentTab() == ConnectionTab) {
        setEnabledRemoveButton(ui->connectionView->selectionModel()->hasSelection());
        setEnabledAddButton(true);
    } else if (currentTab() == BindingTab) {
        setEnabledRemoveButton(ui->bindingView->selectionModel()->hasSelection());
        BindingModel *bindingModel = qobject_cast<BindingModel*>(ui->bindingView->model());
        setEnabledAddButton(bindingModel->connectionView()->model() &&
            bindingModel->connectionView()->selectedModelNodes().count() == 1);

    } else if (currentTab() == DynamicPropertiesTab) {
        setEnabledRemoveButton(ui->dynamicPropertiesView->selectionModel()->hasSelection());
        DynamicPropertiesModel *dynamicPropertiesModel = qobject_cast<DynamicPropertiesModel*>(ui->dynamicPropertiesView->model());
        setEnabledAddButton(dynamicPropertiesModel->connectionView()->model() &&
            dynamicPropertiesModel->connectionView()->selectedModelNodes().count() == 1);
    }
}

QTableView *ConnectionViewWidget::connectionTableView() const
{
    return ui->connectionView;
}

QTableView *ConnectionViewWidget::bindingTableView() const
{
    return ui->bindingView;
}

QTableView *ConnectionViewWidget::dynamicPropertiesTableView() const
{
    return ui->dynamicPropertiesView;
}

void ConnectionViewWidget::handleTabChanged(int)
{
    invalidateButtonStatus();
}

void ConnectionViewWidget::removeButtonClicked()
{
    if (currentTab() == ConnectionTab) {
        int currentRow =  ui->connectionView->selectionModel()->selectedRows().first().row();
        ConnectionModel *connectionModel = qobject_cast<ConnectionModel*>(ui->connectionView->model());
        if (connectionModel) {
            connectionModel->deleteConnectionByRow(currentRow);
        }
    } else if (currentTab() == BindingTab) {
        int currentRow =  ui->bindingView->selectionModel()->selectedRows().first().row();
        BindingModel *bindingModel = qobject_cast<BindingModel*>(ui->bindingView->model());
        if (bindingModel) {
            bindingModel->deleteBindindByRow(currentRow);
        }
    } else if (currentTab() == DynamicPropertiesTab) {
        int currentRow =  ui->dynamicPropertiesView->selectionModel()->selectedRows().first().row();
        DynamicPropertiesModel *dynamicPropertiesModel = qobject_cast<DynamicPropertiesModel*>(ui->dynamicPropertiesView->model());
        if (dynamicPropertiesModel)
            dynamicPropertiesModel->deleteDynamicPropertyByRow(currentRow);
    }

    invalidateButtonStatus();
}

void ConnectionViewWidget::addButtonClicked()
{

    if (currentTab() == ConnectionTab) {
        ConnectionModel *connectionModel = qobject_cast<ConnectionModel*>(ui->connectionView->model());
        if (connectionModel) {
            connectionModel->addConnection();
        }
    } else if (currentTab() == BindingTab) {
        BindingModel *bindingModel = qobject_cast<BindingModel*>(ui->bindingView->model());
        if (bindingModel) {
            bindingModel->addBindingForCurrentNode();
        }

    } else if (currentTab() == DynamicPropertiesTab) {
        DynamicPropertiesModel *dynamicPropertiesModel = qobject_cast<DynamicPropertiesModel*>(ui->dynamicPropertiesView->model());
        if (dynamicPropertiesModel)
            dynamicPropertiesModel->addDynamicPropertyForCurrentNode();
    }

    invalidateButtonStatus();
}

void ConnectionViewWidget::bindingTableViewSelectionChanged(const QModelIndex &current, const QModelIndex & /*previous*/)
{
    if (currentTab() == BindingTab) {
        if (current.isValid()) {
            setEnabledRemoveButton(true);
        } else {
            setEnabledRemoveButton(false);
        }
    }
}

void ConnectionViewWidget::connectionTableViewSelectionChanged(const QModelIndex &current, const QModelIndex & /*previous*/)
{
    if (currentTab() == ConnectionTab) {
        if (current.isValid()) {
            setEnabledRemoveButton(true);
        } else {
            setEnabledRemoveButton(false);
        }
    }
}

void ConnectionViewWidget::dynamicPropertiesTableViewSelectionChanged(const QModelIndex &current, const QModelIndex & /*previous*/)
{
    if (currentTab() == DynamicPropertiesTab) {
        if (current.isValid()) {
            setEnabledRemoveButton(true);
        } else {
            setEnabledRemoveButton(false);
        }
    }
}

} // namespace Internal

} // namespace QmlDesigner
