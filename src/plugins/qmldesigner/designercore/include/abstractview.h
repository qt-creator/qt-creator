/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include <qmldesignercorelib_global.h>

#include <model.h>
#include <modelnode.h>
#include <abstractproperty.h>
#include <documentmessage.h>
#include <rewritertransaction.h>
#include <commondefines.h>

#include <QObject>
#include <QPointer>

QT_BEGIN_NAMESPACE
class QStyle;
class QToolButton;
QT_END_NAMESPACE

namespace QmlDesigner {
    namespace Internal {
        class InternalNode;
        typedef QSharedPointer<InternalNode> InternalNodePointer;
        typedef QWeakPointer<InternalNode> InternalNodeWeakPointer;
    }
}

namespace QmlDesigner {

class NodeInstanceView;
class RewriterView;
class QmlModelState;

enum DesignerWidgetFlags {
    DisableOnError,
    IgnoreErrors
};

class WidgetInfo {

public:
    class ToolBarWidgetFactoryInterface {
    public:
        ToolBarWidgetFactoryInterface()
        {}

        virtual QList<QToolButton*> createToolBarWidgets() = 0;

        virtual ~ToolBarWidgetFactoryInterface()
        {}
    };

    template <class T>
    class ToolBarWidgetDefaultFactory : public ToolBarWidgetFactoryInterface {
    public:
        ToolBarWidgetDefaultFactory(T *t ) : m_t(t)
        {}

        QList<QToolButton*> createToolBarWidgets()
        {
            return m_t->createToolBarWidgets();
        }

    private:
        T * m_t;
    };

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
    QWidget *widget = nullptr;
    int placementPriority;
    PlacementHint placementHint;
    ToolBarWidgetFactoryInterface *toolBarWidgetFactory = nullptr;
    DesignerWidgetFlags widgetFlags = DesignerWidgetFlags::DisableOnError;
};

class QMLDESIGNERCORE_EXPORT AbstractView : public QObject
{
    Q_OBJECT
public:
    Q_FLAGS(PropertyChangeFlag PropertyChangeFlags)

    enum PropertyChangeFlag {
      NoAdditionalChanges = 0x0,
      PropertiesAdded = 0x1,
      EmptyPropertiesRemoved = 0x2
    };
    Q_DECLARE_FLAGS(PropertyChangeFlags, PropertyChangeFlag)
    AbstractView(QObject *parent = 0)
            : QObject(parent) {}

    virtual ~AbstractView();

    Model* model() const;
    bool isAttached() const;

    RewriterTransaction beginRewriterTransaction(const QByteArray &identifier);

    ModelNode createModelNode(const TypeName &typeName,
                         int majorVersion,
                         int minorVersion,
                         const PropertyListType &propertyList = PropertyListType(),
                         const PropertyListType &auxPropertyList = PropertyListType(),
                         const QString &nodeSource = QString(),
                         ModelNode::NodeSourceType nodeSourceType = ModelNode::NodeWithoutSource);

    const ModelNode rootModelNode() const;
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
    QString generateNewId(const QString &prefixName) const;

    ModelNode modelNodeForInternalId(qint32 internalId) const;
    bool hasModelNodeForInternalId(qint32 internalId) const;

    QList<ModelNode> allModelNodes() const;

    void emitDocumentMessage(const QList<DocumentMessage> &errors, const QList<DocumentMessage> &warnings = QList<DocumentMessage>());
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

    void sendTokenToInstances(const QString &token, int number, const QVector<ModelNode> &nodeVector);

    virtual void modelAttached(Model *model);
    virtual void modelAboutToBeDetached(Model *model);

    virtual void nodeCreated(const ModelNode &createdNode);
    virtual void nodeAboutToBeRemoved(const ModelNode &removedNode);
    virtual void nodeRemoved(const ModelNode &removedNode, const NodeAbstractProperty &parentProperty, PropertyChangeFlags propertyChange);
    virtual void nodeAboutToBeReparented(const ModelNode &node, const NodeAbstractProperty &newPropertyParent, const NodeAbstractProperty &oldPropertyParent, AbstractView::PropertyChangeFlags propertyChange);
    virtual void nodeReparented(const ModelNode &node, const NodeAbstractProperty &newPropertyParent, const NodeAbstractProperty &oldPropertyParent, AbstractView::PropertyChangeFlags propertyChange);
    virtual void nodeIdChanged(const ModelNode& node, const QString& newId, const QString& oldId);
    virtual void propertiesAboutToBeRemoved(const QList<AbstractProperty>& propertyList);
    virtual void propertiesRemoved(const QList<AbstractProperty>& propertyList);
    virtual void variantPropertiesChanged(const QList<VariantProperty>& propertyList, PropertyChangeFlags propertyChange);
    virtual void bindingPropertiesChanged(const QList<BindingProperty>& propertyList, PropertyChangeFlags propertyChange);
    virtual void signalHandlerPropertiesChanged(const QVector<SignalHandlerProperty>& propertyList, PropertyChangeFlags propertyChange);
    virtual void rootNodeTypeChanged(const QString &type, int majorVersion, int minorVersion);
    virtual void nodeTypeChanged(const ModelNode& node, const TypeName &type, int majorVersion, int minorVersion);

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

    virtual void nodeOrderChanged(const NodeListProperty &listProperty, const ModelNode &movedNode, int oldIndex);

    virtual void importsChanged(const QList<Import> &addedImports, const QList<Import> &removedImports);

    virtual void auxiliaryDataChanged(const ModelNode &node, const PropertyName &name, const QVariant &data);

    virtual void customNotification(const AbstractView *view, const QString &identifier, const QList<ModelNode> &nodeList, const QList<QVariant> &data);

    virtual void scriptFunctionsChanged(const ModelNode &node, const QStringList &scriptFunctionList);

    virtual void documentMessagesChanged(const QList<DocumentMessage> &errors, const QList<DocumentMessage> &warnings);

    void changeRootNodeType(const TypeName &type, int majorVersion, int minorVersion);

    NodeInstanceView *nodeInstanceView() const;
    RewriterView *rewriterView() const;

    void setCurrentStateNode(const ModelNode &node);
    ModelNode currentStateNode() const;
    QmlModelState currentState() const;

    int majorQtQuickVersion() const;
    int minorQtQuickVersion() const;

    void resetView();

    void resetPuppet();

    virtual bool hasWidget() const;
    virtual WidgetInfo widgetInfo();

    virtual QString contextHelpId() const;

protected:
    void setModel(Model * model);
    void removeModel();
    static WidgetInfo createWidgetInfo(QWidget *widget = 0,
                                       WidgetInfo::ToolBarWidgetFactoryInterface *toolBarWidgetFactory = 0,
                                       const QString &uniqueId = QString(),
                                       WidgetInfo::PlacementHint placementHint = WidgetInfo::NoPane,
                                       int placementPriority = 0,
                                       const QString &tabName = QString(), DesignerWidgetFlags widgetFlags = DesignerWidgetFlags::DisableOnError);

private: //functions
    QList<ModelNode> toModelNodeList(const QList<Internal::InternalNodePointer> &nodeList) const;


private:
    QPointer<Model> m_model;
};

QMLDESIGNERCORE_EXPORT QList<Internal::InternalNodePointer> toInternalNodeList(const QList<ModelNode> &nodeList);
QMLDESIGNERCORE_EXPORT QList<ModelNode> toModelNodeList(const QList<Internal::InternalNodePointer> &nodeList, AbstractView *view);

}
