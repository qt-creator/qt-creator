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
#include "choosetexturepropertydialog.h"
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
#include <designeractionmanager.h>
#include <import.h>

#include <coreplugin/icore.h>

#include <utils/algorithm.h>
#include <utils/qtcassert.h>
#include <utils/utilsicons.h>

#include <QMimeData>
#include <QMessageBox>
#include <QApplication>
#include <QPointF>
#include <QDir>
#include <QFileInfo>
#include <QFile>
#include <QPixmap>

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
    m_actionManager = &QmlDesignerPlugin::instance()->viewManager().designerActionManager();
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

    if (role == ItemIsVisibleRole) // independent of column
        return m_view->isNodeInvisible(modelNode) ? Qt::Unchecked : Qt::Checked;

    if (role == ItemOrAncestorLocked)
        return ModelNode::isThisOrAncestorLocked(modelNode);

    if (role == ModelNodeRole)
        return QVariant::fromValue<ModelNode>(modelNode);

    if (index.column() == ColumnType::Name) {
        if (role == Qt::DisplayRole) {
            return modelNode.displayName();
        } else if (role == Qt::DecorationRole) {
            if (currentQmlObjectNode.hasError())
                return Utils::Icons::WARNING.icon();

            return modelNode.typeIcon();

        } else if (role == Qt::ToolTipRole) {
            if (currentQmlObjectNode.hasError()) {
                QString errorString = currentQmlObjectNode.error();
                if (DesignerSettings::getValue(DesignerSettingsKey::STANDALONE_MODE).toBool()
                        && currentQmlObjectNode.isRootNode()) {
                    errorString.append(QString("\n%1").arg(tr("Changing the setting \"%1\" might solve the issue.").arg(
                                                               tr("Use QML emulation layer that is built with the selected Qt"))));
                }
                return errorString;
            }

            if (modelNode.metaInfo().isValid()) {
                if (m_actionManager->hasModelNodePreviewHandler(modelNode))
                    return {}; // Images have special tooltip popup, so suppress regular one
                else
                    return modelNode.type();
            } else {
                return msgUnknownItem(QString::fromUtf8(modelNode.type()));
            }
        } else if (role == ToolTipImageRole) {
            if (currentQmlObjectNode.hasError()) // Error already shown on regular tooltip
                return {};
            auto op = m_actionManager->modelNodePreviewOperation(modelNode);
            if (op)
                return op(modelNode);
        }
    } else if (index.column() == ColumnType::Alias) { // export
        if (role == Qt::CheckStateRole)
            return currentQmlObjectNode.isAliasExported() ? Qt::Checked : Qt::Unchecked;
        else if (role == Qt::ToolTipRole && !modelNodeForIndex(index).isRootNode())
            return tr("Toggles whether this item is exported as an "
                      "alias property of the root item.");
    } else if (index.column() == ColumnType::Visibility) { // visible
        if (role == Qt::CheckStateRole)
            return m_view->isNodeInvisible(modelNode) ? Qt::Unchecked : Qt::Checked;
        else if (role == Qt::ToolTipRole && !modelNodeForIndex(index).isRootNode())
            return tr("Toggles the visibility of this item in the form editor.\n"
                      "This is independent of the visibility property in QML.");
    } else if (index.column() == ColumnType::Lock) { // lock
        if (role == Qt::CheckStateRole)
            return modelNode.locked() ? Qt::Checked : Qt::Unchecked;
        else if (role == Qt::ToolTipRole && !modelNodeForIndex(index).isRootNode())
            return tr("Toggles whether this item is locked.\n"
                      "Locked items cannot be modified or selected.");
    }

    return QVariant();
}

Qt::ItemFlags NavigatorTreeModel::flags(const QModelIndex &index) const
{
    if (modelNodeForIndex(index).isRootNode()) {
        Qt::ItemFlags flags = Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDropEnabled;
        if (index.column() == ColumnType::Name)
            return flags | Qt::ItemIsEditable;
        else
            return flags;
    }

    const ModelNode modelNode = modelNodeForIndex(index);

    if (index.column() == ColumnType::Alias
        || index.column() == ColumnType::Visibility
        || index.column() == ColumnType::Lock) {
        if (ModelNode::isThisOrAncestorLocked(modelNode))
            return Qt::ItemIsEnabled | Qt::ItemIsUserCheckable;
        else
            return Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsUserCheckable;
    }

    if (ModelNode::isThisOrAncestorLocked(modelNode))
        return Qt::NoItemFlags;

    if (index.column() == ColumnType::Name)
        return Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable | Qt::ItemIsDropEnabled | Qt::ItemIsDragEnabled;

    return Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsUserCheckable;
}

void static appendForcedNodes(const NodeListProperty &property, QList<ModelNode> &list)
{
    const QStringList visibleProperties = NodeHints::fromModelNode(property.parentModelNode()).visibleNonDefaultProperties();
    for (const ModelNode &node : property.parentModelNode().directSubModelNodes()) {
        if (!list.contains(node) && visibleProperties.contains(QString::fromUtf8(node.parentProperty().name())))
            list.append(node);
    }
}

QList<ModelNode> filteredList(const NodeListProperty &property, bool filter, bool reverseOrder)
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

    if (reverseOrder)
        std::reverse(list.begin(), list.end());

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
        modelNode = filteredList(parentModelNode.defaultNodeListProperty(),
                                 m_showOnlyVisibleItems,
                                 m_reverseItemOrder).at(row);

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
        row = filteredList(parentModelNode.parentProperty().toNodeListProperty(),
                           m_showOnlyVisibleItems,
                           m_reverseItemOrder).indexOf(parentModelNode);

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
        rows = filteredList(modelNode.defaultNodeListProperty(),
                            m_showOnlyVisibleItems,
                            m_reverseItemOrder).count();

    return rows;
}

int NavigatorTreeModel::columnCount(const QModelIndex &parent) const
{
    if (parent.column() > 0)
        return 0;

    return ColumnType::Count;
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

    if (m_reverseItemOrder)
        rowNumber = rowCount(dropModelIndex) - rowNumber;

    if (dropModelIndex.model() == this) {
        if (mimeData->hasFormat("application/vnd.bauhaus.itemlibraryinfo")) {
            handleItemLibraryItemDrop(mimeData, rowNumber, dropModelIndex);
        } else if (mimeData->hasFormat("application/vnd.bauhaus.libraryresource.image")) {
            handleItemLibraryImageDrop(mimeData, rowNumber, dropModelIndex);
        } else if (mimeData->hasFormat("application/vnd.bauhaus.libraryresource.font")) {
            handleItemLibraryFontDrop(mimeData, rowNumber, dropModelIndex);
        } else if (mimeData->hasFormat("application/vnd.bauhaus.libraryresource.shader")) {
            handleItemLibraryShaderDrop(mimeData, rowNumber, dropModelIndex);
        } else if (mimeData->hasFormat("application/vnd.bauhaus.libraryresource.sound")) {
            handleItemLibrarySoundDrop(mimeData, rowNumber, dropModelIndex);
        } else if (mimeData->hasFormat("application/vnd.bauhaus.libraryresource.texture3d")) {
            handleItemLibraryTexture3dDrop(mimeData, rowNumber, dropModelIndex);
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
    bool moveNodesAfter = true;

    if (foundTarget) {
        if (!hints.canBeDroppedInNavigator())
            return;

        bool validContainer = false;
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
                        validContainer = true;
                    } else if (targetProperty.parentModelNode().isSubclassOf("QtQuick3D.View3D")) {
                        // see if View3D has environment set to it
                        BindingProperty envNodeProp = targetProperty.parentModelNode().bindingProperty("environment");
                        if (envNodeProp.isValid())  {
                            ModelNode envNode = envNodeProp.resolveToModelNode();
                            if (envNode.isValid())
                                targetEnv = envNode;
                        }
                        validContainer = true;
                    }
                    insertIntoList("effects", targetEnv);
                } else if (newModelNode.isSubclassOf("QtQuick3D.Material")) {
                    if (targetProperty.parentModelNode().isSubclassOf("QtQuick3D.Model")) {
                        // Insert material dropped to a model node into the materials list of the model
                        ModelNode targetModel;
                        targetModel = targetProperty.parentModelNode();
                        insertIntoList("materials", targetModel);
                        validContainer = true;
                    }
                } else {
                    const bool isShader = newModelNode.isSubclassOf("QtQuick3D.Shader");
                    if (isShader || newModelNode.isSubclassOf("QtQuick3D.Command")) {
                        if (targetProperty.parentModelNode().isSubclassOf("QtQuick3D.Pass")) {
                            // Shaders and commands inserted into a Pass will be added to proper list.
                            // They are also moved to the same level as the pass, as passes don't
                            // allow child nodes (QTBUG-86219).
                            ModelNode targetModel;
                            targetModel = targetProperty.parentModelNode();
                            if (isShader)
                                insertIntoList("shaders", targetModel);
                            else
                                insertIntoList("commands", targetModel);
                            NodeAbstractProperty parentProp = targetProperty.parentProperty();
                            if (parentProp.isValid()) {
                                targetProperty = parentProp;
                                targetModel = targetProperty.parentModelNode();
                                targetRowNumber = rowCount(indexForModelNode(targetModel));

                                // Move node to new parent within the same transaction as we don't
                                // want undo to place the node under invalid parent
                                moveNodesAfter = false;
                                moveNodesInteractive(targetProperty, {newQmlObjectNode}, targetRowNumber, false);
                                validContainer = true;
                            }
                        }
                    }
                }
                if (!validContainer) {
                    validContainer = NodeHints::fromModelNode(targetProperty.parentModelNode()).canBeContainerFor(newModelNode);
                    if (!validContainer)
                        newQmlObjectNode.destroy();
                }
            }
        });

        if (validContainer) {
            if (moveNodesAfter && newQmlObjectNode.isValid() && targetProperty.isNodeListProperty()) {
                QList<ModelNode> newModelNodeList;
                newModelNodeList.append(newQmlObjectNode);

                moveNodesInteractive(targetProperty, newModelNodeList, targetRowNumber);
            }

            if (newQmlObjectNode.isValid())
                m_view->setSelectedModelNode(newQmlObjectNode.modelNode());
        }

        const QStringList copyFiles = itemLibraryEntry.extraFilePaths();
        if (!copyFiles.isEmpty()) {
            // Files are copied into the same directory as the current qml document
            for (const auto &copyFile : copyFiles) {
                QFileInfo fi(copyFile);
                const QString targetFile = DocumentManager::currentFilePath().toFileInfo().dir()
                        .absoluteFilePath(fi.fileName());
                // We don't want to overwrite existing default files
                if (!QFileInfo::exists(targetFile)) {
                    if (!QFile::copy(copyFile, targetFile))
                        qWarning() << QStringLiteral("Copying extra file '%1' failed.").arg(copyFile);
                }
            }
        }
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
        const QString imagePath = DocumentManager::currentFilePath().toFileInfo().dir().relativeFilePath(imageSource); // relative to .ui.qml file

        ModelNode newModelNode;

        if (!dropAsImage3dTexture(targetNode, targetProperty, imagePath, newModelNode)) {
            if (targetNode.isSubclassOf("QtQuick.Image")
                    || targetNode.isSubclassOf("QtQuick.BorderImage")) {
                // if dropping an image on an existing image, set the source
                targetNode.variantProperty("source").setValue(imagePath);
            } else {
                m_view->executeInTransaction("NavigatorTreeModel::handleItemLibraryImageDrop", [&] {
                    // create an image
                    QmlItemNode newItemNode = QmlItemNode::createQmlItemNodeFromImage(m_view, imageSource, QPointF(), targetProperty, false);
                    if (NodeHints::fromModelNode(targetProperty.parentModelNode()).canBeContainerFor(newItemNode.modelNode()))
                        newModelNode = newItemNode.modelNode();
                    else
                        newItemNode.destroy();
                });
            }
        }

        if (newModelNode.isValid()) {
            moveNodesInteractive(targetProperty, {newModelNode}, targetRowNumber);
            m_view->setSelectedModelNode(newModelNode);
        }
    }
}

void NavigatorTreeModel::handleItemLibraryFontDrop(const QMimeData *mimeData, int rowNumber,
                                                   const QModelIndex &dropModelIndex)
{
    QTC_ASSERT(m_view, return);
    const QModelIndex rowModelIndex = dropModelIndex.sibling(dropModelIndex.row(), 0);
    int targetRowNumber = rowNumber;
    NodeAbstractProperty targetProperty;

    bool foundTarget = findTargetProperty(rowModelIndex, this, &targetProperty, &targetRowNumber);
    if (foundTarget) {
        ModelNode targetNode(modelNodeForIndex(rowModelIndex));

        const QString fontFamily = QString::fromUtf8(
                    mimeData->data("application/vnd.bauhaus.libraryresource.font"));

        ModelNode newModelNode;

        m_view->executeInTransaction("NavigatorTreeModel::handleItemLibraryFontDrop", [&] {
            if (targetNode.isSubclassOf("QtQuick.Text")) {
                // if dropping into an existing Text, update font
                targetNode.variantProperty("font.family").setValue(fontFamily);
            } else {
                // create a Text node
                QmlItemNode newItemNode = QmlItemNode::createQmlItemNodeFromFont(
                            m_view, fontFamily, QPointF(), targetProperty, false);
                if (NodeHints::fromModelNode(targetProperty.parentModelNode()).canBeContainerFor(newItemNode.modelNode()))
                    newModelNode = newItemNode.modelNode();
                else
                    newItemNode.destroy();
            }
        });

        if (newModelNode.isValid()) {
            moveNodesInteractive(targetProperty, {newModelNode}, targetRowNumber);
            m_view->setSelectedModelNode(newModelNode);
        }
    }
}

void NavigatorTreeModel::handleItemLibraryShaderDrop(const QMimeData *mimeData, int rowNumber,
                                                     const QModelIndex &dropModelIndex)
{
    QTC_ASSERT(m_view, return);
    Import import = Import::createLibraryImport(QStringLiteral("QtQuick3D"));
    bool addImport = false;
    if (!m_view->model()->hasImport(import, true, true)) {
        const QList<Import> possImports = m_view->model()->possibleImports();
        for (const auto &possImport : possImports) {
            if (possImport.url() == import.url()) {
                import = possImport;
                addImport = true;
                m_view->model()->changeImports({import}, {});
                break;
            }
        }
    }

    const QModelIndex rowModelIndex = dropModelIndex.sibling(dropModelIndex.row(), 0);
    int targetRowNumber = rowNumber;
    NodeAbstractProperty targetProperty;

    bool foundTarget = findTargetProperty(rowModelIndex, this, &targetProperty, &targetRowNumber);
    if (foundTarget) {
        ModelNode targetNode(modelNodeForIndex(rowModelIndex));
        ModelNode newModelNode;

        const QString shaderSource = QString::fromUtf8(mimeData->data("application/vnd.bauhaus.libraryresource"));
        const bool fragShader = mimeData->data("application/vnd.bauhaus.libraryresource.shader").startsWith('f');
        const QString relPath = DocumentManager::currentFilePath().toFileInfo().dir().relativeFilePath(shaderSource);

        m_view->executeInTransaction("NavigatorTreeModel::handleItemLibraryShaderDrop", [&] {
            if (targetNode.isSubclassOf("QtQuick3D.Shader")) {
                // if dropping into an existing Shader, update
                if (fragShader)
                    targetNode.variantProperty("stage").setEnumeration("Shader.Fragment");
                else
                    targetNode.variantProperty("stage").setEnumeration("Shader.Vertex");
                targetNode.variantProperty("shader").setValue(relPath);
            } else {
                // create a new Shader
                ItemLibraryEntry itemLibraryEntry;
                itemLibraryEntry.setName("Shader");
                itemLibraryEntry.setType("QtQuick3D.Shader", 1, 0);

                // set shader properties
                PropertyName prop = "shader";
                QString type = "QByteArray";
                QVariant val = relPath;
                itemLibraryEntry.addProperty(prop, type, val);
                prop = "stage";
                type = "enum";
                val = fragShader ? "Shader.Fragment" : "Shader.Vertex";
                itemLibraryEntry.addProperty(prop, type, val);

                // create a texture
                newModelNode = QmlItemNode::createQmlObjectNode(m_view, itemLibraryEntry, {},
                                                                targetProperty, false);

                // Rename the node based on shader source
                QFileInfo fi(relPath);
                newModelNode.setIdWithoutRefactoring(m_view->generateNewId(fi.baseName(),
                                                                           "shader"));
            }
        });

        if (newModelNode.isValid()) {
            moveNodesInteractive(targetProperty, {newModelNode}, targetRowNumber);
            m_view->setSelectedModelNode(newModelNode);
        }

        if (addImport)
            QmlDesignerPlugin::instance()->currentDesignDocument()->updateSubcomponentManager();
    }
}

void NavigatorTreeModel::handleItemLibrarySoundDrop(const QMimeData *mimeData, int rowNumber,
                                                    const QModelIndex &dropModelIndex)
{
    QTC_ASSERT(m_view, return);
    Import import = Import::createLibraryImport(QStringLiteral("QtMultimedia"));
    bool addImport = false;
    if (!m_view->model()->hasImport(import, true, true)) {
        const QList<Import> possImports = m_view->model()->possibleImports();
        for (const auto &possImport : possImports) {
            if (possImport.url() == import.url()) {
                import = possImport;
                addImport = true;
                m_view->model()->changeImports({import}, {});
                break;
            }
        }
    }

    const QModelIndex rowModelIndex = dropModelIndex.sibling(dropModelIndex.row(), 0);
    int targetRowNumber = rowNumber;
    NodeAbstractProperty targetProperty;

    bool foundTarget = findTargetProperty(rowModelIndex, this, &targetProperty, &targetRowNumber);
    if (foundTarget) {
        ModelNode targetNode(modelNodeForIndex(rowModelIndex));
        ModelNode newModelNode;

        const QString soundSource = QString::fromUtf8(mimeData->data("application/vnd.bauhaus.libraryresource"));
        const QString relPath = DocumentManager::currentFilePath().toFileInfo().dir().relativeFilePath(soundSource);

        m_view->executeInTransaction("NavigatorTreeModel::handleItemLibrarySoundDrop", [&] {
            if (targetNode.isSubclassOf("QtMultimedia.SoundEffect")) {
                // if dropping into on an existing SoundEffect, update
                targetNode.variantProperty("source").setValue(relPath);
            } else {
                // create a new SoundEffect
                ItemLibraryEntry itemLibraryEntry;
                itemLibraryEntry.setName("SoundEffect");
                itemLibraryEntry.setType("QtMultimedia.SoundEffect", 1, 0);

                // set shader properties
                PropertyName prop = "source";
                QString type = "QUrl";
                QVariant val = relPath;
                itemLibraryEntry.addProperty(prop, type, val);

                // create a texture
                newModelNode = QmlItemNode::createQmlObjectNode(m_view, itemLibraryEntry, {},
                                                                targetProperty, false);

                // Rename the node based on source
                QFileInfo fi(relPath);
                newModelNode.setIdWithoutRefactoring(m_view->generateNewId(fi.baseName(),
                                                                           "soundEffect"));
            }
        });

        if (newModelNode.isValid()) {
            moveNodesInteractive(targetProperty, {newModelNode}, targetRowNumber);
            m_view->setSelectedModelNode(newModelNode);
        }

        if (addImport)
            QmlDesignerPlugin::instance()->currentDesignDocument()->updateSubcomponentManager();
    }
}

void NavigatorTreeModel::handleItemLibraryTexture3dDrop(const QMimeData *mimeData, int rowNumber,
                                                        const QModelIndex &dropModelIndex)
{
    QTC_ASSERT(m_view, return);
    Import import = Import::createLibraryImport(QStringLiteral("QtQuick3D"));
    if (!m_view->model()->hasImport(import, true, true))
        return;

    const QModelIndex rowModelIndex = dropModelIndex.sibling(dropModelIndex.row(), 0);
    int targetRowNumber = rowNumber;
    NodeAbstractProperty targetProperty;

    bool foundTarget = findTargetProperty(rowModelIndex, this, &targetProperty, &targetRowNumber);
    if (foundTarget) {
        ModelNode targetNode(modelNodeForIndex(rowModelIndex));

        const QString imageSource = QString::fromUtf8(
                mimeData->data("application/vnd.bauhaus.libraryresource")); // absolute path
        const QString imagePath = DocumentManager::currentFilePath().toFileInfo().dir()
                .relativeFilePath(imageSource); // relative to qml file

        ModelNode newModelNode;

        if (!dropAsImage3dTexture(targetNode, targetProperty, imagePath, newModelNode)) {
            m_view->executeInTransaction("NavigatorTreeModel::handleItemLibraryTexture3dDrop", [&] {
                // create a standalone Texture3D at drop location
                newModelNode = createTextureNode(targetProperty, imagePath);
                if (!NodeHints::fromModelNode(targetProperty.parentModelNode()).canBeContainerFor(newModelNode))
                    newModelNode.destroy();
            });
        }

        if (newModelNode.isValid()) {
            moveNodesInteractive(targetProperty, {newModelNode}, targetRowNumber);
            m_view->setSelectedModelNode(newModelNode);
        }
    }
}

bool NavigatorTreeModel::dropAsImage3dTexture(const ModelNode &targetNode,
                                              const NodeAbstractProperty &targetProp,
                                              const QString &imagePath,
                                              ModelNode &newNode)
{
    if (targetNode.isSubclassOf("QtQuick3D.Material")) {
        // if dropping an image on a default material, create a texture instead of image
        ChooseTexturePropertyDialog *dialog = nullptr;
        if (targetNode.isSubclassOf("QtQuick3D.DefaultMaterial") || targetNode.isSubclassOf("QtQuick3D.PrincipledMaterial")) {
            // Show texture property selection dialog
            dialog = new ChooseTexturePropertyDialog(targetNode, Core::ICore::dialogParent());
            dialog->exec();
        }
        if (!dialog || dialog->result() == QDialog::Accepted) {
            m_view->executeInTransaction("NavigatorTreeModel::dropAsImage3dTexture", [&] {
                newNode = createTextureNode(targetProp, imagePath);
                if (newNode.isValid() && dialog) {
                    // Automatically set the texture to selected property
                    targetNode.bindingProperty(dialog->selectedProperty()).setExpression(newNode.validId());
                }
            });
        }
        delete dialog;
        return newNode.isValid();
    } else if (targetNode.isSubclassOf("QtQuick3D.TextureInput")) {
        // If dropping an image on a TextureInput, create a texture on the same level as
        // TextureInput, as the TextureInput doesn't support Texture children (QTBUG-86219)
        m_view->executeInTransaction("NavigatorTreeModel::dropAsImage3dTexture", [&] {
            NodeAbstractProperty parentProp = targetProp.parentProperty();
            newNode = createTextureNode(parentProp, imagePath);
            if (newNode.isValid()) {
                // Automatically set the texture to texture property
                targetNode.bindingProperty("texture").setExpression(newNode.validId());
            }
        });
        return newNode.isValid();
    } else if (targetNode.isSubclassOf("QtQuick3D.Texture")) {
        // if dropping an image on an existing texture, set the source
        targetNode.variantProperty("source").setValue(imagePath);
        return true;
    }

    return false;
}

ModelNode NavigatorTreeModel::createTextureNode(const NodeAbstractProperty &targetProp,
                                                const QString &imagePath)
{
    if (targetProp.isValid()) {
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
        ModelNode newModelNode = QmlItemNode::createQmlObjectNode(m_view, itemLibraryEntry, {},
                                                                  targetProp, false);

        // Rename the node based on source image
        QFileInfo fi(imagePath);
        newModelNode.setIdWithoutRefactoring(m_view->generateNewId(fi.baseName(), "textureImage"));
        return newModelNode;
    }
    return {};
}

TypeName propertyType(const NodeAbstractProperty &property)
{
    return property.parentModelNode().metaInfo().propertyTypeName(property.name());
}

void NavigatorTreeModel::moveNodesInteractive(NodeAbstractProperty &parentProperty,
                                              const QList<ModelNode> &modelNodes,
                                              int targetIndex,
                                              bool executeInTransaction)
{
    QTC_ASSERT(m_view, return);

    auto doMoveNodesInteractive = [&parentProperty, modelNodes, targetIndex](){
        const TypeName propertyQmlType = propertyType(parentProperty);
        int idx = targetIndex;
        for (const ModelNode &modelNode : modelNodes) {
            if (modelNode.isValid()
                    && modelNode != parentProperty.parentModelNode()
                    && !modelNode.isAncestorOf(parentProperty.parentModelNode())
                    && (modelNode.metaInfo().isSubclassOf(propertyQmlType) || propertyQmlType == "alias")) {
                //### todo: allowing alias is just a heuristic
                //once the MetaInfo is part of instances we can do this right

                bool nodeCanBeMovedToParentProperty = removeModelNodeFromNodeProperty(parentProperty, modelNode);
                if (nodeCanBeMovedToParentProperty) {
                    reparentModelNodeToNodeProperty(parentProperty, modelNode);
                    slideModelNodeInList(parentProperty, modelNode, idx++);
                }
            }
        }
    };

    if (executeInTransaction)
        m_view->executeInTransaction("NavigatorTreeModel::moveNodesInteractive", doMoveNodesInteractive);
    else
        doMoveNodesInteractive();
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
    if (index.column() == ColumnType::Alias && role == Qt::CheckStateRole) {
        QTC_ASSERT(m_view, return false);
        m_view->handleChangedExport(modelNode, value.toInt() != 0);
    } else if (index.column() == ColumnType::Visibility && role == Qt::CheckStateRole) {
        QmlVisualNode(modelNode).setVisibilityOverride(value.toInt() == 0);
    } else if (index.column() == ColumnType::Lock && role == Qt::CheckStateRole) {
        modelNode.setLocked(value.toInt() != 0);
    }

    return true;
}

void NavigatorTreeModel::notifyDataChanged(const ModelNode &modelNode)
{
    const QModelIndex index = indexForModelNode(modelNode);
    const QAbstractItemModel *model = index.model();
    const QModelIndex sibling = model ? model->sibling(index.row(), ColumnType::Count - 1, index) : QModelIndex();
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

void NavigatorTreeModel::setOrder(bool reverseItemOrder)
{
    m_reverseItemOrder = reverseItemOrder;
    resetModel();
}

void NavigatorTreeModel::resetModel()
{
    beginResetModel();
    m_nodeIndexHash.clear();
    endResetModel();
}

void NavigatorTreeModel::updateToolTipPixmap(const ModelNode &node, const QPixmap &pixmap)
{
    emit toolTipPixmapUpdated(node.id(), pixmap);
}

} // QmlDesigner
