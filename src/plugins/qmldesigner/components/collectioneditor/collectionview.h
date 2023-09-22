// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "abstractview.h"
#include "modelnode.h"

#include <QDateTime>

namespace QmlDesigner {

class CollectionWidget;

class CollectionView : public AbstractView
{
    Q_OBJECT

public:
    explicit CollectionView(ExternalDependenciesInterface &externalDependencies);

    bool hasWidget() const override;
    WidgetInfo widgetInfo() override;

    void modelAttached(Model *model) override;

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

    void addResource(const QUrl &url, const QString &name, const QString &type);

private:
    void refreshModel();
    NodeMetaInfo jsonCollectionMetaInfo() const;
    NodeMetaInfo csvCollectionMetaInfo() const;
    void ensureStudioModelImport();

    QPointer<CollectionWidget> m_widget;
};
} // namespace QmlDesigner
