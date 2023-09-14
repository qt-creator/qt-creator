// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "abstractview.h"
#include "createtexture.h"
#include "nodemetainfo.h"

#include <QObject>
#include <QPointer>

namespace QmlDesigner {

class ContentLibraryEffect;
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
    void importsChanged(const Imports &addedImports, const Imports &removedImports) override;
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
    void updateBundleEffectsImportedState();
    void updateBundlesQuick3DVersion();
#ifdef QDS_USE_PROJECTSTORAGE
    void applyBundleMaterialToDropTarget(const ModelNode &bundleMat, const TypeName &typeName = {});
#else
    void applyBundleMaterialToDropTarget(const ModelNode &bundleMat,
                                         const NodeMetaInfo &metaInfo = {});
#endif
    ModelNode getBundleMaterialDefaultInstance(const TypeName &type);
#ifdef QDS_USE_PROJECTSTORAGE
    ModelNode createMaterial(const TypeName &typeName);
#else
    ModelNode createMaterial(const NodeMetaInfo &metaInfo);
#endif
    QPointer<ContentLibraryWidget> m_widget;
    QList<ModelNode> m_bundleMaterialTargets;
    ModelNode m_bundleEffectTarget; // target of the dropped bundle effect
    QVariant m_bundleEffectPos; // pos of the dropped bundle effect
    QList<ModelNode> m_selectedModels; // selected 3D model nodes
    ContentLibraryMaterial *m_draggedBundleMaterial = nullptr;
    ContentLibraryTexture *m_draggedBundleTexture = nullptr;
    ContentLibraryEffect *m_draggedBundleEffect = nullptr;
    bool m_bundleMaterialAddToSelected = false;
    bool m_hasQuick3DImport = false;
    qint32 m_sceneId = -1;
    CreateTexture m_createTexture;
};

} // namespace QmlDesigner
