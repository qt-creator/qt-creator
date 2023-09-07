// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "model.h"
#include "modelnode.h"
#include "qmldesignercorelib_global.h"
#include <commondefines.h>

#include <QObject>
#include <QPointer>

QT_BEGIN_NAMESPACE
class QImage;
class QPixmap;
class QVector3D;
QT_END_NAMESPACE

namespace QmlDesigner {

class AbstractProperty;
class DocumentMessage;
class ExternalDependenciesInterface;
class NodeInstanceView;
class QmlModelState;
class QmlTimeline;
class RewriterTransaction;
class RewriterView;

namespace Internal {
class InternalNode;
using InternalNodePointer = std::shared_ptr<InternalNode>;
}

enum DesignerWidgetFlags {
    DisableOnError,
    IgnoreErrors
};

class WidgetInfo {

public:
    enum PlacementHint {
        NoPane,
        LeftPane,
        RightPane,
        BottomPane,
        TopPane, // not used
        CentralPane
    };

    QString uniqueId;
    QString tabName;
    QString feedbackDisplayName;
    QWidget *widget = nullptr;
    int placementPriority;
    PlacementHint placementHint;
    DesignerWidgetFlags widgetFlags = DesignerWidgetFlags::DisableOnError;
};

class QMLDESIGNERCORE_EXPORT AbstractView : public QObject
{
    Q_OBJECT

public:
    Q_FLAGS(PropertyChangeFlag PropertyChangeFlags)

    enum PropertyChangeFlag {
      NoAdditionalChanges    = 0x0,
      PropertiesAdded        = 0x1,
      EmptyPropertiesRemoved = 0x2
    };
    Q_DECLARE_FLAGS(PropertyChangeFlags, PropertyChangeFlag)

    AbstractView(ExternalDependenciesInterface &externalDependencies)
        : m_externalDependencies{externalDependencies}
    {}

    ~AbstractView() override;

    Model *model() const;
    bool isAttached() const;

    RewriterTransaction beginRewriterTransaction(const QByteArray &identifier);

    ModelNode createModelNode(const TypeName &typeName);

    ModelNode createModelNode(const TypeName &typeName,
                              int majorVersion,
                              int minorVersion,
                              const PropertyListType &propertyList = PropertyListType(),
                              const AuxiliaryDatas &auxPropertyList = {},
                              const QString &nodeSource = {},
                              ModelNode::NodeSourceType nodeSourceType = ModelNode::NodeWithoutSource,
                              const QString &behaviorPropertyName = {});

    ModelNode rootModelNode() const;
    ModelNode rootModelNode();

    void setSelectedModelNodes(const QList<ModelNode> &selectedNodeList);
    void setSelectedModelNode(const ModelNode &modelNode);

    void selectModelNode(const ModelNode &node);
    void deselectModelNode(const ModelNode &node);
    void clearSelectedModelNodes();
    bool hasSelectedModelNodes() const;
    bool hasSingleSelectedModelNode() const;
    bool isSelectedModelNode(const ModelNode &modelNode) const;

    QList<ModelNode> selectedModelNodes() const;
    ModelNode firstSelectedModelNode() const;
    ModelNode singleSelectedModelNode() const;

    ModelNode modelNodeForId(const QString &id);
    bool hasId(const QString &id) const;

    ModelNode modelNodeForInternalId(qint32 internalId) const;
    bool hasModelNodeForInternalId(qint32 internalId) const;

    QList<ModelNode> allModelNodes() const;
    QList<ModelNode> allModelNodesUnordered() const;
    QList<ModelNode> allModelNodesOfType(const NodeMetaInfo &typeName) const;

    void emitDocumentMessage(const QList<DocumentMessage> &errors, const QList<DocumentMessage> &warnings = {});
    void emitDocumentMessage(const QString &error);
    void emitCustomNotification(const QString &identifier);
    void emitCustomNotification(const QString &identifier, const QList<ModelNode> &nodeList);
    void emitCustomNotification(const QString &identifier, const QList<ModelNode> &nodeList, const QList<QVariant> &data);

    void emitInstancePropertyChange(const QList<QPair<ModelNode, PropertyName> > &propertyList);
    void emitInstanceErrorChange(const QVector<qint32> &instanceIds);
    void emitInstancesCompleted(const QVector<ModelNode> &nodeList);
    void emitInstanceInformationsChange(const QMultiHash<ModelNode, InformationName> &informationChangeHash);
    void emitInstancesRenderImageChanged(const QVector<ModelNode> &nodeList);
    void emitInstancesPreviewImageChanged(const QVector<ModelNode> &nodeList);
    void emitInstancesChildrenChanged(const QVector<ModelNode> &nodeList);
    void emitRewriterBeginTransaction();
    void emitRewriterEndTransaction();
    void emitInstanceToken(const QString &token, int number, const QVector<ModelNode> &nodeVector);
    void emitRenderImage3DChanged(const QImage &image);
    void emitUpdateActiveScene3D(const QVariantMap &sceneState);
    void emitModelNodelPreviewPixmapChanged(const ModelNode &node, const QPixmap &pixmap);
    void emitImport3DSupportChanged(const QVariantMap &supportMap);
    void emitNodeAtPosResult(const ModelNode &modelNode, const QVector3D &pos3d);
    void emitView3DAction(View3DActionType type, const QVariant &value);

    virtual void modelAttached(Model *model);
    virtual void modelAboutToBeDetached(Model *model);

    virtual void refreshMetaInfos(const TypeIds &deletedTypeIds);

    virtual void nodeCreated(const ModelNode &createdNode);
    virtual void nodeAboutToBeRemoved(const ModelNode &removedNode);
    virtual void nodeRemoved(const ModelNode &removedNode, const NodeAbstractProperty &parentProperty,
                             PropertyChangeFlags propertyChange);
    virtual void nodeAboutToBeReparented(const ModelNode &node, const NodeAbstractProperty &newPropertyParent,
                                         const NodeAbstractProperty &oldPropertyParent, PropertyChangeFlags propertyChange);
    virtual void nodeReparented(const ModelNode &node, const NodeAbstractProperty &newPropertyParent,
                                const NodeAbstractProperty &oldPropertyParent, PropertyChangeFlags propertyChange);
    virtual void nodeIdChanged(const ModelNode &node, const QString &newId, const QString &oldId);
    virtual void propertiesAboutToBeRemoved(const QList<AbstractProperty> &propertyList);
    virtual void propertiesRemoved(const QList<AbstractProperty> &propertyList);
    virtual void variantPropertiesChanged(const QList<VariantProperty> &propertyList,
                                          PropertyChangeFlags propertyChange);
    virtual void bindingPropertiesAboutToBeChanged(const QList<BindingProperty> &propertyList);
    virtual void bindingPropertiesChanged(const QList<BindingProperty> &propertyList,
                                          PropertyChangeFlags propertyChange);
    virtual void signalHandlerPropertiesChanged(const QVector<SignalHandlerProperty> &propertyList,
                                                PropertyChangeFlags propertyChange);
    virtual void signalDeclarationPropertiesChanged(const QVector<SignalDeclarationProperty> &propertyList,
                                                    PropertyChangeFlags propertyChange);
    virtual void rootNodeTypeChanged(const QString &type, int majorVersion, int minorVersion);
    virtual void nodeTypeChanged(const ModelNode &node, const TypeName &type, int majorVersion, int minorVersion);

    virtual void instancePropertyChanged(const QList<QPair<ModelNode, PropertyName> > &propertyList);
    virtual void instanceErrorChanged(const QVector<ModelNode> &errorNodeList);
    virtual void instancesCompleted(const QVector<ModelNode> &completedNodeList);
    virtual void instanceInformationsChanged(const QMultiHash<ModelNode, InformationName> &informationChangeHash);
    virtual void instancesRenderImageChanged(const QVector<ModelNode> &nodeList);
    virtual void instancesPreviewImageChanged(const QVector<ModelNode> &nodeList);
    virtual void instancesChildrenChanged(const QVector<ModelNode> &nodeList);
    virtual void instancesToken(const QString &tokenName, int tokenNumber, const QVector<ModelNode> &nodeVector);

    virtual void nodeSourceChanged(const ModelNode &modelNode, const QString &newNodeSource);

    virtual void rewriterBeginTransaction();
    virtual void rewriterEndTransaction();

    virtual void currentStateChanged(const ModelNode &node); // base state is a invalid model node

    virtual void selectedNodesChanged(const QList<ModelNode> &selectedNodeList,
                                      const QList<ModelNode> &lastSelectedNodeList);

    virtual void fileUrlChanged(const QUrl &oldUrl, const QUrl &newUrl);

    virtual void nodeOrderChanged(const NodeListProperty &listProperty);
    virtual void nodeOrderChanged(const NodeListProperty &listProperty,
                                  const ModelNode &movedNode,
                                  int oldIndex);

    virtual void importsChanged(const Imports &addedImports, const Imports &removedImports);
    virtual void possibleImportsChanged(const Imports &possibleImports);
    virtual void usedImportsChanged(const Imports &usedImports);

    virtual void auxiliaryDataChanged(const ModelNode &node,
                                      AuxiliaryDataKeyView type,
                                      const QVariant &data);

    virtual void customNotification(const AbstractView *view, const QString &identifier, const QList<ModelNode> &nodeList, const QList<QVariant> &data);

    virtual void scriptFunctionsChanged(const ModelNode &node, const QStringList &scriptFunctionList);

    virtual void documentMessagesChanged(const QList<DocumentMessage> &errors, const QList<DocumentMessage> &warnings);

    virtual void currentTimelineChanged(const ModelNode &node);

    virtual void renderImage3DChanged(const QImage &image);
    virtual void updateActiveScene3D(const QVariantMap &sceneState);
    virtual void updateImport3DSupport(const QVariantMap &supportMap);
    virtual void nodeAtPosReady(const ModelNode &modelNode, const QVector3D &pos3d);
    virtual void modelNodePreviewPixmapChanged(const ModelNode &node, const QPixmap &pixmap);

    virtual void view3DAction(View3DActionType type, const QVariant &value);

    virtual void active3DSceneChanged(qint32 sceneId);

    virtual void dragStarted(QMimeData *mimeData);
    virtual void dragEnded();

    void changeRootNodeType(const TypeName &type, int majorVersion, int minorVersion);

    void ensureMaterialLibraryNode();
    ModelNode materialLibraryNode();
    bool isPartOfMaterialLibrary(const ModelNode &node);
    ModelNode active3DSceneNode();
    ModelNode getTextureDefaultInstance(const QString &source);

    const NodeInstanceView *nodeInstanceView() const;
    RewriterView *rewriterView() const;

    void setCurrentStateNode(const ModelNode &node);
    ModelNode currentStateNode() const;
    QmlModelState currentState() const;
    QmlTimeline currentTimeline() const;

    int majorQtQuickVersion() const;
    int minorQtQuickVersion() const;

    void resetView();
    void resetPuppet();

    virtual bool hasWidget() const;
    virtual WidgetInfo widgetInfo();
    virtual void disableWidget();
    virtual void enableWidget();

    QString contextHelpId() const;

    void setCurrentTimeline(const ModelNode &timeline);
    void activateTimelineRecording(const ModelNode &timeline);
    void deactivateTimelineRecording();

    using OperationBlock = std::function<void()>;
    bool executeInTransaction(const QByteArray &identifier, const OperationBlock &lambda);

    bool isEnabled() const;
    void setEnabled(bool b);

    ExternalDependenciesInterface &externalDependencies() const { return m_externalDependencies; }

    bool isBlockingNotifications() const { return m_isBlockingNotifications; }

    class NotificationBlocker
    {
    public:
        NotificationBlocker(AbstractView *view)
            : m_view{view}
        {
            m_view->m_isBlockingNotifications = true;
        }

        ~NotificationBlocker() { m_view->m_isBlockingNotifications = false; }

    private:
        AbstractView *m_view;
    };

protected:
    void setModel(Model *model);
    void removeModel();
    static WidgetInfo createWidgetInfo(
        QWidget *widget = nullptr,
        const QString &uniqueId = QString(),
        WidgetInfo::PlacementHint placementHint = WidgetInfo::NoPane,
        int placementPriority = 0,
        const QString &tabName = QString(),
        const QString &feedbackDisplayName = QString(),
        DesignerWidgetFlags widgetFlags = DesignerWidgetFlags::DisableOnError);

private:
    QList<ModelNode> toModelNodeList(const QList<Internal::InternalNodePointer> &nodeList) const;

    QPointer<Model> m_model;
    ExternalDependenciesInterface &m_externalDependencies;
    bool m_enabled = true;
    bool m_isBlockingNotifications = false;
};

QMLDESIGNERCORE_EXPORT QList<Internal::InternalNodePointer> toInternalNodeList(const QList<ModelNode> &nodeList);
QMLDESIGNERCORE_EXPORT QList<ModelNode> toModelNodeList(
    const QList<Internal::InternalNodePointer> &nodeList, Model *model, AbstractView *view);
} // namespace QmlDesigner
