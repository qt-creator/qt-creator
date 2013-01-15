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

#include <model.h>
#include <modelnode.h>
#include "internalnode_p.h"
#include "invalidpropertyexception.h"
#include "invalidargumentexception.h"

#include <QFile>
#include <QByteArray>
#include <QWeakPointer>
#include <QFileInfo>

#include <QUndoStack>
#include <QXmlStreamReader>
#include <QDebug>
#include <QPlainTextEdit>
#include <QHashIterator>

#include "abstractview.h"
#include "nodeinstanceview.h"
#include "metainfo.h"
#include "nodemetainfo.h"
#include "model_p.h"
#include "subcomponentmanager.h"
#include "internalproperty.h"
#include "internalnodelistproperty.h"
#include "internalnodeabstractproperty.h"
#include "invalidmodelnodeexception.h"
#include "invalidmodelstateexception.h"
#include "invalidslideindexexception.h"

#include "abstractproperty.h"
#include "variantproperty.h"
#include "bindingproperty.h"
#include "nodeabstractproperty.h"
#include "nodelistproperty.h"
#include "rewritertransaction.h"
#include "rewriterview.h"
#include "rewritingexception.h"
#include "invalididexception.h"

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
    m_acutalStateNode = m_rootInternalNode;
}

ModelPrivate::~ModelPrivate()
{
    detachAllViews();
}

void ModelPrivate::detachAllViews()
{
    foreach (const QWeakPointer<AbstractView> &view, m_viewList)
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

Model *ModelPrivate::create(QString type, int major, int minor, Model *metaInfoPropxyModel)
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
    } catch (RewritingException &e) {
        description = e.description();
        resetModel = true;
    }

    NodeMetaInfo::clearCache();

    if (nodeInstanceView())
        nodeInstanceView()->importsChanged(addedImports, removedImports);

    foreach (const QWeakPointer<AbstractView> &view, m_viewList)
        view->importsChanged(addedImports, removedImports);

    if (resetModel)
        resetModelByRewriter(description);
}

QUrl ModelPrivate::fileUrl() const
{
    return m_fileUrl;
}

void ModelPrivate::setFileUrl(const QUrl &fileUrl)
{
    QUrl oldPath = m_fileUrl;

    if (oldPath != fileUrl) {
        m_fileUrl = fileUrl;

        foreach (const QWeakPointer<AbstractView> &view, m_viewList)
            view->fileUrlChanged(oldPath, fileUrl);
    }
}

InternalNode::Pointer ModelPrivate::createNode(const QString &typeString,
                                               int majorVersion,
                                               int minorVersion,
                                               const QList<QPair<QString, QVariant> > &propertyList,
                                               const QList<QPair<QString, QVariant> > &auxPropertyList,
                                               const QString &nodeSource,
                                               ModelNode::NodeSourceType nodeSourceType,
                                               bool isRootNode)
{
    if (typeString.isEmpty())
        throw InvalidArgumentException(__LINE__, __FUNCTION__, __FILE__, tr("invalid type"));

    qint32 internalId = 0;

    if (!isRootNode)
        internalId = m_internalIdCounter++;

    InternalNode::Pointer newInternalNodePointer = InternalNode::create(typeString, majorVersion, minorVersion, internalId);
    newInternalNodePointer->setNodeSourceType(nodeSourceType);

    typedef QPair<QString, QVariant> PropertyPair;

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

    return newInternalNodePointer;
}

void ModelPrivate::removeNodeFromModel(const InternalNodePointer &node)
{
    Q_ASSERT(!node.isNull());

    node->resetParentProperty();

    if (!node->id().isEmpty())
        m_idNodeHash.remove(node->id());
    node->setValid(false);
    m_nodeSet.remove(node);
    m_internalIdNodeHash.remove(node->internalId());
}

void ModelPrivate::removeAllSubNodes(const InternalNode::Pointer &node)
{
    foreach (const InternalNodePointer &subNode, node->allSubNodes()) {
        removeNodeFromModel(subNode);
    }
}

void ModelPrivate::removeNode(const InternalNode::Pointer &node)
{
    Q_ASSERT(!node.isNull());

    AbstractView::PropertyChangeFlags propertyChangeFlags = AbstractView::NoAdditionalChanges;

    notifyNodeAboutToBeRemoved(node);

    InternalNodeAbstractProperty::Pointer oldParentProperty(node->parentProperty());

    removeAllSubNodes(node);
    removeNodeFromModel(node);

    InternalNode::Pointer parentNode;
    QString parentPropertyName;
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
    } catch (RewritingException &e) {
        throw InvalidIdException(__LINE__, __FUNCTION__, __FILE__, id, e.description());
    }
}

void ModelPrivate::checkPropertyName(const QString &propertyName)
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

void ModelPrivate::notifyAuxiliaryDataChanged(const InternalNodePointer &internalNode, const QString &name, const QVariant &data)
{
    bool resetModel = false;
    QString description;

    try {
        if (rewriterView()) {
            ModelNode node(internalNode, model(), rewriterView());
            rewriterView()->auxiliaryDataChanged(node, name, data);
        }
    } catch (RewritingException &e) {
        description = e.description();
        resetModel = true;
    }

    foreach (const QWeakPointer<AbstractView> &view, m_viewList) {
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
    } catch (RewritingException &e) {
        description = e.description();
        resetModel = true;
    }

    foreach (const QWeakPointer<AbstractView> &view, m_viewList) {
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
    } catch (RewritingException &e) {
        description = e.description();
        resetModel = true;
    }

    if (nodeInstanceView())
        nodeInstanceView()->rootNodeTypeChanged(type, majorVersion, minorVersion);

    foreach (const QWeakPointer<AbstractView> &view, m_viewList) {
        Q_ASSERT(view != 0);
        view->rootNodeTypeChanged(type, majorVersion, minorVersion);

    }

    if (resetModel)
        resetModelByRewriter(description);
}

void ModelPrivate::notifyInstancePropertyChange(const QList<QPair<ModelNode, QString> > &propertyPairList)
{
    // no need to notify the rewriter or the instance view

    typedef QPair<ModelNode, QString> ModelNodePropertyPair;
    foreach (const QWeakPointer<AbstractView> &view, m_viewList) {
        Q_ASSERT(view != 0);

        QList<QPair<ModelNode, QString> > adaptedPropertyList;
        foreach (const ModelNodePropertyPair &propertyPair, propertyPairList) {
            ModelNodePropertyPair newPair(ModelNode(propertyPair.first.internalNode(), model(), view.data()), propertyPair.second);
            adaptedPropertyList.append(newPair);
        }

        view->instancePropertyChange(adaptedPropertyList);
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
    } catch (RewritingException &e) {
        description = e.description();
        resetModel = true;
    }

    foreach (const QWeakPointer<AbstractView> &view, m_viewList) {
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
            rewriterView()->instanceInformationsChange(convertModelNodeInformationHash(informationChangeHash, rewriterView()));
    } catch (RewritingException &e) {
        description = e.description();
        resetModel = true;
    }

    foreach (const QWeakPointer<AbstractView> &view, m_viewList) {
        Q_ASSERT(view != 0);
        view->instanceInformationsChange(convertModelNodeInformationHash(informationChangeHash, view.data()));
    }

    if (nodeInstanceView())
        nodeInstanceView()->instanceInformationsChange(convertModelNodeInformationHash(informationChangeHash, nodeInstanceView()));

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
    } catch (RewritingException &e) {
        description = e.description();
        resetModel = true;
    }

    foreach (const QWeakPointer<AbstractView> &view, m_viewList) {
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
    } catch (RewritingException &e) {
        description = e.description();
        resetModel = true;
    }

    foreach (const QWeakPointer<AbstractView> &view, m_viewList) {
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
    } catch (RewritingException &e) {
        description = e.description();
        resetModel = true;
    }

    foreach (const QWeakPointer<AbstractView> &view, m_viewList) {
        Q_ASSERT(view != 0);
        view->instancesChildrenChanged(toModelNodeVector(internalVector, view.data()));
    }

    if (nodeInstanceView())
        nodeInstanceView()->instancesChildrenChanged(toModelNodeVector(internalVector, nodeInstanceView()));

    if (resetModel)
        resetModelByRewriter(description);
}

void ModelPrivate::notifyActualStateChanged(const ModelNode &node)
{
    bool resetModel = false;
    QString description;

    m_acutalStateNode = node.internalNode();

    try {
        if (rewriterView())
            rewriterView()->actualStateChanged(ModelNode(node.internalNode(), model(), rewriterView()));
    } catch (RewritingException &e) {
        description = e.description();
        resetModel = true;
    }

    foreach (const QWeakPointer<AbstractView> &view, m_viewList) {
        Q_ASSERT(view != 0);
        view->actualStateChanged(ModelNode(node.internalNode(), model(), view.data()));
    }

    if (nodeInstanceView())
        nodeInstanceView()->actualStateChanged(ModelNode(node.internalNode(), model(), nodeInstanceView()));

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
    } catch (RewritingException &e) {
        description = e.description();
        resetModel = true;
    }

    foreach (const QWeakPointer<AbstractView> &view, m_viewList) {
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
    } catch (RewritingException &e) {
        description = e.description();
        resetModel = true;
    }

    foreach (const QWeakPointer<AbstractView> &view, m_viewList) {
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
    } catch (RewritingException &e) {
        description = e.description();
        resetModel = true;
    }

    foreach (const QWeakPointer<AbstractView> &view, m_viewList) {
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
    } catch (RewritingException &e) {
        description = e.description();
        resetModel = true;
    }

    foreach (const QWeakPointer<AbstractView> &view, m_viewList) {
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
    } catch (RewritingException &e) {
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

    foreach (const QWeakPointer<AbstractView> &view, m_viewList) {
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
    } catch (RewritingException &e) {
        description = e.description();
        resetModel = true;
    }

    foreach (const QWeakPointer<AbstractView> &view, m_viewList) {
        QList<AbstractProperty> propertyList;
        Q_ASSERT(view != 0);
        foreach (const InternalProperty::Pointer &property, internalPropertyList) {
            AbstractProperty newProperty(property->name(), property->propertyOwner(), model(), view.data());
            propertyList.append(newProperty);
        }
        try {
            view->propertiesAboutToBeRemoved(propertyList);
        } catch (RewritingException &e) {
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

void ModelPrivate::setAuxiliaryData(const InternalNode::Pointer& node, const QString &name, const QVariant &data)
{
    node->setAuxiliaryData(name, data);
    notifyAuxiliaryDataChanged(node, name,data);
}

void ModelPrivate::resetModelByRewriter(const QString &description)
{
    if (rewriterView())
        rewriterView()->resetToLastCorrectQml();

    throw RewritingException(__LINE__, __FUNCTION__, __FILE__, description, rewriterView()->textModifierContent());
}


void ModelPrivate::attachView(AbstractView *view)
{
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
    } catch (RewritingException &e) {
        description = e.description();
        resetModel = true;
    }

    if (nodeInstanceView()) {
        ModelNode createdNode(newInternalNodePointer, model(), nodeInstanceView());
        nodeInstanceView()->nodeCreated(createdNode);
    }

    foreach (const QWeakPointer<AbstractView> &view, m_viewList) {
        Q_ASSERT(view != 0);
        ModelNode createdNode(newInternalNodePointer, model(), view.data());
        view->nodeCreated(createdNode);
    }

    if (resetModel)
        resetModelByRewriter(description);
}

void ModelPrivate::notifyNodeAboutToBeRemoved(const InternalNode::Pointer &nodePointer)
{
    bool resetModel = false;
    QString description;

    try {
        if (rewriterView()) {
            ModelNode node(nodePointer, model(), rewriterView());
            rewriterView()->nodeAboutToBeRemoved(node);
        }
    } catch (RewritingException &e) {
        description = e.description();
        resetModel = true;
    }

    foreach (const QWeakPointer<AbstractView> &view, m_viewList) {
        Q_ASSERT(view != 0);
        ModelNode node(nodePointer, model(), view.data());
        view->nodeAboutToBeRemoved(node);
    }

    if (nodeInstanceView()) {
        ModelNode node(nodePointer, model(), nodeInstanceView());
        nodeInstanceView()->nodeAboutToBeRemoved(node);
    }

    if (resetModel)
        resetModelByRewriter(description);
}

void ModelPrivate::notifyNodeRemoved(const InternalNodePointer &nodePointer, const InternalNodePointer &parentNodePointer, const QString &parentPropertyName, AbstractView::PropertyChangeFlags propertyChange)
{
    bool resetModel = false;
    QString description;

    try {
        if (rewriterView()) {
            ModelNode node(nodePointer, model(), rewriterView());
            NodeAbstractProperty parentProperty(parentPropertyName, parentNodePointer, model(), rewriterView());
            rewriterView()->nodeRemoved(node, parentProperty, propertyChange);
        }
    } catch (RewritingException &e) {
        description = e.description();
        resetModel = true;
    }

    if (nodeInstanceView()) {
        ModelNode node(nodePointer, model(), nodeInstanceView());
        NodeAbstractProperty parentProperty(parentPropertyName, parentNodePointer, model(), nodeInstanceView());
        nodeInstanceView()->nodeRemoved(node, parentProperty, propertyChange);
    }

    foreach (const QWeakPointer<AbstractView> &view, m_viewList) {
        Q_ASSERT(view != 0);
        ModelNode node(nodePointer, model(), view.data());
        NodeAbstractProperty parentProperty(parentPropertyName, parentNodePointer, model(), view.data());
        view->nodeRemoved(node, parentProperty, propertyChange);

    }

    if (resetModel)
        resetModelByRewriter(description);
}

void ModelPrivate::notifyNodeIdChanged(const InternalNode::Pointer& nodePointer, const QString& newId, const QString& oldId)
{
    bool resetModel = false;
    QString description;

    try {
        if (rewriterView()) {
            ModelNode node(nodePointer, model(), rewriterView());
            rewriterView()->nodeIdChanged(node, newId, oldId);
        }
    } catch (RewritingException &e) {
        description = e.description();
        resetModel = true;
    }

    foreach (const QWeakPointer<AbstractView> &view, m_viewList) {
        Q_ASSERT(view != 0);
        ModelNode node(nodePointer, model(), view.data());
        view->nodeIdChanged(node, newId, oldId);
    }

    if (nodeInstanceView()) {
        ModelNode node(nodePointer, model(), nodeInstanceView());
        nodeInstanceView()->nodeIdChanged(node, newId, oldId);
    }

    if (resetModel)
        resetModelByRewriter(description);
}

void ModelPrivate::notifyBindingPropertiesChanged(const QList<InternalBindingPropertyPointer> &internalBropertyList, AbstractView::PropertyChangeFlags propertyChange)
{
    bool resetModel = false;
    QString description;

    try {
        if (rewriterView()) {
            QList<BindingProperty> propertyList;
            foreach (const InternalBindingPropertyPointer &bindingProperty, internalBropertyList) {
                propertyList.append(BindingProperty(bindingProperty->name(), bindingProperty->propertyOwner(), model(), rewriterView()));
            }
            rewriterView()->bindingPropertiesChanged(propertyList, propertyChange);
        }
    } catch (RewritingException &e) {
        description = e.description();
        resetModel = true;
    }

    foreach (const QWeakPointer<AbstractView> &view, m_viewList) {
        Q_ASSERT(view != 0);
        QList<BindingProperty> propertyList;
        foreach (const InternalBindingPropertyPointer &bindingProperty, internalBropertyList) {
            propertyList.append(BindingProperty(bindingProperty->name(), bindingProperty->propertyOwner(), model(), view.data()));
        }
        view->bindingPropertiesChanged(propertyList, propertyChange);

    }

    if (nodeInstanceView()) {
        QList<BindingProperty> propertyList;
        foreach (const InternalBindingPropertyPointer &bindingProperty, internalBropertyList) {
            propertyList.append(BindingProperty(bindingProperty->name(), bindingProperty->propertyOwner(), model(), nodeInstanceView()));
        }
        nodeInstanceView()->bindingPropertiesChanged(propertyList, propertyChange);
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
    } catch (RewritingException &e) {
        description = e.description();
        resetModel = true;
    }

    if (nodeInstanceView()) {
        ModelNode node(internalNodePointer, model(), nodeInstanceView());
        nodeInstanceView()->scriptFunctionsChanged(node, scriptFunctionList);
    }

    foreach (const QWeakPointer<AbstractView> &view, m_viewList) {
        Q_ASSERT(view != 0);

        ModelNode node(internalNodePointer, model(), view.data());
        view->scriptFunctionsChanged(node, scriptFunctionList);

    }

    if (resetModel)
        resetModelByRewriter(description);
}


void ModelPrivate::notifyVariantPropertiesChanged(const InternalNodePointer &internalNodePointer, const QStringList& propertyNameList, AbstractView::PropertyChangeFlags propertyChange)
{
    bool resetModel = false;
    QString description;

    try {
        if (rewriterView()) {
            QList<VariantProperty> propertyList;
            foreach (const QString &propertyName, propertyNameList) {
                Q_ASSERT(internalNodePointer->hasProperty(propertyName));
                Q_ASSERT(internalNodePointer->property(propertyName)->isVariantProperty());
                VariantProperty property(propertyName, internalNodePointer, model(), rewriterView());
                propertyList.append(property);
            }

            ModelNode node(internalNodePointer, model(), rewriterView());
            rewriterView()->variantPropertiesChanged(propertyList, propertyChange);
        }
    } catch (RewritingException &e) {
        description = e.description();
        resetModel = true;
    }


    foreach (const QWeakPointer<AbstractView> &view, m_viewList) {
        QList<VariantProperty> propertyList;
        Q_ASSERT(view != 0);
        foreach (const QString &propertyName, propertyNameList) {
            Q_ASSERT(internalNodePointer->hasProperty(propertyName));
            Q_ASSERT(internalNodePointer->property(propertyName)->isVariantProperty());
            VariantProperty property(propertyName, internalNodePointer, model(), view.data());
            propertyList.append(property);
        }
        ModelNode node(internalNodePointer, model(), view.data());
        view->variantPropertiesChanged(propertyList, propertyChange);

    }

    if (nodeInstanceView()) {
        QList<VariantProperty> propertyList;
        foreach (const QString &propertyName, propertyNameList) {
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

void ModelPrivate::notifyNodeAboutToBeReparent(const InternalNodePointer &internalNodePointer, const InternalNodeAbstractPropertyPointer &newPropertyParent, const InternalNodePointer &oldParent, const QString &oldPropertyName, AbstractView::PropertyChangeFlags propertyChange)
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
            rewriterView()->nodeAboutToBeReparented(node, newProperty, oldProperty, propertyChange);
        }
    } catch (RewritingException &e) {
        description = e.description();
        resetModel = true;
    }

    foreach (const QWeakPointer<AbstractView> &view, m_viewList) {
        NodeAbstractProperty newProperty;
        NodeAbstractProperty oldProperty;

        Q_ASSERT(!view.isNull());
        if (!oldPropertyName.isEmpty() && oldParent->isValid())
            oldProperty = NodeAbstractProperty(oldPropertyName, oldParent, model(), view.data());

        if (!newPropertyParent.isNull())
            newProperty = NodeAbstractProperty(newPropertyParent, model(), view.data());
        ModelNode node(internalNodePointer, model(), view.data());

        view->nodeAboutToBeReparented(node, newProperty, oldProperty, propertyChange);

    }

    if (nodeInstanceView()) {
        NodeAbstractProperty newProperty;
        NodeAbstractProperty oldProperty;

        if (!oldPropertyName.isEmpty() && oldParent->isValid())
            oldProperty = NodeAbstractProperty(oldPropertyName, oldParent, model(), nodeInstanceView());

        if (!newPropertyParent.isNull())
            newProperty = NodeAbstractProperty(newPropertyParent, model(), nodeInstanceView());
        ModelNode node(internalNodePointer, model(), nodeInstanceView());
        nodeInstanceView()->nodeAboutToBeReparented(node, newProperty, oldProperty, propertyChange);
    }

    if (resetModel)
        resetModelByRewriter(description);
}


void ModelPrivate::notifyNodeReparent(const InternalNode::Pointer &internalNodePointer, const InternalNodeAbstractProperty::Pointer &newPropertyParent, const InternalNodePointer &oldParent, const QString &oldPropertyName, AbstractView::PropertyChangeFlags propertyChange)
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
    } catch (RewritingException &e) {
        description = e.description();
        resetModel = true;
    }

    foreach (const QWeakPointer<AbstractView> &view, m_viewList) {
        NodeAbstractProperty newProperty;
        NodeAbstractProperty oldProperty;

        Q_ASSERT(!view.isNull());
        if (!oldPropertyName.isEmpty() && oldParent->isValid())
            oldProperty = NodeAbstractProperty(oldPropertyName, oldParent, model(), view.data());

        if (!newPropertyParent.isNull())
            newProperty = NodeAbstractProperty(newPropertyParent, model(), view.data());
        ModelNode node(internalNodePointer, model(), view.data());

        view->nodeReparented(node, newProperty, oldProperty, propertyChange);

    }

    if (nodeInstanceView()) {
        NodeAbstractProperty newProperty;
        NodeAbstractProperty oldProperty;

        if (!oldPropertyName.isEmpty() && oldParent->isValid())
            oldProperty = NodeAbstractProperty(oldPropertyName, oldParent, model(), nodeInstanceView());

        if (!newPropertyParent.isNull())
            newProperty = NodeAbstractProperty(newPropertyParent, model(), nodeInstanceView());
        ModelNode node(internalNodePointer, model(), nodeInstanceView());
        nodeInstanceView()->nodeReparented(node, newProperty, oldProperty, propertyChange);
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
    } catch (RewritingException &e) {
        description = e.description();
        resetModel = true;
    }

    foreach (const QWeakPointer<AbstractView> &view, m_viewList) {
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
    qSort(sortedSelectedList);

    if (sortedSelectedList == m_selectedNodeList)
        return;


    const QList<InternalNode::Pointer> lastSelectedNodeList = m_selectedNodeList;
    m_selectedNodeList = sortedSelectedList;

    changeSelectedNodes(sortedSelectedList, lastSelectedNodeList);
}


void ModelPrivate::clearSelectedNodes()
{
    const QList<InternalNode::Pointer> lastSelectedNodeList = m_selectedNodeList;
    m_selectedNodeList.clear();
    changeSelectedNodes(m_selectedNodeList, lastSelectedNodeList);
}

QList<ModelNode> ModelPrivate::toModelNodeList(const QList<InternalNode::Pointer> &nodeList, AbstractView *view) const
{
    QList<ModelNode> newNodeList;
    foreach (const Internal::InternalNode::Pointer &node, nodeList)
        newNodeList.append(ModelNode(node, model(), view));

    return newNodeList;
}

QVector<ModelNode> ModelPrivate::toModelNodeVector(const QVector<InternalNode::Pointer> &nodeVector, AbstractView *view) const
{
    QVector<ModelNode> newNodeVector;
    foreach (const Internal::InternalNode::Pointer &node, nodeVector)
        newNodeVector.append(ModelNode(node, model(), view));

    return newNodeVector;
}

QList<Internal::InternalNode::Pointer> ModelPrivate::toInternalNodeList(const QList<ModelNode> &nodeList) const
{
    QList<Internal::InternalNode::Pointer> newNodeList;
    foreach (const ModelNode &node, nodeList)
        newNodeList.append(node.internalNode());

    return newNodeList;
}

QVector<Internal::InternalNode::Pointer> ModelPrivate::toInternalNodeVector(const QVector<ModelNode> &nodeVector) const
{
    QVector<Internal::InternalNode::Pointer> newNodeVector;
    foreach (const ModelNode &node, nodeVector)
        newNodeVector.append(node.internalNode());

    return newNodeVector;
}

void ModelPrivate::changeSelectedNodes(const QList<InternalNode::Pointer> &newSelectedNodeList,
                                       const QList<InternalNode::Pointer> &oldSelectedNodeList)
{
    foreach (const QWeakPointer<AbstractView> &view, m_viewList) {
        Q_ASSERT(view != 0);
        view->selectedNodesChanged(toModelNodeList(newSelectedNodeList, view.data()), toModelNodeList(oldSelectedNodeList, view.data()));
    }
}

QList<InternalNode::Pointer> ModelPrivate::selectedNodes() const
{
    foreach (const InternalNode::Pointer &node, m_selectedNodeList) {
        if (!node->isValid())
            throw new InvalidModelNodeException(__LINE__, __FUNCTION__, __FILE__);
    }

    return m_selectedNodeList;
}

void ModelPrivate::selectNode(const InternalNode::Pointer &node)
{
    if (selectedNodes().contains(node))
        return;

    QList<InternalNode::Pointer> selectedNodeList(selectedNodes());
    selectedNodeList += node;
    setSelectedNodes(selectedNodeList);
}

void ModelPrivate::deselectNode(const InternalNode::Pointer &node)
{
    QList<InternalNode::Pointer> selectedNodeList(selectedNodes());
    bool isRemoved = selectedNodeList.removeOne(node);

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
        propertyPairList.append(qMakePair(property->propertyOwner(), property->name()));

    return propertyPairList;

}

void ModelPrivate::removeProperty(const InternalProperty::Pointer &property)
{
    notifyPropertiesAboutToBeRemoved(QList<InternalProperty::Pointer>() << property);

    QList<PropertyPair> propertyPairList = toPropertyPairList(QList<InternalProperty::Pointer>() << property);

    removePropertyWithoutNotification(property);

    notifyPropertiesRemoved(propertyPairList);
}

void ModelPrivate::setBindingProperty(const InternalNode::Pointer &internalNode, const QString &name, const QString &expression)
{
    AbstractView::PropertyChangeFlags propertyChange = AbstractView::NoAdditionalChanges;
    if (!internalNode->hasProperty(name)) {
        internalNode->addBindingProperty(name);
        propertyChange = AbstractView::PropertiesAdded;
    }

    InternalBindingProperty::Pointer bindingProperty = internalNode->bindingProperty(name);
    bindingProperty->setExpression(expression);
    notifyBindingPropertiesChanged(QList<InternalBindingPropertyPointer>() << bindingProperty, propertyChange);
}

void ModelPrivate::setVariantProperty(const InternalNode::Pointer &internalNode, const QString &name, const QVariant &value)
{
    AbstractView::PropertyChangeFlags propertyChange = AbstractView::NoAdditionalChanges;
    if (!internalNode->hasProperty(name)) {
        internalNode->addVariantProperty(name);
        propertyChange = AbstractView::PropertiesAdded;
    }

    internalNode->variantProperty(name)->setValue(value);
    internalNode->variantProperty(name)->resetDynamicTypeName();
    notifyVariantPropertiesChanged(internalNode, QStringList() << name, propertyChange);
}

void ModelPrivate::setDynamicVariantProperty(const InternalNodePointer &internalNode, const QString &name, const QString &dynamicPropertyType, const QVariant &value)
{
    AbstractView::PropertyChangeFlags propertyChange = AbstractView::NoAdditionalChanges;
    if (!internalNode->hasProperty(name)) {
        internalNode->addVariantProperty(name);
        propertyChange = AbstractView::PropertiesAdded;
    }

    internalNode->variantProperty(name)->setDynamicValue(dynamicPropertyType, value);
    notifyVariantPropertiesChanged(internalNode, QStringList() << name, propertyChange);
}

void ModelPrivate::setDynamicBindingProperty(const InternalNodePointer &internalNode, const QString &name, const QString &dynamicPropertyType, const QString &expression)
{
    AbstractView::PropertyChangeFlags propertyChange = AbstractView::NoAdditionalChanges;
    if (!internalNode->hasProperty(name)) {
        internalNode->addBindingProperty(name);
        propertyChange = AbstractView::PropertiesAdded;
    }

    InternalBindingProperty::Pointer bindingProperty = internalNode->bindingProperty(name);
    bindingProperty->setDynamicExpression(dynamicPropertyType, expression);
    notifyBindingPropertiesChanged(QList<InternalBindingPropertyPointer>() << bindingProperty, propertyChange);
}

void ModelPrivate::reparentNode(const InternalNode::Pointer &newParentNode, const QString &name, const InternalNode::Pointer &node, bool list)
{
    AbstractView::PropertyChangeFlags propertyChange = AbstractView::NoAdditionalChanges;
    if (!newParentNode->hasProperty(name)) {
        if (list)
            newParentNode->addNodeListProperty(name);
        else
            newParentNode->addNodeProperty(name);
        propertyChange |= AbstractView::PropertiesAdded;
    }

    InternalNodeAbstractProperty::Pointer oldParentProperty(node->parentProperty());
    InternalNode::Pointer oldParentNode;
    QString oldParentPropertyName;
    if (oldParentProperty && oldParentProperty->isValid()) {
        oldParentNode = node->parentProperty()->propertyOwner();
        oldParentPropertyName = node->parentProperty()->name();
    }

    InternalNodeAbstractProperty::Pointer newParentProperty(newParentNode->nodeAbstractProperty(name));
    Q_ASSERT(!newParentProperty.isNull());

    notifyNodeAboutToBeReparent(node, newParentProperty, oldParentNode, oldParentPropertyName, propertyChange);

    if (newParentProperty)
        node->setParentProperty(newParentProperty);


    if (oldParentProperty && oldParentProperty->isValid() && oldParentProperty->isEmpty()) {
        removePropertyWithoutNotification(oldParentProperty);

        propertyChange |= AbstractView::EmptyPropertiesRemoved;
    }

    notifyNodeReparent(node, newParentProperty, oldParentNode, oldParentPropertyName, propertyChange);
}

void ModelPrivate::clearParent(const InternalNodePointer &node)
{

    InternalNodeAbstractProperty::Pointer oldParentProperty(node->parentProperty());
    InternalNode::Pointer oldParentNode;
    QString oldParentPropertyName;
    if (oldParentProperty->isValid()) {
        oldParentNode = node->parentProperty()->propertyOwner();
        oldParentPropertyName = node->parentProperty()->name();
    }

    node->resetParentProperty();
    notifyNodeReparent(node, InternalNodeAbstractProperty::Pointer(), oldParentNode, oldParentPropertyName, AbstractView::NoAdditionalChanges);
}

void ModelPrivate::changeRootNodeType(const QString &type, int majorVersion, int minorVersion)
{
    Q_ASSERT(!rootNode().isNull());
    rootNode()->setType(type);
    rootNode()->setMajorVersion(majorVersion);
    rootNode()->setMinorVersion(minorVersion);
    notifyRootNodeTypeChanged(type, majorVersion, minorVersion);
}

void ModelPrivate::setScriptFunctions(const InternalNode::Pointer &internalNode, const QStringList &scriptFunctionList)
{
    internalNode->setScriptFunctions(scriptFunctionList);

    notifyScriptFunctionsChanged(internalNode, scriptFunctionList);
}

void ModelPrivate::setNodeSource(const InternalNodePointer &internalNode, const QString &nodeSource)
{
    internalNode->setNodeSource(nodeSource);
    notifyNodeSourceChanged(internalNode, nodeSource);
}

void ModelPrivate::changeNodeOrder(const InternalNode::Pointer &internalParentNode, const QString &listPropertyName, int from, int to)
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

InternalNode::Pointer ModelPrivate::actualStateNode() const
{
    return m_acutalStateNode;
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


Model *Model::create(QString type, int major, int minor, Model *metaInfoPropxyModel)
{
    return Internal::ModelPrivate::create(type, major, minor, metaInfoPropxyModel);
}

QList<Import> Model::imports() const
{
    return d->imports();
}

void Model::changeImports(const QList<Import> &importsToBeAdded, const QList<Import> &importsToBeRemoved)
{
    d->changeImports(importsToBeAdded, importsToBeRemoved);
}


static bool compareVersions(const QString &version1, const QString &version2, bool allowHigherVersion)
{
    if (version1 == version2)
        return true;
    if (!allowHigherVersion)
        return false;
    QStringList version1List = version1.split('.');
    QStringList version2List = version2.split('.');
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

   return  rewriterView()->pathForImport(import);
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

#if 0
/*!
 \brief Creates a new empty model
 \param uiFilePath path to the ui file
 \param[out] errorMessage returns a error message
 \return new created model
*/
Model *Model::create(const QString &rootType)
{
    return Internal::ModelPrivate::create(rootType);
}
#endif

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

bool Model::hasNodeMetaInfo(const QString &typeName, int majorVersion, int minorVersion)
{
    return NodeMetaInfo(metaInfoProxyModel(), typeName, majorVersion, minorVersion).isValid();
}

NodeMetaInfo Model::metaInfo(const QString &typeName, int majorVersion, int minorVersion)
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
    RewriterView *rewriterView = qobject_cast<RewriterView*>(view);
    if (rewriterView) {
        return;
    }

    NodeInstanceView *nodeInstanceView = qobject_cast<NodeInstanceView*>(view);
    if (nodeInstanceView) {
        return;
    }

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
    if (rewriterView) {
        return;
    }

    NodeInstanceView *nodeInstanceView = qobject_cast<NodeInstanceView*>(view);
    if (nodeInstanceView) {
        return;
    }

    d->detachView(view, emitNotify);
}


}
