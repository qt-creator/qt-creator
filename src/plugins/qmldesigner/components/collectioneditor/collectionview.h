// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "abstractview.h"
#include "datastoremodelnode.h"
#include "modelnode.h"

namespace QmlJS {
class Document;
}
namespace QmlDesigner {

class CollectionWidget;
class DataStoreModelNode;

class CollectionView : public AbstractView
{
    Q_OBJECT

public:
    explicit CollectionView(ExternalDependenciesInterface &externalDependencies);

    bool hasWidget() const override;
    WidgetInfo widgetInfo() override;

    void modelAttached(Model *model) override;
    void modelAboutToBeDetached(Model *model) override;

    void nodeReparented(const ModelNode &node,
                        const NodeAbstractProperty &newPropertyParent,
                        const NodeAbstractProperty &oldPropertyParent,
                        PropertyChangeFlags propertyChange) override;

    void nodeAboutToBeRemoved(const ModelNode &removedNode) override;

    void nodeRemoved(const ModelNode &removedNode,
                     const NodeAbstractProperty &parentProperty,
                     PropertyChangeFlags propertyChange) override;

    void variantPropertiesChanged(const QList<VariantProperty> &propertyList,
                                  PropertyChangeFlags propertyChange) override;

    void selectedNodesChanged(const QList<ModelNode> &selectedNodeList,
                              const QList<ModelNode> &lastSelectedNodeList) override;

    void customNotification(const AbstractView *view,
                            const QString &identifier,
                            const QList<ModelNode> &nodeList,
                            const QList<QVariant> &data) override;

    void addResource(const QUrl &url, const QString &name, const QString &type);

    void assignCollectionToNode(const QString &collectionName, const ModelNode &node);
    void assignCollectionToSelectedNode(const QString &collectionName);

    static void registerDeclarativeType();

    void resetDataStoreNode();
    ModelNode dataStoreNode() const;
    void ensureDataStoreExists();
    QString collectionNameFromDataStoreChildren(const PropertyName &childPropertyName) const;

    bool isDataStoreReady() const { return m_libraryInfoIsUpdated; }

private:
    void refreshModel();
    NodeMetaInfo jsonCollectionMetaInfo() const;
    NodeMetaInfo csvCollectionMetaInfo() const;
    void ensureStudioModelImport();
    void onItemLibraryNodeCreated(const ModelNode &node);
    void onDocumentUpdated(const QSharedPointer<const QmlJS::Document> &doc);

    QPointer<CollectionWidget> m_widget;
    std::unique_ptr<DataStoreModelNode> m_dataStore;
    QSet<Utils::FilePath> m_expectedDocumentUpdates;
    QMetaObject::Connection m_documentUpdateConnection;
    bool m_libraryInfoIsUpdated = false;
};

class DelayedAssignCollectionToItem : public QObject
{
    Q_OBJECT

public:
    DelayedAssignCollectionToItem(CollectionView *parent,
                                  const ModelNode &node,
                                  const QString &collectionName);

public slots:
    void checkAndAssign();

private:
    QPointer<CollectionView> m_collectionView;
    ModelNode m_node;
    QString m_name;
    int m_counter = 0;
    bool m_rewriterAmended = false;
};

} // namespace QmlDesigner
