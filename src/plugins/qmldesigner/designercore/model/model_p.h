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
class QPixmap;
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

using InternalNodePointer = QSharedPointer<InternalNode>;
using InternalPropertyPointer = QSharedPointer<InternalProperty>;
using InternalBindingPropertyPointer = QSharedPointer<InternalBindingProperty>;
using InternalSignalHandlerPropertyPointer = QSharedPointer<InternalSignalHandlerProperty>;
using InternalVariantPropertyPointer = QSharedPointer<InternalVariantProperty>;
using InternalNodeAbstractPropertyPointer = QSharedPointer<InternalNodeAbstractProperty>;
using InternalNodeListPropertyPointer = QSharedPointer<InternalNodeListProperty>;
using PropertyPair = QPair<InternalNodePointer, PropertyName>;

class ModelPrivate;

class WriteLocker
{
public:
    WriteLocker(Model *model);
    WriteLocker(ModelPrivate *model);
    ~WriteLocker();

private:
    QPointer<ModelPrivate> m_model;
};

class ModelPrivate : public QObject {
    Q_OBJECT
    Q_DISABLE_COPY(ModelPrivate)

    friend class QmlDesigner::Model;
    friend class QmlDesigner::Internal::WriteLocker;
    friend class QmlDesigner::Internal::NodeMetaInfoPrivate;

public:
     ModelPrivate(Model *model);
    ~ModelPrivate() override;

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
    void changeNodeId(const InternalNodePointer &node, const QString &id);
    void changeNodeType(const InternalNodePointer &node, const TypeName &typeName, int majorVersion, int minorVersion);

    InternalNodePointer rootNode() const;
    InternalNodePointer findNode(const QString &id) const;

    MetaInfo metaInfo() const;
    void setMetaInfo(const MetaInfo &metaInfo);

    void attachView(AbstractView *view);
    void detachView(AbstractView *view, bool notifyView);
    void detachAllViews();

    template<typename Callable>
    void notifyNodeInstanceViewLast(Callable call);
    template<typename Callable>
    void notifyNormalViewsLast(Callable call);
    template<typename Callable>
    void notifyInstanceChanges(Callable call);

    void notifyNodeCreated(const InternalNodePointer &newNode);
    void notifyNodeAboutToBeReparent(const InternalNodePointer &node,
                                     const InternalNodeAbstractPropertyPointer &newPropertyParent,
                                     const InternalNodePointer &oldParent,
                                     const PropertyName &oldPropertyName,
                                     AbstractView::PropertyChangeFlags propertyChange);
    void notifyNodeReparent(const InternalNodePointer &node,
                            const InternalNodeAbstractPropertyPointer &newPropertyParent,
                            const InternalNodePointer &oldParent,
                            const PropertyName &oldPropertyName,
                            AbstractView::PropertyChangeFlags propertyChange);
    void notifyNodeAboutToBeRemoved(const InternalNodePointer &node);
    void notifyNodeRemoved(const InternalNodePointer &removedNode,
                           const InternalNodePointer &parentNode,
                           const PropertyName &parentPropertyName,
                           AbstractView::PropertyChangeFlags propertyChange);
    void notifyNodeIdChanged(const InternalNodePointer &node, const QString &newId, const QString &oldId);
    void notifyNodeTypeChanged(const InternalNodePointer &node, const TypeName &type, int majorVersion, int minorVersion);

    void notifyPropertiesRemoved(const QList<PropertyPair> &propertyList);
    void notifyPropertiesAboutToBeRemoved(const QList<InternalPropertyPointer> &internalPropertyList);
    void notifyBindingPropertiesAboutToBeChanged(
        const QList<InternalBindingPropertyPointer> &internalPropertyList);
    void notifyBindingPropertiesChanged(const QList<InternalBindingPropertyPointer> &internalPropertyList, AbstractView::PropertyChangeFlags propertyChange);
    void notifySignalHandlerPropertiesChanged(const QVector<InternalSignalHandlerPropertyPointer> &propertyList, AbstractView::PropertyChangeFlags propertyChange);
    void notifyVariantPropertiesChanged(const InternalNodePointer &node, const PropertyNameList &propertyNameList, AbstractView::PropertyChangeFlags propertyChange);
    void notifyScriptFunctionsChanged(const InternalNodePointer &node, const QStringList &scriptFunctionList);

    void notifyNodeOrderChanged(const InternalNodeListPropertyPointer &internalListProperty, const InternalNodePointer &node, int oldIndex);
    void notifyAuxiliaryDataChanged(const InternalNodePointer &node, const PropertyName &name, const QVariant &data);
    void notifyNodeSourceChanged(const InternalNodePointer &node, const QString &newNodeSource);

    void notifyRootNodeTypeChanged(const QString &type, int majorVersion, int minorVersion);

    void notifyCustomNotification(const AbstractView *senderView, const QString &identifier, const QList<ModelNode> &modelNodeList, const QList<QVariant> &data);
    void notifyInstancePropertyChange(const QList<QPair<ModelNode, PropertyName> > &propertyList);
    void notifyInstanceErrorChange(const QVector<qint32> &instanceIds);
    void notifyInstancesCompleted(const QVector<ModelNode> &modelNodeVector);
    void notifyInstancesInformationsChange(const QMultiHash<ModelNode, InformationName> &informationChangeHash);
    void notifyInstancesRenderImageChanged(const QVector<ModelNode> &modelNodeVector);
    void notifyInstancesPreviewImageChanged(const QVector<ModelNode> &modelNodeVector);
    void notifyInstancesChildrenChanged(const QVector<ModelNode> &modelNodeVector);
    void notifyInstanceToken(const QString &token, int number, const QVector<ModelNode> &modelNodeVector);

    void notifyCurrentStateChanged(const ModelNode &node);
    void notifyCurrentTimelineChanged(const ModelNode &node);

    void notifyRenderImage3DChanged(const QImage &image);
    void notifyUpdateActiveScene3D(const QVariantMap &sceneState);
    void notifyModelNodePreviewPixmapChanged(const ModelNode &node, const QPixmap &pixmap);
    void notifyImport3DSupportChanged(const QVariantMap &supportMap);

    void setDocumentMessages(const QList<DocumentMessage> &errors, const QList<DocumentMessage> &warnings);

    void notifyRewriterBeginTransaction();
    void notifyRewriterEndTransaction();

    void setSelectedNodes(const QList<InternalNodePointer> &selectedNodeList);
    void clearSelectedNodes();
    QList<InternalNodePointer> selectedNodes() const;
    void selectNode(const InternalNodePointer &node);
    void deselectNode(const InternalNodePointer &node);
    void changeSelectedNodes(const QList<InternalNodePointer> &newSelectedNodeList,
                             const QList<InternalNodePointer> &oldSelectedNodeList);

    void setAuxiliaryData(const InternalNodePointer &node, const PropertyName &name, const QVariant &data);
    void removeAuxiliaryData(const InternalNodePointer &node, const PropertyName &name);
    [[noreturn]] void resetModelByRewriter(const QString &description);

    // Imports:
    const QList<Import> &imports() const { return m_imports; }
    void addImport(const Import &import);
    void removeImport(const Import &import);
    void changeImports(const QList<Import> &importsToBeAdded, const QList<Import> &importToBeRemoved);
    void notifyImportsChanged(const QList<Import> &addedImports, const QList<Import> &removedImports);
    void notifyPossibleImportsChanged(const QList<Import> &possibleImports);
    void notifyUsedImportsChanged(const QList<Import> &usedImportsChanged);

    //node state property manipulation
    void addProperty(const InternalNodePointer &node, const PropertyName &name);
    void setPropertyValue(const InternalNodePointer &node,const PropertyName &name, const QVariant &value);
    void removeProperty(const InternalPropertyPointer &property);

    void setBindingProperty(const InternalNodePointer &node, const PropertyName &name, const QString &expression);
    void setSignalHandlerProperty(const InternalNodePointer &node, const PropertyName &name, const QString &source);
    void setVariantProperty(const InternalNodePointer &node, const PropertyName &name, const QVariant &value);
    void setDynamicVariantProperty(const InternalNodePointer &node, const PropertyName &name, const TypeName &propertyType, const QVariant &value);
    void setDynamicBindingProperty(const InternalNodePointer &node, const PropertyName &name, const TypeName &dynamicPropertyType, const QString &expression);
    void reparentNode(const InternalNodePointer &parentNode, const PropertyName &name, const InternalNodePointer &childNode,
                      bool list = true, const TypeName &dynamicTypeName = TypeName());
    void changeNodeOrder(const InternalNodePointer &parentNode, const PropertyName &listPropertyName, int from, int to);
    void checkPropertyName(const PropertyName &propertyName);
    void clearParent(const InternalNodePointer &node);
    void changeRootNodeType(const TypeName &type, int majorVersion, int minorVersion);
    void setScriptFunctions(const InternalNodePointer &node, const QStringList &scriptFunctionList);
    void setNodeSource(const InternalNodePointer &node, const QString &nodeSource);

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

    void updateEnabledViews();

private:
    void removePropertyWithoutNotification(const InternalPropertyPointer &property);
    void removeAllSubNodes(const InternalNodePointer &node);
    void removeNodeFromModel(const InternalNodePointer &node);
    QList<InternalNodePointer> toInternalNodeList(const QList<ModelNode> &modelNodeList) const;
    QList<ModelNode> toModelNodeList(const QList<InternalNodePointer> &nodeList, AbstractView *view) const;
    QVector<ModelNode> toModelNodeVector(const QVector<InternalNodePointer> &nodeVector, AbstractView *view) const;
    QVector<InternalNodePointer> toInternalNodeVector(const QVector<ModelNode> &modelNodeVector) const;
    const QList<QPointer<AbstractView>> enabledViews() const;

    Model *m_model = nullptr;
    MetaInfo m_metaInfo;
    QList<Import> m_imports;
    QList<Import> m_possibleImportList;
    QList<Import> m_usedImportList;
    QList<QPointer<AbstractView>> m_viewList;
    QList<QPointer<AbstractView>> m_enabledViewList;
    QList<InternalNodePointer> m_selectedInternalNodeList;
    QHash<QString,InternalNodePointer> m_idNodeHash;
    QHash<qint32, InternalNodePointer> m_internalIdNodeHash;
    QSet<InternalNodePointer> m_nodeSet;
    InternalNodePointer m_currentStateNode;
    InternalNodePointer m_rootInternalNode;
    InternalNodePointer m_currentTimelineNode;
    QUrl m_fileUrl;
    QPointer<RewriterView> m_rewriterView;
    QPointer<NodeInstanceView> m_nodeInstanceView;
    QPointer<TextModifier> m_textModifier;
    QPointer<Model> m_metaInfoProxyModel;
    QHash<TypeName, QSharedPointer<NodeMetaInfoPrivate>> m_nodeMetaInfoCache;
    bool m_writeLock = false;
    qint32 m_internalIdCounter = 1;
};

} // namespace Internal
} // namespace QmlDesigner
