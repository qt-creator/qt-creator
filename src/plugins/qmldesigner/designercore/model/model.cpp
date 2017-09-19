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

#include "model.h"
#include "model_p.h"
#include <modelnode.h>
#include "internalnode_p.h"
#include "invalidpropertyexception.h"
#include "invalidargumentexception.h"

#include <QPointer>
#include <QFileInfo>
#include <QHashIterator>

#include <utils/algorithm.h>

#include "abstractview.h"
#include "nodeinstanceview.h"
#include "metainfo.h"
#include "nodemetainfo.h"
#include "internalproperty.h"
#include "internalnodelistproperty.h"
#include "internalsignalhandlerproperty.h"
#include "internalnodeabstractproperty.h"
#include "invalidmodelnodeexception.h"

#include "abstractproperty.h"
#include "variantproperty.h"
#include "bindingproperty.h"
#include "signalhandlerproperty.h"
#include "nodeabstractproperty.h"
#include "nodelistproperty.h"
#include "rewriterview.h"
#include "rewritingexception.h"
#include "invalididexception.h"
#include "textmodifier.h"

#include <qmljs/qmljsmodelmanagerinterface.h>

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

ModelPrivate::ModelPrivate(Model *model) :
        m_q(model),
        m_writeLock(false),
        m_internalIdCounter(1)
{
    m_rootInternalNode = createNode("QtQuick.Item", 1, 0, PropertyListType(), PropertyListType(), QString(), ModelNode::NodeWithoutSource,true);
    m_currentStateNode = m_rootInternalNode;
    m_currentTimelineMutatorNode = m_rootInternalNode;
}

ModelPrivate::~ModelPrivate()
{
    detachAllViews();
}

void ModelPrivate::detachAllViews()
{
    foreach (const QPointer<AbstractView> &view, m_viewList)
        detachView(view.data(), true);

    m_viewList.clear();

    if (m_nodeInstanceView) {
        m_nodeInstanceView->modelAboutToBeDetached(m_q);
        m_nodeInstanceView.clear();
    }

    if (m_rewriterView) {
        m_rewriterView->modelAboutToBeDetached(m_q);
        m_rewriterView.clear();
    }
}

Model *ModelPrivate::create(const TypeName &type, int major, int minor, Model *metaInfoPropxyModel)
{
    Model *model = new Model;

    model->d->m_metaInfoProxyModel = metaInfoPropxyModel;
    model->d->rootNode()->setType(type);
    model->d->rootNode()->setMajorVersion(major);
    model->d->rootNode()->setMinorVersion(minor);

    return model;
}

void ModelPrivate::changeImports(const QList<Import> &toBeAddedImportList, const QList<Import> &toBeRemovedImportList)
{
    QList<Import> removedImportList;
    foreach (const Import &import, toBeRemovedImportList) {
        if (m_imports.contains(import)) {
            removedImportList.append(import);
            m_imports.removeOne(import);
        }
    }

    QList<Import> addedImportList;
    foreach (const Import &import, toBeAddedImportList) {
        if (!m_imports.contains(import)) {
            addedImportList.append(import);
            m_imports.append(import);
        }
    }

    if (!removedImportList.isEmpty() || !addedImportList.isEmpty())
        notifyImportsChanged(addedImportList, removedImportList);
}

void ModelPrivate::notifyImportsChanged(const QList<Import> &addedImports, const QList<Import> &removedImports)
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

    NodeMetaInfo::clearCache();

    if (nodeInstanceView())
        nodeInstanceView()->importsChanged(addedImports, removedImports);

    foreach (const QPointer<AbstractView> &view, m_viewList)
        view->importsChanged(addedImports, removedImports);

    if (resetModel)
        resetModelByRewriter(description);
}

QUrl ModelPrivate::fileUrl() const
{
    return m_fileUrl;
}

void ModelPrivate::setDocumentMessages(const QList<DocumentMessage> &errors, const QList<DocumentMessage> &warnings)
{
    foreach (const QPointer<AbstractView> &view, m_viewList)
        view->documentMessagesChanged(errors, warnings);
}

void ModelPrivate::setFileUrl(const QUrl &fileUrl)
{
    QUrl oldPath = m_fileUrl;

    if (oldPath != fileUrl) {
        m_fileUrl = fileUrl;

        foreach (const QPointer<AbstractView> &view, m_viewList)
            view->fileUrlChanged(oldPath, fileUrl);
    }
}

void ModelPrivate::changeNodeType(const InternalNodePointer &internalNodePointer, const TypeName &typeName, int majorVersion, int minorVersion)
{
    internalNodePointer->setType(typeName);
    internalNodePointer->setMajorVersion(majorVersion);
    internalNodePointer->setMinorVersion(minorVersion);

    try {
        notifyNodeTypeChanged(internalNodePointer, typeName, majorVersion, minorVersion);

    } catch (const RewritingException &e) {
        throw InvalidArgumentException(__LINE__, __FUNCTION__, __FILE__, e.description().toUtf8());

    }

}

InternalNode::Pointer ModelPrivate::createNode(const TypeName &typeName,
                                               int majorVersion,
                                               int minorVersion,
                                               const QList<QPair<PropertyName, QVariant> > &propertyList,
                                               const QList<QPair<PropertyName, QVariant> > &auxPropertyList,
                                               const QString &nodeSource,
                                               ModelNode::NodeSourceType nodeSourceType,
                                               bool isRootNode)
{
    if (typeName.isEmpty())
        throw InvalidArgumentException(__LINE__, __FUNCTION__, __FILE__, tr("invalid type").toUtf8());

    qint32 internalId = 0;

    if (!isRootNode)
        internalId = m_internalIdCounter++;

    InternalNode::Pointer newInternalNodePointer = InternalNode::create(typeName, majorVersion, minorVersion, internalId);
    newInternalNodePointer->setNodeSourceType(nodeSourceType);

    typedef QPair<PropertyName, QVariant> PropertyPair;

    foreach (const PropertyPair &propertyPair, propertyList) {
        newInternalNodePointer->addVariantProperty(propertyPair.first);
        newInternalNodePointer->variantProperty(propertyPair.first)->setValue(propertyPair.second);
    }

    foreach (const PropertyPair &propertyPair, auxPropertyList) {
        newInternalNodePointer->setAuxiliaryData(propertyPair.first, propertyPair.second);
    }

    m_nodeSet.insert(newInternalNodePointer);
    m_internalIdNodeHash.insert(newInternalNodePointer->internalId(), newInternalNodePointer);

    if (!nodeSource.isNull())
        newInternalNodePointer->setNodeSource(nodeSource);

    notifyNodeCreated(newInternalNodePointer);

    if (!newInternalNodePointer->propertyNameList().isEmpty())
        notifyVariantPropertiesChanged(newInternalNodePointer, newInternalNodePointer->propertyNameList(), AbstractView::PropertiesAdded);

    return newInternalNodePointer;
}

void ModelPrivate::removeNodeFromModel(const InternalNodePointer &internalNodePointer)
{
    Q_ASSERT(!internalNodePointer.isNull());

    internalNodePointer->resetParentProperty();

    if (!internalNodePointer->id().isEmpty())
        m_idNodeHash.remove(internalNodePointer->id());
    internalNodePointer->setValid(false);
    m_nodeSet.remove(internalNodePointer);
    m_internalIdNodeHash.remove(internalNodePointer->internalId());
}

void ModelPrivate::removeAllSubNodes(const InternalNode::Pointer &internalNodePointer)
{
    foreach (const InternalNodePointer &subNode, internalNodePointer->allSubNodes()) {
        removeNodeFromModel(subNode);
    }
}

void ModelPrivate::removeNode(const InternalNode::Pointer &internalNodePointer)
{
    Q_ASSERT(!internalNodePointer.isNull());

    AbstractView::PropertyChangeFlags propertyChangeFlags = AbstractView::NoAdditionalChanges;

    notifyNodeAboutToBeRemoved(internalNodePointer);

    InternalNodeAbstractProperty::Pointer oldParentProperty(internalNodePointer->parentProperty());

    removeAllSubNodes(internalNodePointer);
    removeNodeFromModel(internalNodePointer);

    InternalNode::Pointer parentNode;
    PropertyName parentPropertyName;
    if (oldParentProperty) {
        parentNode = oldParentProperty->propertyOwner();
        parentPropertyName = oldParentProperty->name();
    }

    if (oldParentProperty && oldParentProperty->isEmpty()) {
        removePropertyWithoutNotification(oldParentProperty);

        propertyChangeFlags |= AbstractView::EmptyPropertiesRemoved;
    }

    notifyNodeRemoved(internalNodePointer, parentNode, parentPropertyName, propertyChangeFlags);
}

InternalNode::Pointer ModelPrivate::rootNode() const
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

void ModelPrivate::changeNodeId(const InternalNode::Pointer& internalNodePointer, const QString &id)
{
    const QString oldId = internalNodePointer->id();

    internalNodePointer->setId(id);
    if (!oldId.isEmpty())
        m_idNodeHash.remove(oldId);
    if (!id.isEmpty())
        m_idNodeHash.insert(id, internalNodePointer);

    try {
        notifyNodeIdChanged(internalNodePointer, id, oldId);

    } catch (const RewritingException &e) {
        throw InvalidIdException(__LINE__, __FUNCTION__, __FILE__, id.toUtf8(), e.description().toUtf8());

    }
}

void ModelPrivate::checkPropertyName(const PropertyName &propertyName)
{
    if (propertyName.isEmpty()) {
        Q_ASSERT_X(propertyName.isEmpty(), Q_FUNC_INFO, "empty property name");
        throw InvalidPropertyException(__LINE__, __FUNCTION__, __FILE__, "<empty property name>");
    }

    if (propertyName == "id") {
        Q_ASSERT_X(propertyName != "id", Q_FUNC_INFO, "cannot add property id");
        throw InvalidPropertyException(__LINE__, __FUNCTION__, __FILE__, propertyName);
    }
}

void ModelPrivate::notifyAuxiliaryDataChanged(const InternalNodePointer &internalNode, const PropertyName &name, const QVariant &data)
{
    bool resetModel = false;
    QString description;

    try {
        if (rewriterView()) {
            ModelNode node(internalNode, model(), rewriterView());
            rewriterView()->auxiliaryDataChanged(node, name, data);
        }
    } catch (const RewritingException &e) {
        description = e.description();
        resetModel = true;
    }

    foreach (const QPointer<AbstractView> &view, m_viewList) {
        Q_ASSERT(view != 0);
        ModelNode node(internalNode, model(), view.data());
        view->auxiliaryDataChanged(node, name, data);

    }

    if (nodeInstanceView()) {
        ModelNode node(internalNode, model(), nodeInstanceView());
        nodeInstanceView()->auxiliaryDataChanged(node, name, data);
    }

    if (resetModel)
        resetModelByRewriter(description);
}

void ModelPrivate::notifyNodeSourceChanged(const InternalNodePointer &internalNode, const QString &newNodeSource)
{
    bool resetModel = false;
    QString description;

    try {
        if (rewriterView()) {
            ModelNode node(internalNode, model(), rewriterView());
            rewriterView()->nodeSourceChanged(node, newNodeSource);
        }
    } catch (const RewritingException &e) {
        description = e.description();
        resetModel = true;
    }

    foreach (const QPointer<AbstractView> &view, m_viewList) {
        Q_ASSERT(view != 0);
        ModelNode node(internalNode, model(), view.data());
        view->nodeSourceChanged(node, newNodeSource);

    }

    if (nodeInstanceView()) {
        ModelNode node(internalNode, model(), nodeInstanceView());
        nodeInstanceView()->nodeSourceChanged(node, newNodeSource);
    }

    if (resetModel)
        resetModelByRewriter(description);
}

void ModelPrivate::notifyRootNodeTypeChanged(const QString &type, int majorVersion, int minorVersion)
{
    bool resetModel = false;
    QString description;

    try {
        if (rewriterView())
            rewriterView()->rootNodeTypeChanged(type, majorVersion, minorVersion);
    } catch (const RewritingException &e) {
        description = e.description();
        resetModel = true;
    }

    if (nodeInstanceView())
        nodeInstanceView()->rootNodeTypeChanged(type, majorVersion, minorVersion);

    foreach (const QPointer<AbstractView> &view, m_viewList) {
        Q_ASSERT(view != 0);
        view->rootNodeTypeChanged(type, majorVersion, minorVersion);

    }

    if (resetModel)
        resetModelByRewriter(description);
}

void ModelPrivate::notifyInstancePropertyChange(const QList<QPair<ModelNode, PropertyName> > &propertyPairList)
{
    // no need to notify the rewriter or the instance view

    typedef QPair<ModelNode, PropertyName> ModelNodePropertyPair;
    foreach (const QPointer<AbstractView> &view, m_viewList) {
        Q_ASSERT(view != 0);

        QList<QPair<ModelNode, PropertyName> > adaptedPropertyList;
        foreach (const ModelNodePropertyPair &propertyPair, propertyPairList) {
            ModelNodePropertyPair newPair(ModelNode(propertyPair.first.internalNode(), model(), view.data()), propertyPair.second);
            adaptedPropertyList.append(newPair);
        }

        view->instancePropertyChanged(adaptedPropertyList);
    }
}

void ModelPrivate::notifyInstanceErrorChange(const QVector<qint32> &instanceIds)
{
    // no need to notify the rewriter or the instance view

    QVector<ModelNode> errorNodeList;
    foreach (const QPointer<AbstractView> &view, m_viewList) {
        Q_ASSERT(view != 0);
        foreach (qint32 instanceId, instanceIds)
            errorNodeList.append(ModelNode(model()->d->nodeForInternalId(instanceId), model(), view));
        view->instanceErrorChanged(errorNodeList);
    }
}

void ModelPrivate::notifyInstancesCompleted(const QVector<ModelNode> &nodeVector)
{
    bool resetModel = false;
    QString description;

    QVector<Internal::InternalNode::Pointer> internalVector(toInternalNodeVector(nodeVector));

    try {
        if (rewriterView())
            rewriterView()->instancesCompleted(toModelNodeVector(internalVector, rewriterView()));
    } catch (const RewritingException &e) {
        description = e.description();
        resetModel = true;
    }

    foreach (const QPointer<AbstractView> &view, m_viewList) {
        Q_ASSERT(view != 0);
        view->instancesCompleted(toModelNodeVector(internalVector, view.data()));
    }

    if (nodeInstanceView())
        nodeInstanceView()->instancesCompleted(toModelNodeVector(internalVector, nodeInstanceView()));

    if (resetModel)
        resetModelByRewriter(description);
}

QMultiHash<ModelNode, InformationName> convertModelNodeInformationHash(const QMultiHash<ModelNode, InformationName> &informationChangeHash, AbstractView *view)
{
    QMultiHash<ModelNode, InformationName>  convertedModelNodeInformationHash;

    QHashIterator<ModelNode, InformationName> hashIterator(informationChangeHash);
    while (hashIterator.hasNext()) {
        hashIterator.next();
        convertedModelNodeInformationHash.insert(ModelNode(hashIterator.key(), view), hashIterator.value());
    }

    return convertedModelNodeInformationHash;
}

void ModelPrivate::notifyInstancesInformationsChange(const QMultiHash<ModelNode, InformationName> &informationChangeHash)
{
    bool resetModel = false;
    QString description;

    try {
        if (rewriterView())
            rewriterView()->instanceInformationsChanged(convertModelNodeInformationHash(informationChangeHash, rewriterView()));
    } catch (const RewritingException &e) {
        description = e.description();
        resetModel = true;
    }

    foreach (const QPointer<AbstractView> &view, m_viewList) {
        Q_ASSERT(view != 0);
        view->instanceInformationsChanged(convertModelNodeInformationHash(informationChangeHash, view.data()));
    }

    if (nodeInstanceView())
        nodeInstanceView()->instanceInformationsChanged(convertModelNodeInformationHash(informationChangeHash, nodeInstanceView()));

    if (resetModel)
        resetModelByRewriter(description);
}

void ModelPrivate::notifyInstancesRenderImageChanged(const QVector<ModelNode> &nodeVector)
{
    bool resetModel = false;
    QString description;

    QVector<Internal::InternalNode::Pointer> internalVector(toInternalNodeVector(nodeVector));

    try {
        if (rewriterView())
            rewriterView()->instancesRenderImageChanged(toModelNodeVector(internalVector, rewriterView()));
    } catch (const RewritingException &e) {
        description = e.description();
        resetModel = true;
    }

    foreach (const QPointer<AbstractView> &view, m_viewList) {
        Q_ASSERT(view != 0);
        view->instancesRenderImageChanged(toModelNodeVector(internalVector, view.data()));
    }

    if (nodeInstanceView())
        nodeInstanceView()->instancesRenderImageChanged(toModelNodeVector(internalVector, nodeInstanceView()));

    if (resetModel)
        resetModelByRewriter(description);
}

void ModelPrivate::notifyInstancesPreviewImageChanged(const QVector<ModelNode> &nodeVector)
{
    bool resetModel = false;
    QString description;

    QVector<Internal::InternalNode::Pointer> internalVector(toInternalNodeVector(nodeVector));

    try {
        if (rewriterView())
            rewriterView()->instancesPreviewImageChanged(toModelNodeVector(internalVector, rewriterView()));
    } catch (const RewritingException &e) {
        description = e.description();
        resetModel = true;
    }

    foreach (const QPointer<AbstractView> &view, m_viewList) {
        Q_ASSERT(view != 0);
        view->instancesPreviewImageChanged(toModelNodeVector(internalVector, view.data()));
    }

    if (nodeInstanceView())
        nodeInstanceView()->instancesPreviewImageChanged(toModelNodeVector(internalVector, nodeInstanceView()));

    if (resetModel)
        resetModelByRewriter(description);
}

void ModelPrivate::notifyInstancesChildrenChanged(const QVector<ModelNode> &nodeVector)
{
    bool resetModel = false;
    QString description;

    QVector<Internal::InternalNode::Pointer> internalVector(toInternalNodeVector(nodeVector));

    try {
        if (rewriterView())
            rewriterView()->instancesChildrenChanged(toModelNodeVector(internalVector, rewriterView()));
    } catch (const RewritingException &e) {
        description = e.description();
        resetModel = true;
    }

    foreach (const QPointer<AbstractView> &view, m_viewList) {
        Q_ASSERT(view != 0);
        view->instancesChildrenChanged(toModelNodeVector(internalVector, view.data()));
    }

    if (nodeInstanceView())
        nodeInstanceView()->instancesChildrenChanged(toModelNodeVector(internalVector, nodeInstanceView()));

    if (resetModel)
        resetModelByRewriter(description);
}

void ModelPrivate::notifyCurrentStateChanged(const ModelNode &node)
{
    bool resetModel = false;
    QString description;

    m_currentStateNode = node.internalNode();

    try {
        if (rewriterView())
            rewriterView()->currentStateChanged(ModelNode(node.internalNode(), model(), rewriterView()));
    } catch (const RewritingException &e) {
        description = e.description();
        resetModel = true;
    }

    foreach (const QPointer<AbstractView> &view, m_viewList) {
        Q_ASSERT(view != 0);
        view->currentStateChanged(ModelNode(node.internalNode(), model(), view.data()));
    }

    if (nodeInstanceView())
        nodeInstanceView()->currentStateChanged(ModelNode(node.internalNode(), model(), nodeInstanceView()));

    if (resetModel)
        resetModelByRewriter(description);
}

void ModelPrivate::notifyCurrentTimelineChanged(const ModelNode &node)
{
    bool resetModel = false;
    QString description;

    m_currentTimelineMutatorNode = node.internalNode();

    try {
        if (rewriterView())
            rewriterView()->currentTimelineChanged(ModelNode(node.internalNode(), model(), rewriterView()));
    } catch (const RewritingException &e) {
        description = e.description();
        resetModel = true;
    }

    for (const QPointer<AbstractView> &view : m_viewList) {
        Q_ASSERT(view != 0);
        view->currentTimelineChanged(ModelNode(node.internalNode(), model(), view.data()));
    }

    if (nodeInstanceView())
        nodeInstanceView()->currentTimelineChanged(ModelNode(node.internalNode(), model(), nodeInstanceView()));

    if (resetModel)
        resetModelByRewriter(description);
}

void ModelPrivate::notifyRewriterBeginTransaction()
{
    bool resetModel = false;
    QString description;

    try {
        if (rewriterView())
            rewriterView()->rewriterBeginTransaction();
    } catch (const RewritingException &e) {
        description = e.description();
        resetModel = true;
    }

    foreach (const QPointer<AbstractView> &view, m_viewList) {
        Q_ASSERT(view != 0);
        view->rewriterBeginTransaction();
    }

    if (nodeInstanceView())
        nodeInstanceView()->rewriterBeginTransaction();

    if (resetModel)
        resetModelByRewriter(description);
}

void ModelPrivate::notifyRewriterEndTransaction()
{
    bool resetModel = false;
    QString description;

    try {
        if (rewriterView())
            rewriterView()->rewriterEndTransaction();
    } catch (const RewritingException &e) {
        description = e.description();
        resetModel = true;
    }

    foreach (const QPointer<AbstractView> &view, m_viewList) {
        Q_ASSERT(view != 0);
        view->rewriterEndTransaction();
    }

    if (nodeInstanceView())
        nodeInstanceView()->rewriterEndTransaction();

    if (resetModel)
        resetModelByRewriter(description);
}

void ModelPrivate::notifyInstanceToken(const QString &token, int number, const QVector<ModelNode> &nodeVector)
{
    bool resetModel = false;
    QString description;

    QVector<Internal::InternalNode::Pointer> internalVector(toInternalNodeVector(nodeVector));


    try {
        if (rewriterView())
            rewriterView()->instancesToken(token, number, toModelNodeVector(internalVector, rewriterView()));
    } catch (const RewritingException &e) {
        description = e.description();
        resetModel = true;
    }

    foreach (const QPointer<AbstractView> &view, m_viewList) {
        Q_ASSERT(view != 0);
        view->instancesToken(token, number, toModelNodeVector(internalVector, view.data()));
    }

    if (nodeInstanceView())
        nodeInstanceView()->instancesToken(token, number, toModelNodeVector(internalVector, nodeInstanceView()));

    if (resetModel)
        resetModelByRewriter(description);
}

void ModelPrivate::notifyCustomNotification(const AbstractView *senderView, const QString &identifier, const QList<ModelNode> &nodeList, const QList<QVariant> &data)
{
    bool resetModel = false;
    QString description;

    QList<Internal::InternalNode::Pointer> internalList(toInternalNodeList(nodeList));

    try {
        if (rewriterView())
            rewriterView()->customNotification(senderView, identifier, toModelNodeList(internalList, rewriterView()), data);
    } catch (const RewritingException &e) {
        description = e.description();
        resetModel = true;
    }

    foreach (const QPointer<AbstractView> &view, m_viewList) {
        Q_ASSERT(view != 0);
        view->customNotification(senderView, identifier, toModelNodeList(internalList, view.data()), data);
    }

    if (nodeInstanceView())
        nodeInstanceView()->customNotification(senderView, identifier, toModelNodeList(internalList, nodeInstanceView()), data);

    if (resetModel)
        resetModelByRewriter(description);
}


void ModelPrivate::notifyPropertiesRemoved(const QList<PropertyPair> &propertyPairList)
{
    bool resetModel = false;
    QString description;

    try {
        if (rewriterView()) {
            QList<AbstractProperty> propertyList;
            foreach (const PropertyPair &propertyPair, propertyPairList) {
                AbstractProperty newProperty(propertyPair.second, propertyPair.first, model(), rewriterView());
                propertyList.append(newProperty);
            }

            rewriterView()->propertiesRemoved(propertyList);
        }
    } catch (const RewritingException &e) {
        description = e.description();
        resetModel = true;
    }

    if (nodeInstanceView()) {
        QList<AbstractProperty> propertyList;
        foreach (const PropertyPair &propertyPair, propertyPairList) {
            AbstractProperty newProperty(propertyPair.second, propertyPair.first, model(), nodeInstanceView());
            propertyList.append(newProperty);
        }

        nodeInstanceView()->propertiesRemoved(propertyList);
    }

    foreach (const QPointer<AbstractView> &view, m_viewList) {
        QList<AbstractProperty> propertyList;
        Q_ASSERT(view != 0);
        foreach (const PropertyPair &propertyPair, propertyPairList) {
            AbstractProperty newProperty(propertyPair.second, propertyPair.first, model(), view.data());
            propertyList.append(newProperty);
        }

        view->propertiesRemoved(propertyList);
    }

    if (resetModel)
        resetModelByRewriter(description);
}

void ModelPrivate::notifyPropertiesAboutToBeRemoved(const QList<InternalProperty::Pointer> &internalPropertyList)
{
    bool resetModel = false;
    QString description;

    try {
        if (rewriterView()) {
            QList<AbstractProperty> propertyList;
            foreach (const InternalProperty::Pointer &property, internalPropertyList) {
                AbstractProperty newProperty(property->name(), property->propertyOwner(), model(), rewriterView());
                propertyList.append(newProperty);
            }

            rewriterView()->propertiesAboutToBeRemoved(propertyList);
        }
    } catch (const RewritingException &e) {
        description = e.description();
        resetModel = true;
    }

    foreach (const QPointer<AbstractView> &view, m_viewList) {
        QList<AbstractProperty> propertyList;
        Q_ASSERT(view != 0);
        foreach (const InternalProperty::Pointer &property, internalPropertyList) {
            AbstractProperty newProperty(property->name(), property->propertyOwner(), model(), view.data());
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
        foreach (const InternalProperty::Pointer &property, internalPropertyList) {
            AbstractProperty newProperty(property->name(), property->propertyOwner(), model(), nodeInstanceView());
            propertyList.append(newProperty);
        }

        nodeInstanceView()->propertiesAboutToBeRemoved(propertyList);
    }

    if (resetModel)
        resetModelByRewriter(description);
}

void ModelPrivate::setAuxiliaryData(const InternalNode::Pointer& node, const PropertyName &name, const QVariant &data)
{
    if (data.isValid())
        node->setAuxiliaryData(name, data);
    else
        node->removeAuxiliaryData(name);

    notifyAuxiliaryDataChanged(node, name,data);
}

void ModelPrivate::resetModelByRewriter(const QString &description)
{
    if (rewriterView())
        rewriterView()->resetToLastCorrectQml();

    throw RewritingException(__LINE__, __FUNCTION__, __FILE__, description.toUtf8(), rewriterView()->textModifierContent());
}


void ModelPrivate::attachView(AbstractView *view)
{
    Q_ASSERT(view);

    if (m_viewList.contains(view))
       return;

    m_viewList.append(view);

    view->modelAttached(m_q);
}

void ModelPrivate::detachView(AbstractView *view, bool notifyView)
{
    if (notifyView)
        view->modelAboutToBeDetached(m_q);
    m_viewList.removeOne(view);
}

void ModelPrivate::notifyNodeCreated(const InternalNode::Pointer &newInternalNodePointer)
{
    Q_ASSERT(newInternalNodePointer->isValid());

    bool resetModel = false;
    QString description;

    try {
        if (rewriterView()) {
            ModelNode createdNode(newInternalNodePointer, model(), rewriterView());
            rewriterView()->nodeCreated(createdNode);
        }
    } catch (const RewritingException &e) {
        description = e.description();
        resetModel = true;
    }

    if (nodeInstanceView()) {
        ModelNode createdNode(newInternalNodePointer, model(), nodeInstanceView());
        nodeInstanceView()->nodeCreated(createdNode);
    }

    foreach (const QPointer<AbstractView> &view, m_viewList) {
        Q_ASSERT(view != 0);
        ModelNode createdNode(newInternalNodePointer, model(), view.data());
        view->nodeCreated(createdNode);
    }

    if (resetModel)
        resetModelByRewriter(description);
}

void ModelPrivate::notifyNodeAboutToBeRemoved(const InternalNode::Pointer &internalNodePointer)
{
    bool resetModel = false;
    QString description;

    try {
        if (rewriterView()) {
            ModelNode modelNode(internalNodePointer, model(), rewriterView());
            rewriterView()->nodeAboutToBeRemoved(modelNode);
        }
    } catch (const RewritingException &e) {
        description = e.description();
        resetModel = true;
    }

    foreach (const QPointer<AbstractView> &view, m_viewList) {
        Q_ASSERT(view != 0);
        ModelNode modelNode(internalNodePointer, model(), view.data());
        view->nodeAboutToBeRemoved(modelNode);
    }

    if (nodeInstanceView()) {
        ModelNode modelNode(internalNodePointer, model(), nodeInstanceView());
        nodeInstanceView()->nodeAboutToBeRemoved(modelNode);
    }

    if (resetModel)
        resetModelByRewriter(description);
}

void ModelPrivate::notifyNodeRemoved(const InternalNodePointer &internalNodePointer,
                                     const InternalNodePointer &parentNodePointer,
                                     const PropertyName &parentPropertyName,
                                     AbstractView::PropertyChangeFlags propertyChange)
{
    bool resetModel = false;
    QString description;

    try {
        if (rewriterView()) {
            ModelNode modelNode(internalNodePointer, model(), rewriterView());
            NodeAbstractProperty parentProperty(parentPropertyName, parentNodePointer, model(), rewriterView());
            rewriterView()->nodeRemoved(modelNode, parentProperty, propertyChange);
        }
    } catch (const RewritingException &e) {
        description = e.description();
        resetModel = true;
    }

    if (nodeInstanceView()) {
        ModelNode modelNode(internalNodePointer, model(), nodeInstanceView());
        NodeAbstractProperty parentProperty(parentPropertyName, parentNodePointer, model(), nodeInstanceView());
        nodeInstanceView()->nodeRemoved(modelNode, parentProperty, propertyChange);
    }

    foreach (const QPointer<AbstractView> &view, m_viewList) {
        Q_ASSERT(view != 0);
        ModelNode modelNode(internalNodePointer, model(), view.data());
        NodeAbstractProperty parentProperty(parentPropertyName, parentNodePointer, model(), view.data());
        view->nodeRemoved(modelNode, parentProperty, propertyChange);

    }

    if (resetModel)
        resetModelByRewriter(description);
}

void ModelPrivate::notifyNodeTypeChanged(const InternalNodePointer &internalNodePointer, const TypeName &type, int majorVersion, int minorVersion)
{
    bool resetModel = false;
    QString description;

    try {
        if (rewriterView()) {
            ModelNode modelNode(internalNodePointer, model(), rewriterView());
            rewriterView()->nodeTypeChanged(modelNode, type, majorVersion, minorVersion);
        }
    } catch (const RewritingException &e) {
        description = e.description();
        resetModel = true;
    }

    foreach (const QPointer<AbstractView> &view, m_viewList) {
        Q_ASSERT(view != 0);
        ModelNode modelNode(internalNodePointer, model(), view.data());
        view->nodeTypeChanged(modelNode, type, majorVersion, minorVersion);
    }

    if (nodeInstanceView()) {
        ModelNode modelNode(internalNodePointer, model(), nodeInstanceView());
        nodeInstanceView()->nodeTypeChanged(modelNode, type, majorVersion, minorVersion);
    }

    if (resetModel)
        resetModelByRewriter(description);

}

void ModelPrivate::notifyNodeIdChanged(const InternalNode::Pointer& internalNodePointer, const QString& newId, const QString& oldId)
{
    bool resetModel = false;
    QString description;

    try {
        if (rewriterView()) {
            ModelNode modelNode(internalNodePointer, model(), rewriterView());
            rewriterView()->nodeIdChanged(modelNode, newId, oldId);
        }
    } catch (const RewritingException &e) {
        description = e.description();
        resetModel = true;
    }

    foreach (const QPointer<AbstractView> &view, m_viewList) {
        Q_ASSERT(view != 0);
        ModelNode modelNode(internalNodePointer, model(), view.data());
        view->nodeIdChanged(modelNode, newId, oldId);
    }

    if (nodeInstanceView()) {
        ModelNode modelNode(internalNodePointer, model(), nodeInstanceView());
        nodeInstanceView()->nodeIdChanged(modelNode, newId, oldId);
    }

    if (resetModel)
        resetModelByRewriter(description);
}

void ModelPrivate::notifyBindingPropertiesChanged(const QList<InternalBindingPropertyPointer> &internalPropertyList,
                                                  AbstractView::PropertyChangeFlags propertyChange)
{
    bool resetModel = false;
    QString description;

    try {
        if (rewriterView()) {
            QList<BindingProperty> propertyList;
            foreach (const InternalBindingPropertyPointer &bindingProperty, internalPropertyList) {
                propertyList.append(BindingProperty(bindingProperty->name(), bindingProperty->propertyOwner(), model(), rewriterView()));
            }
            rewriterView()->bindingPropertiesChanged(propertyList, propertyChange);
        }
    } catch (const RewritingException &e) {
        description = e.description();
        resetModel = true;
    }

    foreach (const QPointer<AbstractView> &view, m_viewList) {
        Q_ASSERT(view != 0);
        QList<BindingProperty> propertyList;
        foreach (const InternalBindingPropertyPointer &bindingProperty, internalPropertyList) {
            propertyList.append(BindingProperty(bindingProperty->name(), bindingProperty->propertyOwner(), model(), view.data()));
        }
        view->bindingPropertiesChanged(propertyList, propertyChange);

    }

    if (nodeInstanceView()) {
        QList<BindingProperty> propertyList;
        foreach (const InternalBindingPropertyPointer &bindingProperty, internalPropertyList) {
            propertyList.append(BindingProperty(bindingProperty->name(), bindingProperty->propertyOwner(), model(), nodeInstanceView()));
        }
        nodeInstanceView()->bindingPropertiesChanged(propertyList, propertyChange);
    }

    if (resetModel)
        resetModelByRewriter(description);
}

void ModelPrivate::notifySignalHandlerPropertiesChanged(const QVector<InternalSignalHandlerPropertyPointer> &internalPropertyList,
                                                        AbstractView::PropertyChangeFlags propertyChange)
{
    bool resetModel = false;
    QString description;

    try {
        if (rewriterView()) {
            QVector<SignalHandlerProperty> propertyList;
            foreach (const InternalSignalHandlerPropertyPointer &signalHandlerProperty, internalPropertyList) {
                propertyList.append(SignalHandlerProperty(signalHandlerProperty->name(), signalHandlerProperty->propertyOwner(), model(), rewriterView()));
            }
            rewriterView()->signalHandlerPropertiesChanged(propertyList, propertyChange);
        }
    } catch (const RewritingException &e) {
        description = e.description();
        resetModel = true;
    }

    foreach (const QPointer<AbstractView> &view, m_viewList) {
        Q_ASSERT(view != 0);
        QVector<SignalHandlerProperty> propertyList;
        foreach (const InternalSignalHandlerPropertyPointer &signalHandlerProperty, internalPropertyList) {
            propertyList.append(SignalHandlerProperty(signalHandlerProperty->name(), signalHandlerProperty->propertyOwner(), model(), view.data()));
        }
        view->signalHandlerPropertiesChanged(propertyList, propertyChange);

    }

    if (nodeInstanceView()) {
        QVector<SignalHandlerProperty> propertyList;
        foreach (const InternalSignalHandlerPropertyPointer &signalHandlerProperty, internalPropertyList) {
            propertyList.append(SignalHandlerProperty(signalHandlerProperty->name(), signalHandlerProperty->propertyOwner(), model(), nodeInstanceView()));
        }
        nodeInstanceView()->signalHandlerPropertiesChanged(propertyList, propertyChange);
    }

    if (resetModel)
        resetModelByRewriter(description);
}

void ModelPrivate::notifyScriptFunctionsChanged(const InternalNodePointer &internalNodePointer, const QStringList &scriptFunctionList)
{
    bool resetModel = false;
    QString description;

    try {
        if (rewriterView()) {
            ModelNode node(internalNodePointer, model(), rewriterView());
            rewriterView()->scriptFunctionsChanged(node, scriptFunctionList);
        }
    } catch (const RewritingException &e) {
        description = e.description();
        resetModel = true;
    }

    if (nodeInstanceView()) {
        ModelNode node(internalNodePointer, model(), nodeInstanceView());
        nodeInstanceView()->scriptFunctionsChanged(node, scriptFunctionList);
    }

    foreach (const QPointer<AbstractView> &view, m_viewList) {
        Q_ASSERT(view != 0);

        ModelNode node(internalNodePointer, model(), view.data());
        view->scriptFunctionsChanged(node, scriptFunctionList);

    }

    if (resetModel)
        resetModelByRewriter(description);
}


void ModelPrivate::notifyVariantPropertiesChanged(const InternalNodePointer &internalNodePointer,
                                                  const PropertyNameList &propertyNameList,
                                                  AbstractView::PropertyChangeFlags propertyChange)
{
    bool resetModel = false;
    QString description;

    try {
        if (rewriterView()) {
            QList<VariantProperty> propertyList;
            foreach (const PropertyName &propertyName, propertyNameList) {
                VariantProperty property(propertyName, internalNodePointer, model(), rewriterView());
                propertyList.append(property);
            }

            ModelNode node(internalNodePointer, model(), rewriterView());
            rewriterView()->variantPropertiesChanged(propertyList, propertyChange);
        }
    } catch (const RewritingException &e) {
        description = e.description();
        resetModel = true;
    }


    foreach (const QPointer<AbstractView> &view, m_viewList) {
        QList<VariantProperty> propertyList;
        Q_ASSERT(view != 0);
        foreach (const PropertyName &propertyName, propertyNameList) {
            VariantProperty property(propertyName, internalNodePointer, model(), view.data());
            propertyList.append(property);
        }

        view->variantPropertiesChanged(propertyList, propertyChange);

    }

    if (nodeInstanceView()) {
        QList<VariantProperty> propertyList;
        foreach (const PropertyName &propertyName, propertyNameList) {
            Q_ASSERT(internalNodePointer->hasProperty(propertyName));
            Q_ASSERT(internalNodePointer->property(propertyName)->isVariantProperty());
            VariantProperty property(propertyName, internalNodePointer, model(), nodeInstanceView());
            propertyList.append(property);
        }

        ModelNode node(internalNodePointer, model(), nodeInstanceView());
        nodeInstanceView()->variantPropertiesChanged(propertyList, propertyChange);
    }

    if (resetModel)
        resetModelByRewriter(description);
}

void ModelPrivate::notifyNodeAboutToBeReparent(const InternalNodePointer &internalNodePointer,
                                               const InternalNodeAbstractPropertyPointer &newPropertyParent,
                                               const InternalNodePointer &oldParent,
                                               const PropertyName &oldPropertyName,
                                               AbstractView::PropertyChangeFlags propertyChange)
{
    bool resetModel = false;
    QString description;

    try {
        if (rewriterView()) {
            NodeAbstractProperty newProperty;
            NodeAbstractProperty oldProperty;

            if (!oldPropertyName.isEmpty() && oldParent->isValid())
                oldProperty = NodeAbstractProperty(oldPropertyName, oldParent, model(), rewriterView());

            if (!newPropertyParent.isNull())
                newProperty = NodeAbstractProperty(newPropertyParent, model(), rewriterView());
            ModelNode modelNode(internalNodePointer, model(), rewriterView());
            rewriterView()->nodeAboutToBeReparented(modelNode, newProperty, oldProperty, propertyChange);
        }
    } catch (const RewritingException &e) {
        description = e.description();
        resetModel = true;
    }

    foreach (const QPointer<AbstractView> &view, m_viewList) {
        NodeAbstractProperty newProperty;
        NodeAbstractProperty oldProperty;

        Q_ASSERT(!view.isNull());
        if (!oldPropertyName.isEmpty() && oldParent->isValid())
            oldProperty = NodeAbstractProperty(oldPropertyName, oldParent, model(), view.data());

        if (!newPropertyParent.isNull())
            newProperty = NodeAbstractProperty(newPropertyParent, model(), view.data());
        ModelNode modelNode(internalNodePointer, model(), view.data());

        view->nodeAboutToBeReparented(modelNode, newProperty, oldProperty, propertyChange);

    }

    if (nodeInstanceView()) {
        NodeAbstractProperty newProperty;
        NodeAbstractProperty oldProperty;

        if (!oldPropertyName.isEmpty() && oldParent->isValid())
            oldProperty = NodeAbstractProperty(oldPropertyName, oldParent, model(), nodeInstanceView());

        if (!newPropertyParent.isNull())
            newProperty = NodeAbstractProperty(newPropertyParent, model(), nodeInstanceView());
        ModelNode modelNode(internalNodePointer, model(), nodeInstanceView());
        nodeInstanceView()->nodeAboutToBeReparented(modelNode, newProperty, oldProperty, propertyChange);
    }

    if (resetModel)
        resetModelByRewriter(description);
}


void ModelPrivate::notifyNodeReparent(const InternalNode::Pointer &internalNodePointer,
                                      const InternalNodeAbstractProperty::Pointer &newPropertyParent,
                                      const InternalNodePointer &oldParent,
                                      const PropertyName &oldPropertyName,
                                      AbstractView::PropertyChangeFlags propertyChange)
{
    bool resetModel = false;
    QString description;

    try {
        if (rewriterView()) {
            NodeAbstractProperty newProperty;
            NodeAbstractProperty oldProperty;

            if (!oldPropertyName.isEmpty() && oldParent->isValid())
                oldProperty = NodeAbstractProperty(oldPropertyName, oldParent, model(), rewriterView());

            if (!newPropertyParent.isNull())
                newProperty = NodeAbstractProperty(newPropertyParent, model(), rewriterView());
            ModelNode node(internalNodePointer, model(), rewriterView());
            rewriterView()->nodeReparented(node, newProperty, oldProperty, propertyChange);
        }
    } catch (const RewritingException &e) {
        description = e.description();
        resetModel = true;
    }

    foreach (const QPointer<AbstractView> &view, m_viewList) {
        NodeAbstractProperty newProperty;
        NodeAbstractProperty oldProperty;

        Q_ASSERT(!view.isNull());
        if (!oldPropertyName.isEmpty() && oldParent->isValid())
            oldProperty = NodeAbstractProperty(oldPropertyName, oldParent, model(), view.data());

        if (!newPropertyParent.isNull())
            newProperty = NodeAbstractProperty(newPropertyParent, model(), view.data());
        ModelNode modelNode(internalNodePointer, model(), view.data());

        view->nodeReparented(modelNode, newProperty, oldProperty, propertyChange);

    }

    if (nodeInstanceView()) {
        NodeAbstractProperty newProperty;
        NodeAbstractProperty oldProperty;

        if (!oldPropertyName.isEmpty() && oldParent->isValid())
            oldProperty = NodeAbstractProperty(oldPropertyName, oldParent, model(), nodeInstanceView());

        if (!newPropertyParent.isNull())
            newProperty = NodeAbstractProperty(newPropertyParent, model(), nodeInstanceView());
        ModelNode modelNode(internalNodePointer, model(), nodeInstanceView());
        nodeInstanceView()->nodeReparented(modelNode, newProperty, oldProperty, propertyChange);
    }

    if (resetModel)
        resetModelByRewriter(description);
}

void ModelPrivate::notifyNodeOrderChanged(const InternalNodeListPropertyPointer &internalListPropertyPointer,
                                          const InternalNode::Pointer &internalNodePointer,
                                          int oldIndex)
{
    bool resetModel = false;
    QString description;

    try {
        if (rewriterView())
            rewriterView()->nodeOrderChanged(NodeListProperty(internalListPropertyPointer, model(), rewriterView()),
                               ModelNode(internalNodePointer, model(), rewriterView()),
                               oldIndex);
    } catch (const RewritingException &e) {
        description = e.description();
        resetModel = true;
    }

    foreach (const QPointer<AbstractView> &view, m_viewList) {
        Q_ASSERT(!view.isNull());
        view->nodeOrderChanged(NodeListProperty(internalListPropertyPointer, model(), view.data()),
                                   ModelNode(internalNodePointer, model(), view.data()),
                                   oldIndex);
    }

    if (nodeInstanceView())
        nodeInstanceView()->nodeOrderChanged(NodeListProperty(internalListPropertyPointer, model(), nodeInstanceView()),
                           ModelNode(internalNodePointer, model(), nodeInstanceView()),
                           oldIndex);

    if (resetModel)
        resetModelByRewriter(description);
}

void ModelPrivate::setSelectedNodes(const QList<InternalNode::Pointer> &selectedNodeList)
{

    QList<InternalNode::Pointer> sortedSelectedList(selectedNodeList);
    QMutableListIterator<InternalNode::Pointer> iterator(sortedSelectedList);
    while (iterator.hasNext()) {
        InternalNode::Pointer node(iterator.next());
        if (!node->isValid())
            iterator.remove();
    }

    sortedSelectedList = sortedSelectedList.toSet().toList();
    Utils::sort(sortedSelectedList);

    if (sortedSelectedList == m_selectedInternalNodeList)
        return;


    const QList<InternalNode::Pointer> lastSelectedNodeList = m_selectedInternalNodeList;
    m_selectedInternalNodeList = sortedSelectedList;

    changeSelectedNodes(sortedSelectedList, lastSelectedNodeList);
}


void ModelPrivate::clearSelectedNodes()
{
    const QList<InternalNode::Pointer> lastSelectedNodeList = m_selectedInternalNodeList;
    m_selectedInternalNodeList.clear();
    changeSelectedNodes(m_selectedInternalNodeList, lastSelectedNodeList);
}

void ModelPrivate::removeAuxiliaryData(const InternalNodePointer &node, const PropertyName &name)
{
    node->removeAuxiliaryData(name);

    notifyAuxiliaryDataChanged(node, name, QVariant());
}

QList<ModelNode> ModelPrivate::toModelNodeList(const QList<InternalNode::Pointer> &internalNodeList, AbstractView *view) const
{
    QList<ModelNode> newNodeList;
    foreach (const Internal::InternalNode::Pointer &node, internalNodeList)
        newNodeList.append(ModelNode(node, model(), view));

    return newNodeList;
}

QVector<ModelNode> ModelPrivate::toModelNodeVector(const QVector<InternalNode::Pointer> &internalNodeVector, AbstractView *view) const
{
    QVector<ModelNode> newNodeVector;
    foreach (const Internal::InternalNode::Pointer &node, internalNodeVector)
        newNodeVector.append(ModelNode(node, model(), view));

    return newNodeVector;
}

QList<Internal::InternalNode::Pointer> ModelPrivate::toInternalNodeList(const QList<ModelNode> &internalNodeList) const
{
    QList<Internal::InternalNode::Pointer> newNodeList;
    foreach (const ModelNode &node, internalNodeList)
        newNodeList.append(node.internalNode());

    return newNodeList;
}

QVector<Internal::InternalNode::Pointer> ModelPrivate::toInternalNodeVector(const QVector<ModelNode> &internalNodeVector) const
{
    QVector<Internal::InternalNode::Pointer> newNodeVector;
    foreach (const ModelNode &node, internalNodeVector)
        newNodeVector.append(node.internalNode());

    return newNodeVector;
}

void ModelPrivate::changeSelectedNodes(const QList<InternalNode::Pointer> &newSelectedNodeList,
                                       const QList<InternalNode::Pointer> &oldSelectedNodeList)
{
    foreach (const QPointer<AbstractView> &view, m_viewList) {
        Q_ASSERT(view != 0);
        view->selectedNodesChanged(toModelNodeList(newSelectedNodeList, view.data()), toModelNodeList(oldSelectedNodeList, view.data()));
    }
}

QList<InternalNode::Pointer> ModelPrivate::selectedNodes() const
{
    foreach (const InternalNode::Pointer &node, m_selectedInternalNodeList) {
        if (!node->isValid())
            throw new InvalidModelNodeException(__LINE__, __FUNCTION__, __FILE__);
    }

    return m_selectedInternalNodeList;
}

void ModelPrivate::selectNode(const InternalNode::Pointer &internalNodePointer)
{
    if (selectedNodes().contains(internalNodePointer))
        return;

    QList<InternalNode::Pointer> selectedNodeList(selectedNodes());
    selectedNodeList += internalNodePointer;
    setSelectedNodes(selectedNodeList);
}

void ModelPrivate::deselectNode(const InternalNode::Pointer &internalNodePointer)
{
    QList<InternalNode::Pointer> selectedNodeList(selectedNodes());
    bool isRemoved = selectedNodeList.removeOne(internalNodePointer);

    if (!isRemoved)
        return;

    setSelectedNodes(selectedNodeList);
}

void ModelPrivate::removePropertyWithoutNotification(const InternalPropertyPointer &property)
{
    if (property->isNodeAbstractProperty()) {
        foreach (const InternalNode::Pointer & internalNode, property->toNodeAbstractProperty()->allSubNodes())
            removeNodeFromModel(internalNode);
    }

    property->remove();
}

static QList<PropertyPair> toPropertyPairList(const QList<InternalProperty::Pointer> &propertyList)
{
    QList<PropertyPair> propertyPairList;

    foreach (const InternalProperty::Pointer &property, propertyList)
        propertyPairList.append({property->propertyOwner(), property->name()});

    return propertyPairList;

}

void ModelPrivate::removeProperty(const InternalProperty::Pointer &property)
{
    notifyPropertiesAboutToBeRemoved({property});

    const QList<PropertyPair> propertyPairList = toPropertyPairList({property});

    removePropertyWithoutNotification(property);

    notifyPropertiesRemoved(propertyPairList);
}

void ModelPrivate::setBindingProperty(const InternalNode::Pointer &internalNodePointer, const PropertyName &name, const QString &expression)
{
    AbstractView::PropertyChangeFlags propertyChange = AbstractView::NoAdditionalChanges;
    if (!internalNodePointer->hasProperty(name)) {
        internalNodePointer->addBindingProperty(name);
        propertyChange = AbstractView::PropertiesAdded;
    }

    InternalBindingProperty::Pointer bindingProperty = internalNodePointer->bindingProperty(name);
    bindingProperty->setExpression(expression);
    notifyBindingPropertiesChanged({bindingProperty}, propertyChange);
}

void ModelPrivate::setSignalHandlerProperty(const InternalNodePointer &internalNodePointer, const PropertyName &name, const QString &source)
{
    AbstractView::PropertyChangeFlags propertyChange = AbstractView::NoAdditionalChanges;
    if (!internalNodePointer->hasProperty(name)) {
        internalNodePointer->addSignalHandlerProperty(name);
        propertyChange = AbstractView::PropertiesAdded;
    }

    InternalSignalHandlerProperty::Pointer signalHandlerProperty = internalNodePointer->signalHandlerProperty(name);
    signalHandlerProperty->setSource(source);
    notifySignalHandlerPropertiesChanged({signalHandlerProperty}, propertyChange);
}

void ModelPrivate::setVariantProperty(const InternalNode::Pointer &internalNodePointer, const PropertyName &name, const QVariant &value)
{
    AbstractView::PropertyChangeFlags propertyChange = AbstractView::NoAdditionalChanges;
    if (!internalNodePointer->hasProperty(name)) {
        internalNodePointer->addVariantProperty(name);
        propertyChange = AbstractView::PropertiesAdded;
    }

    internalNodePointer->variantProperty(name)->setValue(value);
    internalNodePointer->variantProperty(name)->resetDynamicTypeName();
    notifyVariantPropertiesChanged(internalNodePointer, PropertyNameList({name}), propertyChange);
}

void ModelPrivate::setDynamicVariantProperty(const InternalNodePointer &internalNodePointer,
                                             const PropertyName &name,
                                             const TypeName &dynamicPropertyType,
                                             const QVariant &value)
{
    AbstractView::PropertyChangeFlags propertyChange = AbstractView::NoAdditionalChanges;
    if (!internalNodePointer->hasProperty(name)) {
        internalNodePointer->addVariantProperty(name);
        propertyChange = AbstractView::PropertiesAdded;
    }

    internalNodePointer->variantProperty(name)->setDynamicValue(dynamicPropertyType, value);
    notifyVariantPropertiesChanged(internalNodePointer, PropertyNameList({name}), propertyChange);
}

void ModelPrivate::setDynamicBindingProperty(const InternalNodePointer &internalNodePointer,
                                             const PropertyName &name,
                                             const TypeName &dynamicPropertyType,
                                             const QString &expression)
{
    AbstractView::PropertyChangeFlags propertyChange = AbstractView::NoAdditionalChanges;
    if (!internalNodePointer->hasProperty(name)) {
        internalNodePointer->addBindingProperty(name);
        propertyChange = AbstractView::PropertiesAdded;
    }

    InternalBindingProperty::Pointer bindingProperty = internalNodePointer->bindingProperty(name);
    bindingProperty->setDynamicExpression(dynamicPropertyType, expression);
    notifyBindingPropertiesChanged({bindingProperty}, propertyChange);
}

void ModelPrivate::reparentNode(const InternalNode::Pointer &newParentNode,
                                const PropertyName &name,
                                const InternalNode::Pointer &internalNodePointer,
                                bool list,
                                const TypeName &dynamicTypeName)
{
    AbstractView::PropertyChangeFlags propertyChange = AbstractView::NoAdditionalChanges;
    if (!newParentNode->hasProperty(name)) {
        if (list)
            newParentNode->addNodeListProperty(name);
        else
            newParentNode->addNodeProperty(name, dynamicTypeName);
        propertyChange |= AbstractView::PropertiesAdded;
    }

    InternalNodeAbstractProperty::Pointer oldParentProperty(internalNodePointer->parentProperty());
    InternalNode::Pointer oldParentNode;
    PropertyName oldParentPropertyName;
    if (oldParentProperty && oldParentProperty->isValid()) {
        oldParentNode = internalNodePointer->parentProperty()->propertyOwner();
        oldParentPropertyName = internalNodePointer->parentProperty()->name();
    }

    InternalNodeAbstractProperty::Pointer newParentProperty(newParentNode->nodeAbstractProperty(name));
    Q_ASSERT(!newParentProperty.isNull());

    notifyNodeAboutToBeReparent(internalNodePointer, newParentProperty, oldParentNode, oldParentPropertyName, propertyChange);

    if (newParentProperty)
        internalNodePointer->setParentProperty(newParentProperty);


    if (oldParentProperty && oldParentProperty->isValid() && oldParentProperty->isEmpty()) {
        removePropertyWithoutNotification(oldParentProperty);

        propertyChange |= AbstractView::EmptyPropertiesRemoved;
    }

    notifyNodeReparent(internalNodePointer, newParentProperty, oldParentNode, oldParentPropertyName, propertyChange);
}

void ModelPrivate::clearParent(const InternalNodePointer &internalNodePointer)
{

    InternalNodeAbstractProperty::Pointer oldParentProperty(internalNodePointer->parentProperty());
    InternalNode::Pointer oldParentNode;
    PropertyName oldParentPropertyName;
    if (oldParentProperty->isValid()) {
        oldParentNode = internalNodePointer->parentProperty()->propertyOwner();
        oldParentPropertyName = internalNodePointer->parentProperty()->name();
    }

    internalNodePointer->resetParentProperty();
    notifyNodeReparent(internalNodePointer, InternalNodeAbstractProperty::Pointer(), oldParentNode, oldParentPropertyName, AbstractView::NoAdditionalChanges);
}

void ModelPrivate::changeRootNodeType(const TypeName &type, int majorVersion, int minorVersion)
{
    Q_ASSERT(!rootNode().isNull());
    rootNode()->setType(type);
    rootNode()->setMajorVersion(majorVersion);
    rootNode()->setMinorVersion(minorVersion);
    notifyRootNodeTypeChanged(QString::fromUtf8(type), majorVersion, minorVersion);
}

void ModelPrivate::setScriptFunctions(const InternalNode::Pointer &internalNodePointer, const QStringList &scriptFunctionList)
{
    internalNodePointer->setScriptFunctions(scriptFunctionList);

    notifyScriptFunctionsChanged(internalNodePointer, scriptFunctionList);
}

void ModelPrivate::setNodeSource(const InternalNodePointer &internalNodePointer, const QString &nodeSource)
{
    internalNodePointer->setNodeSource(nodeSource);
    notifyNodeSourceChanged(internalNodePointer, nodeSource);
}

void ModelPrivate::changeNodeOrder(const InternalNode::Pointer &internalParentNode, const PropertyName &listPropertyName, int from, int to)
{
    InternalNodeListProperty::Pointer nodeList(internalParentNode->nodeListProperty(listPropertyName));
    Q_ASSERT(!nodeList.isNull());
    nodeList->slide(from, to);

    const InternalNodePointer internalNode = nodeList->nodeList().at(to);
    notifyNodeOrderChanged(nodeList, internalNode, from);
}

void  ModelPrivate::setRewriterView(RewriterView *rewriterView)
{
    if (rewriterView == m_rewriterView.data())
        return;

    Q_ASSERT(!(rewriterView && m_rewriterView));

    if (m_rewriterView)
            m_rewriterView->modelAboutToBeDetached(model());

    m_rewriterView = rewriterView;

    if (rewriterView)
        rewriterView->modelAttached(model());
}

RewriterView  *ModelPrivate::rewriterView() const
{
    return m_rewriterView.data();
}

void ModelPrivate::setNodeInstanceView(NodeInstanceView *nodeInstanceView)
{
    if (nodeInstanceView == m_nodeInstanceView.data())
        return;

    if (m_nodeInstanceView)
        m_nodeInstanceView->modelAboutToBeDetached(m_q);

    m_nodeInstanceView = nodeInstanceView;

    if (nodeInstanceView)
        nodeInstanceView->modelAttached(m_q);

}

NodeInstanceView *ModelPrivate::nodeInstanceView() const
{
    return m_nodeInstanceView.data();
}

InternalNodePointer ModelPrivate::currentTimelineNode() const
{
    return m_currentTimelineMutatorNode;
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
    // the item must be ordered!

    QList<InternalNodePointer> nodeList;

    if (m_rootInternalNode.isNull() || !m_rootInternalNode->isValid())
        return nodeList;

    nodeList.append(m_rootInternalNode);
    nodeList.append(m_rootInternalNode->allSubNodes());
    nodeList.append((m_nodeSet - nodeList.toSet()).toList());

    return nodeList;
}

bool ModelPrivate::isWriteLocked() const
{
    return m_writeLock;
}

InternalNode::Pointer ModelPrivate::currentStateNode() const
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
    : m_model(model->d)
{
    Q_ASSERT(model->d);
    if (m_model->m_writeLock)
        qWarning() << "QmlDesigner: Misbehaving view calls back to model!!!";
    // FIXME: Enable it again
     Q_ASSERT(!m_model->m_writeLock);
    m_model->m_writeLock = true;
}

WriteLocker::~WriteLocker()
{
    if (!m_model->m_writeLock)
        qWarning() << "QmlDesigner: Misbehaving view calls back to model!!!";
    // FIXME: Enable it again
     Q_ASSERT(m_model->m_writeLock);
    m_model->m_writeLock = false;
}

} //namespace internal


Model::Model()  :
   QObject(),
   d(new Internal::ModelPrivate(this))
{
}


Model::~Model()
{
    delete d;
}


Model *Model::create(TypeName type, int major, int minor, Model *metaInfoPropxyModel)
{
    return Internal::ModelPrivate::create(type, major, minor, metaInfoPropxyModel);
}

QList<Import> Model::imports() const
{
    return d->imports();
}

QList<Import> Model::possibleImports() const
{
    return d->m_possibleImportList;
}

QList<Import> Model::usedImports() const
{
    return d->m_usedImportList;
}

void Model::changeImports(const QList<Import> &importsToBeAdded, const QList<Import> &importsToBeRemoved)
{
    d->changeImports(importsToBeAdded, importsToBeRemoved);
}

void Model::setPossibleImports(const QList<Import> &possibleImports)
{
    d->m_possibleImportList = possibleImports;
}

void Model::setUsedImports(const QList<Import> &usedImports)
{
    d->m_usedImportList = usedImports;
}


static bool compareVersions(const QString &version1, const QString &version2, bool allowHigherVersion)
{
    if (version1 == version2)
        return true;
    if (!allowHigherVersion)
        return false;
    QStringList version1List = version1.split(QLatin1Char('.'));
    QStringList version2List = version2.split(QLatin1Char('.'));
    if (version1List.count() == 2 && version2List.count() == 2) {
        bool ok;
        int major1 = version1List.first().toInt(&ok);
        if (!ok)
            return false;
        int major2 = version2List.first().toInt(&ok);
        if (!ok)
            return false;
        if (major1 >= major2) {
            int minor1 = version1List.last().toInt(&ok);
            if (!ok)
                return false;
            int minor2 = version2List.last().toInt(&ok);
            if (!ok)
                return false;
            if (minor1 >= minor2)
                return true;
        }
    }

    return false;
}

bool Model::hasImport(const Import &import, bool ignoreAlias, bool allowHigherVersion)
{
    if (imports().contains(import))
        return true;
    if (!ignoreAlias)
        return false;

    foreach (const Import &existingImport, imports()) {
        if (existingImport.isFileImport() && import.isFileImport())
            if (existingImport.file() == import.file())
                return true;
        if (existingImport.isLibraryImport() && import.isLibraryImport())
            if (existingImport.url() == import.url()  && compareVersions(existingImport.version(), import.version(), allowHigherVersion))
                return true;
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

    QStringList importPathList;

    QString documentDirectoryPath = QFileInfo(fileUrl().toLocalFile()).absolutePath();

    if (!documentDirectoryPath.isEmpty())
        importPathList.append(documentDirectoryPath);

    return importPathList;
}

RewriterView *Model::rewriterView() const
{
    return d->rewriterView();
}

void Model::setRewriterView(RewriterView *rewriterView)
{
    d->setRewriterView(rewriterView);
}

NodeInstanceView *Model::nodeInstanceView() const
{
    return d->nodeInstanceView();
}

void Model::setNodeInstanceView(NodeInstanceView *nodeInstanceView)
{
    d->setNodeInstanceView(nodeInstanceView);
}

/*!
 \brief Returns the model that is used for metainfo
 \return Return itself if not other metaInfoProxyModel does exist
*/
Model *Model::metaInfoProxyModel()
{
    if (d->m_metaInfoProxyModel)
        return d->m_metaInfoProxyModel->metaInfoProxyModel();
    else
        return this;
}

TextModifier *Model::textModifier() const
{
    return d->m_textModifier.data();
}

void Model::setTextModifier(TextModifier *textModifier)
{
    d->m_textModifier = textModifier;
}

void Model::setDocumentMessages(const QList<DocumentMessage> &errors, const QList<DocumentMessage> &warnings)
{
    d->setDocumentMessages(errors, warnings);
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
    Internal::WriteLocker locker(d);
    d->setFileUrl(url);
}

/*!
  \brief Returns list of QML types available within the model.
  */
const MetaInfo Model::metaInfo() const
{
    return d->metaInfo();
}

bool Model::hasNodeMetaInfo(const TypeName &typeName, int majorVersion, int minorVersion)
{
    return NodeMetaInfo(metaInfoProxyModel(), typeName, majorVersion, minorVersion).isValid();
}

NodeMetaInfo Model::metaInfo(const TypeName &typeName, int majorVersion, int minorVersion)
{
    return NodeMetaInfo(metaInfoProxyModel(), typeName, majorVersion, minorVersion);
}

/*!
  \brief Returns list of QML types available within the model.
  */
MetaInfo Model::metaInfo()
{
    return d->metaInfo();
}

/*! \name Undo Redo Interface
    here you can find a facade to the internal undo redo framework
*/


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
    RewriterView *castedRewriterView = qobject_cast<RewriterView*>(view);
    if (castedRewriterView) {
        if (rewriterView() == castedRewriterView)
            return;
        setRewriterView(castedRewriterView);

        return;
    }

    NodeInstanceView *nodeInstanceView = qobject_cast<NodeInstanceView*>(view);
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

    RewriterView *rewriterView = qobject_cast<RewriterView*>(view);
    if (rewriterView)
        return;

    NodeInstanceView *nodeInstanceView = qobject_cast<NodeInstanceView*>(view);
    if (nodeInstanceView)
        return;

    d->detachView(view, emitNotify);
}


}
