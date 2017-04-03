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
#include <modelnodecontextmenu.h>
#include <qmlitemnode.h>

#include <coreplugin/icore.h>

#include <utils/qtcassert.h>
#include <utils/utilsicons.h>

#include <QMimeData>
#include <QMessageBox>
#include <QApplication>
#include <QPointF>

#include <coreplugin/messagebox.h>

#include <QtDebug>

#define DISABLE_VISIBLE_PROPERTIES

namespace QmlDesigner {

#ifndef DISABLE_VISIBLE_PROPERTIES
static PropertyNameList visibleProperties(const ModelNode &node)
{
    PropertyNameList propertyList;

    foreach (const PropertyName &propertyName, node.metaInfo().propertyNames()) {
        if (!propertyName.contains('.') //do not show any dot properties, since they are tricky and unlikely to make sense
                && node.metaInfo().propertyIsWritable(propertyName)
                && propertyName != "parent"
                && node.metaInfo().propertyTypeName(propertyName) != TypeName("Component")
                && !node.metaInfo().propertyIsEnumType(propertyName) //Some enums have the same name as Qml types (e. g. Flow)
                && !node.metaInfo().propertyIsPrivate(propertyName) //Do not show private properties
                && propertyName != node.metaInfo().defaultPropertyName()) { // TODO: ask the node instances

            TypeName qmlType = node.metaInfo().propertyTypeName(propertyName);
            if (node.model()->metaInfo(qmlType).isValid() &&
                node.model()->metaInfo(qmlType).isSubclassOf("QtQuick.Item", -1, -1)) {
                propertyList.append(propertyName);
            }
        }
    }

    return propertyList;
}
#endif

static QList<ModelNode> acceptedModelNodeChildren(const ModelNode &parentNode)
{
    QList<ModelNode> children;
    PropertyNameList properties;

    if (parentNode.metaInfo().hasDefaultProperty())
        properties.append(parentNode.metaInfo().defaultPropertyName());

#ifndef DISABLE_VISIBLE_PROPERTIES
    properties.append(visibleProperties(parentNode));
#endif

    foreach (const PropertyName &propertyName, properties) {
        AbstractProperty property(parentNode.property(propertyName));
        if (property.isNodeAbstractProperty())
            children.append(property.toNodeAbstractProperty().directSubNodes());
    }

    return children;
}

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

    connect(this, &QStandardItemModel::itemChanged,
            this, &NavigatorTreeModel::handleChangedItem);
}

NavigatorTreeModel::~NavigatorTreeModel()
{
}

Qt::DropActions NavigatorTreeModel::supportedDropActions() const
{
    return Qt::LinkAction | Qt::MoveAction;
}

Qt::DropActions NavigatorTreeModel::supportedDragActions() const
{
    return Qt::LinkAction;
}

QStringList NavigatorTreeModel::mimeTypes() const
{
     QStringList types;
     types.append(QLatin1String("application/vnd.modelnode.list"));
     types.append(QLatin1String("application/vnd.bauhaus.itemlibraryinfo"));
     types.append(QLatin1String("application/vnd.bauhaus.libraryresource"));

     return types;
}

QByteArray encodeModelNodes(const QModelIndexList &modelIndexList)
{
    QByteArray encodedModelNodeData;
    QDataStream encodedModelNodeDataStream(&encodedModelNodeData, QIODevice::WriteOnly);
    QSet<QModelIndex> rowAlreadyUsedSet;

    foreach (const QModelIndex &modelIndex, modelIndexList) {
        if (modelIndex.isValid()) {
            QModelIndex idModelIndex = modelIndex.sibling(modelIndex.row(), 0);
            if (!rowAlreadyUsedSet.contains(idModelIndex)) {
                rowAlreadyUsedSet.insert(idModelIndex);
                encodedModelNodeDataStream << idModelIndex.data(NavigatorTreeModel::InternalIdRole).toInt();
            }
        }
    }

    return encodedModelNodeData;
}

QMimeData *NavigatorTreeModel::mimeData(const QModelIndexList &modelIndexList) const
{
     QMimeData *mimeData = new QMimeData();

     QByteArray encodedModelNodeData = encodeModelNodes(modelIndexList);

     mimeData->setData(QLatin1String("application/vnd.modelnode.list"), encodedModelNodeData);

     return mimeData;
}

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

static bool computeTarget(const QModelIndex &rowModelIndex,
                          NavigatorTreeModel *navigatorTreeModel,
                          NodeAbstractProperty  *targetProperty,
                          int *targetRowNumber)
{
    QModelIndex targetItemIndex;
    PropertyName targetPropertyName;

    if (*targetRowNumber < 0 || *targetRowNumber > navigatorTreeModel->rowCount(rowModelIndex))
        *targetRowNumber = navigatorTreeModel->rowCount(rowModelIndex);

    if (navigatorTreeModel->hasNodeForIndex(rowModelIndex)) {
        targetItemIndex = rowModelIndex;
        ModelNode targetNode = navigatorTreeModel->nodeForIndex(targetItemIndex);
        if (!targetNode.metaInfo().hasDefaultProperty())
            return false;
#ifndef DISABLE_VISIBLE_PROPERTIES
        *targetRowNumber -= visibleProperties(targetNode).count();
#endif
        targetPropertyName = targetNode.metaInfo().defaultPropertyName();
    } else {
        targetItemIndex = rowModelIndex.parent();
        targetPropertyName = rowModelIndex.data(Qt::DisplayRole).toByteArray();
    }

    // Disallow dropping items between properties, which are listed first.
    if (*targetRowNumber < 0)
        return false;

    ModelNode targetNode(navigatorTreeModel->nodeForIndex(targetItemIndex));
    *targetProperty = targetNode.nodeAbstractProperty(targetPropertyName);

    return true;
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
        if (mimeData->hasFormat(QLatin1String("application/vnd.bauhaus.itemlibraryinfo"))) {
            handleItemLibraryItemDrop(mimeData, rowNumber, dropModelIndex);
        } else if (mimeData->hasFormat(QLatin1String("application/vnd.bauhaus.libraryresource"))) {
            handleItemLibraryImageDrop(mimeData, rowNumber, dropModelIndex);
        } else if (mimeData->hasFormat(QLatin1String("application/vnd.modelnode.list"))) {
            handleInternalDrop(mimeData, rowNumber, dropModelIndex);
        }
    }

    return false; // don't let the view do drag&drop on its own
}

static inline QString msgUnknownItem(const QString &t)
{
    return NavigatorTreeModel::tr("Unknown item: %1").arg(t);
}

static QIcon getTypeIcon(const ModelNode &modelNode)
{
    if (modelNode.isValid()) {
        // if node has no own icon, search for it in the itemlibrary
        const ItemLibraryInfo *libraryInfo = modelNode.model()->metaInfo().itemLibraryInfo();
        QList <ItemLibraryEntry> itemLibraryEntryList = libraryInfo->entriesForType(
            modelNode.type(), modelNode.majorVersion(), modelNode.minorVersion());
        if (!itemLibraryEntryList.isEmpty())
            return itemLibraryEntryList.first().typeIcon();
        else if (modelNode.metaInfo().isValid())
            return QIcon(QStringLiteral(":/ItemLibrary/images/item-default-icon.png"));
    }

    return QIcon(QStringLiteral(":/ItemLibrary/images/item-invalid-icon.png"));
}

ItemRow NavigatorTreeModel::createItemRow(const ModelNode &modelNode)
{
    Q_ASSERT(modelNode.isValid());

    const bool dropEnabled = modelNode.metaInfo().isValid();

    QStandardItem *idItem = new QStandardItem;
    idItem->setDragEnabled(true);
    idItem->setDropEnabled(dropEnabled);
    idItem->setEditable(true);
    idItem->setData(modelNode.internalId(), InternalIdRole);
    idItem->setData(modelNode.simplifiedTypeName(), SimplifiedTypeNameRole);
    if (modelNode.hasId())
        idItem->setText(modelNode.id());
    idItem->setIcon(getTypeIcon(modelNode));
    if (modelNode.metaInfo().isValid())
        idItem->setToolTip(QString::fromUtf8(modelNode.type()));
    else
        idItem->setToolTip(msgUnknownItem(QString::fromUtf8(modelNode.type())));
#    ifdef _LOCK_ITEMS_
    QStandardItem *lockItem = new QStandardItem;
    lockItem->setDragEnabled(true);
    lockItem->setDropEnabled(dropEnabled);
    lockItem->setEditable(false);
    lockItem->setCheckable(true);
    lockItem->setData(hash, NavigatorRole);
#    endif

    QStandardItem *exportItem = new QStandardItem;
    exportItem->setDropEnabled(dropEnabled);
    exportItem->setCheckable(true);
    exportItem->setEditable(false);
    exportItem->setData(modelNode.internalId(), InternalIdRole);
    exportItem->setToolTip(tr("Toggles whether this item is exported as an "
        "alias property of the root item."));
    if (modelNode.isRootNode())
        exportItem->setCheckable(false);

    QStandardItem *visibilityItem = new QStandardItem;
    visibilityItem->setDropEnabled(dropEnabled);
    visibilityItem->setCheckable(true);
    visibilityItem->setEditable(false);
    visibilityItem->setData(modelNode.internalId(), InternalIdRole);
    visibilityItem->setToolTip(tr("Toggles the visibility of this item in the form editor.\n"
        "This is independent of the visibility property in QML."));
    if (modelNode.isRootNode())
        visibilityItem->setCheckable(false);

    QMap<QString, QStandardItem *> propertyItems;
#ifndef DISABLE_VISIBLE_PROPERTIES
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
#endif

#   ifdef _LOCK_ITEMS_
    ItemRow newRow =  ItemRow(idItem, lockItem, visibilityItem, propertyItems);
#   else
    ItemRow newRow = ItemRow(idItem, exportItem, visibilityItem, propertyItems);
#   endif

    m_nodeItemHash.insert(modelNode, newRow);
    updateItemRow(modelNode, newRow);

    return newRow;
}

void NavigatorTreeModel::updateItemRow(const ModelNode &modelNode, ItemRow items)
{
    QmlObjectNode currentQmlObjectNode(modelNode);

    bool blockSignal = blockItemChangedSignal(true);

    items.idItem->setText(modelNode.id());
    items.idItem->setData(modelNode.simplifiedTypeName(), SimplifiedTypeNameRole);

    bool isInvisible = modelNode.auxiliaryData("invisible").toBool();
    items.idItem->setData(isInvisible, InvisibleRole);

    items.visibilityItem->setCheckState(isInvisible ? Qt::Unchecked : Qt::Checked);
    items.exportItem->setCheckState(currentQmlObjectNode.isAliasExported() ? Qt::Checked : Qt::Unchecked);

    if (currentQmlObjectNode.hasError()) {
        items.idItem->setData(true, ErrorRole);
        QString errorString = currentQmlObjectNode.error();
        if (currentQmlObjectNode.isRootNode()) {
            errorString.append(QString("\n%1").arg(tr("Changing the setting \"%1\" might solve the issue.").arg(
                tr("Use QML emulation layer that is built with the selected Qt"))));
        }
        items.idItem->setToolTip(errorString);
        items.idItem->setIcon(Utils::Icons::WARNING.icon());
    } else {
        items.idItem->setData(false, ErrorRole);
        if (modelNode.metaInfo().isValid())
            items.idItem->setToolTip(QString::fromUtf8(modelNode.type()));
        else
            items.idItem->setToolTip(msgUnknownItem(QString::fromUtf8(modelNode.type())));
        items.idItem->setIcon(getTypeIcon(modelNode));
    }

    blockItemChangedSignal(blockSignal);
}

/**
  Update the information shown for a node / property
  */
void NavigatorTreeModel::updateItemRow(const ModelNode &node)
{
    if (!isInTree(node))
        return;

    updateItemRow(node, itemRowForNode(node));
}

static void handleWrongId(QStandardItem *item, const ModelNode &modelNode, const QString &errorTitle, const QString &errorMessage, NavigatorTreeModel *treeModel)
{
    Core::AsynchronousMessageBox::warning(errorTitle,  errorMessage);
    bool blockSingals = treeModel->blockItemChangedSignal(true);
    item->setText(modelNode.id());
    treeModel->blockItemChangedSignal(blockSingals);
}


void NavigatorTreeModel::handleChangedIdItem(QStandardItem *idItem, ModelNode &modelNode)
{
    const QString newId = idItem->text();
    if (!modelNode.isValidId(newId)) {
        handleWrongId(idItem, modelNode, tr("Invalid Id"), tr("%1 is an invalid id.").arg(newId), this);
    } else if (modelNode.view()->hasId(newId)) {
        handleWrongId(idItem, modelNode, tr("Invalid Id"), tr("%1 already exists.").arg(newId), this);
    } else  {
        modelNode.setIdWithRefactoring(newId);
    }
}

void NavigatorTreeModel::handleChangedExportItem(QStandardItem *exportItem, ModelNode &modelNode)
{
    bool exported = (exportItem->checkState() == Qt::Checked);

    QTC_ASSERT(m_view, return);
    ModelNode rootModelNode = m_view->rootModelNode();
    Q_ASSERT(rootModelNode.isValid());
    PropertyName modelNodeId = modelNode.id().toUtf8();
    if (rootModelNode.hasProperty(modelNodeId))
        rootModelNode.removeProperty(modelNodeId);
    if (exported) {

        try {
            RewriterTransaction transaction =
                    m_view->beginRewriterTransaction(QByteArrayLiteral("NavigatorTreeModel:exportItem"));

            QmlObjectNode qmlObjectNode(modelNode);
            qmlObjectNode.ensureAliasExport();
        }  catch (RewritingException &exception) { //better safe than sorry! There always might be cases where we fail
            exception.showException();
        }
    }
}

void NavigatorTreeModel::handleChangedVisibilityItem(QStandardItem *visibilityItem, ModelNode &modelNode)
{
    bool invisible = (visibilityItem->checkState() == Qt::Unchecked);

    if (invisible)
        modelNode.setAuxiliaryData("invisible", invisible);
    else
        modelNode.removeAuxiliaryData("invisible");
}

void NavigatorTreeModel::handleChangedItem(QStandardItem *item)
{
    QVariant internalIdVariant = data(item->index(), InternalIdRole);
    if (!m_blockItemChangedSignal && internalIdVariant.isValid()) {
        ModelNode modelNode = m_view->modelNodeForInternalId(internalIdVariant.toInt());
        ItemRow itemRow = itemRowForNode(modelNode);
        if (item == itemRow.idItem) {
            handleChangedIdItem(item, modelNode);
        } else if (item == itemRow.exportItem) {
            handleChangedExportItem(item, modelNode);
        } else if (item == itemRow.visibilityItem) {
            handleChangedVisibilityItem(item, modelNode);
        }
    }
}

ItemRow NavigatorTreeModel::itemRowForNode(const ModelNode &node)
{
    Q_ASSERT(node.isValid());
    return m_nodeItemHash.value(node);
}

void NavigatorTreeModel::setView(AbstractView *view)
{
    m_view = view;
    if (view)
        addSubTree(view->rootModelNode());
}

void NavigatorTreeModel::clearView()
{
    setView(0);
    m_view.clear();
    m_nodeItemHash.clear();
}

QModelIndex NavigatorTreeModel::indexForNode(const ModelNode &node) const
{
    Q_ASSERT(node.isValid());
    if (!isInTree(node))
        return QModelIndex();
    ItemRow row = m_nodeItemHash.value(node);
    return row.idItem->index();
}

ModelNode NavigatorTreeModel::nodeForIndex(const QModelIndex &index) const
{
    qint32 internalId = index.data(InternalIdRole).toInt();
    return m_view ? m_view->modelNodeForInternalId(internalId) : ModelNode();
}

bool NavigatorTreeModel::hasNodeForIndex(const QModelIndex &index) const
{
    QVariant internalIdVariant = index.data(InternalIdRole);
    if (m_view && internalIdVariant.isValid()) {
        qint32 internalId = internalIdVariant.toInt();
        return m_view->hasModelNodeForInternalId(internalId);
    }

    return false;
}

bool NavigatorTreeModel::isInTree(const ModelNode &node) const
{
    return m_nodeItemHash.contains(node);
}

bool NavigatorTreeModel::isNodeInvisible(const QModelIndex &index) const
{
    return isNodeInvisible(nodeForIndex(index));
}

static bool isInvisbleInHierachy(const ModelNode &modelNode)
{
    if (modelNode.auxiliaryData("invisible").toBool())
        return true;

    if (modelNode.hasParentProperty())
        return isInvisbleInHierachy(modelNode.parentProperty().parentModelNode());

    return false;
}

bool NavigatorTreeModel::isNodeInvisible(const ModelNode &modelNode) const
{
    return isInvisbleInHierachy(modelNode);
}

static bool isRootNodeOrAcceptedChild(const ModelNode &modelNode)
{
    return modelNode.isRootNode() || acceptedModelNodeChildren(modelNode.parentProperty().parentModelNode()).contains(modelNode);
}

static bool nodeCanBeHandled(const ModelNode &modelNode)
{
    return modelNode.metaInfo().isGraphicalItem() && isRootNodeOrAcceptedChild(modelNode);
}

static void appendNodeToEndOfTheRow(const ModelNode &modelNode, const ItemRow &newItemRow, NavigatorTreeModel *treeModel)
{
    if (modelNode.hasParentProperty()) {
        NodeAbstractProperty parentProperty(modelNode.parentProperty());
        ItemRow parentRow = treeModel->itemRowForNode(parentProperty.parentModelNode());
        if (parentRow.propertyItems.contains(QString::fromUtf8(parentProperty.name()))) {
            QStandardItem *parentPropertyItem = parentRow.propertyItems.value(QString::fromUtf8(parentProperty.name()));
            parentPropertyItem->appendRow(newItemRow.toList());
        } else {
            QStandardItem *parentDefaultPropertyItem = parentRow.idItem;
            if (parentDefaultPropertyItem)
                parentDefaultPropertyItem->appendRow(newItemRow.toList());
        }
    } else { // root node
        treeModel->appendRow(newItemRow.toList());
    }
}

void NavigatorTreeModel::addSubTree(const ModelNode &modelNode)
{
    if (nodeCanBeHandled(modelNode)) {

        ItemRow newItemRow = createItemRow(modelNode);

        foreach (const ModelNode &childNode, acceptedModelNodeChildren(modelNode))
            addSubTree(childNode);

        appendNodeToEndOfTheRow(modelNode, newItemRow, this);
    }
}

static QList<QStandardItem*> takeWholeRow(const ItemRow &itemRow)
{
    if (itemRow.idItem->parent())
        return  itemRow.idItem->parent()->takeRow(itemRow.idItem->row());
    else if (itemRow.idItem->model())
        return itemRow.idItem->model()->takeRow(itemRow.idItem->row());
    else
        return itemRow.toList();
}

void NavigatorTreeModel::removeSubTree(const ModelNode &node)
{
    if (isInTree(node)) {
        ItemRow itemRow = itemRowForNode(node);

        QList<QStandardItem*> rowList = takeWholeRow(itemRow);

        foreach (const ModelNode &childNode, acceptedModelNodeChildren(node))
            removeSubTree(childNode);

        qDeleteAll(rowList);
        m_nodeItemHash.remove(node);
    }

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

void NavigatorTreeModel::moveNodesInteractive(NodeAbstractProperty &parentProperty, const QList<ModelNode> &modelNodes, int targetIndex)
{
    QTC_ASSERT(m_view, return);
    try {
        TypeName propertyQmlType = parentProperty.parentModelNode().metaInfo().propertyTypeName(parentProperty.name());

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

void NavigatorTreeModel::handleInternalDrop(const QMimeData *mimeData,
                                            int rowNumber,
                                            const QModelIndex &dropModelIndex)
{
    QTC_ASSERT(m_view, return);
    QModelIndex rowModelIndex = dropModelIndex.sibling(dropModelIndex.row(), 0);
    int targetRowNumber = rowNumber;
    NodeAbstractProperty targetProperty;

    bool foundTarget = computeTarget(rowModelIndex, this, &targetProperty, &targetRowNumber);

    if (foundTarget) {
        QList<ModelNode> modelNodeList = modelNodesFromMimeData(mimeData, m_view);

        if (fitsToTargetProperty(targetProperty, modelNodeList))
            moveNodesInteractive(targetProperty, modelNodeList, targetRowNumber);
    }
}

static ItemLibraryEntry itemLibraryEntryFromData(const QByteArray &data)
{
    QDataStream stream(data);

    ItemLibraryEntry itemLibraryEntry;
    stream >> itemLibraryEntry;

    return itemLibraryEntry;
}

void NavigatorTreeModel::handleItemLibraryItemDrop(const QMimeData *mimeData, int rowNumber, const QModelIndex &dropModelIndex)
{
    QTC_ASSERT(m_view, return);
    QModelIndex rowModelIndex = dropModelIndex.sibling(dropModelIndex.row(), 0);
    int targetRowNumber = rowNumber;
    NodeAbstractProperty targetProperty;

    bool foundTarget = computeTarget(rowModelIndex, this, &targetProperty, &targetRowNumber);

    if (foundTarget) {
        ItemLibraryEntry itemLibraryEntry = itemLibraryEntryFromData(mimeData->data(QLatin1String("application/vnd.bauhaus.itemlibraryinfo")));

         if (!NodeHints::fromItemLibraryEntry(itemLibraryEntry).canBeDroppedInNavigator())
             return;

        QmlItemNode newQmlItemNode = QmlItemNode::createQmlItemNode(m_view, itemLibraryEntry, QPointF(), targetProperty);

        if (newQmlItemNode.isValid() && targetProperty.isNodeListProperty()) {
            QList<ModelNode> newModelNodeList;
            newModelNodeList.append(newQmlItemNode);

            moveNodesInteractive(targetProperty, newModelNodeList, targetRowNumber);
        }
    }
}

void NavigatorTreeModel::handleItemLibraryImageDrop(const QMimeData *mimeData, int rowNumber, const QModelIndex &dropModelIndex)
{
    QTC_ASSERT(m_view, return);
    QModelIndex rowModelIndex = dropModelIndex.sibling(dropModelIndex.row(), 0);
    int targetRowNumber = rowNumber;
    NodeAbstractProperty targetProperty;

    bool foundTarget = computeTarget(rowModelIndex, this, &targetProperty, &targetRowNumber);

    if (foundTarget) {
        QString imageFileName = QString::fromUtf8(mimeData->data(QLatin1String("application/vnd.bauhaus.libraryresource")));
        QmlItemNode newQmlItemNode = QmlItemNode::createQmlItemNodeFromImage(m_view, imageFileName, QPointF(), targetProperty);

        if (newQmlItemNode.isValid()) {
            QList<ModelNode> newModelNodeList;
            newModelNodeList.append(newQmlItemNode);

            moveNodesInteractive(targetProperty, newModelNodeList, targetRowNumber);
        }
    }
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

void NavigatorTreeModel::setExported(const QModelIndex &index, bool exported)
{
    ModelNode node = nodeForIndex(index);
    ItemRow itemRow = itemRowForNode(node);
    itemRow.exportItem->setCheckState(exported ? Qt::Checked : Qt::Unchecked);
}


void NavigatorTreeModel::openContextMenu(const QPoint &position)
{
    QTC_ASSERT(m_view, return);
    ModelNodeContextMenu::showContextMenu(m_view.data(), position, QPoint(), false);
}

} // QmlDesigner
