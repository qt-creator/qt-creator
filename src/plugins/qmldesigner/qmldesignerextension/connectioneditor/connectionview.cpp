/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
****************************************************************************/
#include "connectionview.h"
#include "connectionviewwidget.h"

#include "bindingmodel.h"
#include "connectionmodel.h"
#include "dynamicpropertiesmodel.h"

#include <bindingproperty.h>
#include <nodeabstractproperty.h>
#include <variantproperty.h>

namespace QmlDesigner {

namespace  Internal {

ConnectionView::ConnectionView(QObject *parent) : AbstractView(parent),
    m_connectionViewWidget(new ConnectionViewWidget()),
    m_connectionModel(new ConnectionModel(this)),
    m_bindingModel(new BindingModel(this)),
    m_dynamicPropertiesModel(new DynamicPropertiesModel(this))
{
    connectionViewWidget()->setBindingModel(m_bindingModel);
    connectionViewWidget()->setConnectionModel(m_connectionModel);
    connectionViewWidget()->setDynamicPropertiesModelModel(m_dynamicPropertiesModel);
}

ConnectionView::~ConnectionView()
{
}

void ConnectionView::modelAttached(Model *model)
{
    AbstractView::modelAttached(model);
    bindingModel()->selectionChanged(QList<ModelNode>());
    dynamicPropertiesModel()->selectionChanged(QList<ModelNode>());
    connectionModel()->resetModel();
    connectionViewWidget()->resetItemViews();
}

void ConnectionView::modelAboutToBeDetached(Model *model)
{
    AbstractView::modelAboutToBeDetached(model);
    bindingModel()->selectionChanged(QList<ModelNode>());
    dynamicPropertiesModel()->selectionChanged(QList<ModelNode>());
    connectionModel()->resetModel();
    connectionViewWidget()->resetItemViews();
}

void ConnectionView::nodeCreated(const ModelNode & /*createdNode*/)
{
//bindings
    connectionModel()->resetModel();
}

void ConnectionView::nodeRemoved(const ModelNode & /*removedNode*/,
                                 const NodeAbstractProperty & /*parentProperty*/,
                                 AbstractView::PropertyChangeFlags /*propertyChange*/)
{
     connectionModel()->resetModel();
}

void ConnectionView::nodeReparented(const ModelNode & /*node*/, const NodeAbstractProperty & /*newPropertyParent*/,
                               const NodeAbstractProperty & /*oldPropertyParent*/, AbstractView::PropertyChangeFlags /*propertyChange*/)
{
    connectionModel()->resetModel();
}

void ConnectionView::nodeIdChanged(const ModelNode & /*node*/, const QString & /*newId*/, const QString & /*oldId*/)
{
    connectionModel()->resetModel();
}

void ConnectionView::propertiesAboutToBeRemoved(const QList<AbstractProperty> & propertyList)
{
    foreach (const AbstractProperty &property, propertyList) {
        if (property.isBindingProperty()) {
            bindingModel()->bindingRemoved(property.toBindingProperty());
            dynamicPropertiesModel()->bindingRemoved(property.toBindingProperty());
        } else if (property.isVariantProperty()) {
            //### dynamicPropertiesModel->bindingRemoved(property.toVariantProperty());
        }
    }
}

void ConnectionView::variantPropertiesChanged(const QList<VariantProperty> &propertyList,
                                         AbstractView::PropertyChangeFlags /*propertyChange*/)
{
    foreach (const VariantProperty &variantProperty, propertyList) {
        if (variantProperty.isDynamic())
            dynamicPropertiesModel()->variantPropertyChanged(variantProperty);

        connectionModel()->variantPropertyChanged(variantProperty);
    }

}

void ConnectionView::bindingPropertiesChanged(const QList<BindingProperty> &propertyList,
                                         AbstractView::PropertyChangeFlags /*propertyChange*/)
{
    foreach (const BindingProperty &bindingProperty, propertyList) {
        bindingModel()->bindingChanged(bindingProperty);
        if (bindingProperty.isDynamic())
            dynamicPropertiesModel()->bindingPropertyChanged(bindingProperty);

        connectionModel()->bindingPropertyChanged(bindingProperty);
    }
}

void ConnectionView::selectedNodesChanged(const QList<ModelNode> & selectedNodeList,
                                     const QList<ModelNode> & /*lastSelectedNodeList*/)
{
    bindingModel()->selectionChanged(selectedNodeList);
    dynamicPropertiesModel()->selectionChanged(selectedNodeList);
    connectionViewWidget()->bindingTableViewSelectionChanged(QModelIndex(), QModelIndex());
    connectionViewWidget()->dynamicPropertiesTableViewSelectionChanged(QModelIndex(), QModelIndex());

    if (connectionViewWidget()->currentTab() == ConnectionViewWidget::BindingTab
            || connectionViewWidget()->currentTab() == ConnectionViewWidget::DynamicPropertiesTab)
        connectionViewWidget()->setEnabledAddButton(selectedNodeList.count() == 1);
}

WidgetInfo ConnectionView::widgetInfo()
{
    return createWidgetInfo(m_connectionViewWidget.data(),
                            new WidgetInfo::ToolBarWidgetDefaultFactory<ConnectionViewWidget>(connectionViewWidget()),
                            QLatin1String("ConnectionView"),
                            WidgetInfo::LeftPane,
                            0,
                            tr("Connection View"));
}

bool ConnectionView::hasWidget() const
{
    return true;
}

QTableView *ConnectionView::connectionTableView() const
{
    return connectionViewWidget()->connectionTableView();
}

QTableView *ConnectionView::bindingTableView() const
{
    return connectionViewWidget()->bindingTableView();
}

QTableView *ConnectionView::dynamicPropertiesTableView() const
{
    return connectionViewWidget()->dynamicPropertiesTableView();
}

ConnectionViewWidget *ConnectionView::connectionViewWidget() const
{
    return m_connectionViewWidget.data();
}

ConnectionModel *ConnectionView::connectionModel() const
{
    return m_connectionModel;
}

BindingModel *ConnectionView::bindingModel() const
{
    return m_bindingModel;
}

DynamicPropertiesModel *ConnectionView::dynamicPropertiesModel() const
{
    return m_dynamicPropertiesModel;
}

} // namesapce Internal

} // namespace QmlDesigner
