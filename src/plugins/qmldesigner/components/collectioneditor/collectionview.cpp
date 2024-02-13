// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "collectionview.h"

#include "collectiondetailsmodel.h"
#include "collectioneditorconstants.h"
#include "collectioneditorutils.h"
#include "collectionsourcemodel.h"
#include "collectionwidget.h"
#include "datastoremodelnode.h"
#include "designmodecontext.h"
#include "nodeabstractproperty.h"
#include "nodemetainfo.h"
#include "qmldesignerplugin.h"
#include "variantproperty.h"

#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectmanager.h>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include <coreplugin/icore.h>
#include <utils/algorithm.h>
#include <utils/qtcassert.h>

namespace {

inline bool isStudioCollectionModel(const QmlDesigner::ModelNode &node)
{
    using namespace QmlDesigner::CollectionEditorConstants;
    return node.metaInfo().typeName() == JSONCOLLECTIONMODEL_TYPENAME
           || node.metaInfo().typeName() == CSVCOLLECTIONMODEL_TYPENAME;
}

} // namespace

namespace QmlDesigner {

CollectionView::CollectionView(ExternalDependenciesInterface &externalDependencies)
    : AbstractView(externalDependencies)
    , m_dataStore(std::make_unique<DataStoreModelNode>())

{
    connect(ProjectExplorer::ProjectManager::instance(),
            &ProjectExplorer::ProjectManager::startupProjectChanged, this, [this] {
        resetDataStoreNode();
        if (m_widget.get())
            m_widget->collectionDetailsModel()->removeAllCollections();
    });
    resetDataStoreNode();
}

bool CollectionView::hasWidget() const
{
    return true;
}

QmlDesigner::WidgetInfo CollectionView::widgetInfo()
{
    if (m_widget.isNull()) {
        m_widget = new CollectionWidget(this);
        m_widget->setMinimumSize(m_widget->minimumSizeHint());

        auto collectionEditorContext = new Internal::CollectionEditorContext(m_widget.data());
        Core::ICore::addContextObject(collectionEditorContext);
        CollectionSourceModel *sourceModel = m_widget->sourceModel().data();

        connect(sourceModel,
                &CollectionSourceModel::collectionSelected,
                this,
                [this](const QString &collection) {
                    m_widget->collectionDetailsModel()->loadCollection(dataStoreNode(), collection);
                });

        connect(sourceModel, &CollectionSourceModel::isEmptyChanged, this, [this](bool isEmpty) {
            if (isEmpty)
                m_widget->collectionDetailsModel()->loadCollection({}, {});
        });

        connect(sourceModel,
                &CollectionSourceModel::collectionNamesInitialized,
                this,
                [this](const QStringList &collectionNames) {
                    m_dataStore->setCollectionNames(collectionNames);
                });

        connect(sourceModel,
                &CollectionSourceModel::collectionRenamed,
                this,
                [this](const QString &oldName, const QString &newName) {
                    m_dataStore->renameCollection(oldName, newName);
                    m_widget->collectionDetailsModel()->renameCollection(dataStoreNode(),
                                                                         oldName,
                                                                         newName);
                });

        connect(sourceModel,
                &CollectionSourceModel::collectionRemoved,
                this,
                [this](const QString &collectionName) {
                    m_dataStore->removeCollection(collectionName);
                    m_widget->collectionDetailsModel()->removeCollection(dataStoreNode(),
                                                                         collectionName);
                });
    }

    return createWidgetInfo(m_widget.data(),
                            "CollectionEditor",
                            WidgetInfo::LeftPane,
                            0,
                            tr("Model Editor"),
                            tr("Model Editor view"));
}

void CollectionView::modelAttached(Model *model)
{
    AbstractView::modelAttached(model);
    resetDataStoreNode();
}

void CollectionView::nodeReparented(const ModelNode &node,
                                    [[maybe_unused]] const NodeAbstractProperty &newPropertyParent,
                                    [[maybe_unused]] const NodeAbstractProperty &oldPropertyParent,
                                    [[maybe_unused]] PropertyChangeFlags propertyChange)
{
    if (!isStudioCollectionModel(node))
        return;

    refreshModel();

    m_widget->sourceModel()->selectSource(node);
}

void CollectionView::nodeAboutToBeRemoved(const ModelNode &removedNode)
{
    // removing the model lib node
    if (isStudioCollectionModel(removedNode))
        m_widget->sourceModel()->removeSource(removedNode);
}

void CollectionView::nodeRemoved(const ModelNode &removedNode,
                                 [[maybe_unused]] const NodeAbstractProperty &parentProperty,
                                 [[maybe_unused]] PropertyChangeFlags propertyChange)
{
    if (isStudioCollectionModel(removedNode))
        m_widget->sourceModel()->updateSelectedSource(true);
}

void CollectionView::variantPropertiesChanged(const QList<VariantProperty> &propertyList,
                                              [[maybe_unused]] PropertyChangeFlags propertyChange)
{
    for (const VariantProperty &property : propertyList) {
        ModelNode node(property.parentModelNode());
        if (isStudioCollectionModel(node)) {
            if (property.name() == "objectName")
                m_widget->sourceModel()->updateNodeName(node);
            else if (property.name() == CollectionEditorConstants::SOURCEFILE_PROPERTY)
                m_widget->sourceModel()->updateNodeSource(node);
        }
    }
}

void CollectionView::selectedNodesChanged(const QList<ModelNode> &selectedNodeList,
                                          [[maybe_unused]] const QList<ModelNode> &lastSelectedNodeList)
{
    QList<ModelNode> selectedCollectionNodes = Utils::filtered(selectedNodeList,
                                                               &isStudioCollectionModel);

    bool singleNonCollectionNodeSelected = selectedNodeList.size() == 1
                                           && selectedCollectionNodes.isEmpty();

    bool singleSelectedHasModelProperty = false;
    if (singleNonCollectionNodeSelected) {
        const ModelNode selectedNode = selectedNodeList.first();
        singleSelectedHasModelProperty = CollectionEditorUtils::canAcceptCollectionAsModel(
            selectedNode);
    }

    m_widget->setTargetNodeSelected(singleSelectedHasModelProperty);

    // More than one model is selected. So ignore them
    if (selectedCollectionNodes.size() > 1)
        return;

    if (selectedCollectionNodes.size() == 1) { // If exactly one model is selected
        m_widget->sourceModel()->selectSource(selectedCollectionNodes.first());
        return;
    }
}

void CollectionView::addResource(const QUrl &url, const QString &name, const QString &type)
{
    executeInTransaction(Q_FUNC_INFO, [this, &url, &name, &type]() {
        ensureStudioModelImport();
        QString sourceAddress;
        if (url.isLocalFile()) {
            Utils::FilePath fp = QmlDesignerPlugin::instance()->currentDesignDocument()->fileName().parentDir();
            sourceAddress = Utils::FilePath::calcRelativePath(url.toLocalFile(),
                                                              fp.absoluteFilePath().toString());
        } else {
            sourceAddress = url.toString();
        }

        const NodeMetaInfo resourceMetaInfo = type.compare("json", Qt::CaseInsensitive) == 0
                                                  ? jsonCollectionMetaInfo()
                                                  : csvCollectionMetaInfo();
        ModelNode resourceNode = createModelNode(resourceMetaInfo.typeName(),
                                                 resourceMetaInfo.majorVersion(),
                                                 resourceMetaInfo.minorVersion());
        VariantProperty sourceProperty = resourceNode.variantProperty(
            CollectionEditorConstants::SOURCEFILE_PROPERTY);
        VariantProperty nameProperty = resourceNode.variantProperty("objectName");
        sourceProperty.setValue(sourceAddress);
        nameProperty.setValue(name);
        resourceNode.setIdWithoutRefactoring(model()->generateIdFromName(name, "model"));
        rootModelNode().defaultNodeAbstractProperty().reparentHere(resourceNode);
    });
}

void CollectionView::assignCollectionToSelectedNode(const QString &collectionName)
{
    QTC_ASSERT(dataStoreNode() && hasSingleSelectedModelNode(), return);
    m_dataStore->assignCollectionToNode(
        this,
        singleSelectedModelNode(),
        collectionName,
        [&](const QString &collectionName, const QString &columnName) -> bool {
            const CollectionReference reference{dataStoreNode(), collectionName};
            return m_widget->collectionDetailsModel()->collectionHasColumn(reference, columnName);
        },
        [&](const QString &collectionName) -> QString {
            const CollectionReference reference{dataStoreNode(), collectionName};
            return m_widget->collectionDetailsModel()->getFirstColumnName(reference);
        });
}

void CollectionView::registerDeclarativeType()
{
    CollectionDetails::registerDeclarativeType();
    CollectionJsonSourceFilterModel::registerDeclarativeType();
}

void CollectionView::resetDataStoreNode()
{
    m_dataStore->reloadModel();
    refreshModel();
}

ModelNode CollectionView::dataStoreNode() const
{
    return m_dataStore->modelNode();
}

void CollectionView::refreshModel()
{
    if (!model())
        return;

    // Load Model Groups
    ModelNodes collectionSourceNodes;

    if (ModelNode dataStore = m_dataStore->modelNode())
        collectionSourceNodes << dataStore;

    m_widget->sourceModel()->setSources(collectionSourceNodes);
}

NodeMetaInfo CollectionView::jsonCollectionMetaInfo() const
{
    return model()->metaInfo(CollectionEditorConstants::JSONCOLLECTIONMODEL_TYPENAME);
}

NodeMetaInfo CollectionView::csvCollectionMetaInfo() const
{
    return model()->metaInfo(CollectionEditorConstants::CSVCOLLECTIONMODEL_TYPENAME);
}

void CollectionView::ensureStudioModelImport()
{
    executeInTransaction(__FUNCTION__, [&] {
        Import import = Import::createLibraryImport(CollectionEditorConstants::COLLECTIONMODEL_IMPORT);
        try {
            if (!model()->hasImport(import, true, true))
                model()->changeImports({import}, {});
        } catch (const Exception &) {
            QTC_ASSERT(false, return);
        }
    });
}

} // namespace QmlDesigner
