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

#include "abstractview.h"

#include "model.h"
#include "model_p.h"
#include "nodeproperty.h"
#include "bindingproperty.h"
#include "internalnode_p.h"
#include <qmlmodelview.h>

namespace QmlDesigner {


/*!
\class QmlDesigner::AbstractView
\ingroup CoreModel
\brief An abstract interface views and editors can implement to be notified about model changes.

\see QmlDesigner::WidgetQueryView, QmlDesigner::NodeInstanceView
*/

AbstractView::~AbstractView()
{
    if (m_model)
        m_model.data()->detachView(this, Model::DoNotNotifyView);
}

/*!
\brief sets the view of the model. this is handled automatically by AbstractView::modelAttached.
\param model new Model
*/
void AbstractView::setModel(Model *model)
{
    Q_ASSERT(model != 0);
    if (model == m_model.data())
        return;

    if (m_model)
        m_model.data()->detachView(this);

    m_model = model;
}

RewriterTransaction AbstractView::beginRewriterTransaction()
{
    return RewriterTransaction(this);
}

ModelNode AbstractView::createModelNode(const QString &typeString,
                            int majorVersion,
                            int minorVersion,
                            const QList<QPair<QString, QVariant> > &propertyList,
                            const QList<QPair<QString, QVariant> > &auxPropertyList,
                            const QString &nodeSource,
                            ModelNode::NodeSourceType nodeSourceType)
{
    return ModelNode(model()->d->createNode(typeString, majorVersion, minorVersion, propertyList, auxPropertyList, nodeSource, nodeSourceType), model(), this);
}


/*! \brief returns the root model node
\return constant root model node

*/

const ModelNode AbstractView::rootModelNode() const
{
    Q_ASSERT(model());
    return ModelNode(model()->d->rootNode(), model(), const_cast<AbstractView*>(this));
}


/*! \brief returns the root model node
\return root model node

*/

ModelNode AbstractView::rootModelNode()
{
    Q_ASSERT(model());
    return  ModelNode(model()->d->rootNode(), model(), this);
}

/*!
\brief sets the reference to model to a null pointer

*/
void AbstractView::removeModel()
{
    m_model.clear();
}

/*!
\name Model related functions
\{
*/

/*!
\brief returns the model
\return the model of the view
*/
Model* AbstractView::model() const
{
    return m_model.data();
}

bool AbstractView::isAttached() const
{
    return model();
}

/*!
\brief is called if a view is being attached to a model
\param model which is being attached
The default implementation is setting the reference of the model to the view.
\see Model::attachView
*/
void AbstractView::modelAttached(Model *model)
{
    setModel(model);
}

/*!
\brief is called before a view is being detached from a model
\param model which is being detached

This is not called if Model::detachViewWithOutNotification is used! The default implementation
is removing the reference to the model from the view.

\see Model::detachView
*/
void AbstractView::modelAboutToBeDetached(Model *)
{
    removeModel();
}

//\}


/*!
\name Property related functions
\{
 */

/*!
\fn void QmlDesigner::AbstractView::propertyAdded(const ModelNode &, const AbstractProperty &)
\brief node notifies about that this property is added
\param node node to which the property is added
\param property added property
*/


/*!
\fn void AbstractView::propertyValueChanged(const ModelNode &, const AbstractProperty& , const QVariant& , const QVariant& )
\brief this notifies about that the value of this proeprty will be changes
\param node node of the property
\param property changed property
\param newValue the variant of the new value
\param oldValue the variant of the old value
*/
//\}

/*!
\name Node related functions
\{
 */

/*!
\fn void AbstractView::nodeCreated(const ModelNode &)
\brief this function is called if a new node was created
\param createdNode created node
*/


/*!
\fn AbstractView::fileUrlChanged(const QUrl &oldBaseUrl, const QUrl &newBaseUrl)
\brief Called when the file url (e.g. needed to to resolve relative paths against) has changed
\param oldBaseUrl old search path
\param newBaseUrl new search path
*/
void AbstractView::fileUrlChanged(const QUrl &/*oldUrl*/, const QUrl &/*newUrl*/)
{
}

/*!
\fn void AbstractView::nodeAboutToBeRemoved(const ModelNode &)
\brief this is called if a node will be removed
\param removedNode to be removed node
*/

/*!
\brief this is called after a propererty was removed
\param propertyList removed property list
*/
void AbstractView::propertiesRemoved(const QList<AbstractProperty>& /*propertyList*/)
{
}

/*!
\fn void AbstractView::nodeReparented(const ModelNode &, const ModelNode &, const ModelNode &)
\brief this is called if a node was reparented
\param node the parent for this node will be changed
\param oldParent old parent of the node
\param newParent new parent of the node
*/

/*!
\fn void QmlDesigner::AbstractView::selectedNodesChanged(const QList< ModelNode > &, const QList< ModelNode > &)
\brief this function is called if the selection was changed
\param selectedNodeList the new selection list
\param lastSelectedNodeList the old selection list
*/
//\}

void AbstractView::auxiliaryDataChanged(const ModelNode &/*node*/, const QString &/*name*/, const QVariant &/*data*/)
{

}

void AbstractView::customNotification(const AbstractView * /*view*/, const QString & /*identifier*/, const QList<ModelNode> & /*nodeList*/, const QList<QVariant> & /*data*/)
{
}

QList<ModelNode> AbstractView::toModelNodeList(const QList<Internal::InternalNode::Pointer> &nodeList) const
{
    return QmlDesigner::toModelNodeList(nodeList, const_cast<AbstractView*>(this));
}

QList<ModelNode> toModelNodeList(const QList<Internal::InternalNode::Pointer> &nodeList, AbstractView *view)
{
    QList<ModelNode> newNodeList;
    foreach (const Internal::InternalNode::Pointer &node, nodeList)
        newNodeList.append(ModelNode(node, view->model(), view));

    return newNodeList;
}

QList<Internal::InternalNode::Pointer> toInternalNodeList(const QList<ModelNode> &nodeList)
{
    QList<Internal::InternalNode::Pointer> newNodeList;
    foreach (const ModelNode &node, nodeList)
        newNodeList.append(node.internalNode());

    return newNodeList;
}

/*!
\brief set this list nodes to the actual selected nodes
\param focusNodeList list the selected nodes
*/
void AbstractView::setSelectedModelNodes(const QList<ModelNode> &selectedNodeList)
{
    model()->d->setSelectedNodes(toInternalNodeList(selectedNodeList));
}

/*!
\brief clears the selection
*/
void AbstractView::clearSelectedModelNodes()
{
    model()->d->clearSelectedNodes();
}

/*!
\brief set this list nodes to the actual selected nodes
\return list the selected nodes
*/
QList<ModelNode> AbstractView::selectedModelNodes() const
{
    return toModelNodeList(model()->d->selectedNodes());
}

/*!
\brief adds a node to the selection list
\param node to be added to the selection list
*/
void AbstractView::selectModelNode(const ModelNode &node)
{
    model()->d->selectNode(node.internalNode());
}

/*!
\brief removes a node from the selection list
\param node to be removed from the selection list
*/
void AbstractView::deselectModelNode(const ModelNode &node)
{
    model()->d->deselectNode(node.internalNode());
}

ModelNode AbstractView::modelNodeForId(const QString &id)
{
    return ModelNode(model()->d->nodeForId(id), model(), this);
}

bool AbstractView::hasId(const QString &id) const
{
    return model()->d->hasId(id);
}

ModelNode AbstractView::modelNodeForInternalId(qint32 internalId)
{
     return ModelNode(model()->d->nodeForInternalId(internalId), model(), this);
}

bool AbstractView::hasModelNodeForInternalId(qint32 internalId) const
{
    return model()->d->hasNodeForInternalId(internalId);
}

QmlModelView *AbstractView::toQmlModelView()
{
    return qobject_cast<QmlModelView*>(this);
}

NodeInstanceView *AbstractView::nodeInstanceView() const
{
    if (model())
        return model()->d->nodeInstanceView();
    else
        return 0;
}

RewriterView *AbstractView::rewriterView() const
{
    if (model())
        return model()->d->rewriterView();
    else
        return 0;
}

void AbstractView::resetView()
{
    if (!model())
        return;
    Model *currentModel = model();

    currentModel->detachView(this);
    currentModel->attachView(this);
}

QList<ModelNode> AbstractView::allModelNodes() const
{
   return toModelNodeList(model()->d->allNodes());
}

void AbstractView::emitCustomNotification(const QString &identifier)
{
    emitCustomNotification(identifier, QList<ModelNode>());
}

void AbstractView::emitCustomNotification(const QString &identifier, const QList<ModelNode> &nodeList)
{
    emitCustomNotification(identifier, nodeList, QList<QVariant>());
}

void AbstractView::emitCustomNotification(const QString &identifier, const QList<ModelNode> &nodeList, const QList<QVariant> &data)
{
    model()->d->notifyCustomNotification(this, identifier, nodeList, data);
}

void AbstractView::emitInstancePropertyChange(const QList<QPair<ModelNode, QString> > &propertyList)
{
    if (model() && nodeInstanceView() == this)
        model()->d->notifyInstancePropertyChange(propertyList);
}

void AbstractView::emitInstancesCompleted(const QVector<ModelNode> &nodeVector)
{
    if (model() && nodeInstanceView() == this)
        model()->d->notifyInstancesCompleted(nodeVector);
}

void AbstractView::emitInstanceInformationsChange(const QMultiHash<ModelNode, InformationName> &informationChangeHash)
{
    if (model() && nodeInstanceView() == this)
        model()->d->notifyInstancesInformationsChange(informationChangeHash);
}

void AbstractView::emitInstancesRenderImageChanged(const QVector<ModelNode> &nodeVector)
{
    if (model() && nodeInstanceView() == this)
        model()->d->notifyInstancesRenderImageChanged(nodeVector);
}

void AbstractView::emitInstancesPreviewImageChanged(const QVector<ModelNode> &nodeVector)
{
    if (model() && nodeInstanceView() == this)
        model()->d->notifyInstancesPreviewImageChanged(nodeVector);
}

void AbstractView::emitInstancesChildrenChanged(const QVector<ModelNode> &nodeVector)
{
    if (model() && nodeInstanceView() == this)
        model()->d->notifyInstancesChildrenChanged(nodeVector);
}

void AbstractView::emitRewriterBeginTransaction()
{
    if (model())
        model()->d->notifyRewriterBeginTransaction();
}

void AbstractView::sendTokenToInstances(const QString &token, int number, const QVector<ModelNode> &nodeVector)
{
    if (nodeInstanceView())
        nodeInstanceView()->sendToken(token, number, nodeVector);
}

void AbstractView::emitInstanceToken(const QString &token, int number, const QVector<ModelNode> &nodeVector)
{
    if (nodeInstanceView())
        model()->d->notifyInstanceToken(token, number, nodeVector);
}

void AbstractView::emitRewriterEndTransaction()
{
    if (model())
        model()->d->notifyRewriterEndTransaction();
}

void AbstractView::setAcutalStateNode(const ModelNode &node)
{
    Internal::WriteLocker locker(m_model.data());
    if (model())
        model()->d->notifyActualStateChanged(node);
}

void AbstractView::changeRootNodeType(const QString &type, int majorVersion, int minorVersion)
{
    Internal::WriteLocker locker(m_model.data());

    m_model.data()->d->changeRootNodeType(type, majorVersion, minorVersion);
}

ModelNode AbstractView::actualStateNode() const
{
    if (model())
        return ModelNode(m_model.data()->d->actualStateNode(), m_model.data(), const_cast<AbstractView*>(this));

    return ModelNode();
}

} // namespace QmlDesigner
