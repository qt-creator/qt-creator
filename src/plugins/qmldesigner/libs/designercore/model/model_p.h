// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmldesignercorelib_global.h"

#include <tracing/qmldesignertracing.h>

#include "abstractview.h"
#ifndef QDS_USE_PROJECTSTORAGE
#  include "metainfo.h"
#endif
#include "modelnode.h"

#include <nodemetainfo.h>

#include <utils/uniqueobjectptr.h>

#include <QDrag>
#include <QList>
#include <QPointer>
#include <QSet>
#include <QUrl>
#include <QVector3D>

#include <algorithm>
#include <functional>

QT_BEGIN_NAMESPACE
class QPlainTextEdit;
class QPixmap;
QT_END_NAMESPACE

namespace QmlDesigner {

class AbstractProperty;
class RewriterView;
class NodeInstanceView;
class NodeMetaInfoPrivate;

namespace Internal {

using namespace NanotraceHR::Literals;

class InternalNode;
class InternalProperty;
class InternalBindingProperty;
class InternalSignalHandlerProperty;
class InternalSignalDeclarationProperty;
class InternalVariantProperty;
class InternalNodeAbstractProperty;
class InternalNodeListProperty;

using PropertyPair = std::pair<InternalNodePointer, Utils::SmallString>;

class ModelPrivate;

class WriteLocker
{
public:
    WriteLocker(Model *model);
    WriteLocker(ModelPrivate *model);
    ~WriteLocker();

    static void unlock(Model *model);
    static void lock(Model *model);

private:
    QPointer<ModelPrivate> m_model;
};

class ModelPrivate : public QObject,
                     protected ProjectStorageObserver
{
    Q_OBJECT

    friend Model;
    friend Internal::WriteLocker;

public:
    ModelPrivate(Model *model,
                 ProjectStorageDependencies m_projectStorageDependencies,
                 const TypeName &type,
                 int major,
                 int minor,
                 Model *metaInfoProxyModel,
                 std::unique_ptr<ModelResourceManagementInterface> resourceManagement);
    ModelPrivate(Model *model,
                 ProjectStorageDependencies m_projectStorageDependencies,
                 Utils::SmallStringView typeName,
                 Imports imports,
                 const QUrl &filePath,
                 std::unique_ptr<ModelResourceManagementInterface> resourceManagement);
    ModelPrivate(Model *model,
                 const TypeName &type,
                 int major,
                 int minor,
                 Model *metaInfoProxyModel,
                 std::unique_ptr<ModelResourceManagementInterface> resourceManagement);

    ~ModelPrivate() override;

    ModelPrivate(const ModelPrivate &) = delete;
    ModelPrivate &operator=(const ModelPrivate &) = delete;

    const QUrl &fileUrl() const;
    void setFileUrl(const QUrl &url);

    InternalNodePointer createNode(TypeNameView typeName,
                                   int majorVersion,
                                   int minorVersion,
                                   const QList<QPair<PropertyName, QVariant>> &propertyList,
                                   const AuxiliaryDatas &auxPropertyList,
                                   const QString &nodeSource,
                                   ModelNode::NodeSourceType nodeSourceType,
                                   const QString &behaviorPropertyName,
                                   bool isRootNode = false);

    /*factory methods for internal use in model and rewriter*/
    void removeNodeAndRelatedResources(const InternalNodePointer &node);
    void changeNodeId(const InternalNodePointer &node, const QString &id);
    void changeNodeType(const InternalNodePointer &node, const TypeName &typeName, int majorVersion, int minorVersion);

    InternalNodePointer rootNode() const;
    InternalNodePointer findNode(const QString &id) const;

#ifndef QDS_USE_PROJECTSTORAGE
    MetaInfo metaInfo() const;
    void setMetaInfo(const MetaInfo &metaInfo);
#endif

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
                                     const InternalNodePointer &newParent,
                                     PropertyNameView newPropertyName,
                                     const InternalNodePointer &oldParent,
                                     PropertyNameView oldPropertyName,
                                     AbstractView::PropertyChangeFlags propertyChange);
    void notifyNodeReparent(const InternalNodePointer &node,
                            const InternalNodeAbstractProperty *newPropertyParent,
                            const InternalNodePointer &oldParent,
                            PropertyNameView oldPropertyName,
                            AbstractView::PropertyChangeFlags propertyChange);
    void notifyNodeAboutToBeRemoved(const InternalNodePointer &node);
    void notifyNodeRemoved(const InternalNodePointer &removedNode,
                           const InternalNodePointer &parentNode,
                           PropertyNameView parentPropertyName,
                           AbstractView::PropertyChangeFlags propertyChange);
    void notifyNodeIdChanged(const InternalNodePointer &node, const QString &newId, const QString &oldId);
    void notifyNodeTypeChanged(const InternalNodePointer &node, const TypeName &type, int majorVersion, int minorVersion);

    void notifyPropertiesRemoved(const QList<PropertyPair> &propertyList);
    void notifyPropertiesAboutToBeRemoved(const QList<InternalProperty *> &internalPropertyList);
    void notifyBindingPropertiesAboutToBeChanged(
        const QList<QmlDesigner::Internal::InternalBindingProperty *> &internalPropertyList);
    void notifyBindingPropertiesChanged(
        const QList<QmlDesigner::Internal::InternalBindingProperty *> &internalPropertyList,
        AbstractView::PropertyChangeFlags propertyChange);
    void notifySignalHandlerPropertiesChanged(
        const QVector<QmlDesigner::Internal::InternalSignalHandlerProperty *> &propertyList,
        AbstractView::PropertyChangeFlags propertyChange);
    void notifySignalDeclarationPropertiesChanged(
        const QVector<QmlDesigner::Internal::InternalSignalDeclarationProperty *> &propertyList,
        AbstractView::PropertyChangeFlags propertyChange);
    void notifyVariantPropertiesChanged(const InternalNodePointer &node,
                                        const PropertyNameViews &propertyNameList,
                                        AbstractView::PropertyChangeFlags propertyChange);
    void notifyScriptFunctionsChanged(const InternalNodePointer &node, const QStringList &scriptFunctionList);

    void notifyNodeOrderChanged(const QmlDesigner::Internal::InternalNodeListProperty *internalListProperty,
                                const InternalNodePointer &node,
                                int oldIndex);
    void notifyNodeOrderChanged(const InternalNodeListProperty *internalListProperty);
    void notifyAuxiliaryDataChanged(const InternalNodePointer &node,
                                    AuxiliaryDataKeyView key,
                                    const QVariant &data);
    void notifyNodeSourceChanged(const InternalNodePointer &node, const QString &newNodeSource);

    void notifyRootNodeTypeChanged(const QString &type, int majorVersion, int minorVersion);

    void notifyCustomNotification(const AbstractView *senderView,
                                  const QString &identifier,
                                  const QList<ModelNode> &modelNodeList,
                                  const QList<QVariant> &data);
    void notifyCustomNotificationTo(AbstractView *view, const CustomNotificationPackage &package);
    void notifyInstancePropertyChange(const QList<QPair<ModelNode, PropertyName>> &propertyList);
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
    void notifyModelNodePreviewPixmapChanged(const ModelNode &node,
                                             const QPixmap &pixmap,
                                             const QByteArray &requestId);
    void notifyImport3DSupportChanged(const QVariantMap &supportMap);
    void notifyNodeAtPosResult(const ModelNode &modelNode, const QVector3D &pos3d);
    void notifyView3DAction(View3DActionType type, const QVariant &value);

    void notifyActive3DSceneIdChanged(qint32 sceneId);

    void notifyDragStarted(QMimeData *mimeData);
    void notifyDragEnded();

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

    void setAuxiliaryData(const InternalNodePointer &node,
                          const AuxiliaryDataKeyView &key,
                          const QVariant &data);
    void removeAuxiliaryData(const InternalNodePointer &node, const AuxiliaryDataKeyView &key);
    void resetModelByRewriter(const QString &description);

    // Imports:
    const Imports &imports() const { return m_imports; }
    void changeImports(Imports importsToBeAdded, Imports importToBeRemoved);
    void notifyImportsChanged(const Imports &addedImports, const Imports &removedImports);
    void notifyPossibleImportsChanged(const Imports &possibleImports);
    void notifyUsedImportsChanged(const Imports &usedImportsChanged);

    //node state property manipulation
    void addProperty(const InternalNodePointer &node, PropertyNameView name);
    void setPropertyValue(const InternalNodePointer &node, PropertyNameView name, const QVariant &value);
    void removePropertyAndRelatedResources(InternalProperty *property);
    void removeProperty(InternalProperty *property);
    void removeProperties(const QList<InternalProperty *> &properties);

    void setBindingProperty(const InternalNodePointer &node,
                            PropertyNameView name,
                            const QString &expression);
    void setBindingProperties(const ModelResourceSet::SetExpressions &setExpressions);
    void setSignalHandlerProperty(const InternalNodePointer &node,
                                  PropertyNameView name,
                                  const QString &source);
    void setSignalDeclarationProperty(const InternalNodePointer &node,
                                      PropertyNameView name,
                                      const QString &signature);
    void setVariantProperty(const InternalNodePointer &node,
                            PropertyNameView name,
                            const QVariant &value);
    void setDynamicVariantProperty(const InternalNodePointer &node,
                                   PropertyNameView name,
                                   const TypeName &propertyType,
                                   const QVariant &value);
    void setDynamicBindingProperty(const InternalNodePointer &node,
                                   PropertyNameView name,
                                   const TypeName &dynamicPropertyType,
                                   const QString &expression);
    void reparentNode(const InternalNodePointer &parentNode,
                      PropertyNameView name,
                      const InternalNodePointer &childNode,
                      bool list = true,
                      const TypeName &dynamicTypeName = TypeName());
    void changeNodeOrder(const InternalNodePointer &parentNode,
                         PropertyNameView listPropertyName,
                         int from,
                         int to);
    static bool propertyNameIsValid(PropertyNameView propertyName);
    void clearParent(const InternalNodePointer &node);
    void changeRootNodeType(const TypeName &type, int majorVersion, int minorVersion);
    void setScriptFunctions(const InternalNodePointer &node, const QStringList &scriptFunctionList);
    void setNodeSource(const InternalNodePointer &node, const QString &nodeSource);

    InternalNodePointer nodeForId(const QString &id) const;
    bool hasId(const QString &id) const;

    InternalNodePointer nodeForInternalId(qint32 internalId) const;
    bool hasNodeForInternalId(qint32 internalId) const;

    std::vector<InternalNodePointer> allNodesUnordered() const;
    QList<InternalNodePointer> allNodesOrdered() const;

    bool isWriteLocked() const;

    WriteLocker createWriteLocker() const;

    void setRewriterView(RewriterView *rewriterView);
    RewriterView *rewriterView() const;

    void setNodeInstanceView(AbstractView *nodeInstanceView);
    AbstractView *nodeInstanceView() const;

    InternalNodePointer currentStateNode() const;
    InternalNodePointer currentTimelineNode() const;

    void handleResourceSet(const ModelResourceSet &resourceSet);

    QHash<TypeName, std::shared_ptr<NodeMetaInfoPrivate>> &nodeMetaInfoCache()
    {
        return m_nodeMetaInfoCache;
    }

    void updateModelNodeTypeIds(const TypeIds &removedTypeIds);

protected:
    void removedTypeIds(const TypeIds &removedTypeIds) override;
    void exportedTypesChanged() override;
    void removeNode(const InternalNodePointer &node);

private:
    void removePropertyWithoutNotification(InternalProperty *property);
    void removeAllSubNodes(const InternalNodePointer &node);
    void removeNodeFromModel(const InternalNodePointer &node);
    QList<InternalNodePointer> toInternalNodeList(const QList<ModelNode> &modelNodeList) const;
    QList<ModelNode> toModelNodeList(const QList<InternalNodePointer> &nodeList, AbstractView *view) const;
    QVector<ModelNode> toModelNodeVector(const QVector<InternalNodePointer> &nodeVector, AbstractView *view) const;
    QVector<InternalNodePointer> toInternalNodeVector(const QVector<ModelNode> &modelNodeVector) const;
    static QList<InternalProperty *> toInternalProperties(const AbstractProperties &properties);
    static QList<std::tuple<QmlDesigner::Internal::InternalBindingProperty *, QString>>
    toInternalBindingProperties(const ModelResourceSet::SetExpressions &setExpressions);
    ImportedTypeNameId importedTypeNameId(Utils::SmallStringView typeName);
    void setTypeId(InternalNode *node, Utils::SmallStringView typeName);
    void refreshTypeId(InternalNode *node);

public:
    NotNullPointer<ProjectStorageType> projectStorage = nullptr;
    NotNullPointer<PathCacheType> pathCache = nullptr;
    ModelTracing::AsynchronousToken traceToken = ModelTracing::category().beginAsynchronous("Model"_t);

private:
    Model *m_model = nullptr;
#ifndef QDS_USE_PROJECTSTORAGE
    MetaInfo m_metaInfo;
#endif
    Imports m_imports;
    Imports m_possibleImportList;
    Imports m_usedImportList;
    QList<QPointer<AbstractView>> m_viewList;
    QList<InternalNodePointer> m_selectedInternalNodeList;
    QHash<QString,InternalNodePointer> m_idNodeHash;
    QHash<qint32, InternalNodePointer> m_internalIdNodeHash;
    std::vector<InternalNodePointer> m_nodes;
    InternalNodePointer m_currentStateNode;
    InternalNodePointer m_rootInternalNode;
    InternalNodePointer m_currentTimelineNode;
    std::unique_ptr<ModelResourceManagementInterface> m_resourceManagement;
    QUrl m_fileUrl;
    Utils::PathString m_localPath;
    SourceId m_sourceId;
    QPointer<RewriterView> m_rewriterView;
    QPointer<AbstractView> m_nodeInstanceView;
    QPointer<Model> m_metaInfoProxyModel;
    QHash<TypeName, std::shared_ptr<NodeMetaInfoPrivate>> m_nodeMetaInfoCache;
    Utils::UniqueObjectPtr<QDrag> drag;
    bool m_writeLock = false;
    qint32 m_internalIdCounter = 1;
};

} // namespace Internal
} // namespace QmlDesigner
