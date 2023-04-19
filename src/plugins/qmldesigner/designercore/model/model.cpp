// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "model.h"
#include "internalnode_p.h"
#include "model_p.h"
#include <modelnode.h>

#include "abstractview.h"
#include "auxiliarydataproperties.h"
#include "internalnodeabstractproperty.h"
#include "internalnodelistproperty.h"
#include "internalproperty.h"
#include "internalsignalhandlerproperty.h"
#include "metainfo.h"
#include "nodeinstanceview.h"
#include "nodemetainfo.h"

#include "abstractproperty.h"
#include "bindingproperty.h"
#include "nodeabstractproperty.h"
#include "nodelistproperty.h"
#include "rewriterview.h"
#include "rewritingexception.h"
#include "signalhandlerproperty.h"
#include "textmodifier.h"
#include "variantproperty.h"

#include <projectstorage/projectstorage.h>

#include <qmljs/qmljsmodelmanagerinterface.h>

#include <utils/algorithm.h>

#include <QDrag>
#include <QFileInfo>
#include <QHashIterator>
#include <QPointer>
#include <QRegularExpression>

/*!
\defgroup CoreModel
*/
/*!
\class QmlDesigner::Model
\ingroup CoreModel
\brief This is the facade for the abstract model data.
 All write access is running through this interface

The Model is the central place to access a qml files data (see e.g. rootNode() ) and meta data (see metaInfo() ).
Components that want to be informed about changes in the model can register a subclass of AbstractView via attachView().

\see QmlDesigner::ModelNode, QmlDesigner::AbstractProperty, QmlDesigner::AbstractView
*/

namespace QmlDesigner {
namespace Internal {

ModelPrivate::ModelPrivate(Model *model,
                           ProjectStorage<Sqlite::Database> &projectStorage,
                           const TypeName &typeName,
                           int major,
                           int minor,
                           Model *metaInfoProxyModel)
    : projectStorage{&projectStorage}
    , m_model{model}
{
    m_metaInfoProxyModel = metaInfoProxyModel;

    m_rootInternalNode = createNode(
        typeName, major, minor, {}, {}, {}, ModelNode::NodeWithoutSource, {}, true);

    m_currentStateNode = m_rootInternalNode;
    m_currentTimelineNode = m_rootInternalNode;
}

ModelPrivate::ModelPrivate(
    Model *model, const TypeName &typeName, int major, int minor, Model *metaInfoProxyModel)
    : m_model(model)
{
    m_metaInfoProxyModel = metaInfoProxyModel;

    m_rootInternalNode = createNode(
        typeName, major, minor, {}, {}, {}, ModelNode::NodeWithoutSource, {}, true);

    m_currentStateNode = m_rootInternalNode;
    m_currentTimelineNode = m_rootInternalNode;
}

ModelPrivate::~ModelPrivate() = default;

void ModelPrivate::detachAllViews()
{
    for (const QPointer<AbstractView> &view : std::as_const(m_viewList))
        detachView(view.data(), true);

    m_viewList.clear();
    updateEnabledViews();

    if (m_nodeInstanceView) {
        m_nodeInstanceView->modelAboutToBeDetached(m_model);
        m_nodeInstanceView.clear();
    }

    if (m_rewriterView) {
        m_rewriterView->modelAboutToBeDetached(m_model);
        m_rewriterView.clear();
    }
}

void ModelPrivate::changeImports(const Imports &toBeAddedImportList,
                                 const Imports &toBeRemovedImportList)
{
    Imports removedImportList;
    for (const Import &import : toBeRemovedImportList) {
        if (m_imports.contains(import)) {
            removedImportList.append(import);
            m_imports.removeOne(import);
        }
    }

    Imports addedImportList;
    for (const Import &import : toBeAddedImportList) {
        if (!m_imports.contains(import)) {
            addedImportList.append(import);
            m_imports.append(import);
        }
    }

    std::sort(m_imports.begin(), m_imports.end());

    if (!removedImportList.isEmpty() || !addedImportList.isEmpty())
        notifyImportsChanged(addedImportList, removedImportList);
}

void ModelPrivate::notifyImportsChanged(const Imports &addedImports, const Imports &removedImports)
{
    bool resetModel = false;
    QString description;

    try {
        if (rewriterView())
            rewriterView()->importsChanged(addedImports, removedImports);
    } catch (const RewritingException &e) {
        description = e.description();
        resetModel = true;
    }

    m_nodeMetaInfoCache.clear();

    if (nodeInstanceView())
        nodeInstanceView()->importsChanged(addedImports, removedImports);

    for (const QPointer<AbstractView> &view : enabledViews())
        view->importsChanged(addedImports, removedImports);

    if (resetModel)
        resetModelByRewriter(description);
}

void ModelPrivate::notifyPossibleImportsChanged(const Imports &possibleImports)
{
    for (const QPointer<AbstractView> &view : enabledViews()) {
        Q_ASSERT(view != nullptr);
        view->possibleImportsChanged(possibleImports);
    }
}

void ModelPrivate::notifyUsedImportsChanged(const Imports &usedImports)
{
    for (const QPointer<AbstractView> &view : enabledViews()) {
        Q_ASSERT(view != nullptr);
        view->usedImportsChanged(usedImports);
    }
}

QUrl ModelPrivate::fileUrl() const
{
    return m_fileUrl;
}

void ModelPrivate::setDocumentMessages(const QList<DocumentMessage> &errors,
                                       const QList<DocumentMessage> &warnings)
{
    for (const QPointer<AbstractView> &view : std::as_const(m_viewList))
        view->documentMessagesChanged(errors, warnings);
}

void ModelPrivate::setFileUrl(const QUrl &fileUrl)
{
    QUrl oldPath = m_fileUrl;

    if (oldPath != fileUrl) {
        m_fileUrl = fileUrl;

        for (const QPointer<AbstractView> &view : std::as_const(m_viewList))
            view->fileUrlChanged(oldPath, fileUrl);
    }
}

void ModelPrivate::changeNodeType(const InternalNodePointer &node, const TypeName &typeName,
                                  int majorVersion, int minorVersion)
{
    node->typeName = typeName;
    node->majorVersion = majorVersion;
    node->minorVersion = minorVersion;

    try {
        notifyNodeTypeChanged(node, typeName, majorVersion, minorVersion);
    } catch (const RewritingException &) {
    }
}

InternalNodePointer ModelPrivate::createNode(const TypeName &typeName,
                                             int majorVersion,
                                             int minorVersion,
                                             const QList<QPair<PropertyName, QVariant>> &propertyList,
                                             const AuxiliaryDatas &auxiliaryDatas,
                                             const QString &nodeSource,
                                             ModelNode::NodeSourceType nodeSourceType,
                                             const QString &behaviorPropertyName,
                                             bool isRootNode)
{
    if (typeName.isEmpty())
        return {};

    qint32 internalId = 0;

    if (!isRootNode)
        internalId = m_internalIdCounter++;

    auto newNode = std::make_shared<InternalNode>(typeName, majorVersion, minorVersion, internalId);
    newNode->nodeSourceType = nodeSourceType;

    newNode->behaviorPropertyName = behaviorPropertyName;

    using PropertyPair = QPair<PropertyName, QVariant>;

    for (const PropertyPair &propertyPair : propertyList) {
        newNode->addVariantProperty(propertyPair.first);
        newNode->variantProperty(propertyPair.first)->setValue(propertyPair.second);
    }

    for (const auto &auxiliaryData : auxiliaryDatas)
        newNode->setAuxiliaryData(AuxiliaryDataKeyView{auxiliaryData.first}, auxiliaryData.second);

    m_nodeSet.insert(newNode);
    m_internalIdNodeHash.insert(newNode->internalId, newNode);

    if (!nodeSource.isNull())
        newNode->nodeSource = nodeSource;

    notifyNodeCreated(newNode);

    if (!newNode->propertyNameList().isEmpty())
        notifyVariantPropertiesChanged(newNode, newNode->propertyNameList(), AbstractView::PropertiesAdded);

    return newNode;
}

void ModelPrivate::removeNodeFromModel(const InternalNodePointer &node)
{
    Q_ASSERT(node);

    node->resetParentProperty();

    m_selectedInternalNodeList.removeAll(node);
    if (!node->id.isEmpty())
        m_idNodeHash.remove(node->id);
    node->isValid = false;
    m_nodeSet.remove(node);
    m_internalIdNodeHash.remove(node->internalId);
}

const QList<QPointer<AbstractView>> ModelPrivate::enabledViews() const
{
    return m_enabledViewList;
}

void ModelPrivate::removeAllSubNodes(const InternalNodePointer &node)
{
    for (const InternalNodePointer &subNode : node->allSubNodes())
        removeNodeFromModel(subNode);
}

void ModelPrivate::removeNode(const InternalNodePointer &node)
{
    Q_ASSERT(node);

    AbstractView::PropertyChangeFlags propertyChangeFlags = AbstractView::NoAdditionalChanges;

    notifyNodeAboutToBeRemoved(node);

    InternalNodeAbstractPropertyPointer oldParentProperty(node->parentProperty());

    removeAllSubNodes(node);
    removeNodeFromModel(node);

    InternalNodePointer parentNode;
    PropertyName parentPropertyName;
    if (oldParentProperty) {
        parentNode = oldParentProperty->propertyOwner();
        parentPropertyName = oldParentProperty->name();
    }

    if (oldParentProperty && oldParentProperty->isEmpty()) {
        removePropertyWithoutNotification(oldParentProperty);

        propertyChangeFlags |= AbstractView::EmptyPropertiesRemoved;
    }

    notifyNodeRemoved(node, parentNode, parentPropertyName, propertyChangeFlags);
}

InternalNodePointer ModelPrivate::rootNode() const
{
    return m_rootInternalNode;
}

MetaInfo ModelPrivate::metaInfo() const
{
    return m_metaInfo;
}

void ModelPrivate::setMetaInfo(const MetaInfo &metaInfo)
{
    m_metaInfo = metaInfo;
}

void ModelPrivate::changeNodeId(const InternalNodePointer &node, const QString &id)
{
    const QString oldId = node->id;

    node->id = id;
    if (!oldId.isEmpty())
        m_idNodeHash.remove(oldId);
    if (!id.isEmpty())
        m_idNodeHash.insert(id, node);

    try {
        notifyNodeIdChanged(node, id, oldId);
    } catch (const RewritingException &) {
    }
}

bool ModelPrivate::propertyNameIsValid(const PropertyName &propertyName) const
{
    if (propertyName.isEmpty())
        return false;

    if (propertyName == "id")
        return false;

    return true;
}

template<typename Callable>
void ModelPrivate::notifyNodeInstanceViewLast(Callable call)
{
    bool resetModel = false;
    QString description;

    try {
        if (rewriterView() && !rewriterView()->isBlockingNotifications())
            call(rewriterView());
    } catch (const RewritingException &e) {
        description = e.description();
        resetModel = true;
    }

    for (const QPointer<AbstractView> &view : enabledViews()) {
         try {
             if (!view->isBlockingNotifications())
                 call(view.data());
         } catch (const Exception &e) {
             e.showException(tr("Exception thrown by view %1.").arg(view->widgetInfo().tabName));
         }
    }

    if (nodeInstanceView() && !nodeInstanceView()->isBlockingNotifications())
        call(nodeInstanceView());

    if (resetModel)
        resetModelByRewriter(description);
}

template<typename Callable>
void ModelPrivate::notifyNormalViewsLast(Callable call)
{
    bool resetModel = false;
    QString description;

    try {
        if (rewriterView() && !rewriterView()->isBlockingNotifications())
            call(rewriterView());
    } catch (const RewritingException &e) {
        description = e.description();
        resetModel = true;
    }

    if (nodeInstanceView() && !nodeInstanceView()->isBlockingNotifications())
        call(nodeInstanceView());

    for (const QPointer<AbstractView> &view : enabledViews()) {
        if (!view->isBlockingNotifications())
            call(view.data());
    }

    if (resetModel)
        resetModelByRewriter(description);
}

template<typename Callable>
void ModelPrivate::notifyInstanceChanges(Callable call)
{
    for (const QPointer<AbstractView> &view : enabledViews()) {
        if (!view->isBlockingNotifications())
            call(view.data());
    }
}

void ModelPrivate::notifyAuxiliaryDataChanged(const InternalNodePointer &node,
                                              AuxiliaryDataKeyView key,
                                              const QVariant &data)
{
    notifyNodeInstanceViewLast([&](AbstractView *view) {
        ModelNode modelNode(node, m_model, view);
        view->auxiliaryDataChanged(modelNode, key, data);
    });
}

void ModelPrivate::notifyNodeSourceChanged(const InternalNodePointer &node,
                                           const QString &newNodeSource)
{
    notifyNodeInstanceViewLast([&](AbstractView *view) {
        ModelNode ModelNode(node, m_model, view);
        view->nodeSourceChanged(ModelNode, newNodeSource);
    });
}

void ModelPrivate::notifyRootNodeTypeChanged(const QString &type, int majorVersion, int minorVersion)
{
    notifyNodeInstanceViewLast(
        [&](AbstractView *view) { view->rootNodeTypeChanged(type, majorVersion, minorVersion); });
}

void ModelPrivate::notifyInstancePropertyChange(const QList<QPair<ModelNode, PropertyName>> &propertyPairList)
{
    notifyInstanceChanges([&](AbstractView *view) {
        using ModelNodePropertyPair = QPair<ModelNode, PropertyName>;
        QList<QPair<ModelNode, PropertyName>> adaptedPropertyList;
        for (const ModelNodePropertyPair &propertyPair : propertyPairList) {
            ModelNodePropertyPair newPair(ModelNode{propertyPair.first.internalNode(), m_model, view}, propertyPair.second);
            adaptedPropertyList.append(newPair);
        }
        view->instancePropertyChanged(adaptedPropertyList);
    });
}

void ModelPrivate::notifyInstanceErrorChange(const QVector<qint32> &instanceIds)
{
    notifyInstanceChanges([&](AbstractView *view) {
        QVector<ModelNode> errorNodeList;
        for (qint32 instanceId : instanceIds)
            errorNodeList.append(ModelNode(m_model->d->nodeForInternalId(instanceId), m_model, view));
        view->instanceErrorChanged(errorNodeList);
    });
}

void ModelPrivate::notifyInstancesCompleted(const QVector<ModelNode> &modelNodeVector)
{
    QVector<InternalNodePointer> internalVector(toInternalNodeVector(modelNodeVector));

    notifyInstanceChanges([&](AbstractView *view) {
        view->instancesCompleted(toModelNodeVector(internalVector, view));
    });
}

namespace {
QMultiHash<ModelNode, InformationName> convertModelNodeInformationHash(
    const QMultiHash<ModelNode, InformationName> &informationChangeHash, AbstractView *view)
{
    QMultiHash<ModelNode, InformationName> convertedModelNodeInformationHash;

    for (auto it = informationChangeHash.cbegin(), end = informationChangeHash.cend(); it != end; ++it)
        convertedModelNodeInformationHash.insert(ModelNode(it.key(), view), it.value());

    return convertedModelNodeInformationHash;
}
} // namespace

void ModelPrivate::notifyInstancesInformationsChange(
    const QMultiHash<ModelNode, InformationName> &informationChangeHash)
{
    notifyInstanceChanges([&](AbstractView *view) {
        view->instanceInformationsChanged(convertModelNodeInformationHash(informationChangeHash, view));
    });
}

void ModelPrivate::notifyInstancesRenderImageChanged(const QVector<ModelNode> &modelNodeVector)
{
    QVector<InternalNodePointer> internalVector(toInternalNodeVector(modelNodeVector));

    notifyInstanceChanges([&](AbstractView *view) {
        view->instancesRenderImageChanged(toModelNodeVector(internalVector, view));
    });
}

void ModelPrivate::notifyInstancesPreviewImageChanged(const QVector<ModelNode> &modelNodeVector)
{
    QVector<InternalNodePointer> internalVector(toInternalNodeVector(modelNodeVector));

    notifyInstanceChanges([&](AbstractView *view) {
        view->instancesPreviewImageChanged(toModelNodeVector(internalVector, view));
    });
}

void ModelPrivate::notifyInstancesChildrenChanged(const QVector<ModelNode> &modelNodeVector)
{
    QVector<InternalNodePointer> internalVector(toInternalNodeVector(modelNodeVector));

    notifyInstanceChanges([&](AbstractView *view) {
        view->instancesChildrenChanged(toModelNodeVector(internalVector, view));
    });
}

void ModelPrivate::notifyCurrentStateChanged(const ModelNode &node)
{
    m_currentStateNode = node.internalNode();
    notifyNodeInstanceViewLast([&](AbstractView *view) {
        view->currentStateChanged(ModelNode(node.internalNode(), m_model, view));
    });
}

void ModelPrivate::notifyCurrentTimelineChanged(const ModelNode &node)
{
    m_currentTimelineNode = node.internalNode();
    notifyNodeInstanceViewLast([&](AbstractView *view) {
        view->currentTimelineChanged(ModelNode(node.internalNode(), m_model, view));
    });
}

void ModelPrivate::notifyRenderImage3DChanged(const QImage &image)
{
    notifyInstanceChanges([&](AbstractView *view) { view->renderImage3DChanged(image); });
}

void ModelPrivate::notifyUpdateActiveScene3D(const QVariantMap &sceneState)
{
    notifyInstanceChanges([&](AbstractView *view) { view->updateActiveScene3D(sceneState); });
}

void ModelPrivate::notifyModelNodePreviewPixmapChanged(const ModelNode &node, const QPixmap &pixmap)
{
    notifyInstanceChanges(
        [&](AbstractView *view) { view->modelNodePreviewPixmapChanged(node, pixmap); });
}

void ModelPrivate::notifyImport3DSupportChanged(const QVariantMap &supportMap)
{
    notifyInstanceChanges([&](AbstractView *view) { view->updateImport3DSupport(supportMap); });
}

void ModelPrivate::notifyNodeAtPosResult(const ModelNode &modelNode, const QVector3D &pos3d)
{
    notifyInstanceChanges([&](AbstractView *view) { view->nodeAtPosReady(modelNode, pos3d); });
}

void ModelPrivate::notifyView3DAction(View3DActionType type, const QVariant &value)
{
    notifyNormalViewsLast([&](AbstractView *view) { view->view3DAction(type, value); });
}

void ModelPrivate::notifyActive3DSceneIdChanged(qint32 sceneId)
{
    notifyInstanceChanges([&](AbstractView *view) { view->active3DSceneChanged(sceneId); });
}

void ModelPrivate::notifyDragStarted(QMimeData *mimeData)
{
    notifyInstanceChanges([&](AbstractView *view) { view->dragStarted(mimeData); });
}

void ModelPrivate::notifyDragEnded()
{
    notifyInstanceChanges([&](AbstractView *view) { view->dragEnded(); });
}

void ModelPrivate::notifyRewriterBeginTransaction()
{
    notifyNodeInstanceViewLast([&](AbstractView *view) { view->rewriterBeginTransaction(); });
}

void ModelPrivate::notifyRewriterEndTransaction()
{
    notifyNodeInstanceViewLast([&](AbstractView *view) { view->rewriterEndTransaction(); });
}

void ModelPrivate::notifyInstanceToken(const QString &token, int number,
                                       const QVector<ModelNode> &modelNodeVector)
{
    QVector<InternalNodePointer> internalVector(toInternalNodeVector(modelNodeVector));

    notifyInstanceChanges([&](AbstractView *view) {
        view->instancesToken(token, number, toModelNodeVector(internalVector, view));
    });
}

void ModelPrivate::notifyCustomNotification(const AbstractView *senderView,
                                            const QString &identifier,
                                            const QList<ModelNode> &modelNodeList,
                                            const QList<QVariant> &data)
{
    QList<InternalNodePointer> internalList(toInternalNodeList(modelNodeList));
    notifyNodeInstanceViewLast([&](AbstractView *view) {
        view->customNotification(senderView, identifier, toModelNodeList(internalList, view), data);
    });
}

void ModelPrivate::notifyPropertiesRemoved(const QList<PropertyPair> &propertyPairList)
{
    notifyNormalViewsLast([&](AbstractView *view) {
        QList<AbstractProperty> propertyList;
        propertyList.reserve(propertyPairList.size());
        for (const PropertyPair &propertyPair : propertyPairList) {
            AbstractProperty newProperty(propertyPair.second, propertyPair.first, m_model, view);
            propertyList.append(newProperty);
        }

        view->propertiesRemoved(propertyList);
    });
}

void ModelPrivate::notifyPropertiesAboutToBeRemoved(
    const QList<InternalPropertyPointer> &internalPropertyList)
{
    bool resetModel = false;
    QString description;

    try {
        if (rewriterView()) {
            QList<AbstractProperty> propertyList;
            for (const InternalPropertyPointer &property : internalPropertyList) {
                AbstractProperty newProperty(property->name(), property->propertyOwner(), m_model, rewriterView());
                propertyList.append(newProperty);
            }

            rewriterView()->propertiesAboutToBeRemoved(propertyList);
        }
    } catch (const RewritingException &e) {
        description = e.description();
        resetModel = true;
    }

    for (const QPointer<AbstractView> &view : enabledViews()) {
        QList<AbstractProperty> propertyList;
        Q_ASSERT(view != nullptr);
        for (const InternalPropertyPointer &property : internalPropertyList) {
            AbstractProperty newProperty(property->name(), property->propertyOwner(), m_model, view.data());
            propertyList.append(newProperty);
        }

        try {
            view->propertiesAboutToBeRemoved(propertyList);
        } catch (const RewritingException &e) {
            description = e.description();
            resetModel = true;
        }
    }

    if (nodeInstanceView()) {
        QList<AbstractProperty> propertyList;
        for (const InternalPropertyPointer &property : internalPropertyList) {
            AbstractProperty newProperty(property->name(), property->propertyOwner(), m_model, nodeInstanceView());
            propertyList.append(newProperty);
        }

        nodeInstanceView()->propertiesAboutToBeRemoved(propertyList);
    }

    if (resetModel)
        resetModelByRewriter(description);
}

void ModelPrivate::setAuxiliaryData(const InternalNodePointer &node,
                                    const AuxiliaryDataKeyView &key,
                                    const QVariant &data)
{
    bool changed = false;

    if (data.isValid())
        changed = node->setAuxiliaryData(key, data);
    else
        changed = node->removeAuxiliaryData(key);

    if (changed)
        notifyAuxiliaryDataChanged(node, key, data);
}

void ModelPrivate::resetModelByRewriter(const QString &description)
{
    if (rewriterView()) {
        rewriterView()->resetToLastCorrectQml();

        throw RewritingException(__LINE__,
                                 __FUNCTION__,
                                 __FILE__,
                                 description.toUtf8(),
                                 rewriterView()->textModifierContent());
    }
}

void ModelPrivate::attachView(AbstractView *view)
{
    Q_ASSERT(view);

    if (m_viewList.contains(view))
        return;

    m_viewList.append(view);

    view->modelAttached(m_model);
}

void ModelPrivate::detachView(AbstractView *view, bool notifyView)
{
    if (notifyView)
        view->modelAboutToBeDetached(m_model);
    m_viewList.removeOne(view);
    updateEnabledViews();
}

void ModelPrivate::notifyNodeCreated(const InternalNodePointer &newInternalNodePointer)
{
    notifyNormalViewsLast([&](AbstractView *view) {
        view->nodeCreated(ModelNode{newInternalNodePointer, m_model, view});
    });
}

void ModelPrivate::notifyNodeAboutToBeRemoved(const InternalNodePointer &internalNodePointer)
{
    notifyNodeInstanceViewLast([&](AbstractView *view) {
        view->nodeAboutToBeRemoved(ModelNode{internalNodePointer, m_model, view});
    });
}

void ModelPrivate::notifyNodeRemoved(const InternalNodePointer &removedNode,
                                     const InternalNodePointer &parentNode,
                                     const PropertyName &parentPropertyName,
                                     AbstractView::PropertyChangeFlags propertyChange)
{
    notifyNormalViewsLast([&](AbstractView *view) {
        view->nodeRemoved(ModelNode{removedNode, m_model, view},
                          NodeAbstractProperty{parentPropertyName, parentNode, m_model, view},
                          propertyChange);
    });
}

void ModelPrivate::notifyNodeTypeChanged(const InternalNodePointer &node,
                                         const TypeName &type,
                                         int majorVersion,
                                         int minorVersion)
{
    notifyNodeInstanceViewLast([&](AbstractView *view) {
        view->nodeTypeChanged(ModelNode{node, m_model, view},
                              type,
                              majorVersion,
                              minorVersion);
    });
}

void ModelPrivate::notifyNodeIdChanged(const InternalNodePointer &node,
                                       const QString &newId,
                                       const QString &oldId)
{
    notifyNodeInstanceViewLast([&](AbstractView *view) {
        view->nodeIdChanged(ModelNode{node, m_model, view}, newId, oldId);
    });
}

void ModelPrivate::notifyBindingPropertiesAboutToBeChanged(
    const QList<InternalBindingPropertyPointer> &internalPropertyList)
{
    notifyNodeInstanceViewLast([&](AbstractView *view) {
        QList<BindingProperty> propertyList;
        for (const InternalBindingPropertyPointer &bindingProperty : internalPropertyList) {
            propertyList.append(BindingProperty(bindingProperty->name(),
                                                bindingProperty->propertyOwner(),
                                                m_model,
                                                view));
        }
        view->bindingPropertiesAboutToBeChanged(propertyList);
    });
}

void ModelPrivate::notifyBindingPropertiesChanged(
    const QList<InternalBindingPropertyPointer> &internalPropertyList,
    AbstractView::PropertyChangeFlags propertyChange)
{
    notifyNodeInstanceViewLast([&](AbstractView *view) {
        QList<BindingProperty> propertyList;
        for (const InternalBindingPropertyPointer &bindingProperty : internalPropertyList) {
            propertyList.append(BindingProperty(bindingProperty->name(),
                                                bindingProperty->propertyOwner(),
                                                m_model,
                                                view));
        }
        view->bindingPropertiesChanged(propertyList, propertyChange);
    });
}

void ModelPrivate::notifySignalHandlerPropertiesChanged(
    const QVector<InternalSignalHandlerPropertyPointer> &internalPropertyList,
    AbstractView::PropertyChangeFlags propertyChange)
{
    notifyNodeInstanceViewLast([&](AbstractView *view) {
        QVector<SignalHandlerProperty> propertyList;
        for (const InternalSignalHandlerPropertyPointer &signalHandlerProperty : internalPropertyList) {
            propertyList.append(SignalHandlerProperty(signalHandlerProperty->name(),
                                                      signalHandlerProperty->propertyOwner(),
                                                      m_model,
                                                      view));
        }
        view->signalHandlerPropertiesChanged(propertyList, propertyChange);
    });
}

void ModelPrivate::notifySignalDeclarationPropertiesChanged(
    const QVector<InternalSignalDeclarationPropertyPointer> &internalPropertyList,
    AbstractView::PropertyChangeFlags propertyChange)
{
    notifyNodeInstanceViewLast([&](AbstractView *view) {
        QVector<SignalDeclarationProperty> propertyList;
        for (const InternalSignalDeclarationPropertyPointer &signalHandlerProperty : internalPropertyList) {
            propertyList.append(SignalDeclarationProperty(signalHandlerProperty->name(),
                                                      signalHandlerProperty->propertyOwner(),
                                                      m_model,
                                                      view));
        }
        view->signalDeclarationPropertiesChanged(propertyList, propertyChange);
    });
}

void ModelPrivate::notifyScriptFunctionsChanged(const InternalNodePointer &node,
                                                const QStringList &scriptFunctionList)
{
    notifyNormalViewsLast([&](AbstractView *view) {
        view->scriptFunctionsChanged(ModelNode{node, m_model, view}, scriptFunctionList);
    });
}

void ModelPrivate::notifyVariantPropertiesChanged(const InternalNodePointer &node,
                                                  const PropertyNameList &propertyNameList,
                                                  AbstractView::PropertyChangeFlags propertyChange)
{
    notifyNodeInstanceViewLast([&](AbstractView *view) {
        QList<VariantProperty> propertyList;
        for (const PropertyName &propertyName : propertyNameList) {
            VariantProperty property(propertyName, node, m_model, view);
            propertyList.append(property);
        }

        view->variantPropertiesChanged(propertyList, propertyChange);
    });
}

void ModelPrivate::notifyNodeAboutToBeReparent(const InternalNodePointer &node,
                                               const InternalNodeAbstractPropertyPointer &newPropertyParent,
                                               const InternalNodePointer &oldParent,
                                               const PropertyName &oldPropertyName,
                                               AbstractView::PropertyChangeFlags propertyChange)
{
    notifyNodeInstanceViewLast([&](AbstractView *view) {
        NodeAbstractProperty newProperty;
        NodeAbstractProperty oldProperty;

        if (!oldPropertyName.isEmpty() && oldParent->isValid)
            oldProperty = NodeAbstractProperty(oldPropertyName, oldParent, m_model, view);

        if (!newPropertyParent.isNull())
            newProperty = NodeAbstractProperty(newPropertyParent, m_model, view);

        ModelNode modelNode(node, m_model, view);
        view->nodeAboutToBeReparented(modelNode, newProperty, oldProperty, propertyChange);
    });
}

void ModelPrivate::notifyNodeReparent(const InternalNodePointer &node,
                                      const InternalNodeAbstractPropertyPointer &newPropertyParent,
                                      const InternalNodePointer &oldParent,
                                      const PropertyName &oldPropertyName,
                                      AbstractView::PropertyChangeFlags propertyChange)
{
    notifyNodeInstanceViewLast([&](AbstractView *view) {
        NodeAbstractProperty newProperty;
        NodeAbstractProperty oldProperty;

        if (!oldPropertyName.isEmpty() && oldParent->isValid)
            oldProperty = NodeAbstractProperty(oldPropertyName, oldParent, m_model, view);

        if (!newPropertyParent.isNull())
            newProperty = NodeAbstractProperty(newPropertyParent, m_model, view);
        ModelNode modelNode(node, m_model, view);

        view->nodeReparented(modelNode, newProperty, oldProperty, propertyChange);
    });
}

void ModelPrivate::notifyNodeOrderChanged(const InternalNodeListPropertyPointer &internalListProperty,
                                          const InternalNodePointer &node,
                                          int oldIndex)
{
    notifyNodeInstanceViewLast([&](AbstractView *view) {
        NodeListProperty nodeListProperty(internalListProperty, m_model, view);
        view->nodeOrderChanged(nodeListProperty, ModelNode(node, m_model, view), oldIndex);
    });
}

void ModelPrivate::notifyNodeOrderChanged(const InternalNodeListPropertyPointer &internalListProperty)
{
    notifyNodeInstanceViewLast([&](AbstractView *view) {
        NodeListProperty nodeListProperty(internalListProperty, m_model, view);
        view->nodeOrderChanged(nodeListProperty);
    });
}

void ModelPrivate::setSelectedNodes(const QList<InternalNodePointer> &selectedNodeList)
{
    auto sortedSelectedList = Utils::filtered(selectedNodeList, [](const auto &node) {
        return node && node->isValid;
    });

    Utils::sort(sortedSelectedList);
    sortedSelectedList.erase(std::unique(sortedSelectedList.begin(), sortedSelectedList.end()),
                             sortedSelectedList.end());

    if (sortedSelectedList == m_selectedInternalNodeList)
        return;

    const QList<InternalNodePointer> lastSelectedNodeList = m_selectedInternalNodeList;
    m_selectedInternalNodeList = sortedSelectedList;

    changeSelectedNodes(sortedSelectedList, lastSelectedNodeList);
}

void ModelPrivate::clearSelectedNodes()
{
    const QList<InternalNodePointer> lastSelectedNodeList = m_selectedInternalNodeList;
    m_selectedInternalNodeList.clear();
    changeSelectedNodes(m_selectedInternalNodeList, lastSelectedNodeList);
}

void ModelPrivate::removeAuxiliaryData(const InternalNodePointer &node, const AuxiliaryDataKeyView &key)
{
    bool removed = node->removeAuxiliaryData(key);

    if (removed)
        notifyAuxiliaryDataChanged(node, key, QVariant());
}

QList<ModelNode> ModelPrivate::toModelNodeList(const QList<InternalNodePointer> &nodeList, AbstractView *view) const
{
    QList<ModelNode> modelNodeList;
    modelNodeList.reserve(nodeList.size());
    for (const InternalNodePointer &node : nodeList)
        modelNodeList.append(ModelNode(node, m_model, view));

    return modelNodeList;
}

QVector<ModelNode> ModelPrivate::toModelNodeVector(const QVector<InternalNodePointer> &nodeVector, AbstractView *view) const
{
    QVector<ModelNode> modelNodeVector;
    for (const InternalNodePointer &node : nodeVector)
        modelNodeVector.append(ModelNode(node, m_model, view));

    return modelNodeVector;
}

QList<InternalNodePointer> ModelPrivate::toInternalNodeList(const QList<ModelNode> &modelNodeList) const
{
    QList<InternalNodePointer> newNodeList;
    newNodeList.reserve(modelNodeList.size());
    for (const ModelNode &modelNode : modelNodeList)
        newNodeList.append(modelNode.internalNode());

    return newNodeList;
}

QVector<InternalNodePointer> ModelPrivate::toInternalNodeVector(const QVector<ModelNode> &modelNodeVector) const
{
    QVector<InternalNodePointer> newNodeVector;
    newNodeVector.reserve(modelNodeVector.size());
    for (const ModelNode &modelNode : modelNodeVector)
        newNodeVector.append(modelNode.internalNode());

    return newNodeVector;
}

void ModelPrivate::changeSelectedNodes(const QList<InternalNodePointer> &newSelectedNodeList,
                                       const QList<InternalNodePointer> &oldSelectedNodeList)
{
    for (const QPointer<AbstractView> &view : enabledViews()) {
        Q_ASSERT(view != nullptr);
        view->selectedNodesChanged(toModelNodeList(newSelectedNodeList, view.data()),
                                   toModelNodeList(oldSelectedNodeList, view.data()));
    }

    if (nodeInstanceView()) {
        nodeInstanceView()->selectedNodesChanged(toModelNodeList(newSelectedNodeList, nodeInstanceView()),
                                                 toModelNodeList(oldSelectedNodeList, nodeInstanceView()));
    }
}

QList<InternalNodePointer> ModelPrivate::selectedNodes() const
{
    for (const InternalNodePointer &node : std::as_const(m_selectedInternalNodeList)) {
        if (!node->isValid)
            return {};
    }

    return m_selectedInternalNodeList;
}

void ModelPrivate::selectNode(const InternalNodePointer &node)
{
    if (selectedNodes().contains(node))
        return;

    QList<InternalNodePointer> selectedNodeList(selectedNodes());
    selectedNodeList += node;
    setSelectedNodes(selectedNodeList);
}

void ModelPrivate::deselectNode(const InternalNodePointer &node)
{
    QList<InternalNodePointer> selectedNodeList(selectedNodes());
    bool isRemoved = selectedNodeList.removeOne(node);

    if (isRemoved)
        setSelectedNodes(selectedNodeList);
}

void ModelPrivate::removePropertyWithoutNotification(const InternalPropertyPointer &property)
{
    if (property->isNodeAbstractProperty()) {
        const auto &&allSubNodes = property->toNodeAbstractProperty()->allSubNodes();
        for (const InternalNodePointer &node : allSubNodes)
            removeNodeFromModel(node);
    }

    property->remove();
}

static QList<PropertyPair> toPropertyPairList(const QList<InternalPropertyPointer> &propertyList)
{
    QList<PropertyPair> propertyPairList;
    propertyPairList.reserve(propertyList.size());

    for (const InternalPropertyPointer &property : propertyList)
        propertyPairList.append({property->propertyOwner(), property->name()});

    return propertyPairList;
}

void ModelPrivate::removeProperty(const InternalPropertyPointer &property)
{
    notifyPropertiesAboutToBeRemoved({property});

    const QList<PropertyPair> propertyPairList = toPropertyPairList({property});

    removePropertyWithoutNotification(property);

    notifyPropertiesRemoved(propertyPairList);
}

void ModelPrivate::setBindingProperty(const InternalNodePointer &node, const PropertyName &name, const QString &expression)
{
    AbstractView::PropertyChangeFlags propertyChange = AbstractView::NoAdditionalChanges;
    if (!node->hasProperty(name)) {
        node->addBindingProperty(name);
        propertyChange = AbstractView::PropertiesAdded;
    }

    InternalBindingPropertyPointer bindingProperty = node->bindingProperty(name);
    notifyBindingPropertiesAboutToBeChanged({bindingProperty});
    bindingProperty->setExpression(expression);
    notifyBindingPropertiesChanged({bindingProperty}, propertyChange);
}

void ModelPrivate::setSignalHandlerProperty(const InternalNodePointer &node, const PropertyName &name, const QString &source)
{
    AbstractView::PropertyChangeFlags propertyChange = AbstractView::NoAdditionalChanges;
    if (!node->hasProperty(name)) {
        node->addSignalHandlerProperty(name);
        propertyChange = AbstractView::PropertiesAdded;
    }

    InternalSignalHandlerPropertyPointer signalHandlerProperty = node->signalHandlerProperty(name);
    signalHandlerProperty->setSource(source);
    notifySignalHandlerPropertiesChanged({signalHandlerProperty}, propertyChange);
}

void ModelPrivate::setSignalDeclarationProperty(const InternalNodePointer &node, const PropertyName &name, const QString &signature)
{
    AbstractView::PropertyChangeFlags propertyChange = AbstractView::NoAdditionalChanges;
    if (!node->hasProperty(name)) {
        node->addSignalDeclarationProperty(name);
        propertyChange = AbstractView::PropertiesAdded;
    }

    InternalSignalDeclarationPropertyPointer signalDeclarationProperty = node->signalDeclarationProperty(name);
    signalDeclarationProperty->setSignature(signature);
    notifySignalDeclarationPropertiesChanged({signalDeclarationProperty}, propertyChange);
}

void ModelPrivate::setVariantProperty(const InternalNodePointer &node, const PropertyName &name, const QVariant &value)
{
    AbstractView::PropertyChangeFlags propertyChange = AbstractView::NoAdditionalChanges;
    if (!node->hasProperty(name)) {
        node->addVariantProperty(name);
        propertyChange = AbstractView::PropertiesAdded;
    }

    node->variantProperty(name)->setValue(value);
    node->variantProperty(name)->resetDynamicTypeName();
    notifyVariantPropertiesChanged(node, PropertyNameList({name}), propertyChange);
}

void ModelPrivate::setDynamicVariantProperty(const InternalNodePointer &node,
                                             const PropertyName &name,
                                             const TypeName &dynamicPropertyType,
                                             const QVariant &value)
{
    AbstractView::PropertyChangeFlags propertyChange = AbstractView::NoAdditionalChanges;
    if (!node->hasProperty(name)) {
        node->addVariantProperty(name);
        propertyChange = AbstractView::PropertiesAdded;
    }

    node->variantProperty(name)->setDynamicValue(dynamicPropertyType, value);
    notifyVariantPropertiesChanged(node, PropertyNameList({name}), propertyChange);
}

void ModelPrivate::setDynamicBindingProperty(const InternalNodePointer &node,
                                             const PropertyName &name,
                                             const TypeName &dynamicPropertyType,
                                             const QString &expression)
{
    AbstractView::PropertyChangeFlags propertyChange = AbstractView::NoAdditionalChanges;
    if (!node->hasProperty(name)) {
        node->addBindingProperty(name);
        propertyChange = AbstractView::PropertiesAdded;
    }

    InternalBindingPropertyPointer bindingProperty = node->bindingProperty(name);
    notifyBindingPropertiesAboutToBeChanged({bindingProperty});
    bindingProperty->setDynamicExpression(dynamicPropertyType, expression);
    notifyBindingPropertiesChanged({bindingProperty}, propertyChange);
}

void ModelPrivate::reparentNode(const InternalNodePointer &parentNode,
                                const PropertyName &name,
                                const InternalNodePointer &childNode,
                                bool list,
                                const TypeName &dynamicTypeName)
{
    AbstractView::PropertyChangeFlags propertyChange = AbstractView::NoAdditionalChanges;
    if (!parentNode->hasProperty(name)) {
        if (list)
            parentNode->addNodeListProperty(name);
        else
            parentNode->addNodeProperty(name, dynamicTypeName);
        propertyChange |= AbstractView::PropertiesAdded;
    }

    InternalNodeAbstractPropertyPointer oldParentProperty(childNode->parentProperty());
    InternalNodePointer oldParentNode;
    PropertyName oldParentPropertyName;
    if (oldParentProperty && oldParentProperty->isValid()) {
        oldParentNode = childNode->parentProperty()->propertyOwner();
        oldParentPropertyName = childNode->parentProperty()->name();
    }

    InternalNodeAbstractPropertyPointer newParentProperty(parentNode->nodeAbstractProperty(name));
    Q_ASSERT(!newParentProperty.isNull());

    notifyNodeAboutToBeReparent(childNode, newParentProperty, oldParentNode, oldParentPropertyName, propertyChange);

    if (newParentProperty)
        childNode->setParentProperty(newParentProperty);

    if (oldParentProperty && oldParentProperty->isValid() && oldParentProperty->isEmpty()) {
        removePropertyWithoutNotification(oldParentProperty);

        propertyChange |= AbstractView::EmptyPropertiesRemoved;
    }

    notifyNodeReparent(childNode, newParentProperty, oldParentNode, oldParentPropertyName, propertyChange);
}

void ModelPrivate::clearParent(const InternalNodePointer &node)
{
    InternalNodeAbstractPropertyPointer oldParentProperty(node->parentProperty());
    InternalNodePointer oldParentNode;
    PropertyName oldParentPropertyName;
    if (oldParentProperty->isValid()) {
        oldParentNode = node->parentProperty()->propertyOwner();
        oldParentPropertyName = node->parentProperty()->name();
    }

    node->resetParentProperty();
    notifyNodeReparent(node, InternalNodeAbstractPropertyPointer(), oldParentNode, oldParentPropertyName, AbstractView::NoAdditionalChanges);
}

void ModelPrivate::changeRootNodeType(const TypeName &type, int majorVersion, int minorVersion)
{
    Q_ASSERT(rootNode());
    rootNode()->typeName = type;
    rootNode()->majorVersion = majorVersion;
    rootNode()->minorVersion = minorVersion;
    notifyRootNodeTypeChanged(QString::fromUtf8(type), majorVersion, minorVersion);
}

void ModelPrivate::setScriptFunctions(const InternalNodePointer &node, const QStringList &scriptFunctionList)
{
    node->scriptFunctions = scriptFunctionList;

    notifyScriptFunctionsChanged(node, scriptFunctionList);
}

void ModelPrivate::setNodeSource(const InternalNodePointer &node, const QString &nodeSource)
{
    node->nodeSource = nodeSource;
    notifyNodeSourceChanged(node, nodeSource);
}

void ModelPrivate::changeNodeOrder(const InternalNodePointer &parentNode, const PropertyName &listPropertyName, int from, int to)
{
    InternalNodeListPropertyPointer nodeList(parentNode->nodeListProperty(listPropertyName));
    Q_ASSERT(!nodeList.isNull());
    nodeList->slide(from, to);

    const InternalNodePointer internalNode = nodeList->nodeList().at(to);
    notifyNodeOrderChanged(nodeList, internalNode, from);
}

void ModelPrivate::setRewriterView(RewriterView *rewriterView)
{
    if (rewriterView == m_rewriterView.data())
        return;

    Q_ASSERT(!(rewriterView && m_rewriterView));

    if (m_rewriterView)
        m_rewriterView->modelAboutToBeDetached(m_model);

    m_rewriterView = rewriterView;

    if (rewriterView)
        rewriterView->modelAttached(m_model);
}

RewriterView *ModelPrivate::rewriterView() const
{
    return m_rewriterView.data();
}

void ModelPrivate::setNodeInstanceView(NodeInstanceView *nodeInstanceView)
{
    if (nodeInstanceView == m_nodeInstanceView.data())
        return;

    if (m_nodeInstanceView)
        m_nodeInstanceView->modelAboutToBeDetached(m_model);

    m_nodeInstanceView = nodeInstanceView;

    if (nodeInstanceView)
        nodeInstanceView->modelAttached(m_model);
}

NodeInstanceView *ModelPrivate::nodeInstanceView() const
{
    return m_nodeInstanceView.data();
}

InternalNodePointer ModelPrivate::currentTimelineNode() const
{
    return m_currentTimelineNode;
}

void ModelPrivate::updateEnabledViews()
{
    m_enabledViewList = Utils::filtered(m_viewList, [](QPointer<AbstractView> view) {
        return view->isEnabled();
    });
}

InternalNodePointer ModelPrivate::nodeForId(const QString &id) const
{
    return m_idNodeHash.value(id);
}

bool ModelPrivate::hasId(const QString &id) const
{
    return m_idNodeHash.contains(id);
}

InternalNodePointer ModelPrivate::nodeForInternalId(qint32 internalId) const
{
    return m_internalIdNodeHash.value(internalId);
}

bool ModelPrivate::hasNodeForInternalId(qint32 internalId) const
{
    return m_internalIdNodeHash.contains(internalId);
}

QList<InternalNodePointer> ModelPrivate::allNodes() const
{
    if (!m_rootInternalNode || !m_rootInternalNode->isValid)
        return {};

    // the nodes must be ordered.

    QList<InternalNodePointer> nodeList;
    nodeList.append(m_rootInternalNode);
    nodeList.append(m_rootInternalNode->allSubNodes());
    // FIXME: This is horribly expensive compared to a loop.
    nodeList.append(Utils::toList(m_nodeSet - Utils::toSet(nodeList)));

    return nodeList;
}

bool ModelPrivate::isWriteLocked() const
{
    return m_writeLock;
}

InternalNodePointer ModelPrivate::currentStateNode() const
{
    return m_currentStateNode;
}

WriteLocker::WriteLocker(ModelPrivate *model)
    : m_model(model)
{
    Q_ASSERT(model);
    if (m_model->m_writeLock)
        qWarning() << "QmlDesigner: Misbehaving view calls back to model!!!";
    // FIXME: Enable it again
    Q_ASSERT(!m_model->m_writeLock);
    model->m_writeLock = true;
}

WriteLocker::WriteLocker(Model *model)
    : m_model(model->d.get())
{
    Q_ASSERT(model->d.get());
    if (m_model->m_writeLock)
        qWarning() << "QmlDesigner: Misbehaving view calls back to model!!!";
    // FIXME: Enable it again
    Q_ASSERT(!m_model->m_writeLock);
    m_model->m_writeLock = true;
}

WriteLocker::~WriteLocker()
{
    if (!m_model->m_writeLock)
        qWarning() << "QmlDesigner: WriterLocker out of sync!!!";
    // FIXME: Enable it again
    Q_ASSERT(m_model->m_writeLock);
    m_model->m_writeLock = false;
}

void WriteLocker::unlock(Model *model)
{
    model->d->m_writeLock = false;
}

void WriteLocker::lock(Model *model)
{
    model->d->m_writeLock = true;
}

} // namespace Internal

Model::Model(ProjectStorage<Sqlite::Database> &projectStorage,
             const TypeName &typeName,
             int major,
             int minor,
             Model *metaInfoProxyModel)
    : d(std::make_unique<Internal::ModelPrivate>(
        this, projectStorage, typeName, major, minor, metaInfoProxyModel))
{}

Model::Model(const TypeName &typeName, int major, int minor, Model *metaInfoProxyModel)
    : d(std::make_unique<Internal::ModelPrivate>(this, typeName, major, minor, metaInfoProxyModel))
{}

Model::~Model() = default;

const Imports &Model::imports() const
{
    return d->imports();
}

const Imports &Model::possibleImports() const
{
    return d->m_possibleImportList;
}

const Imports &Model::usedImports() const
{
    return d->m_usedImportList;
}

void Model::changeImports(const Imports &importsToBeAdded, const Imports &importsToBeRemoved)
{
    d->changeImports(importsToBeAdded, importsToBeRemoved);
}

void Model::setPossibleImports(Imports possibleImports)
{
    std::sort(possibleImports.begin(), possibleImports.end());

    if (d->m_possibleImportList != possibleImports) {
        d->m_possibleImportList = std::move(possibleImports);
        d->notifyPossibleImportsChanged(d->m_possibleImportList);
    }
}

void Model::setUsedImports(Imports usedImports)
{
    std::sort(usedImports.begin(), usedImports.end());

    if (d->m_usedImportList != usedImports) {
        d->m_usedImportList = std::move(usedImports);
        d->notifyUsedImportsChanged(d->m_usedImportList);
    }
}

static bool compareVersions(const QString &version1, const QString &version2, bool allowHigherVersion)
{
    if (version2.isEmpty())
        return true;
    if (version1 == version2)
        return true;
    if (!allowHigherVersion)
        return false;
    QStringList version1List = version1.split(QLatin1Char('.'));
    QStringList version2List = version2.split(QLatin1Char('.'));
    if (version1List.count() == 2 && version2List.count() == 2) {
        bool ok;
        int major1 = version1List.constFirst().toInt(&ok);
        if (!ok)
            return false;
        int major2 = version2List.constFirst().toInt(&ok);
        if (!ok)
            return false;
        if (major1 >= major2) {
            int minor1 = version1List.constLast().toInt(&ok);
            if (!ok)
                return false;
            int minor2 = version2List.constLast().toInt(&ok);
            if (!ok)
                return false;
            if (minor1 >= minor2)
                return true;
        }
    }

    return false;
}

bool Model::hasImport(const Import &import, bool ignoreAlias, bool allowHigherVersion) const
{
    if (imports().contains(import))
        return true;

    if (!ignoreAlias)
        return false;

    for (const Import &existingImport : imports()) {
        if (existingImport.isFileImport() && import.isFileImport()) {
            if (existingImport.file() == import.file())
                return true;
        }
        if (existingImport.isLibraryImport() && import.isLibraryImport()) {
            if (existingImport.url() == import.url()
                && compareVersions(existingImport.version(), import.version(), allowHigherVersion)) {
                return true;
            }
        }
    }

    return false;
}

bool Model::hasId(const QString &id) const
{
    return d->hasId(id);
}

bool Model::hasImport(const QString &importUrl) const
{
    return Utils::anyOf(imports(), [&](const Import &import) {
        return import.url() == importUrl;
    });
}

static QString firstCharToLower(const QString &string)
{
    QString resultString = string;

    if (!resultString.isEmpty())
        resultString[0] = resultString.at(0).toLower();

    return resultString;
}

QString Model::generateNewId(const QString &prefixName,
                             const QString &fallbackPrefix,
                             std::optional<std::function<bool(const QString &)>> isDuplicate) const
{
    // First try just the prefixName without number as postfix, then continue with 2 and further
    // as postfix until id does not already exist.
    // Properties of the root node are not allowed for ids, because they are available in the
    // complete context without qualification.

    int counter = 0;

    QString newBaseId = QString(QStringLiteral("%1")).arg(firstCharToLower(prefixName));
    newBaseId.remove(QRegularExpression(QStringLiteral("[^a-zA-Z0-9_]")));

    if (!newBaseId.isEmpty()) {
        QChar firstChar = newBaseId.at(0);
        if (firstChar.isDigit())
            newBaseId.prepend('_');
    } else {
        newBaseId = fallbackPrefix;
    }

    QString newId = newBaseId;

    if (!isDuplicate.has_value())
        isDuplicate = std::bind(&Model::hasId, this, std::placeholders::_1);

    while (!ModelNode::isValidId(newId) || isDuplicate.value()(newId)
           || d->rootNode()->hasProperty(newId.toUtf8())) {
        ++counter;
        newId = QString(QStringLiteral("%1%2")).arg(firstCharToLower(newBaseId)).arg(counter);
    }

    return newId;
}

// Generate a unique camelCase id from a name
// note: this methods does the same as generateNewId(). The 2 methods should be merged into one
QString Model::generateIdFromName(const QString &name, const QString &fallbackId) const
{
    QString newId;
    if (name.isEmpty()) {
        newId = fallbackId;
    } else {
        // convert to camel case
        QStringList nameWords = name.split(" ");
        nameWords[0] = nameWords[0].at(0).toLower() + nameWords[0].mid(1);
        for (int i = 1; i < nameWords.size(); ++i)
            nameWords[i] = nameWords[i].at(0).toUpper() + nameWords[i].mid(1);
        newId = nameWords.join("");

        // if id starts with a number prepend an underscore
        if (newId.at(0).isDigit())
            newId.prepend('_');
    }

    // If the new id is not valid (e.g. qml keyword match), try fixing it by prepending underscore
    if (!ModelNode::isValidId(newId))
        newId.prepend("_");

    QRegularExpression rgx("\\d+$"); // matches a number at the end of a string
    while (hasId(newId)) { // id exists
        QRegularExpressionMatch match = rgx.match(newId);
        if (match.hasMatch()) { // ends with a number, increment it
            QString numStr = match.captured();
            int num = numStr.toInt() + 1;
            newId = newId.mid(0, match.capturedStart()) + QString::number(num);
        } else {
            newId.append('1');
        }
    }

    return newId;
}

void Model::setActive3DSceneId(qint32 sceneId)
{
    auto activeSceneAux = d->rootNode()->auxiliaryData(active3dSceneProperty);
    if (activeSceneAux && activeSceneAux->toInt() == sceneId)
        return;

    d->rootNode()->setAuxiliaryData(active3dSceneProperty, sceneId);
    d->notifyActive3DSceneIdChanged(sceneId);
}

qint32 Model::active3DSceneId() const
{
    auto sceneId = d->rootNode()->auxiliaryData(active3dSceneProperty);
    if (sceneId)
        return sceneId->toInt();
    return -1;
}

void Model::startDrag(QMimeData *mimeData, const QPixmap &icon)
{
    d->notifyDragStarted(mimeData);

    auto drag = new QDrag(this);
    drag->setPixmap(icon);
    drag->setMimeData(mimeData);
    if (drag->exec() == Qt::IgnoreAction)
        endDrag();

    drag->deleteLater();
}

void Model::endDrag()
{
    d->notifyDragEnded();
}

NotNullPointer<const ProjectStorage<Sqlite::Database>> Model::projectStorage() const
{
    return d->projectStorage;
}

void ModelDeleter::operator()(class Model *model)
{
    model->detachAllViews();
    delete model;
}

void Model::detachAllViews()
{
    d->detachAllViews();
}

bool Model::isImportPossible(const Import &import, bool ignoreAlias, bool allowHigherVersion) const
{
    if (imports().contains(import))
        return true;

    if (!ignoreAlias)
        return false;

    for (const Import &possibleImport : possibleImports()) {
        if (possibleImport.isFileImport() && import.isFileImport()) {
            if (possibleImport.file() == import.file())
                return true;
        }

        if (possibleImport.isLibraryImport() && import.isLibraryImport()) {
            if (possibleImport.url() == import.url()
                && compareVersions(possibleImport.version(), import.version(), allowHigherVersion)) {
                return true;
            }
        }
    }

    return false;
}

QString Model::pathForImport(const Import &import)
{
    if (!rewriterView())
        return QString();

    return rewriterView()->pathForImport(import);
}

QStringList Model::importPaths() const
{
    if (rewriterView())
        return rewriterView()->importDirectories();

    QString documentDirectoryPath = QFileInfo(fileUrl().toLocalFile()).absolutePath();
    if (!documentDirectoryPath.isEmpty())
        return {documentDirectoryPath};

    return {};
}

Import Model::highestPossibleImport(const QString &importPath)
{
    Import candidate;

    for (const Import &import : possibleImports()) {
        if (import.url() == importPath) {
            if (candidate.isEmpty() || compareVersions(import.version(), candidate.version(), true))
                candidate = import;
        }
    }

    return candidate;
}

RewriterView *Model::rewriterView() const
{
    return d->rewriterView();
}

void Model::setRewriterView(RewriterView *rewriterView)
{
    d->setRewriterView(rewriterView);
}

const NodeInstanceView *Model::nodeInstanceView() const
{
    return d->nodeInstanceView();
}

void Model::setNodeInstanceView(NodeInstanceView *nodeInstanceView)
{
    d->setNodeInstanceView(nodeInstanceView);
}

/*!
 \brief Returns the model that is used for metainfo
 \return Returns itself if other metaInfoProxyModel does not exist
*/
Model *Model::metaInfoProxyModel() const
{
    if (d->m_metaInfoProxyModel)
        return d->m_metaInfoProxyModel->metaInfoProxyModel();

    return const_cast<Model *>(this);
}

TextModifier *Model::textModifier() const
{
    return d->m_textModifier.data();
}

void Model::setTextModifier(TextModifier *textModifier)
{
    d->m_textModifier = textModifier;
}

void Model::setDocumentMessages(const QList<DocumentMessage> &errors,
                                const QList<DocumentMessage> &warnings)
{
    d->setDocumentMessages(errors, warnings);
}

/*!
 * \brief Returns list of selected nodes for a view
 */
QList<ModelNode> Model::selectedNodes(AbstractView *view) const
{
    return d->toModelNodeList(d->selectedNodes(), view);
}

void Model::clearMetaInfoCache()
{
    d->m_nodeMetaInfoCache.clear();
}

/*!
  \brief Returns the URL against which relative URLs within the model should be resolved.
  \return The base URL.
  */
QUrl Model::fileUrl() const
{
    return d->fileUrl();
}

/*!
  \brief Sets the URL against which relative URLs within the model should be resolved.
  \param url the base URL, i.e. the qml file path.
  */
void Model::setFileUrl(const QUrl &url)
{
    Q_ASSERT(url.isValid() && url.isLocalFile());
    Internal::WriteLocker locker(d.get());
    d->setFileUrl(url);
}

/*!
  \brief Returns list of QML types available within the model.
  */
const MetaInfo Model::metaInfo() const
{
    return d->metaInfo();
}

bool Model::hasNodeMetaInfo(const TypeName &typeName, int majorVersion, int minorVersion) const
{
    return metaInfo(typeName, majorVersion, minorVersion).isValid();
}

void Model::setMetaInfo(const MetaInfo &metaInfo)
{
    d->setMetaInfo(metaInfo);
}

template<const auto &moduleName, const auto &typeName>
NodeMetaInfo Model::createNodeMetaInfo() const
{
    auto typeId = d->projectStorage->commonTypeCache.typeId<moduleName, typeName>();

    return {typeId, d->projectStorage};
}

NodeMetaInfo Model::fontMetaInfo() const
{
    if constexpr (useProjectStorage()) {
        using namespace Storage::Info;
        return createNodeMetaInfo<QtQuick, font>();
    } else {
        return metaInfo("QtQuick.font");
    }
}

NodeMetaInfo Model::qtQuickItemMetaInfo() const
{
    if constexpr (useProjectStorage()) {
        using namespace Storage::Info;
        return createNodeMetaInfo<QtQuick, Item>();
    } else {
        return metaInfo("QtQuick.Item");
    }
}

NodeMetaInfo Model::qtQuickRectangleMetaInfo() const
{
    if constexpr (useProjectStorage()) {
        using namespace Storage::Info;
        return createNodeMetaInfo<QtQuick, Rectangle>();
    } else {
        return metaInfo("QtQuick.Rectangle");
    }
}

NodeMetaInfo Model::qtQuickImageMetaInfo() const
{
    if constexpr (useProjectStorage()) {
        using namespace Storage::Info;
        return createNodeMetaInfo<QtQuick, Image>();
    } else {
        return metaInfo("QtQuick.Image");
    }
}

NodeMetaInfo Model::qtQuickTextMetaInfo() const
{
    if constexpr (useProjectStorage()) {
        using namespace Storage::Info;
        return createNodeMetaInfo<QtQuick, Text>();
    } else {
        return metaInfo("QtQuick.Text");
    }
}

NodeMetaInfo Model::qtQuickPropertyAnimationMetaInfo() const
{
    if constexpr (useProjectStorage()) {
        using namespace Storage::Info;
        return createNodeMetaInfo<QtQuick, PropertyAnimation>();
    } else {
        return metaInfo("QtQuick.PropertyAnimation");
    }
}

NodeMetaInfo Model::flowViewFlowDecisionMetaInfo() const
{
    if constexpr (useProjectStorage()) {
        using namespace Storage::Info;
        return createNodeMetaInfo<FlowView, FlowDecision>();
    } else {
        return metaInfo("FlowView.FlowDecision");
    }
}

NodeMetaInfo Model::flowViewFlowWildcardMetaInfo() const
{
    if constexpr (useProjectStorage()) {
        using namespace Storage::Info;
        return createNodeMetaInfo<FlowView, FlowWildcard>();
    } else {
        return metaInfo("FlowView.FlowWildcard");
    }
}

NodeMetaInfo Model::flowViewFlowTransitionMetaInfo() const
{
    if constexpr (useProjectStorage()) {
        using namespace Storage::Info;
        return createNodeMetaInfo<FlowView, FlowTransition>();
    } else {
        return metaInfo("FlowView.FlowTransition");
    }
}

NodeMetaInfo Model::qtQuickTextEditMetaInfo() const
{
    if constexpr (useProjectStorage()) {
        using namespace Storage::Info;
        return createNodeMetaInfo<QtQuick, TextEdit>();
    } else {
        return metaInfo("QtQuick.TextEdit");
    }
}

NodeMetaInfo Model::qtQuickControlsTextAreaMetaInfo() const
{
    if constexpr (useProjectStorage()) {
        using namespace Storage::Info;
        return createNodeMetaInfo<QtQuick_Controls, TextArea>();
    } else {
        return metaInfo("QtQuick.Controls.TextArea");
    }
}

NodeMetaInfo Model::qtQuick3DNodeMetaInfo() const
{
    if constexpr (useProjectStorage()) {
        using namespace Storage::Info;
        return createNodeMetaInfo<QtQuick3D, Node>();
    } else {
        return metaInfo("QtQuick3D.Node");
    }
}

NodeMetaInfo Model::qtQuick3DTextureMetaInfo() const
{
    if constexpr (useProjectStorage()) {
        using namespace Storage::Info;
        return createNodeMetaInfo<QtQuick3D, Texture>();
    } else {
        return metaInfo("QtQuick3D.Texture");
    }
}

NodeMetaInfo Model::qtQuick3DBakedLightmapMetaInfo() const
{
    if constexpr (useProjectStorage()) {
        using namespace Storage::Info;
        return createNodeMetaInfo<QtQuick3D, BakedLightmap>();
    } else {
        return metaInfo("QtQuick3D.BakedLightmap");
    }
}

NodeMetaInfo Model::qtQuick3DMaterialMetaInfo() const
{
    if constexpr (useProjectStorage()) {
        using namespace Storage::Info;
        return createNodeMetaInfo<QtQuick3D, Material>();
    } else {
        return metaInfo("QtQuick3D.Material");
    }
}

NodeMetaInfo Model::qtQuick3DDefaultMaterialMetaInfo() const
{
    if constexpr (useProjectStorage()) {
        using namespace Storage::Info;
        return createNodeMetaInfo<QtQuick3D, DefaultMaterial>();
    } else {
        return metaInfo("QtQuick3D.DefaultMaterial");
    }
}

NodeMetaInfo Model::qtQuick3DPrincipledMaterialMetaInfo() const
{
    if constexpr (useProjectStorage()) {
        using namespace Storage::Info;
        return createNodeMetaInfo<QtQuick3D, PrincipledMaterial>();
    } else {
        return metaInfo("QtQuick3D.PrincipledMaterial");
    }
}

NodeMetaInfo Model::qtQuickTimelineTimelineMetaInfo() const
{
    if constexpr (useProjectStorage()) {
        using namespace Storage::Info;
        return createNodeMetaInfo<QtQuick_Timeline, Timeline>();
    } else {
        return metaInfo("QtQuick.Timeline.Timeline");
    }
}

NodeMetaInfo Model::qtQuickConnectionsMetaInfo() const
{
    if constexpr (useProjectStorage()) {
        using namespace Storage::Info;
        return createNodeMetaInfo<QtQuick, Connections>();
    } else {
        return metaInfo("QtQuick.Connections");
    }
}

NodeMetaInfo Model::qtQuickStateGroupMetaInfo() const
{
    if constexpr (useProjectStorage()) {
        using namespace Storage::Info;
        return createNodeMetaInfo<QtQuick, StateGroup>();
    } else {
        return metaInfo("QtQuick.StateGroup");
    }
}

NodeMetaInfo Model::vector2dMetaInfo() const
{
    if constexpr (useProjectStorage()) {
        using namespace Storage::Info;
        return createNodeMetaInfo<QtQuick, vector2d>();
    } else {
        return metaInfo("QtQuick.vector2d");
    }
}

NodeMetaInfo Model::vector3dMetaInfo() const
{
    if constexpr (useProjectStorage()) {
        using namespace Storage::Info;
        return createNodeMetaInfo<QtQuick, vector3d>();
    } else {
        return metaInfo("QtQuick.vector3d");
    }
}

NodeMetaInfo Model::vector4dMetaInfo() const
{
    if constexpr (useProjectStorage()) {
        using namespace Storage::Info;
        return createNodeMetaInfo<QtQuick, vector4d>();
    } else {
        return metaInfo("QtQuick.vector4d");
    }
}

NodeMetaInfo Model::qtQuickTimelineKeyframeGroupMetaInfo() const
{
    if constexpr (useProjectStorage()) {
        using namespace Storage::Info;
        return createNodeMetaInfo<QtQuick_Timeline, KeyframeGroup>();
    } else {
        return metaInfo("QtQuick.Timeline.KeyframeGroup");
    }
}

NodeMetaInfo Model::qtQuick3DModelMetaInfo() const
{
    if constexpr (useProjectStorage()) {
        using namespace Storage::Info;
        return createNodeMetaInfo<QtQuick3D, Storage::Info::Model>();
    } else {
        return metaInfo("QtQuick3D.Model");
    }
}

namespace {
[[maybe_unused]] std::pair<Utils::SmallStringView, Utils::SmallStringView> moduleTypeName(
    const TypeName &typeName)
{
    auto foundDot = std::find(typeName.begin(), typeName.end(), '.');

    if (foundDot == typeName.end())
        return {"", typeName};

    return {{typeName.begin(), foundDot}, {std::next(foundDot), typeName.end()}};
}
} // namespace

NodeMetaInfo Model::metaInfo(const TypeName &typeName, int majorVersion, int minorVersion) const
{
    if constexpr (useProjectStorage()) {
        auto [module, componentName] = moduleTypeName(typeName);

        ModuleId moduleId = d->projectStorage->moduleId(module);
        TypeId typeId = d->projectStorage->typeId(moduleId,
                                                  componentName,
                                                  Storage::Synchronization::Version{majorVersion,
                                                                                    minorVersion});
        return NodeMetaInfo(typeId, d->projectStorage);
    } else {
        return NodeMetaInfo(metaInfoProxyModel(), typeName, majorVersion, minorVersion);
    }
}

/*!
  \brief Returns list of QML types available within the model.
  */
MetaInfo Model::metaInfo()
{
    return d->metaInfo();
}

/*! \name View related functions
*/
//\{
/*!
\brief Attaches a view to the model

Registers a "view" that from then on will be informed about changes to the model. Different views
will always be informed in the order in which they registered to the model.

The view is informed that it has been registered within the model by a call to AbstractView::modelAttached .

\param view The view object to register. Must be not null.
\see detachView
*/
void Model::attachView(AbstractView *view)
{
    //    Internal::WriteLocker locker(d);
    auto castedRewriterView = qobject_cast<RewriterView *>(view);
    if (castedRewriterView) {
        if (rewriterView() == castedRewriterView)
            return;

        setRewriterView(castedRewriterView);
        return;
    }

    auto nodeInstanceView = qobject_cast<NodeInstanceView *>(view);
    if (nodeInstanceView)
        return;

    d->attachView(view);
}

/*!
\brief Detaches a view to the model

\param view The view to unregister. Must be not null.
\param emitDetachNotify If set to NotifyView (the default), AbstractView::modelAboutToBeDetached() will be called

\see attachView
*/
void Model::detachView(AbstractView *view, ViewNotification emitDetachNotify)
{
    //    Internal::WriteLocker locker(d);
    bool emitNotify = (emitDetachNotify == NotifyView);

    auto rewriterView = qobject_cast<RewriterView *>(view);
    if (rewriterView)
        return;

    auto nodeInstanceView = qobject_cast<NodeInstanceView *>(view);
    if (nodeInstanceView)
        return;

    d->detachView(view, emitNotify);
}

} // namespace QmlDesigner
