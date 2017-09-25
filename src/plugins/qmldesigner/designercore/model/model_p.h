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

#include <QList>
#include <QPointer>
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
class InternalSignalHandlerProperty;
class InternalVariantProperty;
class InternalNodeAbstractProperty;
class InternalNodeListProperty;

typedef QSharedPointer<InternalNode> InternalNodePointer;
typedef QSharedPointer<InternalProperty> InternalPropertyPointer;
typedef QSharedPointer<InternalBindingProperty> InternalBindingPropertyPointer;
typedef QSharedPointer<InternalSignalHandlerProperty> InternalSignalHandlerPropertyPointer;
typedef QSharedPointer<InternalVariantProperty> InternalVariantPropertyPointer;
typedef QSharedPointer<InternalNodeAbstractProperty> InternalNodeAbstractPropertyPointer;
typedef QSharedPointer<InternalNodeListProperty> InternalNodeListPropertyPointer;
typedef QPair<InternalNodePointer, PropertyName> PropertyPair;


class ModelPrivate;

class WriteLocker
{
public:
    ~WriteLocker();
    WriteLocker(ModelPrivate *model);
    WriteLocker(Model *model);
private: // variables
    QPointer<ModelPrivate> m_model;
};

class ModelPrivate : public QObject {
    Q_OBJECT

    Q_DISABLE_COPY(ModelPrivate)

    friend class QmlDesigner::Model;
    friend class QmlDesigner::Internal::WriteLocker;

public:
     ModelPrivate(Model *model);
    ~ModelPrivate();

    static Model *create(const TypeName &type, int major, int minor, Model *metaInfoPropxyModel);

    QUrl fileUrl() const;
    void setFileUrl(const QUrl &url);

    InternalNodePointer createNode(const TypeName &typeName,
                                     int majorVersion,
                                     int minorVersion,
                                     const QList<QPair<PropertyName, QVariant> > &propertyList,
                                     const QList<QPair<PropertyName, QVariant> > &auxPropertyList,
                                     const QString &nodeSource,
                                     ModelNode::NodeSourceType nodeSourceType,
                                     bool isRootNode = false);


    /*factory methods for internal use in model and rewriter*/

    void removeNode(const InternalNodePointer &node);
    void changeNodeId(const InternalNodePointer& internalNodePointer, const QString& id);
    void changeNodeType(const InternalNodePointer& internalNodePointer, const TypeName &typeName, int majorVersion, int minorVersion);

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
    void notifyNodeAboutToBeReparent(const InternalNodePointer &internalNodePointer, const InternalNodeAbstractPropertyPointer &newPropertyParent, const InternalNodePointer &oldParent, const PropertyName &oldPropertyName, AbstractView::PropertyChangeFlags propertyChange);
    void notifyNodeReparent(const InternalNodePointer &internalNodePointer, const InternalNodeAbstractPropertyPointer &newPropertyParent, const InternalNodePointer &oldParent, const PropertyName &oldPropertyName, AbstractView::PropertyChangeFlags propertyChange);
    void notifyNodeAboutToBeRemoved(const InternalNodePointer &internalNodePointer);
    void notifyNodeRemoved(const InternalNodePointer &internalNodePointer, const InternalNodePointer &parentNodePointer, const PropertyName &parentPropertyName, AbstractView::PropertyChangeFlags propertyChange);
    void notifyNodeIdChanged(const InternalNodePointer& internalNodePointer, const QString& newId, const QString& oldId);
    void notifyNodeTypeChanged(const InternalNodePointer& internalNodePointer, const TypeName &type, int majorVersion, int minorVersion);

    void notifyPropertiesRemoved(const QList<PropertyPair> &propertyList);
    void notifyPropertiesAboutToBeRemoved(const QList<InternalPropertyPointer> &internalPropertyList);
    void notifyBindingPropertiesChanged(const QList<InternalBindingPropertyPointer> &internalPropertyList, AbstractView::PropertyChangeFlags propertyChange);
    void notifySignalHandlerPropertiesChanged(const QVector<InternalSignalHandlerPropertyPointer> &propertyList, AbstractView::PropertyChangeFlags propertyChange);
    void notifyVariantPropertiesChanged(const InternalNodePointer &internalNodePointer, const PropertyNameList &propertyNameList, AbstractView::PropertyChangeFlags propertyChange);
    void notifyScriptFunctionsChanged(const InternalNodePointer &internalNodePointer, const QStringList &scriptFunctionList);

    void notifyNodeOrderChanged(const InternalNodeListPropertyPointer &internalListPropertyPointer, const InternalNodePointer &internalNodePointer, int oldIndex);
    void notifyAuxiliaryDataChanged(const InternalNodePointer &internalNode, const PropertyName &name, const QVariant &data);
    void notifyNodeSourceChanged(const InternalNodePointer &internalNode, const QString &newNodeSource);

    void notifyRootNodeTypeChanged(const QString &type, int majorVersion, int minorVersion);

    void notifyCustomNotification(const AbstractView *senderView, const QString &identifier, const QList<ModelNode> &nodeList, const QList<QVariant> &data);
    void notifyInstancePropertyChange(const QList<QPair<ModelNode, PropertyName> > &propertyList);
    void notifyInstanceErrorChange(const QVector<qint32> &instanceIds);
    void notifyInstancesCompleted(const QVector<ModelNode> &nodeList);
    void notifyInstancesInformationsChange(const QMultiHash<ModelNode, InformationName> &informationChangeHash);
    void notifyInstancesRenderImageChanged(const QVector<ModelNode> &nodeList);
    void notifyInstancesPreviewImageChanged(const QVector<ModelNode> &nodeList);
    void notifyInstancesChildrenChanged(const QVector<ModelNode> &nodeList);
    void notifyInstanceToken(const QString &token, int number, const QVector<ModelNode> &nodeVector);

    void notifyCurrentStateChanged(const ModelNode &node);
    void notifyCurrentTimelineChanged(const ModelNode &node);

    void setDocumentMessages(const QList<DocumentMessage> &errors, const QList<DocumentMessage> &warnings);

    void notifyRewriterBeginTransaction();
    void notifyRewriterEndTransaction();

    void setSelectedNodes(const QList<InternalNodePointer> &selectedNodeList);
    void clearSelectedNodes();
    QList<InternalNodePointer> selectedNodes() const;
    void selectNode(const InternalNodePointer &internalNodePointer);
    void deselectNode(const InternalNodePointer &internalNodePointer);
    void changeSelectedNodes(const QList<InternalNodePointer> &newSelectedsNodeList,
                             const QList<InternalNodePointer> &oldSelectedsNodeList);

    void setAuxiliaryData(const InternalNodePointer& node, const PropertyName &name, const QVariant &data);
    void removeAuxiliaryData(const InternalNodePointer& node, const PropertyName &name);
    void resetModelByRewriter(const QString &description);


    // Imports:
    QList<Import> imports() const { return m_imports; }
    void addImport(const Import &import);
    void removeImport(const Import &import);
    void changeImports(const QList<Import> &importsToBeAdded, const QList<Import> &importToBeRemoved);
    void notifyImportsChanged(const QList<Import> &addedImports, const QList<Import> &removedImports);


    //node state property manipulation

    void addProperty(const InternalNodePointer &node, const PropertyName &name);
    void setPropertyValue(const InternalNodePointer &node,const PropertyName &name, const QVariant &value);
    void removeProperty(const InternalPropertyPointer &property);

    void setBindingProperty(const InternalNodePointer &internalNodePointer, const PropertyName &name, const QString &expression);
    void setSignalHandlerProperty(const InternalNodePointer &internalNodePointer, const PropertyName &name, const QString &source);
    void setVariantProperty(const InternalNodePointer &internalNodePointer, const PropertyName &name, const QVariant &value);
    void setDynamicVariantProperty(const InternalNodePointer &internalNodePointer, const PropertyName &name, const TypeName &propertyType, const QVariant &value);
    void setDynamicBindingProperty(const InternalNodePointer &internalNodePointer, const PropertyName &name, const TypeName &dynamicPropertyType, const QString &expression);
    void reparentNode(const InternalNodePointer &internalNodePointer,
                      const PropertyName &name,
                      const InternalNodePointer &internalNodeToBeAppended,
                      bool list = true, const TypeName &dynamicTypeName = TypeName());
    void changeNodeOrder(const InternalNodePointer &internalParentNode, const PropertyName &listPropertyName, int from, int to);
    void checkPropertyName(const PropertyName &propertyName);
    void clearParent(const InternalNodePointer &internalNodePointer);
    void changeRootNodeType(const TypeName &type, int majorVersion, int minorVersion);
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

    InternalNodePointer currentStateNode() const;
    InternalNodePointer currentTimelineNode() const;

private: //functions
    void removePropertyWithoutNotification(const InternalPropertyPointer &property);
    void removeAllSubNodes(const InternalNodePointer &internalNodePointer);
    void removeNodeFromModel(const InternalNodePointer &internalNodePointer);
    QList<InternalNodePointer> toInternalNodeList(const QList<ModelNode> &internalNodeList) const;
    QList<ModelNode> toModelNodeList(const QList<InternalNodePointer> &internalNodeList, AbstractView *view) const;
    QVector<ModelNode> toModelNodeVector(const QVector<InternalNodePointer> &internalNodeVector, AbstractView *view) const;
    QVector<InternalNodePointer> toInternalNodeVector(const QVector<ModelNode> &internalNodeVector) const;

private:
    Model *m_q;
    MetaInfo m_metaInfo;
    QList<Import> m_imports;
    QList<Import> m_possibleImportList;
    QList<Import> m_usedImportList;
    QList<QPointer<AbstractView> > m_viewList;
    QList<InternalNodePointer> m_selectedInternalNodeList;
    QHash<QString,InternalNodePointer> m_idNodeHash;
    QHash<qint32, InternalNodePointer> m_internalIdNodeHash;
    QSet<InternalNodePointer> m_nodeSet;
    InternalNodePointer m_currentStateNode;
    InternalNodePointer m_rootInternalNode;
    InternalNodePointer m_currentTimelineMutatorNode;
    QUrl m_fileUrl;
    QPointer<RewriterView> m_rewriterView;
    QPointer<NodeInstanceView> m_nodeInstanceView;
    QPointer<TextModifier> m_textModifier;
    QPointer<Model> m_metaInfoProxyModel;
    bool m_writeLock;
    qint32 m_internalIdCounter;
};

}
}
