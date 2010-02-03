/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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

#include "navigatortreemodel.h"

#include <nodeabstractproperty.h>
#include <nodelistproperty.h>
#include <abstractview.h>
#include <qmlitemnode.h>
#include <invalididexception.h>

#include <QMimeData>

namespace QmlDesigner {

NavigatorTreeModel::NavigatorTreeModel(QObject *parent)
    : QStandardItemModel(parent),
      m_blockItemChangedSignal(false)
{
    invisibleRootItem()->setFlags(Qt::NoItemFlags);

    #ifdef _LOCK_ITEMS_
    setColumnCount(3);
    #else
    setColumnCount(2);
    #endif

    setSupportedDragActions(Qt::LinkAction);

    connect(this, SIGNAL(itemChanged(QStandardItem*)),
            this, SLOT(handleChangedItem(QStandardItem*)));
}

NavigatorTreeModel::~NavigatorTreeModel()
{
}

Qt::DropActions NavigatorTreeModel::supportedDropActions() const
{
    return Qt::LinkAction;
}

QStringList NavigatorTreeModel::mimeTypes() const
{
     QStringList types;
     types << "application/vnd.modelnode.list";
     return types;
}

QMimeData *NavigatorTreeModel::mimeData(const QModelIndexList &indexList) const
{
     QMimeData *mimeData = new QMimeData();
     QByteArray encodedData;

     QSet<QModelIndex> rowAlreadyUsedSet;

     QDataStream stream(&encodedData, QIODevice::WriteOnly);

     foreach (const QModelIndex &index, indexList) {
         if (!index.isValid())
             continue;
         QModelIndex idIndex = index.sibling(index.row(), 0);
         if (rowAlreadyUsedSet.contains(idIndex))
             continue;

         rowAlreadyUsedSet.insert(idIndex);
         stream << idIndex.data(Qt::UserRole).toUInt();
     }

     mimeData->setData("application/vnd.modelnode.list", encodedData);

     return mimeData;
}

static bool isAnchestorInList(const ModelNode &node, const QList<ModelNode> &nodeList)
{
    foreach (const ModelNode &nodeInList, nodeList) {
        if (nodeInList.isAncestorOf(node))
            return true;
    }

    return false;
}

bool NavigatorTreeModel::dropMimeData(const QMimeData *data,
                  Qt::DropAction action,
                  int row,
                  int column,
                  const QModelIndex &parentIndex)
{
    if (action == Qt::IgnoreAction)
        return true;
    if (action != Qt::LinkAction)
        return false;
    if (!data->hasFormat("application/vnd.modelnode.list"))
        return false;
    if (column > 1)
        return false;
    if (parentIndex.model() != this)
        return false;


    QModelIndex parentIdIndex = parentIndex;
    parentIdIndex = parentIdIndex.sibling(parentIdIndex.row(), 0);

    Q_ASSERT(parentIdIndex.isValid());

    int targetIndex = 0;
    if (row > -1) {
        targetIndex = row;
    } else {
        targetIndex = rowCount(parentIdIndex);
    }

    QByteArray encodedData = data->data("application/vnd.modelnode.list");
    QDataStream stream(&encodedData, QIODevice::ReadOnly);

    QmlItemNode parentItemNode(nodeForIndex(parentIdIndex));

    QList<ModelNode> nodeList;

    while (!stream.atEnd()) {
        uint nodeHash;
        stream >> nodeHash;
        ModelNode node(nodeForHash(nodeHash));
        if (!node.isValid() || (parentItemNode == node) || node.isAncestorOf(parentItemNode))
            continue;
        nodeList.append(node);
    }

    RewriterTransaction transaction = m_view->beginRewriterTransaction();
    foreach (const ModelNode &node, nodeList) {
        if (!isAnchestorInList(node, nodeList)) {
            if (node.parentProperty().parentModelNode() != parentItemNode) {
                QmlItemNode itemNode(node);
                if (node != parentItemNode) {
                    itemNode.setParent(parentItemNode);
                }
            }

            if (node.parentProperty().isNodeListProperty()) {
                int index = node.parentProperty().toNodeListProperty().toModelNodeList().indexOf(node);
                if (index < targetIndex) { // item is first removed from oldIndex, then inserted at new index
                    --targetIndex;
                }
                if (index != targetIndex) {
                    node.parentProperty().toNodeListProperty().slide(index, targetIndex);
                }
            }
        }
    }

    propagateInvisible(parentItemNode, isNodeInvisible(parentItemNode));

    return false; // don't let the view do drag&drop on its own
}

NavigatorTreeModel::ItemRow NavigatorTreeModel::createItemRow(const ModelNode &node)
{
    Q_ASSERT(node.isValid());

    uint hash = qHash(node);

    QStandardItem *idItem = new QStandardItem;
    idItem->setDragEnabled(true);
    idItem->setDropEnabled(node.metaInfo().isContainer());
    idItem->setEditable(true);
    idItem->setData(hash, Qt::UserRole);

    #ifdef _LOCK_ITEMS_
    QStandardItem *lockItem = new QStandardItem;
    lockItem->setDragEnabled(true);
    lockItem->setDropEnabled(node.metaInfo().isContainer());
    lockItem->setEditable(false);
    lockItem->setCheckable(true);
    lockItem->setData(hash, Qt::UserRole);
    #endif

    QStandardItem *visibilityItem = new QStandardItem;
    visibilityItem->setDropEnabled(node.metaInfo().isContainer());
    visibilityItem->setCheckable(true);
    visibilityItem->setEditable(false);
    visibilityItem->setData(hash, Qt::UserRole);

    #ifdef _LOCK_ITEMS_
    return ItemRow(idItem, lockItem, visibilityItem);
    #else
    return ItemRow(idItem, visibilityItem);
    #endif
}

void NavigatorTreeModel::updateItemRow(const ModelNode &node, ItemRow items)
{
    bool blockSignal = blockItemChangedSignal(true);

    items.idItem->setText(node.id());
    //items.idItem->setToolTip(!node.id().isEmpty()?node.type():"");
    items.idItem->setToolTip(node.type());
    items.visibilityItem->setCheckState(node.auxiliaryData("invisible").toBool() ? Qt::Unchecked : Qt::Checked);

    blockItemChangedSignal(blockSignal);
}

/**
  Update the information shown for a node / property
  */
void NavigatorTreeModel::updateItemRow(const ModelNode &node)
{
    if (!containsNode(node))
        return;

    updateItemRow(node, itemRowForNode(node));
}

/**
  Updates the sibling position of the item, depending on the position in the model.
  */
void NavigatorTreeModel::updateItemRowOrder(const ModelNode &node)
{
    if (!containsNode(node))
        return;

    ItemRow itemRow = itemRowForNode(node);
    int currentRow = itemRow.idItem->row();
    int newRow = currentRow;
    if (node.parentProperty().parentModelNode().isValid())
        newRow = modelNodeChildren(node.parentProperty().parentModelNode()).indexOf(node);
    Q_ASSERT(newRow >= 0);

    if (currentRow != newRow) {
        QStandardItem *parentIdItem = itemRow.idItem->parent();
        QList<QStandardItem*> items = parentIdItem->takeRow(currentRow);
        parentIdItem->insertRow(newRow, items);
    }
}

void NavigatorTreeModel::handleChangedItem(QStandardItem *item)
{
    if (m_blockItemChangedSignal)
        return;

    uint nodeHash = item->data(Qt::UserRole).toUInt();
    Q_ASSERT(nodeHash && containsNodeHash(nodeHash));
    ModelNode node = nodeForHash(nodeHash);

    ItemRow itemRow = itemRowForNode(node);
    if (item == itemRow.idItem) {
        try {
            if (ModelNode::isValidId(item->text()))
                node.setId(item->text());
            else
                item->setText(node.id());
        } catch (InvalidIdException &) {
            item->setText(node.id());
        }
    } else if (item == itemRow.visibilityItem) {
        bool invisible = (item->checkState() == Qt::Unchecked);

        node.setAuxiliaryData("invisible", invisible);
        propagateInvisible(node,isNodeInvisible(node));
    }
}

void NavigatorTreeModel::propagateInvisible(const ModelNode &node, const bool &invisible)
{
    QList <ModelNode> children = node.allDirectSubModelNodes();
    foreach (ModelNode child, children) {
        child.setAuxiliaryData("childOfInvisible",invisible);
        if (!child.auxiliaryData("invisible").toBool())
            propagateInvisible(child,invisible);
    }
}

ModelNode NavigatorTreeModel::nodeForHash(uint hash) const
{
    ModelNode node = m_nodeHash.value(hash);
    Q_ASSERT(node.isValid());
    return node;
}

bool NavigatorTreeModel::containsNodeHash(uint hash) const
{
    return m_nodeHash.contains(hash);
}

bool NavigatorTreeModel::containsNode(const ModelNode &node) const
{
    return m_nodeItemHash.contains(node);
}

NavigatorTreeModel::ItemRow NavigatorTreeModel::itemRowForNode(const ModelNode &node)
{
    Q_ASSERT(node.isValid());
    return m_nodeItemHash.value(node);
}

void NavigatorTreeModel::setView(AbstractView *view)
{
    m_view = view;
    addSubTree(view->rootModelNode());
}

void NavigatorTreeModel::clearView()
{
    m_view.clear();
    m_nodeHash.clear();
    m_nodeItemHash.clear();
    clear();
}

QModelIndex NavigatorTreeModel::indexForNode(const ModelNode &node) const
{
    Q_ASSERT(node.isValid());
    if (!containsNode(node))
        return QModelIndex();
    ItemRow row = m_nodeItemHash.value(node);
    return row.idItem->index();
}

ModelNode NavigatorTreeModel::nodeForIndex(const QModelIndex &index) const
{
    Q_ASSERT(index.isValid());
    uint hash = index.data(Qt::UserRole).toUInt();
    Q_ASSERT(hash);
    Q_ASSERT(containsNodeHash(hash));
    return nodeForHash(hash);
}

bool NavigatorTreeModel::isInTree(const ModelNode &node) const
{
    return m_nodeHash.keys().contains(qHash(node));
}

bool NavigatorTreeModel::isNodeInvisible(const QModelIndex &index) const
{
    return isNodeInvisible(nodeForIndex(index));
}

bool NavigatorTreeModel::isNodeInvisible(const ModelNode &node) const
{
    bool nodeInvisible = node.auxiliaryData("invisible").toBool();
    if (node.hasAuxiliaryData("childOfInvisible"))
        nodeInvisible = nodeInvisible || node.auxiliaryData("childOfInvisible").toBool();
    return nodeInvisible;
}

/**
  Adds node & all children to the visible tree hierarchy (if node should be visible at all).

  It always adds the node to the _end_ of the list of items.
  */
void NavigatorTreeModel::addSubTree(const ModelNode &node)
{
    Q_ASSERT(node.isValid());
    Q_ASSERT(!containsNodeHash(qHash(node)));

    // only add items that are in the modelNodeChildren list (that means, visible in the editor)
    if (!node.isRootNode()
        && !modelNodeChildren(node.parentProperty().parentModelNode()).contains(node)) {
        return;
    }

    ItemRow newRow = createItemRow(node);
    m_nodeHash.insert(qHash(node), node);
    m_nodeItemHash.insert(node, newRow);

    updateItemRow(node, newRow);

    foreach (const ModelNode &childNode, modelNodeChildren(node))
        addSubTree(childNode);

    // We assume that the node is always added to the _end_ of the property list.
    if (node.hasParentProperty()) {
        ItemRow parentRow = itemRowForNode(node.parentProperty().parentModelNode());
        if (parentRow.idItem)
            parentRow.idItem->appendRow(newRow.toList());
    } else {
        appendRow(newRow.toList());
    }
}

/**
  Deletes visual representation for the node (subtree).
  */
void NavigatorTreeModel::removeSubTree(const ModelNode &node)
{
    Q_ASSERT(node.isValid());

    if (!containsNode(node))
        return;

    QList<QStandardItem*> rowList;
    ItemRow itemRow = itemRowForNode(node);
    if (itemRow.idItem->parent()) {
        rowList = itemRow.idItem->parent()->takeRow(itemRow.idItem->row());
    }

    foreach (const ModelNode &node, modelNodeChildren(node)) {
        removeSubTree(node);
    }

    qDeleteAll(rowList);

    m_nodeHash.remove(qHash(node));
    m_nodeItemHash.remove(node);
}

QList<ModelNode> NavigatorTreeModel::modelNodeChildren(const ModelNode &parentNode)
{
    QList<ModelNode> children;
    if (QmlItemNode(parentNode).isValid())
        foreach (const QmlItemNode &childNode, QmlItemNode(parentNode).children())
            children << childNode.modelNode();
    return children;
}

// along the lines of QObject::blockSignals
bool NavigatorTreeModel::blockItemChangedSignal(bool block)
{
    bool oldValue = m_blockItemChangedSignal;
    m_blockItemChangedSignal = block;
    return oldValue;
}


}

