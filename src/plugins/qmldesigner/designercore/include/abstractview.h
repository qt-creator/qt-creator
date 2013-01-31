/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef ABSTRACTVIEW_H
#define ABSTRACTVIEW_H

#include <qmldesignercorelib_global.h>

#include <model.h>
#include <modelnode.h>
#include <abstractproperty.h>
#include <rewritertransaction.h>
#include <commondefines.h>

#include <QObject>

QT_BEGIN_NAMESPACE
class QStyle;
QT_END_NAMESPACE

namespace QmlDesigner {
    namespace Internal {
        class InternalNode;
        typedef QSharedPointer<InternalNode> InternalNodePointer;
        typedef QWeakPointer<InternalNode> InternalNodeWeakPointer;
    }
}

namespace QmlDesigner {

class QmlModelView;
class NodeInstanceView;
class RewriterView;

class QMLDESIGNERCORE_EXPORT AbstractView : public QObject
{
    Q_OBJECT
public:
    Q_FLAGS(PropertyChangeFlag PropertyChangeFlags)
    typedef QWeakPointer<AbstractView> Pointer;

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

    RewriterTransaction beginRewriterTransaction();

    ModelNode createModelNode(const QString &typeString,
                         int majorVersion,
                         int minorVersion,
                         const PropertyListType &propertyList = PropertyListType(),
                         const PropertyListType &auxPropertyList = PropertyListType(),
                         const QString &nodeSource = QString(),
                         ModelNode::NodeSourceType nodeSourceType = ModelNode::NodeWithoutSource);

    const ModelNode rootModelNode() const;
    ModelNode rootModelNode();

    void setSelectedModelNodes(const QList<ModelNode> &selectedNodeList);
    void selectModelNode(const ModelNode &node);
    void deselectModelNode(const ModelNode &node);
    void clearSelectedModelNodes();

    QList<ModelNode> selectedModelNodes() const;

    ModelNode modelNodeForId(const QString &id);
    bool hasId(const QString &id) const;

    ModelNode modelNodeForInternalId(qint32 internalId);
    bool hasModelNodeForInternalId(qint32 internalId) const;

    QList<ModelNode> allModelNodes() const;

    void emitCustomNotification(const QString &identifier);
    void emitCustomNotification(const QString &identifier, const QList<ModelNode> &nodeList);
    void emitCustomNotification(const QString &identifier, const QList<ModelNode> &nodeList, const QList<QVariant> &data);

    void emitInstancePropertyChange(const QList<QPair<ModelNode, QString> > &propertyList);
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

    virtual void nodeCreated(const ModelNode &createdNode) = 0;
    virtual void nodeAboutToBeRemoved(const ModelNode &removedNode) = 0;
    virtual void nodeRemoved(const ModelNode &removedNode, const NodeAbstractProperty &parentProperty, PropertyChangeFlags propertyChange) = 0;
    virtual void nodeAboutToBeReparented(const ModelNode &node, const NodeAbstractProperty &newPropertyParent, const NodeAbstractProperty &oldPropertyParent, AbstractView::PropertyChangeFlags propertyChange) = 0;
    virtual void nodeReparented(const ModelNode &node, const NodeAbstractProperty &newPropertyParent, const NodeAbstractProperty &oldPropertyParent, AbstractView::PropertyChangeFlags propertyChange) = 0;
    virtual void nodeIdChanged(const ModelNode& node, const QString& newId, const QString& oldId) = 0;
    virtual void propertiesAboutToBeRemoved(const QList<AbstractProperty>& propertyList) = 0;
    virtual void propertiesRemoved(const QList<AbstractProperty>& propertyList) = 0;
    virtual void variantPropertiesChanged(const QList<VariantProperty>& propertyList, PropertyChangeFlags propertyChange) = 0;
    virtual void bindingPropertiesChanged(const QList<BindingProperty>& propertyList, PropertyChangeFlags propertyChange) = 0;
    virtual void rootNodeTypeChanged(const QString &type, int majorVersion, int minorVersion) = 0;

    virtual void instancePropertyChange(const QList<QPair<ModelNode, QString> > &propertyList) = 0;
    virtual void instancesCompleted(const QVector<ModelNode> &completedNodeList) = 0;
    virtual void instanceInformationsChange(const QMultiHash<ModelNode, InformationName> &informationChangeHash) = 0;
    virtual void instancesRenderImageChanged(const QVector<ModelNode> &nodeList) = 0;
    virtual void instancesPreviewImageChanged(const QVector<ModelNode> &nodeList) = 0;
    virtual void instancesChildrenChanged(const QVector<ModelNode> &nodeList) = 0;
    virtual void instancesToken(const QString &tokenName, int tokenNumber, const QVector<ModelNode> &nodeVector) = 0;

    virtual void nodeSourceChanged(const ModelNode &modelNode, const QString &newNodeSource) = 0;

    virtual void rewriterBeginTransaction() = 0;
    virtual void rewriterEndTransaction() = 0;

    virtual void actualStateChanged(const ModelNode &node) = 0; // base state is a invalid model node

    virtual void selectedNodesChanged(const QList<ModelNode> &selectedNodeList,
                                      const QList<ModelNode> &lastSelectedNodeList) = 0;

    virtual void fileUrlChanged(const QUrl &oldUrl, const QUrl &newUrl);

    virtual void nodeOrderChanged(const NodeListProperty &listProperty, const ModelNode &movedNode, int oldIndex) = 0;

    virtual void importsChanged(const QList<Import> &addedImports, const QList<Import> &removedImports) = 0;

    virtual void auxiliaryDataChanged(const ModelNode &node, const QString &name, const QVariant &data);

    virtual void customNotification(const AbstractView *view, const QString &identifier, const QList<ModelNode> &nodeList, const QList<QVariant> &data);

    virtual void scriptFunctionsChanged(const ModelNode &node, const QStringList &scriptFunctionList) = 0;

    QmlModelView *toQmlModelView();

    void changeRootNodeType(const QString &type, int majorVersion, int minorVersion);

    NodeInstanceView *nodeInstanceView() const;
    RewriterView *rewriterView() const;

    void setAcutalStateNode(const ModelNode &node);
    ModelNode actualStateNode() const;

    void resetView();

    virtual QWidget *widget() = 0;

protected:
    void setModel(Model * model);
    void removeModel();

private: //functions
    QList<ModelNode> toModelNodeList(const QList<Internal::InternalNodePointer> &nodeList) const;


private:
    QWeakPointer<Model> m_model;
};

QMLDESIGNERCORE_EXPORT QList<Internal::InternalNodePointer> toInternalNodeList(const QList<ModelNode> &nodeList);
QMLDESIGNERCORE_EXPORT QList<ModelNode> toModelNodeList(const QList<Internal::InternalNodePointer> &nodeList, AbstractView *view);

}

#endif // ABSTRACTVIEW_H
