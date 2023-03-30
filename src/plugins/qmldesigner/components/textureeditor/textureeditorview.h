// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <abstractview.h>
#include <itemlibraryinfo.h>

#include <QHash>
#include <QPointer>
#include <QTimer>

QT_BEGIN_NAMESPACE
class QShortcut;
class QStackedWidget;
class QTimer;
class QColorDialog;
QT_END_NAMESPACE

namespace QmlDesigner {

class DynamicPropertiesModel;
class ModelNode;
class TextureEditorQmlBackend;

class TextureEditorView : public AbstractView
{
    Q_OBJECT

public:
    TextureEditorView(class AsynchronousImageCache &imageCache,
                      ExternalDependenciesInterface &externalDependencies);
    ~TextureEditorView() override;

    bool hasWidget() const override;
    WidgetInfo widgetInfo() override;

    void selectedNodesChanged(const QList<ModelNode> &selectedNodeList,
                              const QList<ModelNode> &lastSelectedNodeList) override;

    void propertiesRemoved(const QList<AbstractProperty> &propertyList) override;

    void modelAttached(Model *model) override;
    void modelAboutToBeDetached(Model *model) override;

    void variantPropertiesChanged(const QList<VariantProperty> &propertyList, PropertyChangeFlags propertyChange) override;
    void bindingPropertiesChanged(const QList<BindingProperty> &propertyList, PropertyChangeFlags propertyChange) override;
    void auxiliaryDataChanged(const ModelNode &node,
                              AuxiliaryDataKeyView key,
                              const QVariant &data) override;
    void propertiesAboutToBeRemoved(const QList<AbstractProperty> &propertyList) override;
    void nodeReparented(const ModelNode &node, const NodeAbstractProperty &newPropertyParent,
                        const NodeAbstractProperty &oldPropertyParent,
                        AbstractView::PropertyChangeFlags propertyChange) override;
    void nodeAboutToBeRemoved(const ModelNode &removedNode) override;

    void resetView();
    void currentStateChanged(const ModelNode &node) override;
    void instancePropertyChanged(const QList<QPair<ModelNode, PropertyName> > &propertyList) override;

    void importsChanged(const Imports &addedImports, const Imports &removedImports) override;
    void customNotification(const AbstractView *view, const QString &identifier,
                            const QList<ModelNode> &nodeList, const QList<QVariant> &data) override;

    void dragStarted(QMimeData *mimeData) override;
    void dragEnded() override;

    void changeValue(const QString &name);
    void changeExpression(const QString &name);
    void exportPropertyAsAlias(const QString &name);
    void removeAliasExport(const QString &name);

    bool locked() const;

    void currentTimelineChanged(const ModelNode &node) override;

    DynamicPropertiesModel *dynamicPropertiesModel() const;

    static TextureEditorView *instance();

public slots:
    void handleToolBarAction(int action);

protected:
    void timerEvent(QTimerEvent *event) override;
    void setValue(const QmlObjectNode &fxObjectNode, const PropertyName &name, const QVariant &value);
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    static QString textureEditorResourcesPath();

    void reloadQml();
    void highlightSupportedProperties(bool highlight = true);

    void applyTextureToSelectedModel(const ModelNode &texture);

    void setupQmlBackend();

    void commitVariantValueToModel(const PropertyName &propertyName, const QVariant &value);
    void commitAuxValueToModel(const PropertyName &propertyName, const QVariant &value);
    void removePropertyFromModel(const PropertyName &propertyName);
    void duplicateTexture(const ModelNode &texture);

    bool noValidSelection() const;

    AsynchronousImageCache &m_imageCache;
    ModelNode m_selectedTexture;
    QTimer m_ensureMatLibTimer;
    QShortcut *m_updateShortcut = nullptr;
    int m_timerId = 0;
    QStackedWidget *m_stackedWidget = nullptr;
    ModelNode m_selectedModel;
    QHash<QString, TextureEditorQmlBackend *> m_qmlBackendHash;
    TextureEditorQmlBackend *m_qmlBackEnd = nullptr;
    bool m_locked = false;
    bool m_setupCompleted = false;
    bool m_hasQuick3DImport = false;
    bool m_hasTextureRoot = false;
    bool m_initializingPreviewData = false;

    QPointer<QColorDialog> m_colorDialog;
    QPointer<ItemLibraryInfo> m_itemLibraryInfo;
    DynamicPropertiesModel *m_dynamicPropertiesModel = nullptr;
};

} // namespace QmlDesigner
