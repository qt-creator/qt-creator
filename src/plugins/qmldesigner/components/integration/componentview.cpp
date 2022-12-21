// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "componentview.h"
#include "componentaction.h"

#include <nodemetainfo.h>

#include <QDebug>

#include <nodeabstractproperty.h>
#include <QStandardItemModel>

// silence gcc warnings about unused parameters

namespace QmlDesigner {

ComponentView::ComponentView(ExternalDependenciesInterface &externalDependencies)
    : AbstractView{externalDependencies}
    , m_standardItemModel(new QStandardItemModel(this))
    , m_componentAction(new ComponentAction(this))
{
}

void ComponentView::nodeAboutToBeRemoved(const ModelNode &removedNode)
{
    removeFromListRecursive(removedNode);
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

void ComponentView::removeNodeFromList(const ModelNode &node)
{
    for (int row = 0; row < m_standardItemModel->rowCount(); row++) {
        if (m_standardItemModel->item(row)->data(ModelNodeRole).toInt() == node.internalId())
            m_standardItemModel->removeRow(row);
    }
}

void ComponentView::addNodeToList(const ModelNode &node)
{
    if (hasEntryForNode(node))
        return;

    QString description = descriptionForNode(node);
    auto item = new QStandardItem(description);
    item->setData(QVariant::fromValue(node.internalId()), ModelNodeRole);
    item->setEditable(false);
    m_standardItemModel->appendRow(item);
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

void ComponentView::ensureMasterDocument()
{
    if (!hasMasterEntry()) {
        QStandardItem *item = new QStandardItem(QLatin1String("master"));
        item->setData(QVariant::fromValue(0), ModelNodeRole);
        item->setEditable(false);
        m_standardItemModel->appendRow(item);
    }
}

void ComponentView::maybeRemoveMasterDocument()
{
    int idx = indexOfMaster();
    if (idx >= 0 && m_standardItemModel->rowCount() == 1)
        m_standardItemModel->removeRow(idx);
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

bool ComponentView::isSubComponentNode(const ModelNode &node) const
{
    return node.nodeSourceType() == ModelNode::NodeWithComponentSource
            || (node.hasParentProperty()
                && !node.parentProperty().isDefaultProperty()
                && node.metaInfo().isValid()
                && node.metaInfo().isGraphicalItem());
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
    const auto nodeList = node.allSubModelNodesAndThisNode();
    bool hasMaster = false;
    for (const ModelNode &childNode : nodeList) {
        if (isSubComponentNode(childNode)) {
            if (!hasMaster) {
                hasMaster = true;
                ensureMasterDocument();
            }
            addNodeToList(childNode);
        }
    }
}

void ComponentView::removeFromListRecursive(const ModelNode &node)
{
    const auto nodeList = node.allSubModelNodesAndThisNode();
    for (const ModelNode &childNode : std::as_const(nodeList))
        removeNodeFromList(childNode);
    maybeRemoveMasterDocument();
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

void ComponentView::nodeSourceChanged(const ModelNode &node, const QString &/*newNodeSource*/)
{
    if (isSubComponentNode(node)) {
        if (!hasEntryForNode(node)) {
            ensureMasterDocument();
            addNodeToList(node);
        }
    } else {
        removeNodeFromList(node);
        maybeRemoveMasterDocument();
    }
}
} // namespace QmlDesigner
