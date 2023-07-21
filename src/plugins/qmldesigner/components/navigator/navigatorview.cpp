// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "navigatorview.h"

#include "iconcheckboxitemdelegate.h"
#include "nameitemdelegate.h"
#include "navigatortreemodel.h"
#include "navigatorwidget.h"

#include <assetslibrarywidget.h>
#include <bindingproperty.h>
#include <commontypecache.h>
#include <designersettings.h>
#include <designmodecontext.h>
#include <itemlibraryinfo.h>
#include <model/modelutils.h>
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

inline static void setScenePos(const QmlDesigner::ModelNode &modelNode, const QPointF &pos)
{
    if (modelNode.hasParentProperty() && QmlDesigner::QmlItemNode::isValidQmlItemNode(modelNode.parentProperty().parentModelNode())) {
        QmlDesigner::QmlItemNode parentNode = modelNode.parentProperty().parentQmlObjectNode().toQmlItemNode();

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
            if (oldIndex != index)
                node.parentProperty().toNodeListProperty().slide(oldIndex, index);
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
            if (oldIndex != index)
                node.parentProperty().toNodeListProperty().slide(oldIndex, index);
        }
    }
}

namespace QmlDesigner {

NavigatorView::NavigatorView(ExternalDependenciesInterface &externalDependencies)
    : AbstractView{externalDependencies}
    , m_blockSelectionChangedSignal(false)
{

}

NavigatorView::~NavigatorView()
{
    if (m_widget && !m_widget->parent())
        delete m_widget.data();
}

bool NavigatorView::hasWidget() const
{
    return true;
}

WidgetInfo NavigatorView::widgetInfo()
{
    if (!m_widget)
        setupWidget();

    return createWidgetInfo(m_widget.data(),
                            QStringLiteral("Navigator"),
                            WidgetInfo::LeftPane,
                            0,
                            tr("Navigator"),
                            tr("Navigator view"));
}

void NavigatorView::modelAttached(Model *model)
{
    AbstractView::modelAttached(model);

    QTreeView *treeView = treeWidget();

    treeView->header()->setSectionResizeMode(NavigatorTreeModel::ColumnType::Name, QHeaderView::Stretch);
    treeView->header()->resizeSection(NavigatorTreeModel::ColumnType::Alias, 26);
    treeView->header()->resizeSection(NavigatorTreeModel::ColumnType::Visibility, 26);
    treeView->header()->resizeSection(NavigatorTreeModel::ColumnType::Lock, 26);
    treeView->setIndentation(20);

    m_currentModelInterface->setFilter(false);
    m_currentModelInterface->setNameFilter("");
    m_widget->clearSearch();

    QTimer::singleShot(0, this, [this, treeView]() {
        m_currentModelInterface->setFilter(
                    QmlDesignerPlugin::settings().value(DesignerSettingsKey::NAVIGATOR_SHOW_ONLY_VISIBLE_ITEMS).toBool());

        m_currentModelInterface->setOrder(
                    QmlDesignerPlugin::settings().value(DesignerSettingsKey::NAVIGATOR_REVERSE_ITEM_ORDER).toBool());

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
    QList<ModelNode> allNodes;
    addNodeAndSubModelNodesToList(rootModelNode(), allNodes);
    for (ModelNode node : allNodes) {
        if (node.metaInfo().isFileComponent()) {
            const ProjectExplorer::FileNode *fnode = fileNodeForModelNode(node);
            if (fnode)
                fnode->setHasError(false);
        }
    }
}

void NavigatorView::addNodeAndSubModelNodesToList(const ModelNode &node, QList<ModelNode> &nodes)
{
    nodes.append(node);
    for (ModelNode subNode : node.allSubModelNodes()) {
        addNodeAndSubModelNodesToList(subNode, nodes);
    }
}

void NavigatorView::modelAboutToBeDetached(Model *model)
{
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

    AbstractView::modelAboutToBeDetached(model);
}

void NavigatorView::importsChanged(const Imports &/*addedImports*/, const Imports &/*removedImports*/)
{
    treeWidget()->update();
}

void NavigatorView::bindingPropertiesChanged(const QList<BindingProperty> & propertyList, PropertyChangeFlags /*propertyChange*/)
{
    for (const BindingProperty &bindingProperty : propertyList) {
        /* If a binding property that exports an item using an alias property has
         * changed, we have to update the affected item.
         */

        if (bindingProperty.isAliasExport())
            m_currentModelInterface->notifyDataChanged(modelNodeForId(bindingProperty.expression()));
    }
}

void NavigatorView::dragStarted(QMimeData *mimeData)
{
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

        m_widget->setDragType(texNode.metaInfo().typeName());
        m_widget->update();
    } else if (mimeData->hasFormat(Constants::MIME_TYPE_MATERIAL)) {
        qint32 internalId = mimeData->data(Constants::MIME_TYPE_MATERIAL).toInt();
        ModelNode matNode = modelNodeForInternalId(internalId);

        m_widget->setDragType(matNode.metaInfo().typeName());
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
                // We use arbitrary type name because at this time we don't have effect maker
                // specific type
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
    m_widget->setDragType("");
    m_widget->update();
}

void NavigatorView::customNotification([[maybe_unused]] const AbstractView *view,
                                       const QString &identifier,
                                       [[maybe_unused]] const QList<ModelNode> &nodeList,
                                       [[maybe_unused]] const QList<QVariant> &data)
{
    if (identifier == "asset_import_update")
        m_currentModelInterface->notifyIconsChanged();
}

void NavigatorView::handleChangedExport(const ModelNode &modelNode, bool exported)
{
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
    return QmlVisualNode(modelNode).visibilityOverride();
}

void NavigatorView::disableWidget()
{
    if (m_widget)
        m_widget->disableNavigator();
}

void NavigatorView::enableWidget()
{
    if (m_widget)
        m_widget->enableNavigator();
}

void NavigatorView::modelNodePreviewPixmapChanged(const ModelNode &node, const QPixmap &pixmap)
{
    m_treeModel->updateToolTipPixmap(node, pixmap);
}

ModelNode NavigatorView::modelNodeForIndex(const QModelIndex &modelIndex) const
{
    return modelIndex.model()->data(modelIndex, ModelNodeRole).value<ModelNode>();
}

void NavigatorView::nodeAboutToBeRemoved(const ModelNode & /*removedNode*/)
{
}

void NavigatorView::nodeRemoved(const ModelNode &removedNode,
                                const NodeAbstractProperty & /*parentProperty*/,
                                AbstractView::PropertyChangeFlags /*propertyChange*/)
{
    m_currentModelInterface->notifyModelNodesRemoved({removedNode});
}

void NavigatorView::nodeReparented(const ModelNode &modelNode,
                                   const NodeAbstractProperty & /*newPropertyParent*/,
                                   const NodeAbstractProperty & oldPropertyParent,
                                   AbstractView::PropertyChangeFlags /*propertyChange*/)
{
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
    m_currentModelInterface->notifyDataChanged(modelNode);
}

void NavigatorView::propertiesAboutToBeRemoved(const QList<AbstractProperty> &/*propertyList*/)
{
}

void NavigatorView::propertiesRemoved(const QList<AbstractProperty> &propertyList)
{
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
    m_currentModelInterface->notifyDataChanged(rootModelNode());
}

void NavigatorView::nodeTypeChanged(const ModelNode &modelNode, const TypeName &, int , int)
{
    m_currentModelInterface->notifyDataChanged(modelNode);
}

void NavigatorView::auxiliaryDataChanged(const ModelNode &modelNode,
                                         [[maybe_unused]] AuxiliaryDataKeyView key,
                                         [[maybe_unused]] const QVariant &data)
{
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
    for (const ModelNode &modelNode : errorNodeList) {
        m_currentModelInterface->notifyDataChanged(modelNode);
        propagateInstanceErrorToExplorer(modelNode);
    }
}

void NavigatorView::nodeOrderChanged(const NodeListProperty &listProperty)
{
    m_currentModelInterface->notifyModelNodesMoved(listProperty.directSubNodes());

    // make sure selection is in sync again
    QTimer::singleShot(0, this, &NavigatorView::updateItemSelection);
}

void NavigatorView::changeToComponent(const QModelIndex &index)
{
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
    return m_currentModelInterface->indexForModelNode(modelNode);
}

QAbstractItemModel *NavigatorView::currentModel() const
{
    return treeWidget()->model();
}

const ProjectExplorer::FileNode *NavigatorView::fileNodeForModelNode(const ModelNode &node) const
{
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
    if (index.isValid() && currentModel()->data(index, Qt::UserRole).isValid()) {
        ModelNode node = modelNodeForIndex(index);
        if (node.metaInfo().isFileComponent()) {
            return fileNodeForModelNode(node);
        }
    }

    return nullptr;
}

void NavigatorView::propagateInstanceErrorToExplorer(const ModelNode &modelNode) {
    QModelIndex index = indexForModelNode(modelNode);;

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
    if (selectedModelNodes().size() > 1)
        return; //Semantics are unclear for multi selection.

    bool blocked = blockSelectionChangedSignal(true);
    bool reverse = QmlDesignerPlugin::settings().value(DesignerSettingsKey::NAVIGATOR_REVERSE_ITEM_ORDER).toBool();

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
    bool blocked = blockSelectionChangedSignal(true);
    bool reverse = QmlDesignerPlugin::settings().value(DesignerSettingsKey::NAVIGATOR_REVERSE_ITEM_ORDER).toBool();

    if (reverse)
        moveNodesDown(selectedModelNodes());
    else
        moveNodesUp(selectedModelNodes());

    updateItemSelection();
    blockSelectionChangedSignal(blocked);
}

void NavigatorView::downButtonClicked()
{
    bool blocked = blockSelectionChangedSignal(true);
    bool reverse = QmlDesignerPlugin::settings().value(DesignerSettingsKey::NAVIGATOR_REVERSE_ITEM_ORDER).toBool();

    if (reverse)
        moveNodesUp(selectedModelNodes());
    else
        moveNodesDown(selectedModelNodes());

    updateItemSelection();
    blockSelectionChangedSignal(blocked);
}

void NavigatorView::filterToggled(bool flag)
{
    m_currentModelInterface->setFilter(flag);
    treeWidget()->expandAll();
    QmlDesignerPlugin::settings().insert(DesignerSettingsKey::NAVIGATOR_SHOW_ONLY_VISIBLE_ITEMS, flag);
}

void NavigatorView::reverseOrderToggled(bool flag)
{
    m_currentModelInterface->setOrder(flag);
    treeWidget()->expandAll();
    QmlDesignerPlugin::settings().insert(DesignerSettingsKey::NAVIGATOR_REVERSE_ITEM_ORDER, flag);
}

void NavigatorView::textFilterChanged(const QString &text)
{
    m_treeModel->setNameFilter(text);
    treeWidget()->expandAll();
}

void NavigatorView::changeSelection(const QItemSelection & /*newSelection*/, const QItemSelection &/*deselected*/)
{
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
    // Update selection asynchronously to ensure NavigatorTreeModel's index cache is up to date
    QTimer::singleShot(0, this, &NavigatorView::updateItemSelection);
}

void NavigatorView::updateItemSelection()
{
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
            ModelNode parentNode = node;
            while (parentNode.hasParentProperty()) {
                parentNode = parentNode.parentProperty().parentQmlObjectNode();
                QModelIndex parentIndex = indexForModelNode(parentNode);
                if (parentIndex.isValid())
                    treeWidget()->expand(parentIndex);
                else
                    break;
            }
         }
    }

    bool blocked = blockSelectionChangedSignal(true);
    treeWidget()->selectionModel()->select(itemSelection, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
    blockSelectionChangedSignal(blocked);

    if (!selectedModelNodes().isEmpty())
        treeWidget()->scrollTo(indexForModelNode(selectedModelNodes().constFirst()));

    // make sure selected nodes are visible
    for (const QModelIndex &selectedIndex : itemSelection.indexes()) {
        if (selectedIndex.column() == 0)
            expandAncestors(selectedIndex);
    }
}

QTreeView *NavigatorView::treeWidget() const
{
    if (m_widget)
        return m_widget->treeView();
    return nullptr;
}

NavigatorTreeModel *NavigatorView::treeModel()
{
    return m_treeModel.data();
}

// along the lines of QObject::blockSignals
bool NavigatorView::blockSelectionChangedSignal(bool block)
{
    bool oldValue = m_blockSelectionChangedSignal;
    m_blockSelectionChangedSignal = block;
    return oldValue;
}

void NavigatorView::expandAncestors(const QModelIndex &index)
{
    QModelIndex currentIndex = index.parent();
    while (currentIndex.isValid()) {
        if (!treeWidget()->isExpanded(currentIndex))
            treeWidget()->expand(currentIndex);
        currentIndex = currentIndex.parent();
    }
}

void NavigatorView::reparentAndCatch(NodeAbstractProperty property, const ModelNode &modelNode)
{
    try {
        property.reparentHere(modelNode);
    } catch (Exception &exception) {
        exception.showException();
    }
}

void NavigatorView::setupWidget()
{
    m_widget = new NavigatorWidget(this);
    m_treeModel = new NavigatorTreeModel(this);

    auto navigatorContext = new Internal::NavigatorContext(m_widget.data());
    Core::ICore::addContextObject(navigatorContext);

    m_treeModel->setView(this);
    m_widget->setTreeModel(m_treeModel.data());
    m_currentModelInterface = m_treeModel;

    connect(treeWidget()->selectionModel(), &QItemSelectionModel::selectionChanged, this, &NavigatorView::changeSelection);

    connect(m_widget.data(), &NavigatorWidget::leftButtonClicked, this, &NavigatorView::leftButtonClicked);
    connect(m_widget.data(), &NavigatorWidget::rightButtonClicked, this, &NavigatorView::rightButtonClicked);
    connect(m_widget.data(), &NavigatorWidget::downButtonClicked, this, &NavigatorView::downButtonClicked);
    connect(m_widget.data(), &NavigatorWidget::upButtonClicked, this, &NavigatorView::upButtonClicked);
    connect(m_widget.data(), &NavigatorWidget::filterToggled, this, &NavigatorView::filterToggled);
    connect(m_widget.data(), &NavigatorWidget::reverseOrderToggled, this, &NavigatorView::reverseOrderToggled);

    connect(m_widget.data(), &NavigatorWidget::textFilterChanged, this, &NavigatorView::textFilterChanged);

    const QString fontName = "qtds_propertyIconFont.ttf";
    const QSize size = QSize(28, 28);

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
