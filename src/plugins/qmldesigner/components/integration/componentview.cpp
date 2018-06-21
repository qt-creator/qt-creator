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

#include "componentview.h"
#include "componentaction.h"

#include <nodemetainfo.h>

#include <QDebug>

#include <nodeabstractproperty.h>
#include <QStandardItemModel>

// silence gcc warnings about unused parameters

namespace QmlDesigner {

ComponentView::ComponentView(QObject *parent)
  : AbstractView(parent),
    m_standardItemModel(new QStandardItemModel(this)),
    m_componentAction(new ComponentAction(this))
{
}

void ComponentView::nodeAboutToBeRemoved(const ModelNode &removedNode)
{
    removeSingleNodeFromList(removedNode);
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
        return modelNodeForInternalId(qint32(item->data(ModelNodeRole).toInt()));
    }

    return ModelNode();
}

void ComponentView::setComponentNode(const ModelNode &node)
{
    m_componentAction->setCurrentIndex(indexForNode(node));
}

void ComponentView::setComponentToMaster()
{
    m_componentAction->setCurrentIndex(indexOfMaster());
}

void ComponentView::removeSingleNodeFromList(const ModelNode &node)
{
    for (int row = 0; row < m_standardItemModel->rowCount(); row++) {
        if (m_standardItemModel->item(row)->data(ModelNodeRole).toInt() == node.internalId())
            m_standardItemModel->removeRow(row);
    }
}

int ComponentView::indexForNode(const ModelNode &node) const
{
    for (int row = 0; row < m_standardItemModel->rowCount(); row++) {
        if (m_standardItemModel->item(row)->data(ModelNodeRole).toInt() == node.internalId())
            return row;
    }
    return -1;
}

int ComponentView::indexOfMaster() const
{
    for (int row = 0; row < m_standardItemModel->rowCount(); row++) {
        if (m_standardItemModel->item(row)->data(ModelNodeRole).toInt() == 0)
            return row;
    }

    return -1;
}

bool ComponentView::hasMasterEntry() const
{
    return indexOfMaster() >= 0;
}

bool ComponentView::hasEntryForNode(const ModelNode &node) const
{
    return indexForNode(node) >= 0;
}

void ComponentView::addMasterDocument()
{
    if (!hasMasterEntry()) {
        QStandardItem *item = new QStandardItem(QLatin1String("master"));
        item->setData(QVariant::fromValue(0), ModelNodeRole);
        item->setEditable(false);
        m_standardItemModel->appendRow(item);
    }
}

void ComponentView::removeMasterDocument()
{
    m_standardItemModel->removeRow(indexOfMaster());
}

QString ComponentView::descriptionForNode(const ModelNode &node) const
{
    QString description;

    if (!node.id().isEmpty()) {
        description = node.id();
    } else if (node.hasParentProperty()) {
        ModelNode parentNode = node.parentProperty().parentModelNode();

        if (parentNode.id().isEmpty())
            description = parentNode.simplifiedTypeName() + ' ';
        else
            description = parentNode.id() + ' ';

        description += QString::fromUtf8(node.parentProperty().name());
    }

    return description;
}

void ComponentView::updateDescription(const ModelNode &node)
{
    int nodeIndex = indexForNode(node);

    if (nodeIndex > -1)
        m_standardItemModel->item(nodeIndex)->setText(descriptionForNode(node));
}

void ComponentView::modelAttached(Model *model)
{
    if (AbstractView::model() == model)
        return;

    QSignalBlocker blocker(m_componentAction);
    m_standardItemModel->clear();

    AbstractView::modelAttached(model);

    searchForComponentAndAddToList(rootModelNode());
}

void ComponentView::modelAboutToBeDetached(Model *model)
{
    QSignalBlocker blocker(m_componentAction);
    m_standardItemModel->clear();
    AbstractView::modelAboutToBeDetached(model);
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
    bool masterNotAdded = true;

    foreach (const ModelNode &node, node.allSubModelNodesAndThisNode()) {
        if (node.nodeSourceType() == ModelNode::NodeWithComponentSource
                || (node.hasParentProperty()
                    && !node.parentProperty().isDefaultProperty()
                    && node.metaInfo().isValid()
                    && node.metaInfo().isGraphicalItem())) {
            if (masterNotAdded) {
                masterNotAdded = true;
                addMasterDocument();
            }

            if (!hasEntryForNode(node)) {
                QString description = descriptionForNode(node);

                QStandardItem *item = new QStandardItem(description);
                item->setData(QVariant::fromValue(node.internalId()), ModelNodeRole);
                item->setEditable(false);
                removeSingleNodeFromList(node); //remove node if already present
                m_standardItemModel->appendRow(item);
            }
        }
    }
}

void ComponentView::searchForComponentAndRemoveFromList(const ModelNode &node)
{
    QList<ModelNode> nodeList;
    nodeList.append(node);
    nodeList.append(node.allSubModelNodes());


    foreach (const ModelNode &childNode, nodeList) {
        if (childNode.nodeSourceType() == ModelNode::NodeWithComponentSource)
            removeSingleNodeFromList(childNode);
    }

    if (m_standardItemModel->rowCount() == 1)
        removeMasterDocument();
}

void ComponentView::nodeReparented(const ModelNode &node, const NodeAbstractProperty &/*newPropertyParent*/, const NodeAbstractProperty &/*oldPropertyParent*/, AbstractView::PropertyChangeFlags /*propertyChange*/)
{
    searchForComponentAndAddToList(node);

    updateDescription(node);
}

void ComponentView::nodeIdChanged(const ModelNode& node, const QString& /*newId*/, const QString& /*oldId*/)
{
    updateDescription(node);
}
} // namespace QmlDesigner
