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

#ifndef MODEL_P_H
#define MODEL_P_H

#include <QList>
#include <QWeakPointer>
#include <QSet>
#include <QUrl>

#include "modelnode.h"
#include "abstractview.h"
#include "metainfo.h"

#include "qmldesignercorelib_global.h"

QT_BEGIN_NAMESPACE
class QPlainTextEdit;
QT_END_NAMESPACE

namespace QmlDesigner {

class AbstractProperty;
class RewriterView;
class NodeInstanceView;

namespace Internal {

class InternalNode;
class InternalProperty;
class InternalBindingProperty;
class InternalVariantProperty;
class InternalNodeAbstractProperty;
class InternalNodeListProperty;

typedef QSharedPointer<InternalNode> InternalNodePointer;
typedef QSharedPointer<InternalProperty> InternalPropertyPointer;
typedef QSharedPointer<InternalBindingProperty> InternalBindingPropertyPointer;
typedef QSharedPointer<InternalVariantProperty> InternalVariantPropertyPointer;
typedef QSharedPointer<InternalNodeAbstractProperty> InternalNodeAbstractPropertyPointer;
typedef QSharedPointer<InternalNodeListProperty> InternalNodeListPropertyPointer;
typedef QPair<InternalNodePointer, QString> PropertyPair;


class ModelPrivate;

class WriteLocker
{
public:
    ~WriteLocker();
    WriteLocker(ModelPrivate *model);
    WriteLocker(Model *model);
private: // variables
    QWeakPointer<ModelPrivate> m_model;
};

class ModelPrivate : public QObject {
    Q_OBJECT

    Q_DISABLE_COPY(ModelPrivate)

    friend class QmlDesigner::Model;
    friend class QmlDesigner::Internal::WriteLocker;

public:
     ModelPrivate(Model *model);
    ~ModelPrivate();

    static Model *create(QString type, int major, int minor, Model *metaInfoPropxyModel);

    QUrl fileUrl() const;
    void setFileUrl(const QUrl &url);

    InternalNodePointer createNode(const QString &typeString,
                                     int majorVersion,
                                     int minorVersion,
                                     const QList<QPair<QString, QVariant> > &propertyList,
                                     const QList<QPair<QString, QVariant> > &auxPropertyList,
                                     const QString &nodeSource,
                                     ModelNode::NodeSourceType nodeSourceType,
                                     bool isRootNode = false);


    /*factory methods for internal use in model and rewriter*/

    void removeNode(const InternalNodePointer &node);
    void changeNodeId(const InternalNodePointer& internalNodePointer, const QString& id);

    InternalNodePointer rootNode() const;
    InternalNodePointer findNode(const QString &id) const;

    MetaInfo metaInfo() const;
    void setMetaInfo(const MetaInfo &metaInfo);

    void attachView(AbstractView *view);
    void detachView(AbstractView *view, bool notifyView);
    void detachAllViews();


    Model *model() const { return m_q; }
    void setModel(Model *q) { m_q = q; }

    void notifyNodeCreated(const InternalNodePointer &newInternalNodePointer);
    void notifyNodeAboutToBeReparent(const InternalNodePointer &internalNodePointer, const InternalNodeAbstractPropertyPointer &newPropertyParent, const InternalNodePointer &oldParent, const QString &oldPropertyName, AbstractView::PropertyChangeFlags propertyChange);
    void notifyNodeReparent(const InternalNodePointer &internalNodePointer, const InternalNodeAbstractPropertyPointer &newPropertyParent, const InternalNodePointer &oldParent, const QString &oldPropertyName, AbstractView::PropertyChangeFlags propertyChange);
    void notifyNodeAboutToBeRemoved(const InternalNodePointer &nodePointer);
    void notifyNodeRemoved(const InternalNodePointer &nodePointer, const InternalNodePointer &parentNodePointer, const QString &parentPropertyName, AbstractView::PropertyChangeFlags propertyChange);
    void notifyNodeIdChanged(const InternalNodePointer& nodePointer, const QString& newId, const QString& oldId);

    void notifyPropertiesRemoved(const QList<PropertyPair> &propertyList);
    void notifyPropertiesAboutToBeRemoved(const QList<InternalPropertyPointer> &propertyList);
    void notifyBindingPropertiesChanged(const QList<InternalBindingPropertyPointer> &propertyList, AbstractView::PropertyChangeFlags propertyChange);
    void notifyVariantPropertiesChanged(const InternalNodePointer &internalNodePointer, const QStringList& propertyNameList, AbstractView::PropertyChangeFlags propertyChange);
    void notifyScriptFunctionsChanged(const InternalNodePointer &internalNodePointer, const QStringList &scriptFunctionList);

    void notifyNodeOrderChanged(const InternalNodeListPropertyPointer &internalListPropertyPointer, const InternalNodePointer &internalNodePointer, int oldIndex);
    void notifyAuxiliaryDataChanged(const InternalNodePointer &internalNode, const QString &name, const QVariant &data);
    void notifyNodeSourceChanged(const InternalNodePointer &internalNode, const QString &newNodeSource);

    void notifyRootNodeTypeChanged(const QString &type, int majorVersion, int minorVersion);

    void notifyCustomNotification(const AbstractView *senderView, const QString &identifier, const QList<ModelNode> &nodeList, const QList<QVariant> &data);
    void notifyInstancePropertyChange(const QList<QPair<ModelNode, QString> > &propertyList);
    void notifyInstancesCompleted(const QVector<ModelNode> &nodeList);
    void notifyInstancesInformationsChange(const QMultiHash<ModelNode, InformationName> &informationChangeHash);
    void notifyInstancesRenderImageChanged(const QVector<ModelNode> &nodeList);
    void notifyInstancesPreviewImageChanged(const QVector<ModelNode> &nodeList);
    void notifyInstancesChildrenChanged(const QVector<ModelNode> &nodeList);
    void notifyInstanceToken(const QString &token, int number, const QVector<ModelNode> &nodeVector);

    void notifyActualStateChanged(const ModelNode &node);

    void notifyRewriterBeginTransaction();
    void notifyRewriterEndTransaction();

    void setSelectedNodes(const QList<InternalNodePointer> &selectedNodeList);
    void clearSelectedNodes();
    QList<InternalNodePointer> selectedNodes() const;
    void selectNode(const InternalNodePointer &node);
    void deselectNode(const InternalNodePointer &node);
    void changeSelectedNodes(const QList<InternalNodePointer> &newSelectedsNodeList,
                             const QList<InternalNodePointer> &oldSelectedsNodeList);

    void setAuxiliaryData(const InternalNodePointer& node, const QString &name, const QVariant &data);
    void resetModelByRewriter(const QString &description);


    // Imports:
    QList<Import> imports() const { return m_imports; }
    void addImport(const Import &import);
    void removeImport(const Import &import);
    void changeImports(const QList<Import> &importsToBeAdded, const QList<Import> &importToBeRemoved);
    void notifyImportsChanged(const QList<Import> &addedImports, const QList<Import> &removedImports);


    //node state property manipulation

    void addProperty(const InternalNodePointer &node, const QString &name);
    void setPropertyValue(const InternalNodePointer &node,const QString &name, const QVariant &value);
    void removeProperty(const InternalPropertyPointer &property);

    void setBindingProperty(const InternalNodePointer &internalNode, const QString &name, const QString &expression);
    void setVariantProperty(const InternalNodePointer &internalNode, const QString &name, const QVariant &value);
    void setDynamicVariantProperty(const InternalNodePointer &internalNode, const QString &name, const QString &propertyType, const QVariant &value);
    void setDynamicBindingProperty(const InternalNodePointer &internalNode, const QString &name, const QString &dynamicPropertyType, const QString &expression);
    void reparentNode(const InternalNodePointer &internalNode, const QString &name, const InternalNodePointer &internalNodeToBeAppended, bool list = true);
    void changeNodeOrder(const InternalNodePointer &internalParentNode, const QString &listPropertyName, int from, int to);
    void checkPropertyName(const QString &propertyName);
    void clearParent(const InternalNodePointer &internalNode);
    void changeRootNodeType(const QString &type, int majorVersion, int minorVersion);
    void setScriptFunctions(const InternalNodePointer &internalNode, const QStringList &scriptFunctionList);
    void setNodeSource(const InternalNodePointer &internalNode, const QString &nodeSource);

    InternalNodePointer nodeForId(const QString &id) const;
    bool hasId(const QString &id) const;

    InternalNodePointer nodeForInternalId(qint32 internalId) const;
    bool hasNodeForInternalId(qint32 internalId) const;

    QList<InternalNodePointer> allNodes() const;

    bool isWriteLocked() const;

    WriteLocker createWriteLocker() const;

    void setRewriterView(RewriterView *rewriterView);
    RewriterView *rewriterView() const;


    void setNodeInstanceView(NodeInstanceView *nodeInstanceView);
    NodeInstanceView *nodeInstanceView() const;

    InternalNodePointer actualStateNode() const;

private: //functions
    void removePropertyWithoutNotification(const InternalPropertyPointer &property);
    void removeAllSubNodes(const InternalNodePointer &node);
    void removeNodeFromModel(const InternalNodePointer &node);
    QList<InternalNodePointer> toInternalNodeList(const QList<ModelNode> &nodeList) const;
    QList<ModelNode> toModelNodeList(const QList<InternalNodePointer> &nodeList, AbstractView *view) const;
    QVector<ModelNode> toModelNodeVector(const QVector<InternalNodePointer> &nodeVector, AbstractView *view) const;
    QVector<InternalNodePointer> toInternalNodeVector(const QVector<ModelNode> &nodeVector) const;

private:
    Model *m_q;
    MetaInfo m_metaInfo;
    QList<Import> m_imports;
    QList<QWeakPointer<AbstractView> > m_viewList;
    QList<InternalNodePointer> m_selectedNodeList;
    QHash<QString,InternalNodePointer> m_idNodeHash;
    QHash<qint32, InternalNodePointer> m_internalIdNodeHash;
    QSet<InternalNodePointer> m_nodeSet;
    InternalNodePointer m_acutalStateNode;
    InternalNodePointer m_rootInternalNode;
    QUrl m_fileUrl;
    QWeakPointer<RewriterView> m_rewriterView;
    QWeakPointer<NodeInstanceView> m_nodeInstanceView;
    QWeakPointer<Model> m_metaInfoProxyModel;
    bool m_writeLock;
    qint32 m_internalIdCounter;
};

}
}
#endif // MODEL_P_H
