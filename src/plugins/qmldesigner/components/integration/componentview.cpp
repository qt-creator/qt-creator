/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "componentview.h"
#include <QtDebug>

#include <nodemetainfo.h>
#include <QStandardItemModel>

// silence gcc warnings about unused parameters

namespace QmlDesigner {

ComponentView::ComponentView(QObject *parent)
  : AbstractView(parent),
    m_standardItemModel(new QStandardItemModel(this)),
    m_listChanged(false)
{
}



void ComponentView::nodeAboutToBeRemoved(const ModelNode &removedNode)
{
    for (int index = 1; index < m_standardItemModel->rowCount(); index++) {
        QStandardItem *item = m_standardItemModel->item(index);
        if (item->data(ModelNodeRole).value<ModelNode>() == removedNode)
            m_standardItemModel->removeRow(index);
    }
}

QStandardItemModel *ComponentView::standardItemModel() const
{
    return m_standardItemModel;
}

ModelNode ComponentView::modelNode(int index) const
{
    if (m_standardItemModel->hasIndex(index, 0)) {
        QStandardItem *item = m_standardItemModel->item(index, 0);
        return item->data(ModelNodeRole).value<ModelNode>();
    }

    return ModelNode();
}

void ComponentView::appendWholeDocumentAsComponent()
{
    QStandardItem *item = new QStandardItem("Whole Document");
    item->setData(QVariant::fromValue(rootModelNode()), ModelNodeRole);
    item->setEditable(false);
    m_standardItemModel->appendRow(item);
}

void ComponentView::modelAttached(Model *model)
{
    if (AbstractView::model() == model)
        return;

    AbstractView::modelAttached(model);

    Q_ASSERT(model->masterModel());
    appendWholeDocumentAsComponent();
    searchForComponentAndAddToList(rootModelNode());
}

void ComponentView::modelAboutToBeDetached(Model *model)
{
    m_standardItemModel->clear();
    AbstractView::modelAboutToBeDetached(model);
}

void ComponentView::nodeCreated(const ModelNode &createdNode)
{
    searchForComponentAndAddToList(createdNode);
}

void ComponentView::searchForComponentAndAddToList(const ModelNode &node)
{
    QList<ModelNode> nodeList;
    nodeList.append(node);
    nodeList.append(node.allSubModelNodes());


    foreach (const ModelNode &childNode, nodeList) {
        if (childNode.type() == "Qt/Component") {
            if (!childNode.id().isEmpty()) {
                QStandardItem *item = new QStandardItem(childNode.id());
                item->setData(QVariant::fromValue(childNode), ModelNodeRole);
                item->setEditable(false);
                m_standardItemModel->appendRow(item);
            }
        } else if (node.metaInfo().isComponent() && !m_componentList.contains(node.type())) {
            m_componentList.append(node.type());
            m_componentList.sort();
            m_listChanged = true;
    }
    }
}

void ComponentView::nodeRemoved(const ModelNode & /*removedNode*/, const NodeAbstractProperty & /*parentProperty*/, PropertyChangeFlags /*propertyChange*/)
{

}

//void ComponentView::searchForComponentAndRemoveFromList(const ModelNode &node)
//{
//    QList<ModelNode> nodeList;
//    nodeList.append(node);
//    nodeList.append(node.allSubModelNodes());
//
//
//    foreach (const ModelNode &childNode, nodeList) {
//        if (node.type() == "Qt/Component") {
//            if (!node.id().isEmpty()) {
//                for(int row = 0; row < m_standardItemModel->rowCount(); row++) {
//                    if (m_standardItemModel->item(row)->text() == node.id())
//                        m_standardItemModel->removeRow(row);
//                }
//            }
//        } else if (node.metaInfo().isComponent() && !m_componentList.contains(node.type())) {
//            m_componentList.append(node.type());
//            m_componentList.sort();
//            m_listChanged = true;
//    }
//    }
//}

void ComponentView::nodeReparented(const ModelNode &/*node*/, const NodeAbstractProperty &/*newPropertyParent*/, const NodeAbstractProperty &/*oldPropertyParent*/, AbstractView::PropertyChangeFlags /*propertyChange*/) {}
void ComponentView::nodeIdChanged(const ModelNode& /*node*/, const QString& /*newId*/, const QString& /*oldId*/) {}
void ComponentView::propertiesAboutToBeRemoved(const QList<AbstractProperty>& /*propertyList*/) {}
void ComponentView::propertiesRemoved(const QList<AbstractProperty>& /*propertyList*/) {}
void ComponentView::variantPropertiesChanged(const QList<VariantProperty>& /*propertyList*/, PropertyChangeFlags /*propertyChange*/) {}
void ComponentView::bindingPropertiesChanged(const QList<BindingProperty>& /*propertyList*/, PropertyChangeFlags /*propertyChange*/) {}
void ComponentView::rootNodeTypeChanged(const QString &/*type*/, int /*majorVersion*/, int /*minorVersion*/) {}



void ComponentView::selectedNodesChanged(const QList<ModelNode> &/*selectedNodeList*/,
                                  const QList<ModelNode> &/*lastSelectedNodeList*/) {}

void ComponentView::fileUrlChanged(const QUrl &/*oldUrl*/, const QUrl &/*newUrl*/) {}

void ComponentView::nodeOrderChanged(const NodeListProperty &/*listProperty*/, const ModelNode & /*movedNode*/, int /*oldIndex*/) {}

void ComponentView::importsChanged() {}

void ComponentView::auxiliaryDataChanged(const ModelNode &/*node*/, const QString &/*name*/, const QVariant &/*data*/) {}

void ComponentView::customNotification(const AbstractView * /*view*/, const QString &/*identifier*/, const QList<ModelNode> &/*nodeList*/, const QList<QVariant> &/*data*/) {}


} // namespace QmlDesigner
