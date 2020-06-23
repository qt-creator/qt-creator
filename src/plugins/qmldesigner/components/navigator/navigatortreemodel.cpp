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
#include "qmldesignerplugin.h"

#include <bindingproperty.h>
#include <designersettings.h>
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
#include <QDir>

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
    bool const canBeContainer =
        NodeHints::fromModelNode(targetProperty.parentModelNode()).canBeContainerFor(modelNodeList.first());
    return !(targetProperty.isNodeProperty() &&
             modelNodeList.count() > 1) && canBeContainer;
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

NavigatorTreeModel::~NavigatorTreeModel() = default;

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
                if (DesignerSettings::getValue(DesignerSettingsKey::STANDALONE_MODE).toBool() &&
                        currentQmlObjectNode.isRootNode())
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

void static appendForcedNodes(const NodeListProperty &property, QList<ModelNode> &list)
{
    const QStringList visibleProperties = NodeHints::fromModelNode(property.parentModelNode()).visibleNonDefaultProperties();
    for (const ModelNode &node : property.parentModelNode().directSubModelNodes()) {
        if (!list.contains(node) && visibleProperties.contains(QString::fromUtf8(node.parentProperty().name())))
            list.append(node);
    }
}

QList<ModelNode> filteredList(const NodeListProperty &property, bool filter)
{
    QList<ModelNode> list;

    if (filter) {
        list.append(Utils::filtered(property.toModelNodeList(), [] (const ModelNode &arg) {
            return QmlItemNode::isValidQmlItemNode(arg) || NodeHints::fromModelNode(arg).visibleInNavigator();
        }));
    } else {
        list = property.toModelNodeList();
    }

    appendForcedNodes(property, list);

    return list;
}

QModelIndex NavigatorTreeModel::index(int row, int column,
                                      const QModelIndex &parent) const
{
    if (!m_view->model())
        return {};

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
        return {};

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
    auto mimeData = new QMimeData();

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
                               int *targetRowNumber,
                               const PropertyName &propertyName = {})
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

        if (propertyName.isEmpty() || !targetNode.metaInfo().hasProperty(propertyName))
            targetPropertyName = targetNode.metaInfo().defaultPropertyName();
        else
            targetPropertyName = propertyName;
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

    const ItemLibraryEntry itemLibraryEntry =
        createItemLibraryEntryFromMimeData(mimeData->data("application/vnd.bauhaus.itemlibraryinfo"));

    const NodeHints hints = NodeHints::fromItemLibraryEntry(itemLibraryEntry);

    const QString targetPropertyName = hints.forceNonDefaultProperty();

    bool foundTarget = findTargetProperty(rowModelIndex, this, &targetProperty, &targetRowNumber, targetPropertyName.toUtf8());

    if (foundTarget) {
        if (!NodeHints::fromItemLibraryEntry(itemLibraryEntry).canBeDroppedInNavigator())
            return;

        QmlObjectNode newQmlObjectNode;
        m_view->executeInTransaction("NavigatorTreeModel::handleItemLibraryItemDrop", [&] {
            newQmlObjectNode = QmlItemNode::createQmlObjectNode(m_view, itemLibraryEntry, QPointF(), targetProperty, false);
            ModelNode newModelNode = newQmlObjectNode.modelNode();
            auto insertIntoList = [&](const QByteArray &listPropertyName, const ModelNode &targetNode) {
                if (targetNode.isValid()) {
                    BindingProperty listProp = targetNode.bindingProperty(listPropertyName);
                    if (listProp.isValid()) {
                        QString expression = listProp.expression();
                        int bracketIndex = expression.indexOf(']');
                        if (expression.isEmpty())
                            expression = newModelNode.validId();
                        else if (bracketIndex == -1)
                            expression = QStringLiteral("[%1,%2]").arg(expression).arg(newModelNode.validId());
                        else
                            expression.insert(bracketIndex, QStringLiteral(",%1").arg(newModelNode.validId()));
                        listProp.setExpression(expression);
                    }
                }
            };
            if (newModelNode.isValid()) {
                if (newModelNode.isSubclassOf("QtQuick3D.Effect")) {
                    // Insert effects dropped to either View3D or SceneEnvironment into the
                    // SceneEnvironment's effects list
                    ModelNode targetEnv;
                    if (targetProperty.parentModelNode().isSubclassOf("QtQuick3D.SceneEnvironment")) {
                        targetEnv = targetProperty.parentModelNode();
                    } else if (targetProperty.parentModelNode().isSubclassOf("QtQuick3D.View3D")) {
                        // see if View3D has environment set to it
                        BindingProperty envNodeProp = targetProperty.parentModelNode().bindingProperty("environment");
                        if (envNodeProp.isValid())  {
                            ModelNode envNode = envNodeProp.resolveToModelNode();
                            if (envNode.isValid())
                                targetEnv = envNode;
                        }
                    }
                    insertIntoList("effects", targetEnv);
                } else if (newModelNode.isSubclassOf("QtQuick3D.Material")) {
                    // Insert material dropped to a model node into the materials list of the model
                    ModelNode targetModel;
                    if (targetProperty.parentModelNode().isSubclassOf("QtQuick3D.Model"))
                        targetModel = targetProperty.parentModelNode();
                    insertIntoList("materials", targetModel);
                }
            }
        });

        if (newQmlObjectNode.isValid() && targetProperty.isNodeListProperty()) {
            QList<ModelNode> newModelNodeList;
            newModelNodeList.append(newQmlObjectNode);

            moveNodesInteractive(targetProperty, newModelNodeList, targetRowNumber);
        }

        if (newQmlObjectNode.isValid())
            m_view->setSelectedModelNode(newQmlObjectNode.modelNode());
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
        ModelNode targetNode(modelNodeForIndex(rowModelIndex));

        const QString imageSource = QString::fromUtf8(mimeData->data("application/vnd.bauhaus.libraryresource")); // absolute path
        const QString imagePath = QmlDesignerPlugin::instance()->documentManager().currentFilePath().toFileInfo().dir().relativeFilePath(imageSource); // relative to .ui.qml file

        ModelNode newModelNode;

        if (targetNode.isSubclassOf("QtQuick3D.DefaultMaterial")) {
            // if dropping an image on a default material, create a texture instead of image
            m_view->executeInTransaction("QmlItemNode::createQmlItemNode", [&] {
                // create a texture item lib
                ItemLibraryEntry itemLibraryEntry;
                itemLibraryEntry.setName("Texture");
                itemLibraryEntry.setType("QtQuick3D.Texture", 1, 0);

                // set texture source
                PropertyName prop = "source";
                QString type = "QUrl";
                QVariant val = imagePath;
                itemLibraryEntry.addProperty(prop, type, val);

                // create a texture
                newModelNode = QmlItemNode::createQmlObjectNode(m_view, itemLibraryEntry, {}, targetProperty, false);

                // set the texture to parent material's diffuseMap property
                // TODO: allow the user to choose which map property to set the texture for
                targetNode.bindingProperty("diffuseMap").setExpression(newModelNode.validId());
            });
        } else if (targetNode.isSubclassOf("QtQuick3D.Texture")) {
            // if dropping an image on a texture, set the texture source
            targetNode.variantProperty("source").setValue(imagePath);
        } else {

            // create an image
            newModelNode = QmlItemNode::createQmlItemNodeFromImage(m_view, imageSource , QPointF(), targetProperty);
        }

        if (newModelNode.isValid()) {
            moveNodesInteractive(targetProperty, {newModelNode}, targetRowNumber);
            m_view->setSelectedModelNode(newModelNode);
        }
    }
}

void NavigatorTreeModel::moveNodesInteractive(NodeAbstractProperty &parentProperty, const QList<ModelNode> &modelNodes, int targetIndex)
{
    QTC_ASSERT(m_view, return);

    m_view->executeInTransaction("NavigatorTreeModel::moveNodesInteractive",[&parentProperty, modelNodes, targetIndex](){
        const TypeName propertyQmlType = parentProperty.parentModelNode().metaInfo().propertyTypeName(parentProperty.name());
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
    });
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
        QmlVisualNode(modelNode).setVisibilityOverride(value.toInt() == 0);
    }

    return true;
}

void NavigatorTreeModel::notifyDataChanged(const ModelNode &modelNode)
{
    const QModelIndex index = indexForModelNode(modelNode);
    const QAbstractItemModel *model = index.model();
    const QModelIndex sibling = model ? model->sibling(index.row(), 2, index) : QModelIndex();
    emit dataChanged(index, sibling);
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

    return Utils::toList(parents);
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
    emit layoutAboutToBeChanged(indexes);
    emit layoutChanged(indexes);
}

void NavigatorTreeModel::notifyModelNodesInserted(const QList<ModelNode> &modelNodes)
{
    QList<QPersistentModelIndex> indexes = nodesToPersistentIndex(collectParents(modelNodes));
    emit layoutAboutToBeChanged(indexes);
    emit layoutChanged(indexes);
}

void NavigatorTreeModel::notifyModelNodesMoved(const QList<ModelNode> &modelNodes)
{
    QList<QPersistentModelIndex> indexes = nodesToPersistentIndex(collectParents(modelNodes));
    emit layoutAboutToBeChanged(indexes);
    emit layoutChanged(indexes);
}

void NavigatorTreeModel::notifyIconsChanged()
{
    emit dataChanged(index(0, 0), index(rowCount(), 0), {Qt::DecorationRole});
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
