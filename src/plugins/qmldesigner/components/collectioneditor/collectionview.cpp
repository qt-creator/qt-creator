// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "collectionview.h"

#include "collectioneditorconstants.h"
#include "collectionsourcemodel.h"
#include "collectionwidget.h"
#include "designmodecontext.h"
#include "nodeabstractproperty.h"
#include "nodemetainfo.h"
#include "qmldesignerplugin.h"
#include "singlecollectionmodel.h"
#include "variantproperty.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include <coreplugin/icore.h>
#include <utils/algorithm.h>
#include <utils/qtcassert.h>

namespace {
inline bool isStudioCollectionModel(const QmlDesigner::ModelNode &node)
{
    using namespace QmlDesigner::CollectionEditor;
    return node.metaInfo().typeName() == JSONCOLLECTIONMODEL_TYPENAME
           || node.metaInfo().typeName() == CSVCOLLECTIONMODEL_TYPENAME;
}

} // namespace

namespace QmlDesigner {

CollectionView::CollectionView(ExternalDependenciesInterface &externalDependencies)
    : AbstractView(externalDependencies)
{}

bool CollectionView::hasWidget() const
{
    return true;
}

QmlDesigner::WidgetInfo CollectionView::widgetInfo()
{
    if (m_widget.isNull()) {
        m_widget = new CollectionWidget(this);

        auto collectionEditorContext = new Internal::CollectionEditorContext(m_widget.data());
        Core::ICore::addContextObject(collectionEditorContext);
        CollectionSourceModel *sourceModel = m_widget->sourceModel().data();

        connect(sourceModel,
                &CollectionSourceModel::collectionSelected,
                this,
                [this](const ModelNode &sourceNode, const QString &collection) {
                    m_widget->singleCollectionModel()->loadCollection(sourceNode, collection);
                });
    }

    return createWidgetInfo(m_widget.data(),
                            "CollectionEditor",
                            WidgetInfo::LeftPane,
                            0,
                            tr("Collection Editor"),
                            tr("Collection Editor view"));
}

void CollectionView::modelAttached(Model *model)
{
    AbstractView::modelAttached(model);
    refreshModel();
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
    // removing the collections lib node
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
            else if (property.name() == CollectionEditor::SOURCEFILE_PROPERTY)
                m_widget->sourceModel()->updateNodeSource(node);
            else if (property.name() == "id")
                m_widget->sourceModel()->updateNodeId(node);
        }
    }
}

void CollectionView::selectedNodesChanged(const QList<ModelNode> &selectedNodeList,
                                          [[maybe_unused]] const QList<ModelNode> &lastSelectedNodeList)
{
    QList<ModelNode> selectedJsonCollections = Utils::filtered(selectedNodeList,
                                                               &isStudioCollectionModel);

    // More than one collections are selected. So ignore them
    if (selectedJsonCollections.size() > 1)
        return;

    if (selectedJsonCollections.size() == 1) { // If exactly one collection is selected
        m_widget->sourceModel()->selectSource(selectedJsonCollections.first());
        return;
    }
}

void CollectionView::addResource(const QUrl &url, const QString &name, const QString &type)
{
    executeInTransaction(Q_FUNC_INFO, [this, &url, &name, &type]() {
        ensureStudioModelImport();
        QString sourceAddress = url.isLocalFile() ? url.toLocalFile() : url.toString();
        const NodeMetaInfo resourceMetaInfo = type.compare("json", Qt::CaseInsensitive) == 0
                                                  ? jsonCollectionMetaInfo()
                                                  : csvCollectionMetaInfo();
        ModelNode resourceNode = createModelNode(resourceMetaInfo.typeName(),
                                                 resourceMetaInfo.majorVersion(),
                                                 resourceMetaInfo.minorVersion());
        VariantProperty sourceProperty = resourceNode.variantProperty(
            CollectionEditor::SOURCEFILE_PROPERTY);
        VariantProperty nameProperty = resourceNode.variantProperty("objectName");
        sourceProperty.setValue(sourceAddress);
        nameProperty.setValue(name);
        rootModelNode().defaultNodeAbstractProperty().reparentHere(resourceNode);
    });
}

void CollectionView::refreshModel()
{
    if (!model())
        return;

    // Load Json Collections
    const ModelNodes jsonSourceNodes = rootModelNode().subModelNodesOfType(jsonCollectionMetaInfo());
    m_widget->sourceModel()->setSources(jsonSourceNodes);
}

NodeMetaInfo CollectionView::jsonCollectionMetaInfo() const
{
    return model()->metaInfo(CollectionEditor::JSONCOLLECTIONMODEL_TYPENAME);
}

NodeMetaInfo CollectionView::csvCollectionMetaInfo() const
{
    return model()->metaInfo(CollectionEditor::CSVCOLLECTIONMODEL_TYPENAME);
}

void CollectionView::ensureStudioModelImport()
{
    executeInTransaction(__FUNCTION__, [&] {
        Import import = Import::createLibraryImport(CollectionEditor::COLLECTIONMODEL_IMPORT);
        try {
            if (!model()->hasImport(import, true, true))
                model()->changeImports({import}, {});
        } catch (const Exception &) {
            QTC_ASSERT(false, return);
        }
    });
}

} // namespace QmlDesigner
