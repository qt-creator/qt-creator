// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <abstractview.h>

#include <QPointer>

namespace QmlDesigner {

class BundleMaterial;
class MaterialBrowserWidget;

class MaterialBrowserView : public AbstractView
{
    Q_OBJECT

public:
    MaterialBrowserView(QObject *parent = nullptr);
    ~MaterialBrowserView() override;

    bool hasWidget() const override;
    WidgetInfo widgetInfo() override;

    // AbstractView
    void modelAttached(Model *model) override;
    void modelAboutToBeDetached(Model *model) override;
    void selectedNodesChanged(const QList<ModelNode> &selectedNodeList,
                              const QList<ModelNode> &lastSelectedNodeList) override;
    void modelNodePreviewPixmapChanged(const ModelNode &node, const QPixmap &pixmap) override;
    void variantPropertiesChanged(const QList<VariantProperty> &propertyList, PropertyChangeFlags propertyChange) override;
    void nodeReparented(const ModelNode &node, const NodeAbstractProperty &newPropertyParent,
                        const NodeAbstractProperty &oldPropertyParent,
                        AbstractView::PropertyChangeFlags propertyChange) override;
    void nodeAboutToBeRemoved(const ModelNode &removedNode) override;
    void nodeRemoved(const ModelNode &removedNode, const NodeAbstractProperty &parentProperty,
                     PropertyChangeFlags propertyChange) override;
    void importsChanged(const QList<Import> &addedImports, const QList<Import> &removedImports) override;
    void customNotification(const AbstractView *view, const QString &identifier,
                            const QList<ModelNode> &nodeList, const QList<QVariant> &data) override;
    void instancesCompleted(const QVector<ModelNode> &completedNodeList) override;

private:
    void refreshModel(bool updateImages);
    bool isMaterial(const ModelNode &node) const;
    void loadPropertyGroups();

    QPointer<MaterialBrowserWidget> m_widget;
    ModelNode m_bundleMaterialDropTarget;
    ModelNode m_selectedModel; // first selected 3D model node
    BundleMaterial *m_draggedBundleMaterial = nullptr;
    bool m_bundleMaterialAddToSelected = false;
    bool m_hasQuick3DImport = false;
    bool m_autoSelectModelMaterial = false; // TODO: wire this to some action
    bool m_puppetResetPending = false;
    bool m_propertyGroupsLoaded = false;
};

} // namespace QmlDesigner
