/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "navigatortreemodel.h"

#include <nodeabstractproperty.h>
#include <nodelistproperty.h>
#include <nodeproperty.h>
#include <variantproperty.h>
#include <metainfo.h>
#include <qgraphicswidget.h>
#include <qmlmodelview.h>
#include <rewriterview.h>
#include <invalididexception.h>
#include <rewritingexception.h>
#include <modelnodecontextmenu.h>

#include <QMimeData>
#include <QMessageBox>
#include <QApplication>
#include <QTransform>
#include <QPointF>

static inline void setScenePos(const QmlDesigner::ModelNode &modelNode,const QPointF &pos)
{
    QmlDesigner::QmlItemNode parentNode = modelNode.parentProperty().parentQmlObjectNode().toQmlItemNode();
    if (parentNode.isValid()) {
        QPointF localPos = parentNode.instanceSceneTransform().inverted().map(pos);
        modelNode.variantProperty(QLatin1String("x")) = localPos.toPoint().x();
        modelNode.variantProperty(QLatin1String("y")) = localPos.toPoint().y();
    }
}

namespace QmlDesigner {

const int NavigatorTreeModel::NavigatorRole = Qt::UserRole;

NavigatorTreeModel::NavigatorTreeModel(QObject *parent)
    : QStandardItemModel(parent),
      m_blockItemChangedSignal(false)
{
    invisibleRootItem()->setFlags(Qt::ItemIsDropEnabled);

#    ifdef _LOCK_ITEMS_
    setColumnCount(3);
#    else
    setColumnCount(2);
#    endif

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
         stream << idIndex.data(NavigatorRole).toUInt();
     }

     mimeData->setData("application/vnd.modelnode.list", encodedData);

     return mimeData;
}

bool NavigatorTreeModel::dropMimeData(const QMimeData *data,
                  Qt::DropAction action,
                  int row,
                  int column,
                  const QModelIndex &dropIndex)
{
    if (action == Qt::IgnoreAction)
        return true;
    if (action != Qt::LinkAction)
        return false;
    if (!data->hasFormat("application/vnd.modelnode.list"))
        return false;
    if (column > 1)
        return false;
    if (dropIndex.model() != this)
        return false;

    QModelIndex parentIndex, parentItemIndex;
    QString parentPropertyName;
    int targetIndex;

    parentIndex = dropIndex.sibling(dropIndex.row(), 0);
    targetIndex = (row > -1)? row : rowCount(parentIndex);

    if (this->data(parentIndex, NavigatorRole).isValid()) {
        parentItemIndex = parentIndex;
        ModelNode parentNode = nodeForIndex(parentItemIndex);
        if (!parentNode.metaInfo().hasDefaultProperty())
            return false;
        targetIndex -= visibleProperties(parentNode).count();
        parentPropertyName = parentNode.metaInfo().defaultPropertyName();
    }
    else {
        parentItemIndex = parentIndex.parent();
        parentPropertyName = parentIndex.data(Qt::DisplayRole).toString();
    }

    // Disallow dropping items between properties, which are listed first.
    if (targetIndex < 0)
        return false;

    Q_ASSERT(parentItemIndex.isValid());

    QByteArray encodedData = data->data("application/vnd.modelnode.list");
    QDataStream stream(&encodedData, QIODevice::ReadOnly);

    QList<ModelNode> nodeList;
    while (!stream.atEnd()) {
        uint nodeHash;
        stream >> nodeHash;
        if (containsNodeHash(nodeHash)) {
            ModelNode node(nodeForHash(nodeHash));
            nodeList.append(node);
        }
    }

    ModelNode parentNode(nodeForIndex(parentItemIndex));
    NodeAbstractProperty parentProperty = parentNode.nodeAbstractProperty(parentPropertyName);

    if (parentProperty.isNodeProperty() &&
        nodeList.count() > 1) {
        return false;
    }

    moveNodesInteractive(parentProperty, nodeList, targetIndex);
    propagateInvisible(parentNode, isNodeInvisible(parentNode));

    return false; // don't let the view do drag&drop on its own
}

static inline QString msgUnknownItem(const QString &t)
{
    return NavigatorTreeModel::tr("Unknown item: %1").arg(t);
}

NavigatorTreeModel::ItemRow NavigatorTreeModel::createItemRow(const ModelNode &node)
{
    Q_ASSERT(node.isValid());

    uint hash = node.internalId();

    const bool dropEnabled = node.metaInfo().isValid();

    QStandardItem *idItem = new QStandardItem;
    idItem->setDragEnabled(true);
    idItem->setDropEnabled(dropEnabled);
    idItem->setEditable(true);
    idItem->setData(hash, NavigatorRole);
    if (node.metaInfo().isValid())
        idItem->setToolTip(node.type());
    else
        idItem->setToolTip(msgUnknownItem(node.type()));
#    ifdef _LOCK_ITEMS_
    QStandardItem *lockItem = new QStandardItem;
    lockItem->setDragEnabled(true);
    lockItem->setDropEnabled(dropEnabled);
    lockItem->setEditable(false);
    lockItem->setCheckable(true);
    lockItem->setData(hash, NavigatorRole);
#    endif

    QStandardItem *visibilityItem = new QStandardItem;
    visibilityItem->setDropEnabled(dropEnabled);
    visibilityItem->setCheckable(true);
    visibilityItem->setEditable(false);
    visibilityItem->setData(hash, NavigatorRole);
    if (node.isRootNode())
        visibilityItem->setCheckable(false);

    QMap<QString, QStandardItem *> propertyItems;
    foreach (const QString &propertyName, visibleProperties(node)) {
        QStandardItem *propertyItem = new QStandardItem;
        propertyItem->setSelectable(false);
        propertyItem->setDragEnabled(false);
        propertyItem->setDropEnabled(dropEnabled);
        propertyItem->setEditable(false);
        propertyItem->setData(propertyName, Qt::DisplayRole);
        propertyItems.insert(propertyName, propertyItem);
        idItem->appendRow(propertyItem);
    }

#   ifdef _LOCK_ITEMS_
    return ItemRow(idItem, lockItem, visibilityItem, propertyItems);
#   else
    return ItemRow(idItem, visibilityItem, propertyItems);
#   endif
}

void NavigatorTreeModel::updateItemRow(const ModelNode &node, ItemRow items)
{
    bool blockSignal = blockItemChangedSignal(true);

    items.idItem->setText(node.id());
    items.visibilityItem->setCheckState(node.auxiliaryData("invisible").toBool() ? Qt::Unchecked : Qt::Checked);
    if (node.metaInfo().isValid())
        items.idItem->setToolTip(node.type());
    else
        items.idItem->setToolTip(msgUnknownItem(node.type()));

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
void NavigatorTreeModel::updateItemRowOrder(const NodeListProperty &listProperty, const ModelNode &node, int oldIndex)
{
    Q_UNUSED(oldIndex);

    if (!containsNode(node))
        return;

    ItemRow itemRow = itemRowForNode(node);
    int currentRow = itemRow.idItem->row();
    int newRow = listProperty.toModelNodeList().indexOf(node);

    Q_ASSERT(newRow >= 0);

    QStandardItem *parentIdItem = 0;
    if (containsNode(listProperty.parentModelNode())) {
        ItemRow parentRow = itemRowForNode(listProperty.parentModelNode());
        parentIdItem = parentRow.propertyItems.value(listProperty.name());
        if (!parentIdItem) {
            parentIdItem = parentRow.idItem;
            newRow += visibleProperties(listProperty.parentModelNode()).count();
        }
    }
    else {
        parentIdItem = itemRow.idItem->parent();
    }
    Q_ASSERT(parentIdItem);

    if (currentRow != newRow) {
        QList<QStandardItem*> items = parentIdItem->takeRow(currentRow);
        parentIdItem->insertRow(newRow, items);
    }
}

void NavigatorTreeModel::handleChangedItem(QStandardItem *item)
{
    if (m_blockItemChangedSignal)
        return;
    if (!data(item->index(), NavigatorRole).isValid())
        return;

    uint nodeHash = item->data(NavigatorRole).toUInt();
    Q_ASSERT(containsNodeHash(nodeHash));
    ModelNode node = nodeForHash(nodeHash);

    ItemRow itemRow = itemRowForNode(node);
    if (item == itemRow.idItem) {
         if (node.isValidId(item->text())  && !node.view()->modelNodeForId(item->text()).isValid()) {
             if (node.id().isEmpty() || item->text().isEmpty()) { //no id
                 try {
                     node.setId(item->text());
                 } catch (InvalidIdException &e) { //better save then sorry
                     QMessageBox::warning(0, tr("Invalid Id"), e.description());
                 }
             } else { //there is already an id, so we refactor
                 if (node.view()->rewriterView())
                     node.view()->rewriterView()->renameId(node.id(), item->text());
             }
        } else {

             if (!node.isValidId(item->text()))
                 QMessageBox::warning(0, tr("Invalid Id"),  tr("%1 is an invalid id").arg(item->text()));
             else
                 QMessageBox::warning(0, tr("Invalid Id"),  tr("%1 already exists").arg(item->text()));
             bool blockSingals = blockItemChangedSignal(true);
             item->setText(node.id());
             blockItemChangedSignal(blockSingals);
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

void NavigatorTreeModel::setView(QmlModelView *view)
{
    m_view = view;
    m_hiddenProperties.clear();
    if (view) {
        m_hiddenProperties.append("parent");
        addSubTree(view->rootModelNode());
    }
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
    uint hash = index.data(NavigatorRole).toUInt();
    Q_ASSERT(containsNodeHash(hash));
    return nodeForHash(hash);
}

bool NavigatorTreeModel::isInTree(const ModelNode &node) const
{
    return m_nodeHash.keys().contains(node.internalId());
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
    Q_ASSERT(!containsNodeHash(node.internalId()));

    //updateItemRow(node, newRow);

    // only add items that are in the modelNodeChildren list (that means, visible in the editor)
    if (!node.isRootNode()
        && !modelNodeChildren(node.parentProperty().parentModelNode()).contains(node)) {
        return;
    }

    ItemRow newRow = createItemRow(node);
    m_nodeHash.insert(node.internalId(), node);
    m_nodeItemHash.insert(node, newRow);

    updateItemRow(node, newRow);

    foreach (const ModelNode &childNode, modelNodeChildren(node))
        addSubTree(childNode);

    // We assume that the node is always added to the _end_ of the property list.
    if (node.hasParentProperty()) {
        AbstractProperty property(node.parentProperty());
        ItemRow parentRow = itemRowForNode(property.parentModelNode());
        QStandardItem *parentItem = parentRow.propertyItems.value(property.name());
        if (!parentItem) {
            // Child nodes in the default property are added directly under the
            // parent.
            parentItem = parentRow.idItem;
        }
        if (parentItem)
            parentItem->appendRow(newRow.toList());
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
    if (itemRow.idItem->parent())
        rowList = itemRow.idItem->parent()->takeRow(itemRow.idItem->row());

    foreach (const ModelNode &childNode, modelNodeChildren(node)) {
        removeSubTree(childNode);
    }

    qDeleteAll(rowList);

    m_nodeHash.remove(node.internalId());
    m_nodeItemHash.remove(node);
}

void NavigatorTreeModel::moveNodesInteractive(NodeAbstractProperty parentProperty, const QList<ModelNode> &modelNodes, int targetIndex)
{
    try {
        QString propertyQmlType = qmlTypeInQtContainer(parentProperty.parentModelNode().metaInfo().propertyTypeName(parentProperty.name()));

        RewriterTransaction transaction = m_view->beginRewriterTransaction();
        foreach (const ModelNode &node, modelNodes) {
            if (!node.isValid())
                continue;

            if (node != parentProperty.parentModelNode() &&
                !node.isAncestorOf(parentProperty.parentModelNode()) &&
                (node.metaInfo().isSubclassOf(propertyQmlType, -1, -1) || propertyQmlType == "alias")) {
                    //### todo: allowing alias is just a heuristic
                    //once the MetaInfo is part of instances we can do this right

                    if (node.parentProperty() != parentProperty) {

                        if (parentProperty.isNodeProperty()) {
                            ModelNode propertyNode = parentProperty.toNodeProperty().modelNode();
                            // Destruction of ancestors is not allowed
                            if (propertyNode.isAncestorOf(node))
                                continue;
                            if (propertyNode.isValid()) {
                                QApplication::setOverrideCursor(Qt::ArrowCursor);
                                if (QMessageBox::warning(0, tr("Warning"), tr("Reparenting the component %1 here will cause the component %2 to be deleted. Do you want to proceed?").arg(node.id(), propertyNode.id()), QMessageBox::Ok | QMessageBox::Cancel) == QMessageBox::Cancel) {
                                    QApplication::restoreOverrideCursor();
                                    continue;
                                }
                                QApplication::restoreOverrideCursor();
                                propertyNode.destroy();
                            }
                        }

                        if (parentProperty.isDefaultProperty() && parentProperty.parentModelNode().metaInfo().isPositioner()) {
                             ModelNode currentNode = node;
                             if (currentNode.hasProperty("x"))
                                 currentNode.removeProperty("x");
                             if (currentNode.hasProperty("y"))
                                 currentNode.removeProperty("y");
                             parentProperty.reparentHere(currentNode);
                        } else {
                            if (QmlItemNode(node).isValid()) {
                                QPointF scenePos = QmlItemNode(node).instanceScenePosition();
                                parentProperty.reparentHere(node);
                                if (!scenePos.isNull())
                                    setScenePos(node, scenePos);
                            } else {
                                parentProperty.reparentHere(node);
                            }
                        }
                    }

                    if (parentProperty.isNodeListProperty()) {
                        int index = parentProperty.toNodeListProperty().toModelNodeList().indexOf(node);
                        if (index < targetIndex) { // item is first removed from oldIndex, then inserted at new index
                            --targetIndex;
                        }
                        if (index != targetIndex)
                            parentProperty.toNodeListProperty().slide(index, targetIndex);
                    }
            }
        }
    }  catch (RewritingException &e) { //better safe than sorry! There always might be cases where we fail
        QMessageBox::warning(0, "Error", e.description());
    }
}

QList<ModelNode> NavigatorTreeModel::modelNodeChildren(const ModelNode &parentNode)
{
    QList<ModelNode> children;
    QStringList properties;

    if (parentNode.metaInfo().hasDefaultProperty())
        properties << parentNode.metaInfo().defaultPropertyName();

    properties << visibleProperties(parentNode);

    foreach (const QString &propertyName, properties) {
        AbstractProperty property(parentNode.property(propertyName));
        if (property.isNodeProperty())
            children << property.toNodeProperty().modelNode();
        else if (property.isNodeListProperty())
            children << property.toNodeListProperty().toModelNodeList();
    }

    return children;
}

QString NavigatorTreeModel::qmlTypeInQtContainer(const QString &qtContainerType) const
{
    QString typeName(qtContainerType);
    if (typeName.startsWith("QDeclarativeListProperty<") &&
        typeName.endsWith('>')) {
        typeName.remove(0, 25);
        typeName.chop(1);
    }
    if (typeName.endsWith('*'))
        typeName.chop(1);

    return typeName;
}


QStringList NavigatorTreeModel::visibleProperties(const ModelNode &node) const
{
    QStringList propertyList;

    foreach (const QString &propertyName, node.metaInfo().propertyNames()) {
        if (!propertyName.contains('.') && //do not show any dot properties, since they are tricky and unlikely to make sense
            node.metaInfo().propertyIsWritable(propertyName) && !m_hiddenProperties.contains(propertyName) &&
            !node.metaInfo().propertyIsEnumType(propertyName) && //Some enums have the same name as Qml types (e. g. Flow)
            propertyName != node.metaInfo().defaultPropertyName()) { // TODO: ask the node instances

            QString qmlType = qmlTypeInQtContainer(node.metaInfo().propertyTypeName(propertyName));
            if (node.model()->metaInfo(qmlType).isValid() &&
                node.model()->metaInfo(qmlType).isSubclassOf("QtQuick.Item", -1, -1)) {
                propertyList.append(propertyName);
            }
        }
    }

    return propertyList;
}

// along the lines of QObject::blockSignals
bool NavigatorTreeModel::blockItemChangedSignal(bool block)
{
    bool oldValue = m_blockItemChangedSignal;
    m_blockItemChangedSignal = block;
    return oldValue;
}

void NavigatorTreeModel::setId(const QModelIndex &index, const QString &id)
{
    ModelNode node = nodeForIndex(index);
    ItemRow itemRow = itemRowForNode(node);
    itemRow.idItem->setText(id);
}

void NavigatorTreeModel::setVisible(const QModelIndex &index, bool visible)
{
    ModelNode node = nodeForIndex(index);
    ItemRow itemRow = itemRowForNode(node);
    itemRow.visibilityItem->setCheckState(visible ? Qt::Checked : Qt::Unchecked);
}

void NavigatorTreeModel::openContextMenu(const QPoint &position)
{
    ModelNodeContextMenu::showContextMenu(m_view.data(), position, QPoint(), false);
}

} // QmlDesigner
