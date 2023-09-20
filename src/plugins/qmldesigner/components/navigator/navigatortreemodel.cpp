// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "navigatortreemodel.h"
#include "navigatorview.h"
#include "navigatorwidget.h"
#include "choosefrompropertylistdialog.h"
#include "qmldesignerconstants.h"
#include "qmldesignerplugin.h"
#include "assetslibrarywidget.h"

#include <abstractview.h>
#include <bindingproperty.h>
#include <coreplugin/icore.h>
#include <designeractionmanager.h>
#include <designersettings.h>
#include <import.h>
#include <invalididexception.h>
#include <materialutils.h>
#include <metainfo.h>
#include <model/modelutils.h>
#include <nodeabstractproperty.h>
#include <nodehints.h>
#include <nodelistproperty.h>
#include <nodeproperty.h>
#include <rewritingexception.h>
#include <variantproperty.h>
#include <qmldesignerconstants.h>
#include <qmlitemnode.h>

#include <qmlprojectmanager/qmlproject.h>

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
#include <QTimer>

#include <coreplugin/messagebox.h>

#include <QtDebug>

namespace QmlDesigner {

static QList<ModelNode> modelNodesFromMimeData(const QMimeData *mineData, AbstractView *view)
{
    QByteArray encodedModelNodeData = mineData->data(Constants::MIME_TYPE_MODELNODE_LIST);
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
    const bool canBeContainer = NodeHints::fromModelNode(targetProperty.parentModelNode())
                                    .canBeContainerFor(modelNodeList.first());
    return !(targetProperty.isNodeProperty() && modelNodeList.size() > 1) && canBeContainer;
}

inline static QString msgUnknownItem(const QString &t)
{
    return NavigatorTreeModel::tr("Unknown component: %1").arg(t);
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
        if (parentProperty.parentModelNode().type().startsWith("Effects."))
            return;

        if (!modelNode.hasParentProperty() || parentProperty != modelNode.parentProperty()) {
            if (isInLayoutable(parentProperty)) {
                removePosition(modelNode);
                parentProperty.reparentHere(modelNode);
            } else {
                if (QmlItemNode::isValidQmlItemNode(modelNode)) {
                    if (modelNode.hasParentProperty() && modelNode.parentProperty().name() == "layer.effect") {
                        parentProperty = parentProperty.parentModelNode().nodeAbstractProperty("layer.effect");
                        QmlItemNode::placeEffectNode(parentProperty, modelNode, true);
                    } else {
                        QPointF scenePosition = QmlItemNode(modelNode).instanceScenePosition();
                        parentProperty.reparentHere(modelNode);
                        if (!scenePosition.isNull())
                            setScenePosition(modelNode, scenePosition);
                    }

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
        return ModelUtils::isThisOrAncestorLocked(modelNode);

    if (role == ModelNodeRole)
        return QVariant::fromValue<ModelNode>(modelNode);

    if (index.column() == ColumnType::Name) {
        if (role == Qt::DisplayRole) {
            return modelNode.displayName();
        } else if (role == Qt::DecorationRole) {
            if (currentQmlObjectNode.hasError())
                return ::Utils::Icons::WARNING.icon();

            return modelNode.typeIcon();

        } else if (role == Qt::ToolTipRole) {
            if (currentQmlObjectNode.hasError()) {
                QString errorString = currentQmlObjectNode.error();
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
            return tr("Toggles whether this component is exported as an "
                      "alias property of the root component.");
    } else if (index.column() == ColumnType::Visibility) { // visible
        if (role == Qt::CheckStateRole)
            return m_view->isNodeInvisible(modelNode) ? Qt::Unchecked : Qt::Checked;
        else if (role == Qt::ToolTipRole && !modelNodeForIndex(index).isRootNode())
            return tr("Toggles the visibility of this component in the 2D and 3D views.\n"
                      "This is independent of the visibility property.");
    } else if (index.column() == ColumnType::Lock) { // lock
        if (role == Qt::CheckStateRole)
            return modelNode.locked() ? Qt::Checked : Qt::Unchecked;
        else if (role == Qt::ToolTipRole && !modelNodeForIndex(index).isRootNode())
            return tr("Toggles whether this component is locked.\n"
                      "Locked components cannot be modified or selected.");
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
        if (ModelUtils::isThisOrAncestorLocked(modelNode))
            return Qt::ItemIsEnabled | Qt::ItemIsUserCheckable;
        else
            return Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsUserCheckable;
    }

    if (ModelUtils::isThisOrAncestorLocked(modelNode))
        return Qt::NoItemFlags;

    if (index.column() == ColumnType::Name)
        return Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable | Qt::ItemIsDropEnabled | Qt::ItemIsDragEnabled;

    return Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsUserCheckable;
}

static void appendForcedNodes(const NodeListProperty &property, QList<ModelNode> &list)
{
    const QSet<ModelNode> set = QSet<ModelNode>(list.constBegin(), list.constEnd());
    for (const ModelNode &node : property.parentModelNode().directSubModelNodes()) {
        if (!set.contains(node)) {
            const QStringList visibleProperties = NodeHints::fromModelNode(property.parentModelNode()).visibleNonDefaultProperties();
            if (visibleProperties.contains(QString::fromUtf8(node.parentProperty().name())))
                list.append(node);
        }
    }
}

QList<ModelNode> NavigatorTreeModel::filteredList(const NodeListProperty &property, bool filter, bool reverseOrder) const
{
    auto it = m_rowCache.find(property.parentModelNode());

    if (it != m_rowCache.end())
        return it.value();

    QList<ModelNode> list;
    QList<ModelNode> propertyNodes = property.toModelNodeList();
    QList<ModelNode> nameFilteredList;

    if (m_nameFilter.isEmpty()) {
        nameFilteredList = propertyNodes;
    } else {
        nameFilteredList.append(::Utils::filtered(propertyNodes, [&](const ModelNode &arg) {
            const bool value = m_nameFilteredList.contains(arg);
            return value;
        }));
    }

    if (filter) {
        list.append(::Utils::filtered(nameFilteredList, [](const ModelNode &arg) {
            const bool value = (QmlItemNode::isValidQmlItemNode(arg) || NodeHints::fromModelNode(arg).visibleInNavigator())
                               && arg.id() != Constants::MATERIAL_LIB_ID;
            return value;
        }));
    } else {
        list = nameFilteredList;
    }

    appendForcedNodes(property, list);

    if (reverseOrder)
        std::reverse(list.begin(), list.end());

    m_rowCache.insert(property.parentModelNode(), list);
    return list;
}

QModelIndex NavigatorTreeModel::index(int row, int column, const QModelIndex &parent) const
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

    if (!parentModelNode.isRootNode() && parentModelNode.parentProperty().isNodeListProperty()) {
        row = filteredList(parentModelNode.parentProperty().toNodeListProperty(),
                           m_showOnlyVisibleItems,
                           m_reverseItemOrder).indexOf(parentModelNode);
    }

    return createIndexFromModelNode(row, 0, parentModelNode);
}

int NavigatorTreeModel::rowCount(const QModelIndex &parent) const
{
    if (!m_view->isAttached() || parent.column() > 0)
        return 0;

    const ModelNode modelNode = modelNodeForIndex(parent);

    if (!modelNode.isValid())
        return 1;

    int rows = 0;

    if (modelNode.defaultNodeListProperty().isValid()) {
        rows = filteredList(modelNode.defaultNodeListProperty(),
                            m_showOnlyVisibleItems,
                            m_reverseItemOrder)
                   .size();
    }

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
    static const QStringList types({Constants::MIME_TYPE_MODELNODE_LIST,
                                    Constants::MIME_TYPE_ITEM_LIBRARY_INFO,
                                    Constants::MIME_TYPE_TEXTURE,
                                    Constants::MIME_TYPE_MATERIAL,
                                    Constants::MIME_TYPE_BUNDLE_TEXTURE,
                                    Constants::MIME_TYPE_BUNDLE_MATERIAL,
                                    Constants::MIME_TYPE_BUNDLE_EFFECT,
                                    Constants::MIME_TYPE_ASSETS});

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

    mimeData->setData(Constants::MIME_TYPE_MODELNODE_LIST, encodedModelNodeData);

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

    NavigatorWidget *widget = qobject_cast<NavigatorWidget *>(m_view->widgetInfo().widget);
    if (widget)
        widget->setDragType("");

    if (dropModelIndex.model() == this) {
        if (mimeData->hasFormat(Constants::MIME_TYPE_ITEM_LIBRARY_INFO)) {
            handleItemLibraryItemDrop(mimeData, rowNumber, dropModelIndex);
        } else if (mimeData->hasFormat(Constants::MIME_TYPE_TEXTURE)) {
            handleTextureDrop(mimeData, dropModelIndex);
        } else if (mimeData->hasFormat(Constants::MIME_TYPE_MATERIAL)) {
            handleMaterialDrop(mimeData, dropModelIndex);
        } else if (mimeData->hasFormat(Constants::MIME_TYPE_BUNDLE_TEXTURE)) {
            QByteArray filePath = mimeData->data(Constants::MIME_TYPE_BUNDLE_TEXTURE);
            ModelNode targetNode(modelNodeForIndex(dropModelIndex));
            if (targetNode.metaInfo().isQtQuick3DModel())
                m_view->emitCustomNotification("apply_asset_to_model3D", {targetNode}, {filePath}); // To MaterialBrowserView
        } else if (mimeData->hasFormat(Constants::MIME_TYPE_BUNDLE_MATERIAL)) {
            ModelNode targetNode(modelNodeForIndex(dropModelIndex));
            if (targetNode.isValid())
                m_view->emitCustomNotification("drop_bundle_material", {targetNode}); // To ContentLibraryView
        } else if (mimeData->hasFormat(Constants::MIME_TYPE_BUNDLE_EFFECT)) {
            ModelNode targetNode(modelNodeForIndex(dropModelIndex));
            if (targetNode.isValid())
                m_view->emitCustomNotification("drop_bundle_effect", {targetNode}); // To ContentLibraryView
        } else if (mimeData->hasFormat(Constants::MIME_TYPE_ASSETS)) {
            const QStringList assetsPaths = QString::fromUtf8(mimeData->data(Constants::MIME_TYPE_ASSETS)).split(',');
            NodeAbstractProperty targetProperty;

            const QModelIndex rowModelIndex = dropModelIndex.sibling(dropModelIndex.row(), 0);
            int targetRowNumber = rowNumber;
            bool foundTarget = findTargetProperty(rowModelIndex, this, &targetProperty, &targetRowNumber);
            if (foundTarget) {
                QList<ModelNode> addedNodes;
                ModelNode currNode;

                // Adding required imports is skipped if we are editing in-file subcomponent
                DesignDocument *document = QmlDesignerPlugin::instance()->currentDesignDocument();
                if (document && !document->inFileComponentModelActive()) {
                    QSet<QString> neededImports;
                    for (const QString &assetPath : assetsPaths) {
                        QString assetType = AssetsLibraryWidget::getAssetTypeAndData(assetPath).first;
                        if (assetType == Constants::MIME_TYPE_ASSET_SHADER)
                            neededImports.insert("QtQuick3D");
                        else if (assetType == Constants::MIME_TYPE_ASSET_SOUND)
                            neededImports.insert("QtMultimedia");

                        if (neededImports.size() == 2)
                            break;
                    };

                    for (const QString &import : std::as_const(neededImports))
                        addImport(import);
                }

                bool moveNodesAfter = true;
                m_view->executeInTransaction("NavigatorTreeModel::dropMimeData", [&] {
                    for (const QString &assetPath : assetsPaths) {
                        auto assetTypeAndData = AssetsLibraryWidget::getAssetTypeAndData(assetPath);
                        QString assetType = assetTypeAndData.first;
                        QString assetData = QString::fromUtf8(assetTypeAndData.second);
                        if (assetType == Constants::MIME_TYPE_ASSET_IMAGE) {
                            currNode = handleItemLibraryImageDrop(assetPath, targetProperty,
                                                                  rowModelIndex, moveNodesAfter);
                        } else if (assetType == Constants::MIME_TYPE_ASSET_FONT) {
                            currNode = handleItemLibraryFontDrop(assetData, // assetData is fontFamily
                                                                 targetProperty, rowModelIndex);
                        } else if (assetType == Constants::MIME_TYPE_ASSET_SHADER) {
                            currNode = handleItemLibraryShaderDrop(assetPath, assetData == "f",
                                                                   targetProperty, rowModelIndex,
                                                                   moveNodesAfter);
                        } else if (assetType == Constants::MIME_TYPE_ASSET_SOUND) {
                            currNode = handleItemLibrarySoundDrop(assetPath, targetProperty,
                                                                  rowModelIndex);
                        } else if (assetType == Constants::MIME_TYPE_ASSET_TEXTURE3D) {
                            currNode = handleItemLibraryTexture3dDrop(assetPath, targetProperty,
                                                                      rowModelIndex, moveNodesAfter);
                        } else if (assetType == Constants::MIME_TYPE_ASSET_EFFECT) {
                            currNode = handleItemLibraryEffectDrop(assetPath, rowModelIndex);
                            moveNodesAfter = false;
                        }

                        if (currNode.isValid())
                            addedNodes.append(currNode);
                    }
                });

                if (!addedNodes.isEmpty()) {
                    if (moveNodesAfter)
                        moveNodesInteractive(targetProperty, addedNodes, rowNumber);
                    m_view->setSelectedModelNodes(addedNodes);
                }
            }
        } else if (mimeData->hasFormat(Constants::MIME_TYPE_MODELNODE_LIST)) {
            handleInternalDrop(mimeData, rowNumber, dropModelIndex);
        }
    }

    if (m_view && m_view->model())
        m_view->model()->endDrag();

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
        createItemLibraryEntryFromMimeData(mimeData->data(Constants::MIME_TYPE_ITEM_LIBRARY_INFO));

    const NodeHints hints = NodeHints::fromItemLibraryEntry(itemLibraryEntry);

    const QString targetPropertyName = hints.forceNonDefaultProperty();

    bool foundTarget = findTargetProperty(rowModelIndex, this, &targetProperty, &targetRowNumber, targetPropertyName.toUtf8());
    bool moveNodesAfter = true;

    if (foundTarget) {
        if (!hints.canBeDroppedInNavigator())
            return;

        bool validContainer = false;
        ModelNode targetNode = targetProperty.parentModelNode();

        QmlObjectNode newQmlObjectNode;
        m_view->executeInTransaction("NavigatorTreeModel::handleItemLibraryItemDrop", [&] {
            newQmlObjectNode = QmlItemNode::createQmlObjectNode(m_view,
                                                                itemLibraryEntry,
                                                                QPointF(),
                                                                targetProperty,
                                                                false);
            if (ModelNode newModelNode = newQmlObjectNode.modelNode()) {
                if (newModelNode.metaInfo().isQtQuick3DMaterial()) {
                    // Don't allow dropping materials on any node but Models
                    if (!targetNode.metaInfo().isQtQuick3DModel()) {
                        newQmlObjectNode.destroy();
                        return;
                    }
                    MaterialUtils::assignMaterialTo3dModel(m_view, targetNode, newModelNode);
                }

                ChooseFromPropertyListDialog *dialog = ChooseFromPropertyListDialog::createIfNeeded(
                            targetNode, newModelNode, Core::ICore::dialogParent());
                if (dialog) {
                    bool soloProperty = dialog->isSoloProperty();
                    if (!soloProperty)
                        dialog->exec();
                    if (soloProperty || dialog->result() == QDialog::Accepted) {
                        TypeName selectedProp = dialog->selectedProperty();

                        // Pass and TextureInput can't have children, so we have to move nodes under parent
                        if (((newModelNode.metaInfo().isQtQuick3DShader()
                              || newModelNode.metaInfo().isQtQuick3DCommand()
                              || newModelNode.metaInfo().isQtQuick3DBuffer())
                             && targetProperty.parentModelNode().metaInfo().isQtQuick3DPass())
                            || (newModelNode.metaInfo().isQtQuick3DTexture()
                                && targetProperty.parentModelNode().metaInfo().isQtQuick3DTextureInput())) {
                            if (moveNodeToParent(targetProperty, newQmlObjectNode)) {
                                targetProperty = targetProperty.parentProperty();
                                moveNodesAfter = false;
                            }
                        }

                        if (targetNode.metaInfo().property(selectedProp).isListProperty()) {
                            BindingProperty listProp = targetNode.bindingProperty(selectedProp);
                            listProp.addModelNodeToArray(newModelNode);
                            validContainer = true;
                        } else {
                            targetNode.bindingProperty(dialog->selectedProperty()).setExpression(newModelNode.validId());
                            validContainer = true;
                        }
                    }
                    delete dialog;
                }

                if (newModelNode.metaInfo().isQtQuick3DView3D()) {
                    const QList<ModelNode> models = newModelNode.subModelNodesOfType(
                        m_view->model()->qtQuick3DModelMetaInfo());
                    QTC_ASSERT(models.size() == 1, return);
                    MaterialUtils::assignMaterialTo3dModel(m_view, models.at(0));
                } else if (newModelNode.metaInfo().isQtQuick3DModel()) {
                    MaterialUtils::assignMaterialTo3dModel(m_view, newModelNode);
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
    }
}

void NavigatorTreeModel::handleTextureDrop(const QMimeData *mimeData, const QModelIndex &dropModelIndex)
{
    QTC_ASSERT(m_view, return);

    const QModelIndex rowModelIndex = dropModelIndex.sibling(dropModelIndex.row(), 0);
    QmlObjectNode targetNode = modelNodeForIndex(rowModelIndex);
    if (!targetNode.isValid())
        return;

    qint32 internalId = mimeData->data(Constants::MIME_TYPE_TEXTURE).toInt();
    ModelNode texNode = m_view->modelNodeForInternalId(internalId);
    QTC_ASSERT(texNode.isValid(), return);

    if (targetNode.modelNode().metaInfo().isQtQuick3DModel()) {
        m_view->emitCustomNotification("apply_texture_to_model3D", {targetNode, texNode});
    } else {
        auto *dialog = ChooseFromPropertyListDialog::createIfNeeded(targetNode, texNode, Core::ICore::dialogParent());
        if (dialog) {
            bool soloProperty = dialog->isSoloProperty();
            if (!soloProperty)
                dialog->exec();

            if (soloProperty || dialog->result() == QDialog::Accepted)
                targetNode.setBindingProperty(dialog->selectedProperty(), texNode.id());

            delete dialog;
        }
    }
}

void NavigatorTreeModel::handleMaterialDrop(const QMimeData *mimeData, const QModelIndex &dropModelIndex)
{
    QTC_ASSERT(m_view, return);

    const QModelIndex rowModelIndex = dropModelIndex.sibling(dropModelIndex.row(), 0);
    ModelNode targetNode = modelNodeForIndex(rowModelIndex);
    if (!targetNode.metaInfo().isQtQuick3DModel())
        return;

    qint32 internalId = mimeData->data(Constants::MIME_TYPE_MATERIAL).toInt();
    ModelNode matNode = m_view->modelNodeForInternalId(internalId);

    m_view->executeInTransaction(__FUNCTION__, [&] {
        MaterialUtils::assignMaterialTo3dModel(m_view, targetNode, matNode);
    });
}

ModelNode NavigatorTreeModel::handleItemLibraryImageDrop(const QString &imagePath,
                                                         NodeAbstractProperty targetProperty,
                                                         const QModelIndex &rowModelIndex,
                                                         bool &outMoveNodesAfter)
{
    QTC_ASSERT(m_view, return {});

    ModelNode targetNode(modelNodeForIndex(rowModelIndex));

    const QString imagePathRelative = DocumentManager::currentFilePath().toFileInfo().dir().relativeFilePath(imagePath); // relative to .ui.qml file

    ModelNode newModelNode;

    if (!dropAsImage3dTexture(targetNode, targetProperty, imagePathRelative, newModelNode, outMoveNodesAfter)) {
        if (targetNode.metaInfo().isQtQuickImage() || targetNode.metaInfo().isQtQuickBorderImage()) {
            // if dropping an image on an existing image, set the source
            targetNode.variantProperty("source").setValue(imagePathRelative);
        } else {
            // create an image
            QmlItemNode newItemNode = QmlItemNode::createQmlItemNodeFromImage(m_view, imagePath, QPointF(), targetProperty, false);
            if (NodeHints::fromModelNode(targetProperty.parentModelNode()).canBeContainerFor(newItemNode.modelNode()))
                newModelNode = newItemNode.modelNode();
            else
                newItemNode.destroy();
        }
    }

    return newModelNode;
}

ModelNode NavigatorTreeModel::handleItemLibraryFontDrop(const QString &fontFamily,
                                                        NodeAbstractProperty targetProperty,
                                                        const QModelIndex &rowModelIndex)
{
    QTC_ASSERT(m_view, return {});

    ModelNode targetNode(modelNodeForIndex(rowModelIndex));

    ModelNode newModelNode;

    if (targetNode.metaInfo().isQtQuickText()) {
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

    return newModelNode;
}

void NavigatorTreeModel::addImport(const QString &importName)
{
    if (!ModelUtils::addImportWithCheck(importName, m_view->model()))
        qWarning() << __FUNCTION__ << "Adding import failed:" << importName;
}

bool QmlDesigner::NavigatorTreeModel::moveNodeToParent(const NodeAbstractProperty &targetProperty,
                                                       const ModelNode &node)
{
    NodeAbstractProperty parentProp = targetProperty.parentProperty();
    if (parentProp.isValid()) {
        ModelNode targetModel = parentProp.parentModelNode();
        int targetRowNumber = rowCount(indexForModelNode(targetModel));
        moveNodesInteractive(parentProp, {node}, targetRowNumber, false);
        return true;
    }
    return false;
}

ModelNode NavigatorTreeModel::handleItemLibraryShaderDrop(const QString &shaderPath, bool isFragShader,
                                                          NodeAbstractProperty targetProperty,
                                                          const QModelIndex &rowModelIndex,
                                                          bool &outMoveNodesAfter)
{
    QTC_ASSERT(m_view, return {});

    ModelNode targetNode(modelNodeForIndex(rowModelIndex));
    ModelNode newModelNode;

    const QString relPath = DocumentManager::currentFilePath().toFileInfo().dir().relativeFilePath(shaderPath);

    if (targetNode.metaInfo().isQtQuick3DShader()) {
        // if dropping into an existing Shader, update
        targetNode.variantProperty("stage").setEnumeration(isFragShader ? "Shader.Fragment"
                                                                        : "Shader.Vertex");
        targetNode.variantProperty("shader").setValue(relPath);
    } else {
        m_view->executeInTransaction("NavigatorTreeModel::handleItemLibraryShaderDrop", [&] {
            // create a new Shader
            ItemLibraryEntry itemLibraryEntry;
            itemLibraryEntry.setName("Shader");
            itemLibraryEntry.setType("QtQuick3D.Shader", 1, 0);

            // set shader properties
            PropertyName prop = "shader";
            QString type = "QUrl";
            QVariant val = relPath;
            itemLibraryEntry.addProperty(prop, type, val);
            prop = "stage";
            type = "enum";
            val = isFragShader ? "Shader.Fragment" : "Shader.Vertex";
            itemLibraryEntry.addProperty(prop, type, val);

            // create a texture
            newModelNode = QmlItemNode::createQmlObjectNode(m_view, itemLibraryEntry, {},
                                                            targetProperty, false);

            // Rename the node based on shader source
            QFileInfo fi(relPath);
            newModelNode.setIdWithoutRefactoring(
                m_view->model()->generateNewId(fi.baseName(), "shader"));
            // Passes can't have children, so move shader node under parent
            if (targetProperty.parentModelNode().metaInfo().isQtQuick3DPass()) {
                BindingProperty listProp = targetNode.bindingProperty("shaders");
                listProp.addModelNodeToArray(newModelNode);
                outMoveNodesAfter = !moveNodeToParent(targetProperty, newModelNode);
            }
        });
    }

    return newModelNode;
}

ModelNode NavigatorTreeModel::handleItemLibrarySoundDrop(const QString &soundPath,
                                                         NodeAbstractProperty targetProperty,
                                                         const QModelIndex &rowModelIndex)
{
    QTC_ASSERT(m_view, return {});

    ModelNode targetNode(modelNodeForIndex(rowModelIndex));
    ModelNode newModelNode;

    const QString relPath = DocumentManager::currentFilePath().toFileInfo().dir().relativeFilePath(soundPath);

    if (targetNode.metaInfo().isQtMultimediaSoundEffect()) {
        // if dropping into on an existing SoundEffect, update
        targetNode.variantProperty("source").setValue(relPath);
    } else {
        // create a new SoundEffect
        ItemLibraryEntry itemLibraryEntry;
        itemLibraryEntry.setName("SoundEffect");
        itemLibraryEntry.setType("QtMultimedia.SoundEffect", 1, 0);

        // set source property
        PropertyName prop = "source";
        QString type = "QUrl";
        QVariant val = relPath;
        itemLibraryEntry.addProperty(prop, type, val);

        // create a texture
        newModelNode = QmlItemNode::createQmlObjectNode(m_view, itemLibraryEntry, {},
                                                        targetProperty, false);

        // Rename the node based on source
        QFileInfo fi(relPath);
        newModelNode.setIdWithoutRefactoring(
            m_view->model()->generateNewId(fi.baseName(), "soundEffect"));
    }

    return newModelNode;
}

ModelNode NavigatorTreeModel::handleItemLibraryTexture3dDrop(const QString &tex3DPath,
                                                             NodeAbstractProperty targetProperty,
                                                             const QModelIndex &rowModelIndex,
                                                             bool &outMoveNodesAfter)
{
    QTC_ASSERT(m_view, return {});

    Import import = Import::createLibraryImport(QStringLiteral("QtQuick3D"));
    if (!m_view->model()->hasImport(import, true, true))
        return {};

    ModelNode targetNode(modelNodeForIndex(rowModelIndex));

    const QString imagePath = DocumentManager::currentFilePath().toFileInfo().dir()
            .relativeFilePath(tex3DPath); // relative to qml file

    ModelNode newModelNode;

    if (!dropAsImage3dTexture(targetNode, targetProperty, imagePath, newModelNode, outMoveNodesAfter)) {
        m_view->executeInTransaction("NavigatorTreeModel::handleItemLibraryTexture3dDrop", [&] {
            // create a standalone Texture3D at drop location
            newModelNode = createTextureNode(targetProperty, imagePath);
            if (!NodeHints::fromModelNode(targetProperty.parentModelNode()).canBeContainerFor(newModelNode))
                newModelNode.destroy();
        });
    }

    return newModelNode;
}

ModelNode NavigatorTreeModel::handleItemLibraryEffectDrop(const QString &effectPath, const QModelIndex &rowModelIndex)
{
    QTC_ASSERT(m_view, return {});

    ModelNode targetNode(modelNodeForIndex(rowModelIndex));
    ModelNode newModelNode;

    if ((targetNode.hasParentProperty() && targetNode.parentProperty().name() == "layer.effect")
            || !targetNode.metaInfo().isQtQuickItem())
        return newModelNode;

    if (ModelNodeOperations::validateEffect(effectPath)) {
        bool layerEffect = ModelNodeOperations::useLayerEffect();
        newModelNode = QmlItemNode::createQmlItemNodeForEffect(m_view, targetNode, effectPath, layerEffect);
    }

    return newModelNode;
}

bool NavigatorTreeModel::dropAsImage3dTexture(const ModelNode &targetNode,
                                              const NodeAbstractProperty &targetProp,
                                              const QString &imagePath,
                                              ModelNode &newNode,
                                              bool &outMoveNodesAfter)
{
    auto bindToProperty = [&](const PropertyName &propName, bool sibling) {
        m_view->executeInTransaction("NavigatorTreeModel::dropAsImage3dTexture", [&] {
            newNode = createTextureNode(targetProp, imagePath);
            if (newNode.isValid()) {
                targetNode.bindingProperty(propName).setExpression(newNode.validId());

                // If dropping an image on e.g. TextureInput, create a texture on the same level as
                // target, as the target doesn't support Texture children (QTBUG-86219)
                if (sibling)
                    outMoveNodesAfter = !moveNodeToParent(targetProp, newNode);
            }
        });
    };

    if (targetNode.metaInfo().isQtQuick3DDefaultMaterial()
        || targetNode.metaInfo().isQtQuick3DPrincipledMaterial()
        || targetNode.metaInfo().isQtQuick3DSpecularGlossyMaterial()) {
        // if dropping an image on a material, create a texture instead of image
        // Show texture property selection dialog
        auto dialog = ChooseFromPropertyListDialog::createIfNeeded(targetNode,
                                                                   m_view->model()->metaInfo(
                                                                       "QtQuick3D.Texture"),
                                                                   Core::ICore::dialogParent());
        if (!dialog)
            return false;

        dialog->exec();

        if (dialog->result() == QDialog::Accepted) {
            m_view->executeInTransaction("NavigatorTreeModel::dropAsImage3dTexture", [&] {
                newNode = createTextureNode(targetProp, imagePath);
                if (newNode.isValid()) // Automatically set the texture to selected property
                    targetNode.bindingProperty(dialog->selectedProperty()).setExpression(newNode.validId());
            });
        }

        delete dialog;
        return true;
    } else if (targetNode.metaInfo().isQtQuick3DTextureInput()) {
        bindToProperty("texture", true);
        return newNode.isValid();
    } else if (targetNode.metaInfo().isQtQuick3DParticles3DSpriteParticle3D()) {
        bindToProperty("sprite", false);
        return newNode.isValid();
    } else if (targetNode.metaInfo().isQtQuick3DSceneEnvironment()) {
        bindToProperty("lightProbe", false);
        return newNode.isValid();
    } else if (targetNode.metaInfo().isQtQuick3DTexture()) {
        // if dropping an image on an existing texture, set the source
        targetNode.variantProperty("source").setValue(imagePath);
        return true;
    } else if (targetNode.metaInfo().isQtQuick3DModel()) {
        QTimer::singleShot(0, this, [this, targetNode, imagePath]() {
            if (m_view && targetNode.isValid()) {
                // To MaterialBrowserView. Done async to avoid custom notification in transaction
                m_view->emitCustomNotification(
                            "apply_asset_to_model3D", {targetNode},
                            {DocumentManager::currentFilePath().absolutePath().pathAppended(imagePath).cleanPath().toString()});
            }
        });
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
        newModelNode.setIdWithoutRefactoring(
            m_view->model()->generateNewId(fi.baseName(), "textureImage"));
        return newModelNode;
    }
    return {};
}

namespace {
NodeMetaInfo propertyType(const NodeAbstractProperty &property)
{
    return property.parentModelNode().metaInfo().property(property.name()).propertyType();
}
} // namespace

void NavigatorTreeModel::moveNodesInteractive(NodeAbstractProperty &parentProperty,
                                              const QList<ModelNode> &modelNodes,
                                              int targetIndex,
                                              bool executeInTransaction)
{
    QTC_ASSERT(m_view, return);

    auto doMoveNodesInteractive = [&parentProperty, modelNodes, targetIndex]() {
        const auto propertyQmlType = propertyType(parentProperty);
        int idx = targetIndex;
        for (const ModelNode &modelNode : modelNodes) {
            if (modelNode.isValid() && modelNode != parentProperty.parentModelNode()
                && !modelNode.isAncestorOf(parentProperty.parentModelNode())
                && (modelNode.metaInfo().isBasedOn(propertyQmlType) || propertyQmlType.isAlias()
                    || parentProperty.name() == "data"
                    || (parentProperty.parentModelNode().metaInfo().defaultPropertyName()
                            == parentProperty.name()
                        && propertyQmlType.isQmlComponent()))) {
                //### todo: allowing alias is just a heuristic
                //once the MetaInfo is part of instances we can do this right

                // We assume above that "data" property in parent accepts all types.
                // This is a workaround for Component parents to accept children, even though they
                // do not have an actual "data" property or apparently any other default property.
                // When the actual reparenting happens, model will create the "data" property if
                // it is missing.

                // We allow move even if target property type doesn't match, if the target property
                // is the default property of the parent and is of Component type.
                // In that case an implicit component will be created.

                bool nodeCanBeMovedToParentProperty = removeModelNodeFromNodeProperty(parentProperty, modelNode);
                if (nodeCanBeMovedToParentProperty) {
                    reparentModelNodeToNodeProperty(parentProperty, modelNode);
                    if (targetIndex > 0)
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
    QTC_ASSERT(m_view, return false);
    ModelNode modelNode = modelNodeForIndex(index);
    if (index.column() == ColumnType::Alias && role == Qt::CheckStateRole) {
        m_view->handleChangedExport(modelNode, value.toInt() != 0);
    } else if (index.column() == ColumnType::Visibility && role == Qt::CheckStateRole) {
        if (m_view->isPartOfMaterialLibrary(modelNode))
            return false;
        QmlVisualNode(modelNode).setVisibilityOverride(value.toInt() == 0);
    } else if (index.column() == ColumnType::Lock && role == Qt::CheckStateRole) {
        if (m_view->isPartOfMaterialLibrary(modelNode))
            return false;
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

    return ::Utils::toList(parents);
}

QList<QPersistentModelIndex> NavigatorTreeModel::nodesToPersistentIndex(const QList<ModelNode> &modelNodes)
{
    return ::Utils::transform(modelNodes, [this](const ModelNode &modelNode) {
        return QPersistentModelIndex(indexForModelNode(modelNode));
    });
}

void NavigatorTreeModel::notifyModelNodesRemoved(const QList<ModelNode> &modelNodes)
{
    m_rowCache.clear();
    QList<QPersistentModelIndex> indexes = nodesToPersistentIndex(collectParents(modelNodes));
    emit layoutAboutToBeChanged(indexes);
    emit layoutChanged(indexes);
}

void NavigatorTreeModel::notifyModelNodesInserted(const QList<ModelNode> &modelNodes)
{
    m_rowCache.clear();
    QList<QPersistentModelIndex> indexes = nodesToPersistentIndex(collectParents(modelNodes));
    emit layoutAboutToBeChanged(indexes);
    emit layoutChanged(indexes);
}

void NavigatorTreeModel::notifyModelNodesMoved(const QList<ModelNode> &modelNodes)
{
    m_rowCache.clear();
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
    m_rowCache.clear();
    resetModel();
}

void NavigatorTreeModel::setNameFilter(const QString &filter)
{
    m_nameFilter = filter;
    m_rowCache.clear();

    ModelNode rootNode = m_view->rootModelNode();
    QList<ModelNode> allNodes = rootNode.allSubModelNodes();
    m_nameFilteredList.clear();

    if (filter.isEmpty()) {
        m_nameFilteredList = allNodes;
    } else {
        for (ModelNode &node : rootNode.allSubModelNodes()) {
            if (node.displayName().contains(filter, Qt::CaseSensitivity::CaseInsensitive)) {
                m_nameFilteredList.append(node);
                ModelNode n = node;
                while (n.hasParentProperty()) {
                    n = n.parentProperty().parentModelNode();
                    if (n.isRootNode() || m_nameFilteredList.contains(n))
                        break;
                    m_nameFilteredList.append(n);
                }
            }
        }
    }

    resetModel();
}

void NavigatorTreeModel::setOrder(bool reverseItemOrder)
{
    m_reverseItemOrder = reverseItemOrder;
    m_rowCache.clear();
    resetModel();
}

void NavigatorTreeModel::resetModel()
{
    beginResetModel();
    m_rowCache.clear();
    m_nodeIndexHash.clear();
    endResetModel();
}

void NavigatorTreeModel::updateToolTipPixmap(const ModelNode &node, const QPixmap &pixmap)
{
    emit toolTipPixmapUpdated(node.id(), pixmap);
}

} // QmlDesigner
