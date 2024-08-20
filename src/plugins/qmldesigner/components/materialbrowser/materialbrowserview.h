// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "abstractview.h"

#include <QPointer>
#include <QSet>
#include <QTimer>

QT_BEGIN_NAMESPACE
class QQuickView;
QT_END_NAMESPACE

namespace QmlDesigner {

class MaterialBrowserWidget;
class QmlObjectNode;

class MaterialBrowserView : public AbstractView
{
    Q_OBJECT

public:
    MaterialBrowserView(class AsynchronousImageCache &imageCache,
                        ExternalDependenciesInterface &externalDependencies);
    ~MaterialBrowserView() override;

    bool hasWidget() const override;
    WidgetInfo widgetInfo() override;

    // AbstractView
    void modelAttached(Model *model) override;
    void modelAboutToBeDetached(Model *model) override;
    void selectedNodesChanged(const QList<ModelNode> &selectedNodeList,
                              const QList<ModelNode> &lastSelectedNodeList) override;
    void modelNodePreviewPixmapChanged(const ModelNode &node,
                                       const QPixmap &pixmap,
                                       const QByteArray &requestId) override;
    void nodeIdChanged(const ModelNode &node, const QString &newId, const QString &oldId) override;
    void variantPropertiesChanged(const QList<VariantProperty> &propertyList, PropertyChangeFlags propertyChange) override;
    void bindingPropertiesChanged(const QList<BindingProperty> &propertyList,
                                  PropertyChangeFlags propertyChange) override;
    void propertiesRemoved(const QList<AbstractProperty> &propertyList) override;
    void nodeReparented(const ModelNode &node, const NodeAbstractProperty &newPropertyParent,
                        const NodeAbstractProperty &oldPropertyParent,
                        AbstractView::PropertyChangeFlags propertyChange) override;
    void nodeAboutToBeRemoved(const ModelNode &removedNode) override;
    void nodeRemoved(const ModelNode &removedNode, const NodeAbstractProperty &parentProperty,
                     PropertyChangeFlags propertyChange) override;
    void importsChanged(const Imports &addedImports, const Imports &removedImports) override;
    void customNotification(const AbstractView *view, const QString &identifier,
                            const QList<ModelNode> &nodeList, const QList<QVariant> &data) override;
    void instancesCompleted(const QVector<ModelNode> &completedNodeList) override;
    void instancePropertyChanged(const QList<QPair<ModelNode, PropertyName> > &propertyList) override;
    void auxiliaryDataChanged(const ModelNode &node,
                              AuxiliaryDataKeyView type,
                              const QVariant &data) override;
    void currentStateChanged(const ModelNode &node) override;

    void applyTextureToModel3D(const QmlObjectNode &model3D, const ModelNode &texture = {});
    void applyTextureToMaterial(const QList<ModelNode> &materials, const ModelNode &texture);

    void createTextures(const QStringList &assetPaths);

    Q_INVOKABLE void updatePropsModel(const QString &matId);
    Q_INVOKABLE void applyTextureToProperty(const QString &matId, const QString &propName);
    Q_INVOKABLE void closeChooseMatPropsView();

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    void active3DSceneChanged(qint32 sceneId);
    void refreshModel(bool updateImages);
    void updateMaterialsPreview();

    template<typename T, typename = typename std::enable_if<std::is_base_of<AbstractProperty, T>::value>::type>
    void updatePropertyList(const QList<T> &propertyList);

    bool isMaterial(const ModelNode &node) const;
    bool isTexture(const ModelNode &node) const;
    void loadPropertyGroups();
    void requestPreviews();
    ModelNode resolveSceneEnv();
    void updateMaterialSelection();
    void updateTextureSelection();
    void handleModelSelectionChange();

    AsynchronousImageCache &m_imageCache;
    QPointer<MaterialBrowserWidget> m_widget;

    bool m_hasQuick3DImport = false;
    bool m_autoSelectModelMaterial = false; // TODO: wire this to some action
    bool m_puppetResetPending = false;
    bool m_propertyGroupsLoaded = false;

    QTimer m_previewTimer;
    QTimer m_selectionTimer; // Compress selection and avoid illegal callbacks to model
    QSet<ModelNode> m_previewRequests;
    QPointer<QQuickView> m_chooseMatPropsView;
    QHash<QString, QList<PropertyName>> m_textureModels;
    QString m_appliedTextureId;
    QString m_appliedTexturePath; // defers texture creation until dialog apply
    int m_sceneId = -1;
    int m_pendingMaterialIndex = -1;
    int m_pendingTextureIndex = -1;
};

} // namespace QmlDesigner
