// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "abstractview.h"
#include "qmldesigner_global.h"

#include <QHash>
#include <QObject>
#include <QTimer>

#include <propertyeditorcomponentgenerator.h>

QT_BEGIN_NAMESPACE
class QEvent;
class QShortcut;
class QStackedWidget;
class QTimer;
QT_END_NAMESPACE

namespace QmlDesigner {

class CollapseButton;
class DynamicPropertiesModel;
class ModelNode;
class PropertyEditorQmlBackend;
class PropertyEditorView;
class PropertyEditorWidget;
class QmlObjectNode;

class QMLDESIGNER_EXPORT PropertyEditorView : public AbstractView
{
    Q_OBJECT

public:
    struct ExtraPropertyViewsCallbacks
    {
        using RegistrationFunc = std::function<void(PropertyEditorView *editor)>;
        using AddFunc = std::function<void(const QString &parentName)>;
        using TargetSelectionFunc = std::function<void(const ModelNode &node)>;

        AddFunc addEditor = [](...) {};
        RegistrationFunc registerEditor = [](...) {};
        RegistrationFunc unregisterEditor = [](...) {};
        TargetSelectionFunc setTargetNode = [](...) {};
    };

    PropertyEditorView(class AsynchronousImageCache &imageCache,
                       ExternalDependenciesInterface &externalDependencies);
    ~PropertyEditorView() override;

    bool hasWidget() const override;
    WidgetInfo widgetInfo() override;
    void setWidgetInfo(WidgetInfo info);
    void selectedNodesChanged(const QList<ModelNode> &selectedNodeList,
                              const QList<ModelNode> &lastSelectedNodeList) override;
    void nodeAboutToBeRemoved(const ModelNode &removedNode) override;
    void nodeRemoved(const ModelNode &removedNode,
                     const NodeAbstractProperty &parentProperty,
                     PropertyChangeFlags propertyChange) override;
    void propertiesRemoved(const QList<AbstractProperty>& propertyList) override;
    void propertiesAboutToBeRemoved(const QList<AbstractProperty> &propertyList) override;

    void modelAttached(Model *model) override;

    void modelAboutToBeDetached(Model *model) override;

    void variantPropertiesChanged(const QList<VariantProperty>& propertyList, PropertyChangeFlags propertyChange) override;
    void bindingPropertiesChanged(const QList<BindingProperty>& propertyList, PropertyChangeFlags propertyChange) override;
    void auxiliaryDataChanged(const ModelNode &node,
                              AuxiliaryDataKeyView key,
                              const QVariant &data) override;

    void signalDeclarationPropertiesChanged(const QVector<SignalDeclarationProperty> &propertyList,
                                            PropertyChangeFlags propertyChange) override;

    void instanceInformationsChanged(const QMultiHash<ModelNode, InformationName> &informationChangedHash) override;

    void nodeIdChanged(const ModelNode& node, const QString& newId, const QString& oldId) override;

    void resetView();
    void currentStateChanged(const ModelNode &node) override;
    void instancePropertyChanged(const QList<QPair<ModelNode, PropertyName> > &propertyList) override;

    void rootNodeTypeChanged(const QString &type) override;
    void nodeTypeChanged(const ModelNode &node, const TypeName &type) override;

    void nodeReparented(const ModelNode &node,
                        const NodeAbstractProperty &newPropertyParent,
                        const NodeAbstractProperty &oldPropertyParent,
                        AbstractView::PropertyChangeFlags propertyChange) override;

    void modelNodePreviewPixmapChanged(const ModelNode &node,
                                       const QPixmap &pixmap,
                                       const QByteArray &requestId) override;

    void importsChanged(const Imports &addedImports, const Imports &removedImports) override;
    void customNotification(const AbstractView *view,
                            const QString &identifier,
                            const QList<ModelNode> &nodeList,
                            const QList<QVariant> &data) override;

    void dragStarted(QMimeData *mimeData) override;
    void dragEnded() override;

    void changeValue(const QString &name);
    void changeExpression(const QString &name);
    void exportPropertyAsAlias(const QString &name);
    void removeAliasExport(const QString &name);
    void demoteCustomManagerRole();
    void setExtraPropertyViewsCallbacks(const ExtraPropertyViewsCallbacks &callbacks);

    bool locked() const;
    bool isSelectionLocked() const { return m_isSelectionLocked; }

    void currentTimelineChanged(const ModelNode &node) override;

    void refreshMetaInfos(const TypeIds &deletedTypeIds) override;

    DynamicPropertiesModel *dynamicPropertiesModel() const;

    void setUnifiedAction(QAction *unifiedAction);
    QAction *unifiedAction() const;

    ModelNode activeNode() const;
    void setTargetNode(const ModelNode &node);

    void setInstancesCount(int n);
    int instancesCount() const;

    virtual void registerWidgetInfo() override;
    virtual void deregisterWidgetInfo() override;

    void showExtraWidget();
    void closeExtraWidget();

    static void setExpressionOnObjectNode(const QmlObjectNode &objectNode,
                                          PropertyNameView name,
                                          const QString &expression);

    static void generateAliasForProperty(const ModelNode &modelNode,
                                         const QString &propertyName);

    static void removeAliasForProperty(const ModelNode &modelNode,
                                         const QString &propertyName);

public slots:
    void handleToolBarAction(int action);

protected:
    void setValue(const QmlObjectNode &fxObjectNode, PropertyNameView name, const QVariant &value);
    bool eventFilter(QObject *obj, QEvent *event) override;

private: //functions
    void reloadQml();
    void updateSize();

    void select();
    void loadLockedNode();
    void saveLockedNode();
    void setActiveNodeToSelection();

    void delayedResetView();
    void setupQmlBackend();

    void commitVariantValueToModel(PropertyNameView propertyName, const QVariant &value);
    void commitAuxValueToModel(PropertyNameView propertyName, const QVariant &value);
    void removePropertyFromModel(PropertyNameView propertyName);

    bool noValidSelection() const;
    void highlightTextureProperties(bool highlight = true);

    void setActiveNode(const ModelNode &node);
    QList<ModelNode> currentNodes() const;

    void setSelectionUnlocked();
    void setIsSelectionLocked(bool locked);

    bool isNodeOrChildSelected(const ModelNode &node) const;
    void setSelectionUnlockedIfNodeRemoved(const ModelNode &removedNode);

    static PropertyEditorView *instance(); // TODO: remove

    NodeMetaInfo findCommonAncestor(const ModelNode &node);
    AuxiliaryDataKey activeNodeAuxKey() const;

    void showAsExtraWidget();

private: //variables
    enum class ManageCustomNotifications { No, Yes };
    AsynchronousImageCache &m_imageCache;
    ModelNode m_activeNode;
    QShortcut *m_updateShortcut;
    QPointer<QAction> m_unifiedAction;
    std::unique_ptr<DynamicPropertiesModel> m_dynamicPropertiesModel;
    PropertyEditorWidget* m_stackedWidget;
    QString m_qmlDir;
    QHash<QString, PropertyEditorQmlBackend *> m_qmlBackendHash;
    PropertyEditorQmlBackend *m_qmlBackEndForCurrentType = nullptr;
    PropertyComponentGenerator m_propertyComponentGenerator;
    PropertyEditorComponentGenerator m_propertyEditorComponentGenerator{m_propertyComponentGenerator};
    bool m_locked;
    bool m_texOrMatAboutToBeRemoved = false;
    bool m_isSelectionLocked = false;
    int m_instancesCount = 0;
    QString m_parentWidgetId = "";
    QString m_uniqueWidgetId = "Properties";
    QString m_widgetTabName = tr("Properties");
    ManageCustomNotifications m_manageNotifications;
    ExtraPropertyViewsCallbacks m_extraPropertyViewsCallbacks;

    friend class PropertyEditorDynamicPropertiesProxyModel;
};

} //QmlDesigner
