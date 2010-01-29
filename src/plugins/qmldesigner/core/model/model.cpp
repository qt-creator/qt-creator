/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include <model.h>
#include <modelnode.h>
#include "internalnode_p.h"
#include "invalidpropertyexception.h"
#include "invalidargumentexception.h"

#include <QtCore/QFile>
#include <QtCore/QByteArray>
#include <QWeakPointer>
#include <QtCore/QFileInfo>

#include <QtGui/QUndoStack>
#include <QtXml/QXmlStreamReader>
#include <QtCore/QDebug>
#include <QPlainTextEdit>

#include "abstractview.h"
#include "widgetqueryview.h"
#include "metainfo.h"
#include "model_p.h"
#include "modelutilities.h"
#include "subcomponentmanager.h"
#include "variantparser.h"
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
        m_rootInternalNode(createNode("Qt/Rectangle", 4, 6, PropertyListType())),
        m_writeLock(false)
{
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
}

Model *ModelPrivate::create(QString type, int major, int minor)
{
    Model *model = new Model;

    model->m_d->rootNode()->setType(type);
    model->m_d->rootNode()->setMajorVersion(major);
    model->m_d->rootNode()->setMinorVersion(minor);

    return model;
}

void ModelPrivate::setImports(const QSet<Import> &imports)
{
    QList<Import> added = QSet<Import>(imports).subtract(m_imports).toList();
    QList<Import> removed = QSet<Import>(m_imports).subtract(imports).toList();

    if (added.isEmpty() && removed.isEmpty())
        return;

    m_imports = imports;

    notifyImportsChanged();
}

void ModelPrivate::addImport(const Import &import)
{
    if (m_imports.contains(import))
        return;

    m_imports.insert(import);
    notifyImportsChanged();
}

void ModelPrivate::removeImport(const Import &import)
{
    if (!m_imports.remove(import))
        return;

    notifyImportsChanged();
}

void ModelPrivate::notifyImportsChanged() const
{
    foreach (const QWeakPointer<AbstractView> &view, m_viewList)
        view->importsChanged();
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
                                               const QList<QPair<QString, QVariant> > &propertyList)
{
    if (typeString.isEmpty())
        throw InvalidArgumentException(__LINE__, __FUNCTION__, __FILE__, "typeString");
    if (!m_metaInfo.nodeMetaInfo(typeString).isValid())
        throw InvalidArgumentException(__LINE__, __FUNCTION__, __FILE__, "typeString");

    InternalNode::Pointer newInternalNodePointer = InternalNode::create(typeString, majorVersion, minorVersion);

    typedef QPair<QString, QVariant> PropertyPair;

    foreach (const PropertyPair &propertyPair, propertyList) {
        newInternalNodePointer->addVariantProperty(propertyPair.first);
        newInternalNodePointer->variantProperty(propertyPair.first)->setValue(propertyPair.second);
    }

    m_nodeSet.insert(newInternalNodePointer);

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
    notifyNodeIdChanged(internalNodePointer, id, oldId);
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

    foreach (const QWeakPointer<AbstractView> &view, m_viewList) {
        Q_ASSERT(view != 0);
        ModelNode node(internalNode, model(), view.data());
        try {
            view->auxiliaryDataChanged(node, name, data);
        } catch (RewritingException &e) {
            description = e.description();
            resetModel = true;
        }
    }

    if (resetModel) {
        resetModelByRewriter(description);
    }
}

void ModelPrivate::notifyNodeTypeChanged(const InternalNodePointer &internalNode, const QString &type, int majorVersion, int minorVersion)
{
    bool resetModel = false;
    QString description;

    foreach (const QWeakPointer<AbstractView> &view, m_viewList) {
        Q_ASSERT(view != 0);
        ModelNode node(internalNode, model(), view.data());
        try {
            view->nodeTypeChanged(node, type, majorVersion, minorVersion);
        } catch (RewritingException &e) {
            description = e.description();
            resetModel = true;
        }
    }

    if (resetModel) {
        resetModelByRewriter(description);
    }
}

void ModelPrivate::notifyCustomNotification(const AbstractView *senderView, const QString &identifier, const QList<ModelNode> &nodeList, const QList<QVariant> &data)
{
    bool resetModel = false;
    QString description;

    QList<Internal::InternalNode::Pointer> internalList(toInternalNodeList(nodeList));
    foreach (const QWeakPointer<AbstractView> &view, m_viewList) {
        Q_ASSERT(view != 0);
        try {
            view->customNotification(senderView, identifier, toModelNodeList(internalList, view.data()), data);
        } catch (RewritingException &e) {
            description = e.description();
            resetModel = true;
        }
    }

    if (resetModel) {
        resetModelByRewriter(description);
    }
}


void ModelPrivate::notifyPropertiesRemoved(const QList<PropertyPair> &propertyPairList)
{
    bool resetModel = false;
    QString description;

    foreach (const QWeakPointer<AbstractView> &view, m_viewList) {
        QList<AbstractProperty> propertyList;
        Q_ASSERT(view != 0);
        foreach (const PropertyPair &propertyPair, propertyPairList) {
            AbstractProperty newProperty(propertyPair.second, propertyPair.first, model(), view.data());
            propertyList.append(newProperty);
        }
        try {
            view->propertiesRemoved(propertyList);
        } catch (RewritingException &e) {
            description = e.description();
            resetModel = true;
        }
    }

    if (resetModel) {
        resetModelByRewriter(description);
    }
}

void ModelPrivate::notifyPropertiesAboutToBeRemoved(const QList<InternalProperty::Pointer> &internalPropertyList)
{
    bool resetModel = false;
    QString description;

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

    if (resetModel) {
        resetModelByRewriter(description);
    }
}

void ModelPrivate::setAuxiliaryData(const InternalNode::Pointer& node, const QString &name, const QVariant &data)
{
    node->setAuxiliaryData(name, data);
    notifyAuxiliaryDataChanged(node, name,data);
}

void ModelPrivate::resetModelByRewriter(const QString &description)
{
    RewriterView* rewriterView = 0;
    foreach (const QWeakPointer<AbstractView> &view, m_viewList) {
        if (!rewriterView)
            rewriterView = qobject_cast<RewriterView*>(view.data());
    }
    Q_ASSERT(rewriterView);
    rewriterView->resetToLastCorrectQml();
    throw RewritingException(__LINE__, __FUNCTION__, __FILE__, description);
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

    foreach (const QWeakPointer<AbstractView> &view, m_viewList) {
        Q_ASSERT(view != 0);
        ModelNode createdNode(newInternalNodePointer, model(), view.data());

        try {
            view->nodeCreated(createdNode);
        } catch (RewritingException &e) {
            description = e.description();
            resetModel = true;
        }
    }
    if (resetModel) {
        resetModelByRewriter(description);
    }
}

void ModelPrivate::notifyNodeAboutToBeRemoved(const InternalNode::Pointer &nodePointer)
{
    bool resetModel = false;
    QString description;

    foreach (const QWeakPointer<AbstractView> &view, m_viewList) {
        Q_ASSERT(view != 0);
        ModelNode node(nodePointer, model(), view.data());
        try {
            view->nodeAboutToBeRemoved(node);
        } catch (RewritingException &e) {
            description = e.description();
            resetModel = true;
        }
    }
    if (resetModel) {
        resetModelByRewriter(description);
    }
}

void ModelPrivate::notifyNodeRemoved(const InternalNodePointer &nodePointer, const InternalNodePointer &parentNodePointer, const QString &parentPropertyName, AbstractView::PropertyChangeFlags propertyChange)
{
    bool resetModel = false;
    QString description;

    foreach (const QWeakPointer<AbstractView> &view, m_viewList) {
        Q_ASSERT(view != 0);
        ModelNode node(nodePointer, model(), view.data());
        NodeAbstractProperty parentProperty(parentPropertyName, parentNodePointer, model(), view.data());
        try {
            view->nodeRemoved(node, parentProperty, propertyChange);
        } catch (RewritingException &e) {
            description = e.description();
            resetModel = true;
        }
    }
    if (resetModel) {
        resetModelByRewriter(description);
    }
}

void ModelPrivate::notifyNodeIdChanged(const InternalNode::Pointer& nodePointer, const QString& newId, const QString& oldId)
{
    bool resetModel = false;
    QString description;

    foreach (const QWeakPointer<AbstractView> &view, m_viewList) {
        Q_ASSERT(view != 0);
        ModelNode node(nodePointer, model(), view.data());
        try {
            view->nodeIdChanged(node, newId, oldId);
        } catch (RewritingException &e) {
            description = e.description();
            resetModel = true;
        }
    }
    if (resetModel) {
        resetModelByRewriter(description);
    }
}

void ModelPrivate::notifyBindingPropertiesChanged(const QList<InternalBindingPropertyPointer> &internalBropertyList, AbstractView::PropertyChangeFlags propertyChange)
{
    bool resetModel = false;
    QString description;

    foreach (const QWeakPointer<AbstractView> &view, m_viewList) {
        QList<BindingProperty> propertyList;
        Q_ASSERT(view != 0);
        foreach (const InternalBindingPropertyPointer &bindingProperty, internalBropertyList) {
            propertyList.append(BindingProperty(bindingProperty->name(), bindingProperty->propertyOwner(), model(), view.data()));
        }
        try {
            view->bindingPropertiesChanged(propertyList, propertyChange);
        } catch (RewritingException &e) {
            description = e.description();
            resetModel = true;
        }
    }
    if (resetModel) {
        resetModelByRewriter(description);
    }
}

void ModelPrivate::notifyVariantPropertiesChanged(const InternalNodePointer &internalNodePointer, const QStringList& propertyNameList, AbstractView::PropertyChangeFlags propertyChange)
{
    bool resetModel = false;
    QString description;

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
        try {
            view->variantPropertiesChanged(propertyList, propertyChange);
        } catch (RewritingException &e) {
            description = e.description();
            resetModel = true;
        }
    }
    if (resetModel) {
        resetModelByRewriter(description);
    }
}

void ModelPrivate::notifyNodeReparent(const InternalNode::Pointer &internalNodePointer, const InternalNodeAbstractProperty::Pointer &newPropertyParent, const InternalNodePointer &oldParent, const QString &oldPropertyName, AbstractView::PropertyChangeFlags propertyChange)
{
    bool resetModel = false;
    QString description;

    foreach (const QWeakPointer<AbstractView> &view, m_viewList) {
        NodeAbstractProperty newProperty;
        NodeAbstractProperty oldProperty;

        Q_ASSERT(!view.isNull());
        if (!oldPropertyName.isEmpty() && oldParent->isValid())
            oldProperty = NodeAbstractProperty(oldPropertyName, oldParent, model(), view.data());

        if (!newPropertyParent.isNull())
            newProperty = NodeAbstractProperty(newPropertyParent, model(), view.data());
        ModelNode node(internalNodePointer, model(), view.data());
        try {
            view->nodeReparented(node, newProperty, oldProperty, propertyChange);
        } catch (RewritingException &e) {
            description = e.description();
            resetModel = true;
        }
    }
    if (resetModel) {
        resetModelByRewriter(description);
    }
}

void ModelPrivate::notifyNodeOrderChanged(const InternalNodeListPropertyPointer &internalListPropertyPointer,
                                          const InternalNode::Pointer &internalNodePointer,
                                          int oldIndex)
{
    bool resetModel = false;
    QString description;

    foreach (const QWeakPointer<AbstractView> &view, m_viewList) {
        Q_ASSERT(!view.isNull());
        try {
            view->nodeOrderChanged(NodeListProperty(internalListPropertyPointer, model(), view.data()),
                                   ModelNode(internalNodePointer, model(), view.data()),
                                   oldIndex);
        } catch (RewritingException &e) {
            description = e.description();
            resetModel = true;
        }
    }
    if (resetModel) {
        resetModelByRewriter(description);
    }
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

QList<Internal::InternalNode::Pointer> ModelPrivate::toInternalNodeList(const QList<ModelNode> &nodeList) const
{
    QList<Internal::InternalNode::Pointer> newNodeList;
    foreach (const ModelNode &node, nodeList)
        newNodeList.append(node.internalNode());

    return newNodeList;
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

void ModelPrivate::changeType(const InternalNodePointer &internalNode, const QString &type, int majorVersion, int minorVersion)
{
    Q_ASSERT(!internalNode.isNull());
    internalNode->setType(type);
    internalNode->setMajorVersion(majorVersion);
    internalNode->setMinorVersion(minorVersion);
    notifyNodeTypeChanged(internalNode, type, majorVersion, minorVersion);
}

void ModelPrivate::changeNodeOrder(const InternalNode::Pointer &internalParentNode, const QString &listPropertyName, int from, int to)
{
    InternalNodeListProperty::Pointer nodeList(internalParentNode->nodeListProperty(listPropertyName));
    Q_ASSERT(!nodeList.isNull());
    nodeList->slide(from, to);

    const InternalNodePointer internalNode = nodeList->nodeList().at(to);
    notifyNodeOrderChanged(nodeList, internalNode, from);
}

void ModelPrivate::setRootNode(const InternalNode::Pointer& newRootNode)
{
    removeNode(m_rootInternalNode);
    m_rootInternalNode = newRootNode;

    if (!m_rootInternalNode.isNull() && m_rootInternalNode->isValid())
        notifyNodeCreated(m_rootInternalNode);
}


InternalNodePointer ModelPrivate::nodeForId(const QString &id) const
{
    return m_idNodeHash.value(id);
}

bool ModelPrivate::hasId(const QString &id) const
{
    return m_idNodeHash.contains(id);
}

QList<InternalNodePointer> ModelPrivate::allNodes() const
{
    return m_nodeSet.toList();
}

bool ModelPrivate::isWriteLocked() const
{
    return m_writeLock;
}


WriteLocker::WriteLocker(ModelPrivate *model)
    : m_model(model)
{
    Q_ASSERT(model);
    if (m_model->m_writeLock)
        qWarning() << "QmlDesigner: Misbehaving view calls back to model!!!";
    // FIXME: Enable it again
    // Q_ASSERT(!m_model->m_writeLock);
    model->m_writeLock = true;
}

WriteLocker::WriteLocker(Model *model)
    : m_model(model->m_d)
{
    Q_ASSERT(model->m_d);
    if (m_model->m_writeLock)
        qWarning() << "QmlDesigner: Misbehaving view calls back to model!!!";
    // FIXME: Enable it again
    // Q_ASSERT(!m_model->m_writeLock);
    m_model->m_writeLock = true;
}

WriteLocker::~WriteLocker()
{
    if (!m_model->m_writeLock)
        qWarning() << "QmlDesigner: Misbehaving view calls back to model!!!";
    // FIXME: Enable it again
    // Q_ASSERT(m_model->m_writeLock);
    m_model->m_writeLock = false;
}

//static QString anchorLinePropertyValue(const InternalNode::Pointer &sourceNode, const InternalNode::Pointer &targetNode, const AnchorLine::Type &targetType)
//{
//    if (targetNode.isNull() || !targetNode->isValid())
//        return QString();
//
//    if (sourceNode.isNull() || !sourceNode->isValid())
//        return QString();
//
//    if (targetNode == sourceNode)
//        return QString();
//
//    QString anchorLineType = InternalNodeAnchors::lineTypeToString(targetType);
//
//    // handle "parent" and "parent.something"
//    if (targetNode == sourceNode->parentNode()) {
//        if (anchorLineType.isNull())
//            return QLatin1String("parent");
//        else
//            return QString("parent.%1").arg(anchorLineType);
//    }
//
//    if (anchorLineType.isNull())
//        return QString(); // for sibling reference, the type cannot be empty anymore.
//
//    foreach (const InternalNode::Pointer &sibling, sourceNode->parentNode()->childNodes()) {
//        if (sibling == targetNode) {
//            return QString("%1.%2").arg(sibling->id(), anchorLineType);
//        }
//    }
//
//    return QString();
//}

}


Model::Model()  :
   QObject(),
   m_d(new Internal::ModelPrivate(this))
{
}


Model::~Model()
{
    delete m_d;
}


Model *Model::create(QString type, int major, int minor)
{
    return Internal::ModelPrivate::create(type, major, minor);
}


/*!
  \brief Creates a model for a component definition

  Creates a model containing the content of a component node. The component node itself is
  not part of the newly created model; it's the first item defined after "Component {"
  that is the root node.

  \arg componentNode must be valid & of type "Qt/Component"
  \return the newly created model. The caller takes ownership of the object life time.
  */
//Model *Model::createComponentModel(const ModelNode &componentNode)
//{
//
//    if (!componentNode.isValid() || componentNode.type() != "Qt/Component") {
//        throw new InvalidArgumentException(__LINE__, __FUNCTION__, __FILE__, QString("componentNode"));
//    }
//
//    // the data property does not include the Component element itself
//    const TextLocation componentRootLocation
//            = componentNode.baseNodeState().m_internalNodeState->propertyLocation("data");
//
//    TextModifier *textModifier
//            = new Internal::ComponentTextModifier(m_d->m_rewriter->textModifier(),
//                                                  componentRootLocation,
//                                                  m_d->m_rootInternalNode->baseNodeState()->location());
//
//    QList<QmlError> errors;
//    Model *subModel = create(textModifier, m_d->m_fileUrl, &errors);
//
//    Q_ASSERT(subModel);
//    Q_ASSERT(errors.size() == 0); // should never happen, after all it was already compiled!
//
//    textModifier->setParent(subModel);
//
//    return subModel;
//}

QSet<Import> Model::imports() const
{
    return m_d->imports();
}

void Model::addImport(const Import &import)
{
    m_d->addImport(import);
}

void Model::removeImport(const Import &import)
{
    m_d->removeImport(import);
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

Model *Model::masterModel() const
{
    return m_d->m_masterModel.data();
}

void Model::setMasterModel(Model *model)
{
    m_d->m_masterModel = model;
}

/*!
  \brief Returns the URL against which relative URLs within the model should be resolved.
  \return The base URL.
  */
QUrl Model::fileUrl() const
{
    return m_d->fileUrl();
}

/*!
  \brief Sets the URL against which relative URLs within the model should be resolved.
  \param url the base URL, i.e. the qml file path.
  */
void Model::setFileUrl(const QUrl &url)
{
    Internal::WriteLocker locker(m_d);
    m_d->setFileUrl(url);
}

/*!
  \brief Returns list of Qml types available within the model.
  */
const MetaInfo Model::metaInfo() const
{
    return m_d->metaInfo();
}

/*!
  \brief Sets a specific Metainfo on this Model
  */
void Model::setMetaInfo(const MetaInfo &metaInfo)
{
    Internal::WriteLocker locker(m_d);
    m_d->setMetaInfo(metaInfo);
}

/*!
  \brief Returns list of Qml types available within the model.
  */
MetaInfo Model::metaInfo()
{
    return m_d->metaInfo();
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
//    Internal::WriteLocker locker(m_d);
    m_d->attachView(view);
}

/*!
\brief Detaches a view to the model

\param view The view to unregister. Must be not null.
\param emitDetachNotify If set to NotifyView (the default), AbstractView::modelAboutToBeDetached() will be called

\see attachView
*/
void Model::detachView(AbstractView *view, ViewNotification emitDetachNotify)
{
//    Internal::WriteLocker locker(m_d);
    bool emitNotify = (emitDetachNotify == NotifyView);
    m_d->detachView(view, emitNotify);
}


}
