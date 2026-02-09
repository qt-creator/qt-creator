// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "navigatorview.h"

#include "iconcheckboxitemdelegate.h"
#include "nameitemdelegate.h"
#include "navigatortracing.h"
#include "navigatortreemodel.h"
#include "navigatorwidget.h"

#include <assetslibrarywidget.h>
#include <bindingproperty.h>
#include <commontypecache.h>
#include <designersettings.h>
#include <itemlibraryentry.h>
#include <modelutils.h>
#include <nodeinstanceview.h>
#include <nodelistproperty.h>
#include <nodeproperty.h>
#include <qmldesignerconstants.h>
#include <qmldesignericons.h>
#include <qmldesignerplugin.h>
#include <qmlitemnode.h>
#include <rewritingexception.h>
#include <theme.h>
#include <variantproperty.h>

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/icore.h>

#include <projectexplorer/project.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/projectnodes.h>

#include <utils/algorithm.h>
#include <utils/icon.h>
#include <utils/utilsicons.h>
#include <utils/stylehelper.h>

#include <QHeaderView>
#include <QMimeData>
#include <QPixmap>
#include <QTimer>

using QmlDesigner::NavigatorTracing::category;

inline static void setScenePos(const QmlDesigner::ModelNode &modelNode, const QPointF &pos)
{
    if (modelNode.hasParentProperty() && QmlDesigner::QmlItemNode::isValidQmlItemNode(modelNode.parentProperty().parentModelNode())) {
        QmlDesigner::QmlItemNode parentNode = modelNode.parentProperty().parentModelNode();

        if (!parentNode.modelNode().metaInfo().isLayoutable()) {
            QPointF localPos = parentNode.instanceSceneTransform().inverted().map(pos);
            modelNode.variantProperty("x").setValue(localPos.toPoint().x());
            modelNode.variantProperty("y").setValue(localPos.toPoint().y());
        } else { //Items in Layouts do not have a position
            modelNode.removeProperty("x");
            modelNode.removeProperty("y");
        }
    }
}

inline static void moveNodesUp(const QList<QmlDesigner::ModelNode> &nodes)
{
    for (const auto &node : nodes) {
        if (!node.isRootNode() && node.parentProperty().isNodeListProperty()) {
            int oldIndex = node.parentProperty().indexOf(node);
            int index = oldIndex;
            index--;
            if (index < 0)
                index = node.parentProperty().count() - 1; //wrap around
            if (oldIndex != index) {
                try {
                    node.parentProperty().toNodeListProperty().slide(oldIndex, index);
                } catch (QmlDesigner::Exception &exception) {
                    exception.showException();
                }
            }
        }
    }
}

inline static void moveNodesDown(const QList<QmlDesigner::ModelNode> &nodes)
{
    for (const auto &node : nodes) {
        if (!node.isRootNode() && node.parentProperty().isNodeListProperty()) {
            int oldIndex = node.parentProperty().indexOf(node);
            int index = oldIndex;
            index++;
            if (index >= node.parentProperty().count())
                index = 0; //wrap around
            if (oldIndex != index) {
                try {
                    node.parentProperty().toNodeListProperty().slide(oldIndex, index);
                } catch (QmlDesigner::Exception &exception) {
                    exception.showException();
                }
            }
        }
    }
}

namespace QmlDesigner {

NavigatorView::NavigatorView(ExternalDependenciesInterface &externalDependencies)
    : AbstractView{externalDependencies}
    , m_blockSelectionChangedSignal(false)
{
    NanotraceHR::Tracer tracer{"navigator view constructor", category()};
}

NavigatorView::~NavigatorView()
{
    NanotraceHR::Tracer tracer{"navigator view destructor", category()};

    if (m_widget && !m_widget->parent())
        delete m_widget.data();
}

bool NavigatorView::hasWidget() const
{
    NanotraceHR::Tracer tracer{"navigator view has widget", category()};

    return true;
}

WidgetInfo NavigatorView::widgetInfo()
{
    NanotraceHR::Tracer tracer{"navigator view widget info", category()};

    if (!m_widget)
        setupWidget();

    return createWidgetInfo(m_widget.data(),
                            QStringLiteral("Navigator"),
                            WidgetInfo::LeftPane,
                            tr("Navigator"),
                            tr("Navigator view"));
}

void NavigatorView::modelAttached(Model *model)
{
    NanotraceHR::Tracer tracer{"navigator view model attached", category()};

    AbstractView::modelAttached(model);

    QTreeView *treeView = treeWidget();

    treeView->header()->setSectionResizeMode(NavigatorTreeModel::ColumnType::Name,
                                             QHeaderView::Stretch);
    treeView->header()->setSectionResizeMode(NavigatorTreeModel::ColumnType::Alias,
                                             QHeaderView::Fixed);
    treeView->header()->setSectionResizeMode(NavigatorTreeModel::ColumnType::Visibility,
                                             QHeaderView::Fixed);
    treeView->header()->setSectionResizeMode(NavigatorTreeModel::ColumnType::Lock, QHeaderView::Fixed);

    treeView->header()->setStretchLastSection(false);
    treeView->header()->setMinimumSectionSize(24);
    treeView->header()->setDefaultSectionSize(24);

    treeView->header()->resizeSection(NavigatorTreeModel::ColumnType::Alias, 24);
    treeView->header()->resizeSection(NavigatorTreeModel::ColumnType::Visibility, 24);
    // Make last column a bit wider to compensate the shift to the left due to vertical scrollbar
    treeView->header()->resizeSection(NavigatorTreeModel::ColumnType::Lock, 32);
    treeView->setIndentation(20);

    m_currentModelInterface->setFilter(false);
    m_currentModelInterface->setNameFilter("");
    m_widget->clearSearch();

    QTimer::singleShot(0, this, [this, treeView]() {
        m_currentModelInterface->showReferences(
            designerSettings().navigatorShowReferenceNodes());

        m_currentModelInterface->setFilter(
                    designerSettings().navigatorShowOnlyVisibleItems());

        m_currentModelInterface->setOrder(
                    designerSettings().navigatorReverseItemOrder());

        // Expand everything to begin with to ensure model node to index cache is populated
        treeView->expandAll();

        if (AbstractView::model() && m_expandMap.contains(AbstractView::model()->fileUrl())) {
            const QHash<QString, bool> localExpandMap = m_expandMap[AbstractView::model()->fileUrl()];
            auto it = localExpandMap.constBegin();
            while (it != localExpandMap.constEnd()) {
                const ModelNode node = modelNodeForId(it.key());
                // When editing subcomponent, the current root node may be marked collapsed in the
                // full file view, but we never want to actually collapse it, so skip it.
                if (!node.isRootNode()) {
                    const QModelIndex index = indexForModelNode(node);
                    if (index.isValid())
                        treeWidget()->setExpanded(index, it.value());
                }
                ++it;
            }
        }
    });

    clearExplorerWarnings();
}

void NavigatorView::clearExplorerWarnings()
{
    NanotraceHR::Tracer tracer{"navigator view clear explorer warnings", category()};

    QList<ModelNode> allNodes;
    allNodes.append(rootModelNode());
    allNodes.append(rootModelNode().allSubModelNodes());
    for (const ModelNode &node : std::as_const(allNodes)) {
        if (node.metaInfo().isFileComponent()) {
            const ProjectExplorer::FileNode *fNode = fileNodeForModelNode(node);
            if (fNode)
                fNode->setHasError(false);
        }
    }
}

void NavigatorView::modelAboutToBeDetached(Model *model)
{
    NanotraceHR::Tracer tracer{"navigator view model about to be detached", category()};

    QHash<QString, bool> &localExpandMap = m_expandMap[model->fileUrl()];

    // If detaching full document model, recreate expand map from scratch to remove stale entries.
    // Otherwise just update it (subcomponent edit case).
    bool fullUpdate = true;
    if (DesignDocument *document = QmlDesignerPlugin::instance()->currentDesignDocument())
        fullUpdate = !document->inFileComponentModelActive();
    if (fullUpdate)
        localExpandMap.clear();

    if (currentModel()) {
        // Store expand state of the navigator tree
        const ModelNode rootNode = rootModelNode();
        const QModelIndex rootIndex = indexForModelNode(rootNode);

        std::function<void(const QModelIndex &)> gatherExpandedState;
        gatherExpandedState = [&](const QModelIndex &index) {
            if (index.isValid()) {
                const int rowCount = currentModel()->rowCount(index);
                for (int i = 0; i < rowCount; ++i) {
                    const QModelIndex childIndex = currentModel()->index(i, 0, index);
                    if (const ModelNode node = modelNodeForIndex(childIndex)) {
                        // Just store collapsed states as everything is expanded by default
                        if (!treeWidget()->isExpanded(childIndex))
                            localExpandMap.insert(node.id(), false);
                        else if (!fullUpdate)
                            localExpandMap.remove(node.id());
                    }
                    gatherExpandedState(childIndex);
                }
            }
        };
        gatherExpandedState(rootIndex);
    }

    m_currentModelInterface->resetModel();

    AbstractView::modelAboutToBeDetached(model);
}

void NavigatorView::importsChanged(const Imports &/*addedImports*/, const Imports &/*removedImports*/)
{
    NanotraceHR::Tracer tracer{"navigator view imports changed", category()};

    treeWidget()->update();
}

void NavigatorView::bindingPropertiesChanged(const QList<BindingProperty> &propertyList,
                                             PropertyChangeFlags /*propertyChange*/)
{
    NanotraceHR::Tracer tracer{"navigator view binding properties changed", category()};

    QSet<ModelNode> owners;

    for (const BindingProperty &bindingProperty : propertyList) {
        /* If a binding property that exports an item using an alias property has
         * changed, we have to update the affected item.
         */

        if (bindingProperty.isAliasExport())
            m_currentModelInterface->notifyDataChanged(modelNodeForId(bindingProperty.expression()));

        if (m_currentModelInterface->isReferenceNodesVisible() && bindingProperty.canBeReference()) {
            const QList<ModelNode> modelNodes = bindingProperty.resolveToModelNodes();
            for (const ModelNode &modelNode : modelNodes) {
                if (m_currentModelInterface->canBeReference(modelNode)) {
                    owners.insert(bindingProperty.parentModelNode());
                    break;
                }
            }
        }
    }

    if (!owners.empty())
        m_currentModelInterface->notifyModelReferenceNodesUpdated(owners.values());
}

void NavigatorView::dragStarted(QMimeData *mimeData)
{
    NanotraceHR::Tracer tracer{"navigator view drag started", category()};

    if (mimeData->hasFormat(Constants::MIME_TYPE_ITEM_LIBRARY_INFO)) {
        QByteArray data = mimeData->data(Constants::MIME_TYPE_ITEM_LIBRARY_INFO);
        QDataStream stream(data);
        ItemLibraryEntry itemLibraryEntry;
        stream >> itemLibraryEntry;

        m_widget->setDragType(itemLibraryEntry.typeName());
        m_widget->update();
    } else if (mimeData->hasFormat(Constants::MIME_TYPE_TEXTURE)) {
        qint32 internalId = mimeData->data(Constants::MIME_TYPE_TEXTURE).toInt();
        ModelNode texNode = modelNodeForInternalId(internalId);
#ifdef QDS_USE_PROJECTSTORAGE
        m_widget->setDragType(texNode.type());
#else
        m_widget->setDragType(texNode.metaInfo().typeName());
#endif
        m_widget->update();
    } else if (mimeData->hasFormat(Constants::MIME_TYPE_MATERIAL)) {
        qint32 internalId = mimeData->data(Constants::MIME_TYPE_MATERIAL).toInt();
        ModelNode matNode = modelNodeForInternalId(internalId);
#ifdef QDS_USE_PROJECTSTORAGE
        m_widget->setDragType(matNode.type());
#else
        m_widget->setDragType(matNode.metaInfo().typeName());
#endif
        m_widget->update();
    } else if (mimeData->hasFormat(Constants::MIME_TYPE_BUNDLE_ITEM_2D)) {
        m_widget->setDragType(Constants::MIME_TYPE_BUNDLE_ITEM_2D);
        m_widget->update();
    } else if (mimeData->hasFormat(Constants::MIME_TYPE_BUNDLE_ITEM_3D)) {
        QByteArray data = mimeData->data(Constants::MIME_TYPE_BUNDLE_ITEM_3D);
        QDataStream stream(data);
        TypeName bundleItemType;
        stream >> bundleItemType;

        if (bundleItemType.contains("UserMaterials"))
            m_widget->setDragType(Constants::MIME_TYPE_BUNDLE_MATERIAL);
        else
            m_widget->setDragType(Constants::MIME_TYPE_BUNDLE_ITEM_3D);
        m_widget->update();
    } else if (mimeData->hasFormat(Constants::MIME_TYPE_BUNDLE_TEXTURE)) {
        m_widget->setDragType(Constants::MIME_TYPE_BUNDLE_TEXTURE);
        m_widget->update();
    } else if (mimeData->hasFormat(Constants::MIME_TYPE_BUNDLE_MATERIAL)) {
        QByteArray data = mimeData->data(Constants::MIME_TYPE_BUNDLE_MATERIAL);
        QDataStream stream(data);
        TypeName bundleMatType;
        stream >> bundleMatType;

        m_widget->setDragType(bundleMatType);
        m_widget->update();
    } else if (mimeData->hasFormat(Constants::MIME_TYPE_ASSETS)) {
        const QStringList assetsPaths = QString::fromUtf8(mimeData->data(Constants::MIME_TYPE_ASSETS)).split(',');
        if (assetsPaths.size() > 0) {
            auto assetTypeAndData = AssetsLibraryWidget::getAssetTypeAndData(assetsPaths[0]);
            QString assetType = assetTypeAndData.first;
            if (assetType == Constants::MIME_TYPE_ASSET_EFFECT) {
                m_widget->setDragType(Constants::MIME_TYPE_ASSET_EFFECT);
                m_widget->update();
            } else if (assetType == Constants::MIME_TYPE_ASSET_IMPORTED3D) {
                m_widget->setDragType(Constants::MIME_TYPE_ASSET_IMPORTED3D);
                m_widget->update();
            } else if (assetType == Constants::MIME_TYPE_ASSET_TEXTURE3D) {
                m_widget->setDragType(Constants::MIME_TYPE_ASSET_TEXTURE3D);
                m_widget->update();
            } else if (assetType == Constants::MIME_TYPE_ASSET_IMAGE) {
                m_widget->setDragType(Constants::MIME_TYPE_ASSET_IMAGE);
                m_widget->update();
            }
        }
    }
}

void NavigatorView::dragEnded()
{
    NanotraceHR::Tracer tracer{"navigator view drag ended", category()};

    m_widget->setDragType("");
    m_widget->update();
}

void NavigatorView::customNotification([[maybe_unused]] const AbstractView *view,
                                       const QString &identifier,
                                       [[maybe_unused]] const QList<ModelNode> &nodeList,
                                       [[maybe_unused]] const QList<QVariant> &data)
{
    NanotraceHR::Tracer tracer{"navigator view custom notification", category()};

    if (identifier == "asset_import_update")
        m_currentModelInterface->notifyIconsChanged();
}

void NavigatorView::handleChangedExport(const ModelNode &modelNode, bool exported)
{
    NanotraceHR::Tracer tracer{"navigator view handle changed export", category()};

    const ModelNode rootNode = rootModelNode();
    Q_ASSERT(rootNode.isValid());
    const PropertyName modelNodeId = modelNode.id().toUtf8();
    if (rootNode.hasProperty(modelNodeId))
        rootNode.removeProperty(modelNodeId);
    if (exported) {
        executeInTransaction("NavigatorTreeModel:exportItem", [modelNode](){
            QmlObjectNode qmlObjectNode(modelNode);
            qmlObjectNode.ensureAliasExport();
        });
    }
}

bool NavigatorView::isNodeInvisible(const ModelNode &modelNode) const
{
    NanotraceHR::Tracer tracer{"navigator view is node invisible", category()};

    return QmlVisualNode(modelNode).visibilityOverride();
}

void NavigatorView::disableWidget()
{
    NanotraceHR::Tracer tracer{"navigator view disable widget", category()};

    if (m_widget)
        m_widget->disableNavigator();
}

void NavigatorView::enableWidget()
{
    NanotraceHR::Tracer tracer{"navigator view enable widget", category()};

    if (m_widget)
        m_widget->enableNavigator();
}

void NavigatorView::modelNodePreviewPixmapChanged(const ModelNode &node,
                                                  const QPixmap &pixmap,
                                                  const QByteArray &requestId)
{
    NanotraceHR::Tracer tracer{"navigator view model node preview pixmap changed", category()};

    // There might be multiple requests for different preview pixmap sizes.
    // Here only the one with the default size is picked.
    if (requestId.isEmpty())
        m_treeModel->updateToolTipPixmap(node, pixmap);
}

ModelNode NavigatorView::modelNodeForIndex(const QModelIndex &modelIndex) const
{
    NanotraceHR::Tracer tracer{"navigator view model node for index", category()};

    return modelIndex.model()->data(modelIndex, ModelNodeRole).value<ModelNode>();
}

void NavigatorView::nodeAboutToBeRemoved(const ModelNode & /*removedNode*/)
{
}

void NavigatorView::nodeRemoved(const ModelNode &removedNode,
                                const NodeAbstractProperty & /*parentProperty*/,
                                AbstractView::PropertyChangeFlags /*propertyChange*/)
{
    NanotraceHR::Tracer tracer{"navigator view node removed", category()};

    m_currentModelInterface->notifyModelNodesRemoved({removedNode});
}

void NavigatorView::nodeReparented(const ModelNode &modelNode,
                                   const NodeAbstractProperty & /*newPropertyParent*/,
                                   const NodeAbstractProperty & oldPropertyParent,
                                   AbstractView::PropertyChangeFlags /*propertyChange*/)
{
    NanotraceHR::Tracer tracer{"navigator view node reparented", category()};

    if (!oldPropertyParent.isValid())
        m_currentModelInterface->notifyModelNodesInserted({modelNode});
    else
        m_currentModelInterface->notifyModelNodesMoved({modelNode});
    treeWidget()->expand(indexForModelNode(modelNode));

    // make sure selection is in sync again
    QTimer::singleShot(0, this, &NavigatorView::updateItemSelection);
}

void NavigatorView::nodeIdChanged(const ModelNode& modelNode, const QString & /*newId*/, const QString & /*oldId*/)
{
    NanotraceHR::Tracer tracer{"navigator view node id changed", category()};

    m_currentModelInterface->notifyDataChanged(modelNode);
}

void NavigatorView::propertiesAboutToBeRemoved(const QList<AbstractProperty> &/*propertyList*/)
{
}

void NavigatorView::propertiesRemoved(const QList<AbstractProperty> &propertyList)
{
    NanotraceHR::Tracer tracer{"navigator view properties removed", category()};

    QList<ModelNode> modelNodes;
    for (const AbstractProperty &property : propertyList) {
        if (property.isNodeAbstractProperty()) {
            NodeAbstractProperty nodeAbstractProperty(property.toNodeListProperty());
            modelNodes.append(nodeAbstractProperty.directSubNodes());
        }
    }

    m_currentModelInterface->notifyModelNodesRemoved(modelNodes);
}

void NavigatorView::rootNodeTypeChanged(const QString &/*type*/, int /*majorVersion*/, int /*minorVersion*/)
{
    NanotraceHR::Tracer tracer{"navigator view root node type changed", category()};

    m_currentModelInterface->notifyDataChanged(rootModelNode());
}

void NavigatorView::nodeTypeChanged(const ModelNode &modelNode, const TypeName &, int , int)
{
    NanotraceHR::Tracer tracer{"navigator view node type changed", category()};

    m_currentModelInterface->notifyDataChanged(modelNode);
}

void NavigatorView::auxiliaryDataChanged(const ModelNode &modelNode,
                                         [[maybe_unused]] AuxiliaryDataKeyView key,
                                         [[maybe_unused]] const QVariant &data)
{
    NanotraceHR::Tracer tracer{"navigator view auxiliary data changed", category()};

    m_currentModelInterface->notifyDataChanged(modelNode);

    if (key == lockedProperty) {
        // Also notify data changed on child nodes to redraw them
        const QList<ModelNode> childNodes = modelNode.allSubModelNodes();
        for (const auto &childNode : childNodes)
            m_currentModelInterface->notifyDataChanged(childNode);
    }
}

void NavigatorView::instanceErrorChanged(const QVector<ModelNode> &errorNodeList)
{
    NanotraceHR::Tracer tracer{"navigator view instance error changed", category()};

    for (const ModelNode &modelNode : errorNodeList) {
        m_currentModelInterface->notifyDataChanged(modelNode);
        propagateInstanceErrorToExplorer(modelNode);
    }
}

void NavigatorView::nodeOrderChanged(const NodeListProperty &listProperty)
{
    NanotraceHR::Tracer tracer{"navigator view node order changed", category()};

    m_currentModelInterface->notifyModelNodesMoved(listProperty.directSubNodes());

    // make sure selection is in sync again
    QTimer::singleShot(0, this, &NavigatorView::updateItemSelection);
}

void NavigatorView::changeToComponent(const QModelIndex &index)
{
    NanotraceHR::Tracer tracer{"navigator view change to component", category()};

    if (index.isValid() && currentModel()->data(index, Qt::UserRole).isValid()) {
        const ModelNode doubleClickNode = modelNodeForIndex(index);
        if (doubleClickNode.metaInfo().isFileComponent())
            Core::EditorManager::openEditor(Utils::FilePath::fromString(
                                                ModelUtils::componentFilePath(doubleClickNode)),
                                            Utils::Id(),
                                            Core::EditorManager::DoNotMakeVisible);
    }
}

QModelIndex NavigatorView::indexForModelNode(const ModelNode &modelNode) const
{
    NanotraceHR::Tracer tracer{"navigator view index for model node", category()};

    return m_currentModelInterface->indexForModelNode(modelNode);
}

QAbstractItemModel *NavigatorView::currentModel() const
{
    NanotraceHR::Tracer tracer{"navigator view current model", category()};

    return treeWidget()->model();
}

const ProjectExplorer::FileNode *NavigatorView::fileNodeForModelNode(const ModelNode &node) const
{
    NanotraceHR::Tracer tracer{"navigator view file node for model node", category()};

    QString filename = ModelUtils::componentFilePath(node);
    Utils::FilePath filePath = Utils::FilePath::fromString(filename);
    ProjectExplorer::Project *currentProject = ProjectExplorer::ProjectManager::projectForFile(
        filePath);

    if (!currentProject) {
        filePath = Utils::FilePath::fromString(node.model()->fileUrl().toLocalFile());

        /* If the component does not belong to the project then we can fallback to the current file */
        currentProject = ProjectExplorer::ProjectManager::projectForFile(filePath);
    }
    if (!currentProject)
        return nullptr;

    const auto fileNode = currentProject->nodeForFilePath(filePath);
    QTC_ASSERT(fileNode, return nullptr);

    return fileNode->asFileNode();
}

const ProjectExplorer::FileNode *NavigatorView::fileNodeForIndex(const QModelIndex &index) const
{
    NanotraceHR::Tracer tracer{"navigator view file node for index", category()};

    if (index.isValid() && currentModel()->data(index, Qt::UserRole).isValid()) {
        ModelNode node = modelNodeForIndex(index);
        if (node.metaInfo().isFileComponent()) {
            return fileNodeForModelNode(node);
        }
    }

    return nullptr;
}

void NavigatorView::propagateInstanceErrorToExplorer(const ModelNode &modelNode)
{
    NanotraceHR::Tracer tracer{"navigator view propagate instance error to explorer", category()};

    QModelIndex index = indexForModelNode(modelNode);

    do {
        const ProjectExplorer::FileNode *fnode = fileNodeForIndex(index);
        if (fnode) {
            fnode->setHasError(true);
            return;
        }
        else {
            index = index.parent();
        }
    } while (index.isValid());
}

void NavigatorView::leftButtonClicked()
{
    NanotraceHR::Tracer tracer{"navigator view left button clicked", category()};

    if (selectedModelNodes().size() > 1)
        return; //Semantics are unclear for multi selection.

    bool blocked = blockSelectionChangedSignal(true);

    for (const ModelNode &node : selectedModelNodes()) {
        if (!node.isRootNode() && !node.parentProperty().parentModelNode().isRootNode()) {
            if (QmlItemNode::isValidQmlItemNode(node)) {
                QPointF scenePos = QmlItemNode(node).instanceScenePosition();
                reparentAndCatch(node.parentProperty().parentProperty(), node);
                if (!scenePos.isNull())
                    setScenePos(node, scenePos);
            } else {
                reparentAndCatch(node.parentProperty().parentProperty(), node);
            }
        }
    }

    updateItemSelection();
    blockSelectionChangedSignal(blocked);
}

void NavigatorView::rightButtonClicked()
{
    NanotraceHR::Tracer tracer{"navigator view right button clicked", category()};

    if (selectedModelNodes().size() > 1)
        return; //Semantics are unclear for multi selection.

    bool blocked = blockSelectionChangedSignal(true);
    bool reverse = designerSettings().navigatorReverseItemOrder();

    for (const ModelNode &node : selectedModelNodes()) {
        if (!node.isRootNode() && node.parentProperty().isNodeListProperty()
            && node.parentProperty().count() > 1) {
            int index = node.parentProperty().indexOf(node);

            bool indexOk = false;

            if (reverse) {
                index++;
                indexOk = (index < node.parentProperty().count());
            } else {
                index--;
                indexOk = (index >= 0);
            }

            if (indexOk) { //for the first node the semantics are not clear enough. Wrapping would be irritating.
                ModelNode newParent = node.parentProperty().toNodeListProperty().at(index);

                if (QmlItemNode::isValidQmlItemNode(node)
                        && QmlItemNode::isValidQmlItemNode(newParent)
                        && !newParent.metaInfo().defaultPropertyIsComponent()) {
                    QPointF scenePos = QmlItemNode(node).instanceScenePosition();
                    reparentAndCatch(newParent.nodeAbstractProperty(newParent.metaInfo().defaultPropertyName()), node);
                    if (!scenePos.isNull())
                        setScenePos(node, scenePos);
                } else {
                    if (newParent.metaInfo().isValid() && !newParent.metaInfo().defaultPropertyIsComponent())
                        reparentAndCatch(newParent.nodeAbstractProperty(newParent.metaInfo().defaultPropertyName()), node);
                }
            }
        }
    }
    updateItemSelection();
    blockSelectionChangedSignal(blocked);
}

void NavigatorView::upButtonClicked()
{
    NanotraceHR::Tracer tracer{"navigator view up button clicked", category()};

    bool blocked = blockSelectionChangedSignal(true);
    bool reverse = designerSettings().navigatorReverseItemOrder();

    if (reverse)
        moveNodesDown(selectedModelNodes());
    else
        moveNodesUp(selectedModelNodes());

    updateItemSelection();
    blockSelectionChangedSignal(blocked);
}

void NavigatorView::downButtonClicked()
{
    NanotraceHR::Tracer tracer{"navigator view down button clicked", category()};

    bool blocked = blockSelectionChangedSignal(true);
    bool reverse = designerSettings().navigatorReverseItemOrder();

    if (reverse)
        moveNodesUp(selectedModelNodes());
    else
        moveNodesDown(selectedModelNodes());

    updateItemSelection();
    blockSelectionChangedSignal(blocked);
}

void NavigatorView::colorizeToggled(bool flag)
{
    NanotraceHR::Tracer tracer{"navigator view colorize toggled", category()};

    designerSettings().navigatorColorizeIcons.setValue(flag);
    m_currentModelInterface->notifyIconsChanged();
}

void NavigatorView::referenceToggled(bool flag)
{
    NanotraceHR::Tracer tracer{"navigator view reference toggled", category()};

    m_currentModelInterface->showReferences(flag);
    treeWidget()->expandAll();
    designerSettings().navigatorShowReferenceNodes.setValue(flag);
}

void NavigatorView::filterToggled(bool flag)
{
    NanotraceHR::Tracer tracer{"navigator view filter toggled", category()};

    m_currentModelInterface->setFilter(flag);
    treeWidget()->expandAll();
    designerSettings().navigatorShowOnlyVisibleItems.setValue(flag);
}

void NavigatorView::reverseOrderToggled(bool flag)
{
    NanotraceHR::Tracer tracer{"navigator view reverse order toggled", category()};

    m_currentModelInterface->setOrder(flag);
    treeWidget()->expandAll();
    designerSettings().navigatorReverseItemOrder.setValue(flag);
}

void NavigatorView::textFilterChanged(const QString &text)
{
    NanotraceHR::Tracer tracer{"navigator view text filter changed", category()};

    m_treeModel->setNameFilter(text);
    treeWidget()->expandAll();
}

void NavigatorView::changeSelection(const QItemSelection & /*newSelection*/, const QItemSelection &/*deselected*/)
{
    NanotraceHR::Tracer tracer{"navigator view change selection", category()};

    if (m_blockSelectionChangedSignal)
        return;

    QSet<ModelNode> nodeSet;

    for (const QModelIndex &index : treeWidget()->selectionModel()->selectedIndexes()) {
        const ModelNode modelNode = modelNodeForIndex(index);
        if (modelNode.isValid())
            nodeSet.insert(modelNode);
    }

    bool blocked = blockSelectionChangedSignal(true);
    setSelectedModelNodes(Utils::toList(nodeSet));
    blockSelectionChangedSignal(blocked);
}

void NavigatorView::selectedNodesChanged(const QList<ModelNode> &/*selectedNodeList*/, const QList<ModelNode> &/*lastSelectedNodeList*/)
{
    NanotraceHR::Tracer tracer{"navigator view selected nodes changed", category()};

    // Update selection asynchronously to ensure NavigatorTreeModel's index cache is up to date
    QTimer::singleShot(0, this, &NavigatorView::updateItemSelection);
}

void NavigatorView::updateItemSelection()
{
    NanotraceHR::Tracer tracer{"navigator view update item selection", category()};

    if (!isAttached())
        return;

    QItemSelection itemSelection;
    for (const ModelNode &node : selectedModelNodes()) {
        const QModelIndex index = indexForModelNode(node);

        if (index.isValid()) {
            const QModelIndex beginIndex(currentModel()->index(index.row(), 0, index.parent()));
            const QModelIndex endIndex(currentModel()->index(index.row(), currentModel()->columnCount(index.parent()) - 1, index.parent()));
            if (beginIndex.isValid() && endIndex.isValid())
                itemSelection.select(beginIndex, endIndex);
        } else {
            // if the node index is invalid expand ancestors manually if they are valid.
            ModelNode parentNode = node.parentProperty().parentModelNode();
            while (parentNode) {
                QModelIndex parentIndex = indexForModelNode(parentNode);
                if (parentIndex.isValid())
                    treeWidget()->expand(parentIndex);
                else
                    break;
                parentNode = parentNode.parentProperty().parentModelNode();
            }
         }
    }

    bool blocked = blockSelectionChangedSignal(true);
    treeWidget()->selectionModel()->select(itemSelection, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
    blockSelectionChangedSignal(blocked);

    if (!selectedModelNodes().isEmpty())
        treeWidget()->scrollTo(indexForModelNode(selectedModelNodes().constLast()));

    // make sure selected nodes are visible
    for (const QModelIndex &selectedIndex : itemSelection.indexes()) {
        if (selectedIndex.column() == 0)
            expandAncestors(selectedIndex);
    }
}

QTreeView *NavigatorView::treeWidget() const
{
    NanotraceHR::Tracer tracer{"navigator view tree widget", category()};
    if (m_widget)
        return m_widget->treeView();
    return nullptr;
}

NavigatorTreeModel *NavigatorView::treeModel()
{
    NanotraceHR::Tracer tracer{"navigator view tree model", category()};

    return m_treeModel.data();
}

// along the lines of QObject::blockSignals
bool NavigatorView::blockSelectionChangedSignal(bool block)
{
    NanotraceHR::Tracer tracer{"navigator view block selection changed signal", category()};

    bool oldValue = m_blockSelectionChangedSignal;
    m_blockSelectionChangedSignal = block;
    return oldValue;
}

void NavigatorView::expandAncestors(const QModelIndex &index)
{
    NanotraceHR::Tracer tracer{"navigator view expand ancestors", category()};

    QModelIndex currentIndex = index.parent();
    while (currentIndex.isValid()) {
        if (!treeWidget()->isExpanded(currentIndex))
            treeWidget()->expand(currentIndex);
        currentIndex = currentIndex.parent();
    }
}

void NavigatorView::reparentAndCatch(NodeAbstractProperty property, const ModelNode &modelNode)
{
    NanotraceHR::Tracer tracer{"navigator view reparent and catch", category()};

    try {
        property.reparentHere(modelNode);
    } catch (Exception &exception) {
        exception.showException();
    }
}

void NavigatorView::setupWidget()
{
    NanotraceHR::Tracer tracer{"navigator view setup widget", category()};

    m_widget = new NavigatorWidget(this);
    m_treeModel = new NavigatorTreeModel(this);
    m_treeModel->setView(this);
    m_widget->setTreeModel(m_treeModel.data());
    m_currentModelInterface = m_treeModel;

    connect(treeWidget()->selectionModel(), &QItemSelectionModel::selectionChanged, this, &NavigatorView::changeSelection);

    connect(m_widget.data(), &NavigatorWidget::leftButtonClicked, this, &NavigatorView::leftButtonClicked);
    connect(m_widget.data(), &NavigatorWidget::rightButtonClicked, this, &NavigatorView::rightButtonClicked);
    connect(m_widget.data(), &NavigatorWidget::downButtonClicked, this, &NavigatorView::downButtonClicked);
    connect(m_widget.data(), &NavigatorWidget::upButtonClicked, this, &NavigatorView::upButtonClicked);
    connect(m_widget.data(), &NavigatorWidget::colorizeToggled, this, &NavigatorView::colorizeToggled);
    connect(m_widget.data(), &NavigatorWidget::referenceToggled, this, &NavigatorView::referenceToggled);
    connect(m_widget.data(), &NavigatorWidget::filterToggled, this, &NavigatorView::filterToggled);
    connect(m_widget.data(), &NavigatorWidget::reverseOrderToggled, this, &NavigatorView::reverseOrderToggled);

    connect(m_widget.data(), &NavigatorWidget::textFilterChanged, this, &NavigatorView::textFilterChanged);

    const QString fontName = "qtds_propertyIconFont.ttf";
    const QSize size = QSize(32, 32);

    const QString visibilityOnUnicode = Theme::getIconUnicode(Theme::Icon::visibilityOn);
    const QString visibilityOffUnicode = Theme::getIconUnicode(Theme::Icon::visibilityOff);

    const QString aliasOnUnicode = Theme::getIconUnicode(Theme::Icon::alias);
    const QString aliasOffUnicode = aliasOnUnicode;

    const QString lockOnUnicode = Theme::getIconUnicode(Theme::Icon::lockOn);
    const QString lockOffUnicode = Theme::getIconUnicode(Theme::Icon::lockOff);

    auto visibilityIconOffNormal = Utils::StyleHelper::IconFontHelper(
        visibilityOffUnicode, Theme::getColor(Theme::DSnavigatorIcon), size, QIcon::Normal, QIcon::Off);
    auto visibilityIconOffHover = Utils::StyleHelper::IconFontHelper(
        visibilityOffUnicode, Theme::getColor(Theme::DSnavigatorIconHover), size, QIcon::Active, QIcon::Off);
    auto visibilityIconOffSelected = Utils::StyleHelper::IconFontHelper(
        visibilityOffUnicode, Theme::getColor(Theme::DSnavigatorIconSelected), size, QIcon::Selected, QIcon::Off);
    auto visibilityIconOnNormal = Utils::StyleHelper::IconFontHelper(
        visibilityOnUnicode, Theme::getColor(Theme::DSnavigatorIcon), size, QIcon::Normal, QIcon::On);
    auto visibilityIconOnHover = Utils::StyleHelper::IconFontHelper(
        visibilityOnUnicode, Theme::getColor(Theme::DSnavigatorIconHover), size, QIcon::Active, QIcon::On);
    auto visibilityIconOnSelected = Utils::StyleHelper::IconFontHelper(
        visibilityOnUnicode, Theme::getColor(Theme::DSnavigatorIconSelected), size, QIcon::Selected, QIcon::On);

    const QIcon visibilityIcon = Utils::StyleHelper::getIconFromIconFont(
                fontName, {visibilityIconOffNormal,
                           visibilityIconOffHover,
                           visibilityIconOffSelected,
                           visibilityIconOnNormal,
                           visibilityIconOnHover,
                           visibilityIconOnSelected});

    auto aliasIconOffNormal = Utils::StyleHelper::IconFontHelper(
        aliasOffUnicode, Theme::getColor(Theme::DSnavigatorIcon), size, QIcon::Normal, QIcon::Off);
    auto aliasIconOffHover = Utils::StyleHelper::IconFontHelper(
        aliasOffUnicode, Theme::getColor(Theme::DSnavigatorIconHover), size, QIcon::Active, QIcon::Off);
    auto aliasIconOffSelected = Utils::StyleHelper::IconFontHelper(
        aliasOffUnicode, Theme::getColor(Theme::DSnavigatorIconSelected), size, QIcon::Selected, QIcon::Off);
    auto aliasIconOnNormal = Utils::StyleHelper::IconFontHelper(
        aliasOnUnicode, Theme::getColor(Theme::DSnavigatorAliasIconChecked), size, QIcon::Normal, QIcon::On);
    auto aliasIconOnHover = Utils::StyleHelper::IconFontHelper(
        aliasOnUnicode, Theme::getColor(Theme::DSnavigatorAliasIconChecked), size, QIcon::Active, QIcon::On);
    auto aliasIconOnSelected = Utils::StyleHelper::IconFontHelper(
        aliasOnUnicode, Theme::getColor(Theme::DSnavigatorAliasIconChecked), size, QIcon::Selected, QIcon::On);

    const QIcon aliasIcon = Utils::StyleHelper::getIconFromIconFont(
                fontName, {aliasIconOffNormal,
                           aliasIconOffHover,
                           aliasIconOffSelected,
                           aliasIconOnNormal,
                           aliasIconOnHover,
                           aliasIconOnSelected});

    auto lockIconOffNormal = Utils::StyleHelper::IconFontHelper(
        lockOffUnicode, Theme::getColor(Theme::DSnavigatorIcon), size, QIcon::Normal, QIcon::Off);
    auto lockIconOffHover = Utils::StyleHelper::IconFontHelper(
        lockOffUnicode, Theme::getColor(Theme::DSnavigatorIconHover), size, QIcon::Active, QIcon::Off);
    auto lockIconOffSelected = Utils::StyleHelper::IconFontHelper(
        lockOffUnicode, Theme::getColor(Theme::DSnavigatorIconSelected), size, QIcon::Selected, QIcon::Off);
    auto lockIconOnNormal = Utils::StyleHelper::IconFontHelper(
        lockOnUnicode, Theme::getColor(Theme::DSnavigatorIcon), size, QIcon::Normal, QIcon::On);
    auto lockIconOnHover = Utils::StyleHelper::IconFontHelper(
        lockOnUnicode, Theme::getColor(Theme::DSnavigatorIconHover), size, QIcon::Active, QIcon::On);
    auto lockIconOnSelected = Utils::StyleHelper::IconFontHelper(
        lockOnUnicode, Theme::getColor(Theme::DSnavigatorIconSelected), size, QIcon::Selected, QIcon::On);

    const QIcon lockIcon = Utils::StyleHelper::getIconFromIconFont(
                fontName, {lockIconOffNormal,
                           lockIconOffHover,
                           lockIconOffSelected,
                           lockIconOnNormal,
                           lockIconOnHover,
                           lockIconOnSelected});

    auto idDelegate = new NameItemDelegate(this);

    auto visibilityDelegate = new IconCheckboxItemDelegate(this, visibilityIcon);
    auto aliasDelegate = new IconCheckboxItemDelegate(this, aliasIcon);
    auto lockDelegate = new IconCheckboxItemDelegate(this, lockIcon);

    treeWidget()->setItemDelegateForColumn(NavigatorTreeModel::ColumnType::Name, idDelegate);
    treeWidget()->setItemDelegateForColumn(NavigatorTreeModel::ColumnType::Alias, aliasDelegate);
    treeWidget()->setItemDelegateForColumn(NavigatorTreeModel::ColumnType::Visibility, visibilityDelegate);
    treeWidget()->setItemDelegateForColumn(NavigatorTreeModel::ColumnType::Lock, lockDelegate);
}

} // namespace QmlDesigner
