// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "createtexture.h"
#include "abstractview.h"
#include "nodemetainfo.h"

#include <QObject>
#include <QPointer>

namespace QmlDesigner {

class ContentLibraryMaterial;
class ContentLibraryTexture;
class ContentLibraryWidget;
class Model;

class ContentLibraryView : public AbstractView
{
    Q_OBJECT

public:
    ContentLibraryView(ExternalDependenciesInterface &externalDependencies);
    ~ContentLibraryView() override;

    bool hasWidget() const override;
    WidgetInfo widgetInfo() override;

    // AbstractView
    void modelAttached(Model *model) override;
    void modelAboutToBeDetached(Model *model) override;
    void importsChanged(const QList<Import> &addedImports, const QList<Import> &removedImports) override;
    void active3DSceneChanged(qint32 sceneId) override;
    void selectedNodesChanged(const QList<ModelNode> &selectedNodeList,
                              const QList<ModelNode> &lastSelectedNodeList) override;
    void customNotification(const AbstractView *view, const QString &identifier,
                            const QList<ModelNode> &nodeList, const QList<QVariant> &data) override;
    void nodeReparented(const ModelNode &node, const NodeAbstractProperty &newPropertyParent,
                        const NodeAbstractProperty &oldPropertyParent,
                        AbstractView::PropertyChangeFlags propertyChange) override;
    void nodeAboutToBeRemoved(const ModelNode &removedNode) override;

private:
    void updateBundleMaterialsImportedState();
    void updateBundleMaterialsQuick3DVersion();
    void applyBundleMaterialToDropTarget(const ModelNode &bundleMat, const NodeMetaInfo &metaInfo = {});
    ModelNode getBundleMaterialDefaultInstance(const TypeName &type);
    ModelNode createMaterial(const NodeMetaInfo &metaInfo);

    QPointer<ContentLibraryWidget> m_widget;
    QList<ModelNode> m_bundleMaterialTargets;
    QList<ModelNode> m_selectedModels; // selected 3D model nodes
    ContentLibraryMaterial *m_draggedBundleMaterial = nullptr;
    ContentLibraryTexture *m_draggedBundleTexture = nullptr;
    bool m_bundleMaterialAddToSelected = false;
    bool m_hasQuick3DImport = false;
    qint32 m_sceneId = -1;
    CreateTexture m_createTexture;
};

} // namespace QmlDesigner
