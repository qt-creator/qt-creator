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
                            const QList<QPair<QString, QVariant> > &propertyList)
{
    return ModelNode(model()->m_d->createNode(typeString, majorVersion, minorVersion, propertyList), model(), this);
}


/*! \brief returns the root model node
\return constant root model node

*/

const ModelNode AbstractView::rootModelNode() const
{
    Q_ASSERT(model());
    return ModelNode(model()->m_d->rootNode(), model(), const_cast<AbstractView*>(this));
}


/*! \brief returns the root model node
\return root model node

*/

ModelNode AbstractView::rootModelNode()
{
    Q_ASSERT(model());
    return  ModelNode(model()->m_d->rootNode(), model(), this);
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
\fn AbstractView::importAdded(const Import &import)
\brief Called when an import has been added to the model
*/
void AbstractView::importAdded(const Import &/*import*/)
{
}

/*!
\fn AbstractView::importRemoved(const Import &import)
\brief Called when an import has been removed from the model
*/
void AbstractView::importRemoved(const Import &/*import*/)
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
    foreach(const Internal::InternalNode::Pointer &node, nodeList)
        newNodeList.append(ModelNode(node, view->model(), view));

    return newNodeList;
}

QList<Internal::InternalNode::Pointer> toInternalNodeList(const QList<ModelNode> &nodeList)
{
    QList<Internal::InternalNode::Pointer> newNodeList;
    foreach(const ModelNode &node, nodeList)
        newNodeList.append(node.internalNode());

    return newNodeList;
}

/*!
\brief set this list nodes to the actual selected nodes
\param focusNodeList list the selected nodes
*/
void AbstractView::setSelectedModelNodes(const QList<ModelNode> &selectedNodeList)
{
    model()->m_d->setSelectedNodes(toInternalNodeList(selectedNodeList));
}

/*!
\brief clears the selection
*/
void AbstractView::clearSelectedModelNodes()
{
    model()->m_d->clearSelectedNodes();
}

/*!
\brief set this list nodes to the actual selected nodes
\return list the selected nodes
*/
QList<ModelNode> AbstractView::selectedModelNodes() const
{
    return toModelNodeList(model()->m_d->selectedNodes());
}

/*!
\brief adds a node to the selection list
\param node to be added to the selection list
*/
void AbstractView::selectModelNode(const ModelNode &node)
{
    model()->m_d->selectNode(node.internalNode());
}

/*!
\brief removes a node from the selection list
\param node to be removed from the selection list
*/
void AbstractView::deselectModelNode(const ModelNode &node)
{
    model()->m_d->deselectNode(node.internalNode());
}

ModelNode AbstractView::modelNodeForId(const QString &id)
{
    return ModelNode(model()->m_d->nodeForId(id), model(), this);
}

bool AbstractView::hasId(const QString &id) const
{
    return model()->m_d->hasId(id);
}

QmlModelView *AbstractView::toQmlModelView()
{
    return qobject_cast<QmlModelView*>(this);
}

QList<ModelNode> AbstractView::allModelNodes()
{
   return toModelNodeList(model()->m_d->allNodes());
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
    model()->m_d->notifyCustomNotification(this, identifier, nodeList, data);
}

} // namespace QmlDesigner
