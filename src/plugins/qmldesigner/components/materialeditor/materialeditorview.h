// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "abstractview.h"
#include "modelnode.h"

#include <QHash>
#include <QPointer>
#include <QTimer>

QT_BEGIN_NAMESPACE
class QColorDialog;
class QShortcut;
class QStackedWidget;
QT_END_NAMESPACE

namespace QmlDesigner {

class DynamicPropertiesModel;
class ItemLibraryInfo;
class MaterialEditorQmlBackend;

class MaterialEditorView : public AbstractView
{
    Q_OBJECT

public:
    MaterialEditorView(ExternalDependenciesInterface &externalDependencies);
    ~MaterialEditorView() override;

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

    void resetView();
    void currentStateChanged(const ModelNode &node) override;
    void instancePropertyChanged(const QList<QPair<ModelNode, PropertyName> > &propertyList) override;

    void nodeTypeChanged(const ModelNode& node, const TypeName &type, int majorVersion, int minorVersion) override;
    void rootNodeTypeChanged(const QString &type, int majorVersion, int minorVersion) override;
    void modelNodePreviewPixmapChanged(const ModelNode &node, const QPixmap &pixmap) override;
    void importsChanged(const Imports &addedImports, const Imports &removedImports) override;
    void customNotification(const AbstractView *view, const QString &identifier,
                            const QList<ModelNode> &nodeList, const QList<QVariant> &data) override;
    void nodeReparented(const ModelNode &node, const NodeAbstractProperty &newPropertyParent,
                        const NodeAbstractProperty &oldPropertyParent,
                        AbstractView::PropertyChangeFlags propertyChange) override;
    void nodeAboutToBeRemoved(const ModelNode &removedNode) override;

    void dragStarted(QMimeData *mimeData) override;
    void dragEnded() override;

    void changeValue(const QString &name);
    void changeExpression(const QString &name);
    void exportPropertyAsAlias(const QString &name);
    void removeAliasExport(const QString &name);

    bool locked() const;

    void currentTimelineChanged(const ModelNode &node) override;

    DynamicPropertiesModel *dynamicPropertiesModel() const;

    static MaterialEditorView *instance();

public slots:
    void handleToolBarAction(int action);
    void handlePreviewEnvChanged(const QString &envAndValue);
    void handlePreviewModelChanged(const QString &modelStr);

protected:
    void timerEvent(QTimerEvent *event) override;
    void setValue(const QmlObjectNode &fxObjectNode, const PropertyName &name, const QVariant &value);
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    static QString materialEditorResourcesPath();

    void reloadQml();
    void highlightSupportedProperties(bool highlight = true);

    void requestPreviewRender();
    void applyMaterialToSelectedModels(const ModelNode &material, bool add = false);

    void setupQmlBackend();

    void commitVariantValueToModel(const PropertyName &propertyName, const QVariant &value);
    void commitAuxValueToModel(const PropertyName &propertyName, const QVariant &value);
    void removePropertyFromModel(const PropertyName &propertyName);
    void renameMaterial(ModelNode &material, const QString &newName);
    void duplicateMaterial(const ModelNode &material);

    bool noValidSelection() const;

    void initPreviewData();
    void delayedTypeUpdate();
    void updatePossibleTypes();

    ModelNode m_selectedMaterial;
    QTimer m_ensureMatLibTimer;
    QTimer m_typeUpdateTimer;
    QShortcut *m_updateShortcut = nullptr;
    int m_timerId = 0;
    QStackedWidget *m_stackedWidget = nullptr;
    QList<ModelNode> m_selectedModels;
    QHash<QString, MaterialEditorQmlBackend *> m_qmlBackendHash;
    MaterialEditorQmlBackend *m_qmlBackEnd = nullptr;
    bool m_locked = false;
    bool m_setupCompleted = false;
    bool m_hasQuick3DImport = false;
    bool m_hasMaterialRoot = false;
    bool m_initializingPreviewData = false;

    QPointer<QColorDialog> m_colorDialog;
    QPointer<ItemLibraryInfo> m_itemLibraryInfo;
    DynamicPropertiesModel *m_dynamicPropertiesModel = nullptr;
};

} // namespace QmlDesigner
