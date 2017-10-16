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

#include "navigatortreemodel.h"
#include "navigatorview.h"

#include <bindingproperty.h>
#include <nodeabstractproperty.h>
#include <nodehints.h>
#include <nodelistproperty.h>
#include <nodeproperty.h>
#include <variantproperty.h>
#include <metainfo.h>
#include <abstractview.h>
#include <invalididexception.h>
#include <rewritingexception.h>
#include <qmlitemnode.h>

#include <coreplugin/icore.h>

#include <utils/algorithm.h>
#include <utils/qtcassert.h>
#include <utils/utilsicons.h>

#include <QMimeData>
#include <QMessageBox>
#include <QApplication>
#include <QPointF>

#include <coreplugin/messagebox.h>

#include <QtDebug>

namespace QmlDesigner {

static QList<ModelNode> modelNodesFromMimeData(const QMimeData *mineData, AbstractView *view)
{
    QByteArray encodedModelNodeData = mineData->data(QLatin1String("application/vnd.modelnode.list"));
    QDataStream modelNodeStream(&encodedModelNodeData, QIODevice::ReadOnly);

    QList<ModelNode> modelNodeList;
    while (!modelNodeStream.atEnd()) {
        qint32 internalId;
        modelNodeStream >> internalId;
        if (view->hasModelNodeForInternalId(internalId))
            modelNodeList.append(view->modelNodeForInternalId(internalId));
    }

    return modelNodeList;
}

bool fitsToTargetProperty(const NodeAbstractProperty &targetProperty,
                          const QList<ModelNode> &modelNodeList)
{
    return !(targetProperty.isNodeProperty() &&
             modelNodeList.count() > 1);
}

static inline QString msgUnknownItem(const QString &t)
{
    return NavigatorTreeModel::tr("Unknown item: %1").arg(t);
}

static void removePosition(const ModelNode &node)
{
    ModelNode modelNode = node;
    if (modelNode.hasProperty("x"))
        modelNode.removeProperty("x");
    if (modelNode.hasProperty("y"))
        modelNode.removeProperty("y");
}

static void setScenePosition(const QmlDesigner::ModelNode &modelNode,const QPointF &positionInSceneSpace)
{
    if (modelNode.hasParentProperty() && QmlDesigner::QmlItemNode::isValidQmlItemNode(modelNode.parentProperty().parentModelNode())) {
        QmlDesigner::QmlItemNode parentNode = modelNode.parentProperty().parentQmlObjectNode().toQmlItemNode();
        QPointF positionInLocalSpace = parentNode.instanceSceneContentItemTransform().inverted().map(positionInSceneSpace);
        modelNode.variantProperty("x").setValue(positionInLocalSpace.toPoint().x());
        modelNode.variantProperty("y").setValue(positionInLocalSpace.toPoint().y());
    }
}

static bool removeModelNodeFromNodeProperty(NodeAbstractProperty &parentProperty, const ModelNode &modelNode)
{
    if (parentProperty.isNodeProperty()) {
        bool removeNodeInPropertySucceeded = false;
        ModelNode propertyNode = parentProperty.toNodeProperty().modelNode();
        // Destruction of ancestors is not allowed
        if (modelNode != propertyNode && !propertyNode.isAncestorOf(modelNode)) {
            QApplication::setOverrideCursor(Qt::ArrowCursor);

            QMessageBox::StandardButton selectedButton = QMessageBox::warning(Core::ICore::dialogParent(),
                                                                              QCoreApplication::translate("NavigatorTreeModel", "Warning"),
                                                                              QCoreApplication::translate("NavigatorTreeModel","Reparenting the component %1 here will cause the "
                                                                                                                               "component %2 to be deleted. Do you want to proceed?")
                                                                              .arg(modelNode.id(), propertyNode.id()),
                                                                              QMessageBox::Ok | QMessageBox::Cancel);
            if (selectedButton == QMessageBox::Ok) {
                propertyNode.destroy();
                removeNodeInPropertySucceeded = true;
            }

            QApplication::restoreOverrideCursor();
        }

        return removeNodeInPropertySucceeded;
    }

    return true;
}

static void slideModelNodeInList(NodeAbstractProperty &parentProperty, const ModelNode &modelNode, int targetIndex)
{
    if (parentProperty.isNodeListProperty()) {
        int index = parentProperty.indexOf(modelNode);
        if (index < targetIndex) { // item is first removed from oldIndex, then inserted at new index
            --targetIndex;
        }
        if (index != targetIndex)
            parentProperty.toNodeListProperty().slide(index, targetIndex);
    }
}

static bool isInLayoutable(NodeAbstractProperty &parentProperty)
{
    return parentProperty.isDefaultProperty() && parentProperty.parentModelNode().metaInfo().isLayoutable();
}

static void reparentModelNodeToNodeProperty(NodeAbstractProperty &parentProperty, const ModelNode &modelNode)
{
    try {
        if (!modelNode.hasParentProperty() || parentProperty != modelNode.parentProperty()) {
            if (isInLayoutable(parentProperty)) {
                removePosition(modelNode);
                parentProperty.reparentHere(modelNode);
            } else {
                if (QmlItemNode::isValidQmlItemNode(modelNode)) {
                    QPointF scenePosition = QmlItemNode(modelNode).instanceScenePosition();
                    parentProperty.reparentHere(modelNode);
                    if (!scenePosition.isNull())
                        setScenePosition(modelNode, scenePosition);
                } else {
                    parentProperty.reparentHere(modelNode);
                }
            }
        }
    }  catch (const RewritingException &exception) { //better safe than sorry! There always might be cases where we fail
        exception.showException();
    }
}

NavigatorTreeModel::NavigatorTreeModel(QObject *parent) : QAbstractItemModel(parent)
{
}

NavigatorTreeModel::~NavigatorTreeModel()
{
}

QVariant NavigatorTreeModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    const ModelNode modelNode = modelNodeForIndex(index);
    const QmlObjectNode currentQmlObjectNode(modelNode);

    QTC_ASSERT(m_view, return QVariant());

    if (!modelNode.isValid())
        return QVariant();

    if (role == ItemIsVisibleRole) //independent of column
        return m_view->isNodeInvisible(modelNode) ? Qt::Unchecked : Qt::Checked;

    if (index.column() == 0) {
        if (role == Qt::DisplayRole) {
            return modelNode.displayName();
        } else if (role == Qt::DecorationRole) {
            if (currentQmlObjectNode.hasError())
                return Utils::Icons::WARNING.icon();

            return modelNode.typeIcon();

        } else if (role == Qt::ToolTipRole) {
            if (currentQmlObjectNode.hasError()) {
                QString errorString = currentQmlObjectNode.error();
                if (currentQmlObjectNode.isRootNode())
                    errorString.append(QString("\n%1").arg(tr("Changing the setting \"%1\" might solve the issue.").arg(
                                                               tr("Use QML emulation layer that is built with the selected Qt"))));

                return errorString;
            }
            if (modelNode.metaInfo().isValid())
                return modelNode.type();
            else
                return msgUnknownItem(QString::fromUtf8(modelNode.type()));
        } else if (role == ModelNodeRole) {
            return QVariant::fromValue<ModelNode>(modelNode);
        }
    } else if (index.column() == 1) { //export
        if (role == Qt::CheckStateRole)
            return currentQmlObjectNode.isAliasExported()  ? Qt::Checked : Qt::Unchecked;
        else if (role == Qt::ToolTipRole)
            return tr("Toggles whether this item is exported as an "
                      "alias property of the root item.");
    } else if (index.column() == 2) { //visible
        if (role == Qt::CheckStateRole)
            return m_view->isNodeInvisible(modelNode) ? Qt::Unchecked : Qt::Checked;
        else if (role == Qt::ToolTipRole)
            return tr("Toggles the visibility of this item in the form editor.\n"
                      "This is independent of the visibility property in QML.");
    }

    return QVariant();
}

Qt::ItemFlags NavigatorTreeModel::flags(const QModelIndex &index) const
{
    if (index.column() == 0)
        return Qt::ItemIsEditable | Qt::ItemIsDropEnabled | Qt::ItemIsDragEnabled | Qt::ItemIsSelectable | Qt::ItemIsEnabled;

    return Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsUserCheckable
            | Qt::ItemNeverHasChildren;
}

QList<ModelNode> filteredList(const NodeListProperty &property, bool filter)
{
    if (!filter)
        return property.toModelNodeList();

    return Utils::filtered(property.toModelNodeList(), [] (const ModelNode &arg) {
        return QmlItemNode::isValidQmlItemNode(arg);
    });
}

QModelIndex NavigatorTreeModel::index(int row, int column,
                                      const QModelIndex &parent) const
{
    if (!m_view->model())
        return QModelIndex();

    if (!hasIndex(row, column, parent))
        return QModelIndex();

    if (!parent.isValid())
        return createIndexFromModelNode(0, column, m_view->rootModelNode());

    ModelNode parentModelNode = modelNodeForIndex(parent);

    ModelNode modelNode;
    if (parentModelNode.defaultNodeListProperty().isValid())
        modelNode = filteredList(parentModelNode.defaultNodeListProperty(), m_showOnlyVisibleItems).at(row);

    if (!modelNode.isValid())
        return QModelIndex();

    return createIndexFromModelNode(row, column, modelNode);
}

QVariant NavigatorTreeModel::headerData(int, Qt::Orientation, int) const
{
    return QVariant();
}

QModelIndex NavigatorTreeModel::parent(const QModelIndex &index) const
{
    if (!index.isValid())
        return QModelIndex();

    const ModelNode modelNode = modelNodeForIndex(index);

    if (!modelNode.isValid())
        return QModelIndex();

    if (!modelNode.hasParentProperty())
        return QModelIndex();

    const ModelNode parentModelNode = modelNode.parentProperty().parentModelNode();

    int row = 0;

    if (!parentModelNode.isRootNode() && parentModelNode.parentProperty().isNodeListProperty())
        row = filteredList(parentModelNode.parentProperty().toNodeListProperty(), m_showOnlyVisibleItems).indexOf(parentModelNode);

    return createIndexFromModelNode(row, 0, parentModelNode);
}

int NavigatorTreeModel::rowCount(const QModelIndex &parent) const
{
    if (!m_view->isAttached())
        return 0;

    if (parent.column() > 0)
        return 0;
    const ModelNode modelNode = modelNodeForIndex(parent);

    if (!modelNode.isValid())
        return 1;

    int rows = 0;

    if (modelNode.defaultNodeListProperty().isValid())
        rows = filteredList(modelNode.defaultNodeListProperty(), m_showOnlyVisibleItems).count();

    return rows;
}

int NavigatorTreeModel::columnCount(const QModelIndex &parent) const
{
    if (parent.column() > 0)
        return 0;

    return 3;
}

ModelNode NavigatorTreeModel::modelNodeForIndex(const QModelIndex &index) const
{
    if (!index.isValid())
        return ModelNode();

    if (!m_view || !m_view->model())
        return ModelNode();

    return m_view->modelNodeForInternalId(index.internalId());
}

bool NavigatorTreeModel::hasModelNodeForIndex(const QModelIndex &index) const
{
    if (!index.isValid())
        return false;

    return m_view->modelNodeForInternalId(index.internalId()).isValid();
}

void NavigatorTreeModel::setView(NavigatorView *view)
{
    m_view = view;
}

QStringList NavigatorTreeModel::mimeTypes() const
{
    const static QStringList types({"application/vnd.modelnode.list",
                       "application/vnd.bauhaus.itemlibraryinfo",
                       "application/vnd.bauhaus.libraryresource"});

    return types;
}

QMimeData *NavigatorTreeModel::mimeData(const QModelIndexList &modelIndexList) const
{
    QMimeData *mimeData = new QMimeData();

    QByteArray encodedModelNodeData;
    QDataStream encodedModelNodeDataStream(&encodedModelNodeData, QIODevice::WriteOnly);
    QSet<QModelIndex> rowAlreadyUsedSet;

    for (const QModelIndex &modelIndex : modelIndexList) {
        if (modelIndex.isValid()) {
            const QModelIndex idModelIndex = modelIndex.sibling(modelIndex.row(), 0);
            if (!rowAlreadyUsedSet.contains(idModelIndex)) {
                rowAlreadyUsedSet.insert(idModelIndex);
                encodedModelNodeDataStream << idModelIndex.internalId();
            }
        }
    }

    mimeData->setData("application/vnd.modelnode.list", encodedModelNodeData);

    return mimeData;
}

QModelIndex NavigatorTreeModel::indexForModelNode(const ModelNode &node) const
{
    return m_nodeIndexHash.value(node);
}

QModelIndex NavigatorTreeModel::createIndexFromModelNode(int row, int column, const ModelNode &modelNode) const
{
    QModelIndex index = createIndex(row, column, modelNode.internalId());
    if (column == 0)
        m_nodeIndexHash.insert(modelNode, index);

    return index;
}

bool NavigatorTreeModel::dropMimeData(const QMimeData *mimeData,
                                      Qt::DropAction action,
                                      int rowNumber,
                                      int /*columnNumber*/,
                                      const QModelIndex &dropModelIndex)
{
    if (action == Qt::IgnoreAction)
        return true;

    if (dropModelIndex.model() == this) {
        if (mimeData->hasFormat("application/vnd.bauhaus.itemlibraryinfo")) {
            handleItemLibraryItemDrop(mimeData, rowNumber, dropModelIndex);
        } else if (mimeData->hasFormat("application/vnd.bauhaus.libraryresource")) {
            handleItemLibraryImageDrop(mimeData, rowNumber, dropModelIndex);
        } else if (mimeData->hasFormat("application/vnd.modelnode.list")) {
            handleInternalDrop(mimeData, rowNumber, dropModelIndex);
        }
    }

    return false; // don't let the view do drag&drop on its own
}

static bool findTargetProperty(const QModelIndex &rowModelIndex,
                               NavigatorTreeModel *navigatorTreeModel,
                               NodeAbstractProperty *targetProperty,
                               int *targetRowNumber)
{
    QModelIndex targetItemIndex;
    PropertyName targetPropertyName;

    if (*targetRowNumber < 0 || *targetRowNumber > navigatorTreeModel->rowCount(rowModelIndex))
        *targetRowNumber = navigatorTreeModel->rowCount(rowModelIndex);

    if (navigatorTreeModel->hasModelNodeForIndex(rowModelIndex)) {
        targetItemIndex = rowModelIndex;
        const ModelNode targetNode = navigatorTreeModel->modelNodeForIndex(targetItemIndex);
        if (!targetNode.metaInfo().hasDefaultProperty())
            return false;

        targetPropertyName = targetNode.metaInfo().defaultPropertyName();
    }

    // Disallow dropping items between properties, which are listed first.
    if (*targetRowNumber < 0)
        return false;

    const ModelNode targetNode(navigatorTreeModel->modelNodeForIndex(targetItemIndex));
    *targetProperty = targetNode.nodeAbstractProperty(targetPropertyName);

    return true;
}

void NavigatorTreeModel::handleInternalDrop(const QMimeData *mimeData,
                                            int rowNumber,
                                            const QModelIndex &dropModelIndex)
{
    QTC_ASSERT(m_view, return);
    const QModelIndex rowModelIndex = dropModelIndex.sibling(dropModelIndex.row(), 0);
    int targetRowNumber = rowNumber;
    NodeAbstractProperty targetProperty;

    bool foundTarget = findTargetProperty(rowModelIndex, this, &targetProperty, &targetRowNumber);

    if (foundTarget) {
        QList<ModelNode> modelNodeList = modelNodesFromMimeData(mimeData, m_view);

        if (fitsToTargetProperty(targetProperty, modelNodeList))
            moveNodesInteractive(targetProperty, modelNodeList, targetRowNumber);
    }
}

static ItemLibraryEntry createItemLibraryEntryFromMimeData(const QByteArray &data)
{
    QDataStream stream(data);

    ItemLibraryEntry itemLibraryEntry;
    stream >> itemLibraryEntry;

    return itemLibraryEntry;
}

void NavigatorTreeModel::handleItemLibraryItemDrop(const QMimeData *mimeData, int rowNumber, const QModelIndex &dropModelIndex)
{
    QTC_ASSERT(m_view, return);
    const QModelIndex rowModelIndex = dropModelIndex.sibling(dropModelIndex.row(), 0);
    int targetRowNumber = rowNumber;
    NodeAbstractProperty targetProperty;

    bool foundTarget = findTargetProperty(rowModelIndex, this, &targetProperty, &targetRowNumber);

    if (foundTarget) {
        const ItemLibraryEntry itemLibraryEntry =
                createItemLibraryEntryFromMimeData(mimeData->data("application/vnd.bauhaus.itemlibraryinfo"));

        if (!NodeHints::fromItemLibraryEntry(itemLibraryEntry).canBeDroppedInNavigator())
            return;

        const QmlItemNode newQmlItemNode = QmlItemNode::createQmlItemNode(m_view, itemLibraryEntry, QPointF(), targetProperty);

        if (newQmlItemNode.isValid() && targetProperty.isNodeListProperty()) {
            QList<ModelNode> newModelNodeList;
            newModelNodeList.append(newQmlItemNode);

            moveNodesInteractive(targetProperty, newModelNodeList, targetRowNumber);
        }

        if (newQmlItemNode.isValid())
            m_view->selectModelNode(newQmlItemNode.modelNode());
    }
}

void NavigatorTreeModel::handleItemLibraryImageDrop(const QMimeData *mimeData, int rowNumber, const QModelIndex &dropModelIndex)
{
    QTC_ASSERT(m_view, return);
    const QModelIndex rowModelIndex = dropModelIndex.sibling(dropModelIndex.row(), 0);
    int targetRowNumber = rowNumber;
    NodeAbstractProperty targetProperty;

    bool foundTarget = findTargetProperty(rowModelIndex, this, &targetProperty, &targetRowNumber);

    if (foundTarget) {
        const QString imageFileName = QString::fromUtf8(mimeData->data("application/vnd.bauhaus.libraryresource"));
        const QmlItemNode newQmlItemNode = QmlItemNode::createQmlItemNodeFromImage(m_view, imageFileName, QPointF(), targetProperty);

        if (newQmlItemNode.isValid()) {
            QList<ModelNode> newModelNodeList;
            newModelNodeList.append(newQmlItemNode);

            moveNodesInteractive(targetProperty, newModelNodeList, targetRowNumber);
        }

        if (newQmlItemNode.isValid())
            m_view->selectModelNode(newQmlItemNode.modelNode());
    }
}

void NavigatorTreeModel::moveNodesInteractive(NodeAbstractProperty &parentProperty, const QList<ModelNode> &modelNodes, int targetIndex)
{
    QTC_ASSERT(m_view, return);
    try {
        const TypeName propertyQmlType = parentProperty.parentModelNode().metaInfo().propertyTypeName(parentProperty.name());

        RewriterTransaction transaction = m_view->beginRewriterTransaction(QByteArrayLiteral("NavigatorTreeModel::moveNodesInteractive"));
        foreach (const ModelNode &modelNode, modelNodes) {
            if (modelNode.isValid()
                    && modelNode != parentProperty.parentModelNode()
                    && !modelNode.isAncestorOf(parentProperty.parentModelNode())
                    && (modelNode.metaInfo().isSubclassOf(propertyQmlType) || propertyQmlType == "alias")) {
                //### todo: allowing alias is just a heuristic
                //once the MetaInfo is part of instances we can do this right

                bool nodeCanBeMovedToParentProperty = removeModelNodeFromNodeProperty(parentProperty, modelNode);

                if (nodeCanBeMovedToParentProperty) {
                    reparentModelNodeToNodeProperty(parentProperty, modelNode);
                    slideModelNodeInList(parentProperty, modelNode, targetIndex);
                }
            }
        }
        transaction.commit();
    }  catch (const RewritingException &exception) { //better safe than sorry! There always might be cases where we fail
        exception.showException();
    }
}

Qt::DropActions NavigatorTreeModel::supportedDropActions() const
{
    return Qt::LinkAction | Qt::MoveAction;
}

Qt::DropActions NavigatorTreeModel::supportedDragActions() const
{
    return Qt::LinkAction;
}

bool NavigatorTreeModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    ModelNode modelNode = modelNodeForIndex(index);
    if (index.column() == 1 && role == Qt::CheckStateRole) {
        QTC_ASSERT(m_view, return false);
        m_view->handleChangedExport(modelNode, value.toInt() != 0);
    } else if (index.column() == 2 && role == Qt::CheckStateRole) {
        if (value.toInt() == 0)
            modelNode.setAuxiliaryData("invisible", true);
        else
            modelNode.removeAuxiliaryData("invisible");
    }

    return true;
}

void NavigatorTreeModel::notifyDataChanged(const ModelNode &modelNode)
{
    const QModelIndex index = indexForModelNode(modelNode);
    dataChanged(index, index);
}

static QList<ModelNode> collectParents(const QList<ModelNode> &modelNodes)
{
    QSet<ModelNode> parents;
    for (const ModelNode &modelNode : modelNodes) {
        if (modelNode.isValid() && modelNode.hasParentProperty()) {
            const ModelNode parent = modelNode.parentProperty().parentModelNode();
            parents.insert(parent);
        }
    }

    return parents.toList();
}

QList<QPersistentModelIndex> NavigatorTreeModel::nodesToPersistentIndex(const QList<ModelNode> &modelNodes)
{
    return Utils::transform(modelNodes, [this](const ModelNode &modelNode) {
        return QPersistentModelIndex(indexForModelNode(modelNode));
    });
}

void NavigatorTreeModel::notifyModelNodesRemoved(const QList<ModelNode> &modelNodes)
{
    QList<QPersistentModelIndex> indexes = nodesToPersistentIndex(collectParents(modelNodes));
    layoutAboutToBeChanged(indexes);
    layoutChanged(indexes);
}

void NavigatorTreeModel::notifyModelNodesInserted(const QList<ModelNode> &modelNodes)
{
    QList<QPersistentModelIndex> indexes = nodesToPersistentIndex(collectParents(modelNodes));
    layoutAboutToBeChanged(indexes);
    layoutChanged(indexes);
}

void NavigatorTreeModel::notifyModelNodesMoved(const QList<ModelNode> &modelNodes)
{
    QList<QPersistentModelIndex> indexes = nodesToPersistentIndex(collectParents(modelNodes));
    layoutAboutToBeChanged(indexes);
    layoutChanged(indexes);
}

void NavigatorTreeModel::setFilter(bool showOnlyVisibleItems)
{
    m_showOnlyVisibleItems = showOnlyVisibleItems;
    resetModel();
}

void NavigatorTreeModel::resetModel()
{
    beginResetModel();
    m_nodeIndexHash.clear();
    endResetModel();
}

} // QmlDesigner
