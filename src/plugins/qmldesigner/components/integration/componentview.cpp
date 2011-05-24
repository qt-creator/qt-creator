/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#include "componentview.h"
#include "componentaction.h"
#include <QtDebug>

#include <nodemetainfo.h>
#include <nodeabstractproperty.h>
#include <QStandardItemModel>

// silence gcc warnings about unused parameters

namespace QmlDesigner {

ComponentView::ComponentView(QObject *parent)
  : AbstractView(parent),
    m_standardItemModel(new QStandardItemModel(this)),
    m_listChanged(false),
    m_componentAction(new ComponentAction(this))
{
}



void ComponentView::nodeAboutToBeRemoved(const ModelNode &removedNode)
{
    for (int index = 1; index < m_standardItemModel->rowCount(); index++) {
        QStandardItem *item = m_standardItemModel->item(index);
        if (item->data(ModelNodeRole).value<ModelNode>() == removedNode)
            m_standardItemModel->removeRow(index);
    }
    searchForComponentAndRemoveFromList(removedNode);
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
    QStandardItem *item = new QStandardItem(tr("whole document"));
    item->setData(QVariant::fromValue(rootModelNode()), ModelNodeRole);
    item->setEditable(false);
    m_standardItemModel->appendRow(item);
}

void ComponentView::modelAttached(Model *model)
{
    if (AbstractView::model() == model)
        return;

    bool block = m_componentAction->blockSignals(true);
    m_standardItemModel->clear();

    AbstractView::modelAttached(model);

    Q_ASSERT(model->masterModel());
    appendWholeDocumentAsComponent();
    searchForComponentAndAddToList(rootModelNode());

    m_componentAction->blockSignals(block);
}

void ComponentView::modelAboutToBeDetached(Model *model)
{
    bool block = m_componentAction->blockSignals(true);
    m_standardItemModel->clear();
    AbstractView::modelAboutToBeDetached(model);
    m_componentAction->blockSignals(block);
}

ComponentAction *ComponentView::action()
{
    return m_componentAction;
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
        if (childNode.type() == "QtQuick.Component") {
            if (!childNode.id().isEmpty()) {
                QStandardItem *item = new QStandardItem(childNode.id());
                item->setData(QVariant::fromValue(childNode), ModelNodeRole);
                item->setEditable(false);
                m_standardItemModel->appendRow(item);
            } else {
                QString description;
                if (!childNode.parentProperty().parentModelNode().id().isEmpty())
                    description = childNode.parentProperty().parentModelNode().id() + QLatin1Char(' ');
                else
                    description = childNode.parentProperty().parentModelNode().simplifiedTypeName() + QLatin1Char(' ');
                description += childNode.parentProperty().name();
                QStandardItem *item = new QStandardItem(description);
                item->setData(QVariant::fromValue(childNode), ModelNodeRole);
                item->setEditable(false);
                m_standardItemModel->appendRow(item);
            }
        } else if (node.metaInfo().isValid() && node.metaInfo().isComponent() && !m_componentList.contains(node.type())) {
            m_componentList.append(node.type());
            m_componentList.sort();
            m_listChanged = true;
        }
    }
}

void ComponentView::nodeRemoved(const ModelNode & /* removedNode */, const NodeAbstractProperty & /*parentProperty*/, PropertyChangeFlags /*propertyChange*/)
{
}

void ComponentView::searchForComponentAndRemoveFromList(const ModelNode &node)
{
    QList<ModelNode> nodeList;
    nodeList.append(node);
    nodeList.append(node.allSubModelNodes());


    foreach (const ModelNode &childNode, nodeList) {
        if (childNode.type() == "QtQuick.Component") {
            if (!childNode.id().isEmpty()) {
                for (int row = 0; row < m_standardItemModel->rowCount(); row++) {
                    if (m_standardItemModel->item(row)->text() == childNode.id())
                        m_standardItemModel->removeRow(row);
                }
            }
        } else if (node.metaInfo().isComponent() && !m_componentList.contains(childNode.type())) {
            m_componentList.append(childNode.type());
            m_componentList.sort();
            m_listChanged = true;
        }
    }
}

void ComponentView::nodeAboutToBeReparented(const ModelNode &/*node*/, const NodeAbstractProperty &/*newPropertyParent*/, const NodeAbstractProperty &/*oldPropertyParent*/, AbstractView::PropertyChangeFlags /*propertyChange*/) {}
void ComponentView::nodeReparented(const ModelNode &/*node*/, const NodeAbstractProperty &/*newPropertyParent*/, const NodeAbstractProperty &/*oldPropertyParent*/, AbstractView::PropertyChangeFlags /*propertyChange*/) {}
void ComponentView::nodeIdChanged(const ModelNode& /*node*/, const QString& /*newId*/, const QString& /*oldId*/) {}
void ComponentView::propertiesAboutToBeRemoved(const QList<AbstractProperty>& /*propertyList*/) {}
void ComponentView::propertiesRemoved(const QList<AbstractProperty>& /*propertyList*/) {}
void ComponentView::variantPropertiesChanged(const QList<VariantProperty>& /*propertyList*/, PropertyChangeFlags /*propertyChange*/) {}
void ComponentView::bindingPropertiesChanged(const QList<BindingProperty>& /*propertyList*/, PropertyChangeFlags /*propertyChange*/) {}
void ComponentView::rootNodeTypeChanged(const QString &/*type*/, int /*majorVersion*/, int /*minorVersion*/) {}
void ComponentView::scriptFunctionsChanged(const ModelNode &/*node*/, const QStringList &/*scriptFunctionList*/) {}
void ComponentView::instancePropertyChange(const QList<QPair<ModelNode, QString> > &/*propertyList*/) {}
void ComponentView::instancesCompleted(const QVector<ModelNode> &/*completedNodeList*/) {}
void ComponentView::instanceInformationsChange(const QVector<ModelNode> &/*nodeList*/) {}
void ComponentView::instancesRenderImageChanged(const QVector<ModelNode> &/*nodeList*/) {}
void ComponentView::instancesPreviewImageChanged(const QVector<ModelNode> &/*nodeList*/) {}
void ComponentView::instancesChildrenChanged(const QVector<ModelNode> &/*nodeList*/) {}
void ComponentView::customParserSourceChanged(const ModelNode &, const QString & /*newCustomParserSource*/) {}

void ComponentView::rewriterBeginTransaction() {}
void ComponentView::rewriterEndTransaction() {}
void ComponentView::actualStateChanged(const ModelNode &/*node*/) {}
void ComponentView::selectedNodesChanged(const QList<ModelNode> &/*selectedNodeList*/,
                                  const QList<ModelNode> &/*lastSelectedNodeList*/) {}

void ComponentView::fileUrlChanged(const QUrl &/*oldUrl*/, const QUrl &/*newUrl*/) {}

void ComponentView::nodeOrderChanged(const NodeListProperty &/*listProperty*/, const ModelNode & /*movedNode*/, int /*oldIndex*/) {}


void ComponentView::auxiliaryDataChanged(const ModelNode &/*node*/, const QString &/*name*/, const QVariant &/*data*/) {}

void ComponentView::customNotification(const AbstractView * /*view*/, const QString &/*identifier*/, const QList<ModelNode> &/*nodeList*/, const QList<QVariant> &/*data*/) {}
void ComponentView::importsChanged(const QList<Import> &/*addedImports*/, const QList<Import> &/*removedImports*/) {}

} // namespace QmlDesigner
