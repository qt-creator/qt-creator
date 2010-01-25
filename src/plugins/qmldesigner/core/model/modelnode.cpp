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

#include <modelnode.h>
#include <abstractproperty.h>
#include <abstractview.h>
#include <model.h>
#include <metainfo.h>
#include "internalnode_p.h"
#include <QHash>
#include <QTextStream>
#include "invalidargumentexception.h"
#include "invalididexception.h"
#include "invalidmodelnodeexception.h"
#include "invalidpropertyexception.h"
#include "invalidslideindexexception.h"
#include "model_p.h"
#include "abstractview.h"
#include "abstractproperty.h"
#include "variantproperty.h"
#include "bindingproperty.h"
#include "nodeabstractproperty.h"
#include "nodelistproperty.h"
#include "nodeproperty.h"

namespace QmlDesigner {
using namespace QmlDesigner::Internal;

/*!
\class QmlDesigner::ModelNode
\ingroup CoreModel
\brief The central class to access the node which can represent a widget, layout
            or other items. A Node is a part of a tree and has properties.

Conceptually ModelNode is an opaque handle to the internal data structures.

There is always a root model node in every QmlDesigner::Model:
\code
QmlDesigner::Model *model = QmlDesigner::Model::create();
QmlDesigner::ModelNode rootNode = model->rootNode();
QmlDesigner::ModelNode childNode = rootNode.addChildNode("QLineEdit");
\endcode

You can add a property to a node:
\code
childNode.addProperty("pos", QPoint(2, 12));
\endcode

All the manipulation functions are generating undo commands internally.
*/



/*! \brief internal constructor

*/
ModelNode::ModelNode(const InternalNodePointer &internalNode, Model *model, AbstractView *view):
        m_internalNode(internalNode),
        m_model(model),
        m_view(view)
{
    Q_ASSERT(!m_model || m_view);
}

ModelNode::ModelNode(const ModelNode modelNode, AbstractView *view)
    : m_internalNode(modelNode.m_internalNode),
      m_model(modelNode.model()),
      m_view(view)
{

}

/*! \brief contructs a invalid model node
\return invalid node
\see invalid
*/
ModelNode::ModelNode():
        m_internalNode(new InternalNode)
{

}

ModelNode::ModelNode(const ModelNode &other):
        m_internalNode(other.m_internalNode),
        m_model(other.m_model),
        m_view(other.m_view)
{
}

ModelNode& ModelNode::operator=(const ModelNode &other)
{
    this->m_model = other.m_model;
    this->m_internalNode = other.m_internalNode;
    this->m_view = other.m_view;

    return *this;
}

/*! \brief does nothing
*/
ModelNode::~ModelNode()
{
}

QString ModelNode::generateNewId() const
{
    int counter = 1;
    QString newId = QString("%1%2").arg(simplifiedTypeName().toLower()).arg(counter);

    while(view()->hasId(newId)) {
        counter += 1;
        newId = QString("%1%2").arg(simplifiedTypeName().toLower()).arg(counter);
    }

    return newId;
}

/*! \brief returns the name of node which is a short cut to a property like objectName
\return name of the node
*/
QString ModelNode::id() const
{
    if (!isValid()) {
        throw InvalidModelNodeException(__LINE__, __FUNCTION__, __FILE__);
    }

    return m_internalNode->id();
}

QString ModelNode::validId()
{
    if (id().isEmpty())
        setId(generateNewId());

    return id();
}

static bool idContainsWrongLetter(const QString& id)
{
    static QRegExp idExpr(QLatin1String("[a-zA-Z][a-zA-Z0-9_]*"));
    return !idExpr.exactMatch(id);
}

bool ModelNode::isValidId(const QString &id)
{
    return id.isEmpty() || !idContainsWrongLetter(id);
}

void ModelNode::setId(const QString& id)
{
    Internal::WriteLocker locker(m_model.data());
    if (!isValid()) {
        Q_ASSERT_X(isValid(), Q_FUNC_INFO, "model node is invalid");
        throw InvalidModelNodeException(__LINE__, __FUNCTION__, __FILE__);
    }

    if (!isValidId(id))
        throw InvalidIdException(__LINE__, __FUNCTION__, __FILE__);

    if (id == ModelNode::id())
        return;

    if (view()->hasId(id))
        throw InvalidIdException(__LINE__, __FUNCTION__, __FILE__);

    m_model.data()->m_d->changeNodeId(internalNode(), id);
}

/*! \brief the fully-qualified type name of the node is represented as string
\return type of the node as a string
*/
QString ModelNode::type() const
{
    if (!isValid()) {
        Q_ASSERT_X(isValid(), Q_FUNC_INFO, "model node is invalid");
        throw InvalidModelNodeException(__LINE__, __FUNCTION__, __FILE__);
    }
    return m_internalNode->type();
}

/*! \brief set the fully-qualified type name of the node
*/
void ModelNode::changeType(const QString &type, int majorVersion, int minorVersion)
{
    Internal::WriteLocker locker(m_model.data());
    if (!isValid()) {
        Q_ASSERT_X(isValid(), Q_FUNC_INFO, "model node is invalid");
        throw InvalidModelNodeException(__LINE__, __FUNCTION__, __FILE__);
    }

    m_model.data()->m_d->changeType(internalNode(), type, majorVersion, minorVersion);
}

/*! \brief minor number of the Qml Type
\return minor number
*/
int ModelNode::minorVersion() const
{
    if (!isValid()) {
        Q_ASSERT_X(isValid(), Q_FUNC_INFO, "model node is invalid");
        throw InvalidModelNodeException(__LINE__, __FUNCTION__, __FILE__);
    }
    return m_internalNode->minorVersion();
}

/*! \brief major number of the Qml Type
\return major number
*/
int ModelNode::majorVersion() const
{
    if (!isValid()) {
        Q_ASSERT_X(isValid(), Q_FUNC_INFO, "model node is invalid");
        throw InvalidModelNodeException(__LINE__, __FUNCTION__, __FILE__);
    }
    return m_internalNode->majorVersion();
}


/*! \return the short-hand type name of the node. */
QString ModelNode::simplifiedTypeName() const
{
    if (!isValid()) {
        Q_ASSERT_X(isValid(), Q_FUNC_INFO, "model node is invalid");
        throw InvalidModelNodeException(__LINE__, __FUNCTION__, __FILE__);
    }

    return type().split("/").last();
}

/*! \brief Returns whether the node is valid

A node is valid if its model still exists, and contains this node.
Also, the current state must be a valid one.

A node might become invalid if e.g. it or one of its ancestors is deleted.

\return is a node valid(true) or invalid(false)
*/
bool ModelNode::isValid() const
{
    return !m_model.isNull() && !m_view.isNull() && m_internalNode &&m_internalNode->isValid();
}

/*!
  \brief Returns whether the root node of the model is one of the anchestors of this node.

  Will return true also for the root node itself.
  */
bool ModelNode::isInHierarchy() const
{
    if (!isValid()) {
        Q_ASSERT_X(isValid(), Q_FUNC_INFO, "model node is invalid");
        throw InvalidModelNodeException(__LINE__, __FUNCTION__, __FILE__);
    }
    if (isRootNode())
        return true;
    if (!hasParentProperty())
        return false;
    return parentProperty().parentModelNode().isInHierarchy();
}

/*!
  \brief Returns the property containing this node

  The NodeAbstractProperty is invalid if this ModelNode has no parent.
  NodeAbstractProperty can be a NodeProperty containing a single ModelNode, or
  a NodeListProperty.

  \return the property containing this ModelNode
  */
NodeAbstractProperty ModelNode::parentProperty() const
{
    if (!isValid()) {
        Q_ASSERT_X(isValid(), Q_FUNC_INFO, "model node is invalid");
        throw InvalidModelNodeException(__LINE__, __FUNCTION__, __FILE__);
    }
    if (m_internalNode->parentProperty().isNull())
        return NodeAbstractProperty();

    return NodeAbstractProperty(m_internalNode->parentProperty()->name(), m_internalNode->parentProperty()->propertyOwner(), m_model.data(), view());
}


/*! \brief the command id is used to compress the some commands together.
\param newParentNode parent of this node will be set to this node
\param commandId integer which is used to descripe commands which should compressed together to one command

For example:
\code
node.setParentNode(parentNode1);
node.setParentNode(parentNode2, 212);
node.setParentNode(parentNode3, 212);
model->undoStack()->undo();
ModelNode parentNode4 = node.parentProperty().parentModelNode();
parentNode4 == parentNode1; -> true
\endcode

\see parentNode childNodes addChildNode hasChildNodes Model::undo

*/

void ModelNode::setParentProperty(NodeAbstractProperty parent)
{
    if (!isValid()) {
        Q_ASSERT_X(isValid(), Q_FUNC_INFO, "model node is invalid");
        throw InvalidModelNodeException(__LINE__, __FUNCTION__, __FILE__);
    }

    if (!parent.parentModelNode().isValid()) {
        throw InvalidArgumentException(__LINE__, __FUNCTION__, __FILE__, "newParentNode");
    }

    if (*this == parent.parentModelNode()) {
        Q_ASSERT_X(*this != parent.parentModelNode(), Q_FUNC_INFO, "cannot set parent to itself");
        throw InvalidModelNodeException(__LINE__, __FUNCTION__, __FILE__);
    }

    if (parent == parentProperty())
        return;

    parent.reparentHere(*this);
}

void ModelNode::setParentProperty(const ModelNode &newParentNode, const QString &propertyName)
{
    setParentProperty(newParentNode.nodeAbstractProperty(propertyName));
}

/*! \brief test if there is a parent for this node
\return true is this node has a parent
\see childNodes parentNode setParentNode addChildNode hasChildNodes Model::undo
*/
bool ModelNode::hasParentProperty() const
{
    if (!isValid()) {
        Q_ASSERT_X(isValid(), Q_FUNC_INFO, "model node is invalid");
        throw InvalidModelNodeException(__LINE__, __FUNCTION__, __FILE__);
    }

    if (m_internalNode->parentProperty().isNull())
        return false;

    return true;
}

/*!
  \brief Returns a BindingProperty

  Note that a valid BindingProperty is returned, if the ModelNode is valid,
  even if this property does not exist or is not a BindingProperty.
  Assigning an expression to this BindingProperty will create the property.

  \return BindingProperty named name
  */

BindingProperty ModelNode::bindingProperty(const QString &name) const
{
    if (!isValid())
        throw InvalidModelNodeException(__LINE__, __FUNCTION__, __FILE__);

    return BindingProperty(name, m_internalNode, model(), view());
}


/*!
  \brief Returns a NodeProperty

  Note that a valid NodeProperty is returned, if the ModelNode is valid,
  even if this property does not exist or is not a NodeProperty.
  Assigning a ModelNode to this NodeProperty will create the property.

  \return NodeProperty named name
  */

NodeProperty ModelNode::nodeProperty(const QString &name) const
{
      if (!isValid())
        throw InvalidModelNodeException(__LINE__, __FUNCTION__, __FILE__);

    return NodeProperty(name, m_internalNode, model(), view());
}


/*!
  \brief Returns a NodeListProperty

  Note that a valid NodeListProperty is returned, if the ModelNode is valid,
  even if this property does not exist or is not a NodeListProperty.
  Assigning a ModelNode to this NodeListProperty will create the property.

  \return NodeListProperty named name
  */

NodeListProperty ModelNode::nodeListProperty(const QString &name) const
{
    if (!isValid())
        throw InvalidModelNodeException(__LINE__, __FUNCTION__, __FILE__);

    return NodeListProperty(name, m_internalNode, model(), view());
}

NodeAbstractProperty ModelNode::nodeAbstractProperty(const QString &name) const
{
     if (!isValid())
        throw InvalidModelNodeException(__LINE__, __FUNCTION__, __FILE__);

    return NodeAbstractProperty(name, m_internalNode, model(), view());
}


/*!
  \brief Returns a VariantProperty

  Note that a valid VariantProperty is returned, if the ModelNode is valid,
  even if this property does not exist or is not a VariantProperty.
  Assigning a value to this VariantProperty will create the property.

  \return VariantProperty named name
  */

VariantProperty ModelNode::variantProperty(const QString &name) const
{
    if (!isValid())
        throw InvalidModelNodeException(__LINE__, __FUNCTION__, __FILE__);

    return VariantProperty(name, m_internalNode, model(), view());
}

AbstractProperty ModelNode::property(const QString &name) const
{
    if (!isValid())
        throw InvalidModelNodeException(__LINE__, __FUNCTION__, __FILE__);

    return AbstractProperty(name, m_internalNode, model(), view());
}

/*! \brief returns a property
\param name name of the property
\return returns a node property handle. If the property is not set yet, the node property is still valid (lazy reference).

It is searching only in the local Property.

\see addProperty changePropertyValue removeProperty properties hasProperties
*/

/*! \brief returns a list of all properties
\return list of all properties

The list of properties

*/
QList<AbstractProperty> ModelNode::properties() const
{
    if (!isValid())
        throw InvalidModelNodeException(__LINE__, __FUNCTION__, __FILE__);

    QList<AbstractProperty> propertyList;

    foreach (const QString &propertyName, internalNode()->propertyNameList()) {
        AbstractProperty property(propertyName, internalNode(), model(), view());
        propertyList.append(property);
    }

    return propertyList;
}


/*! \brief returns a list of all VariantProperties
\return list of all VariantProperties

The list of all properties containing just an atomic value.

*/
QList<VariantProperty> ModelNode::variantProperties() const
{
    QList<VariantProperty> propertyList;

    foreach (const AbstractProperty &abstractProperty, properties())
        if (abstractProperty.isVariantProperty())
            propertyList.append(abstractProperty.toVariantProperty());
    return propertyList;
}

QList<NodeAbstractProperty> ModelNode::nodeAbstractProperties() const
{
    QList<NodeAbstractProperty> propertyList;

    foreach (const AbstractProperty &nodeAbstractProperty, properties())
        if (nodeAbstractProperty.isNodeAbstractProperty())
            propertyList.append(nodeAbstractProperty.toNodeAbstractProperty());
    return propertyList;
}

QList<NodeProperty> ModelNode::nodeProperties() const
{
    QList<NodeProperty> propertyList;

    foreach (const AbstractProperty &nodeProperty, properties())
        if (nodeProperty.isNodeProperty())
            propertyList.append(nodeProperty.toNodeProperty());
    return propertyList;
}

QList<NodeListProperty> ModelNode::nodeListProperties() const
{
    QList<NodeListProperty> propertyList;

    foreach (const AbstractProperty &nodeListProperty, properties())
        if (nodeListProperty.isNodeListProperty())
            propertyList.append(nodeListProperty.toNodeListProperty());
    return propertyList;
}


/*! \brief returns a list of all BindingProperties
\return list of all BindingProperties

The list of all properties containing an expression.

*/
QList<BindingProperty> ModelNode::bindingProperties() const
{
    QList<BindingProperty> propertyList;

    foreach (const AbstractProperty &bindingProperty, properties())
        if (bindingProperty.isBindingProperty())
            propertyList.append(bindingProperty.toBindingProperty());
    return propertyList;
}

/*!
\brief removes a property from this node
\param name name of the property

Does nothing if the node state does not set this property.

\see addProperty property  properties hasProperties
*/
void ModelNode::removeProperty(const QString &name)
{
    if (!isValid())
        throw InvalidModelNodeException(__LINE__, __FUNCTION__, __FILE__);

    model()->m_d->checkPropertyName(name);

    if (internalNode()->hasProperty(name))
        model()->m_d->removeProperty(internalNode()->property(name));
}



/*! \brief creates a new node and add it as child to this node
\param nodeTypeString type name of the child node
\param propertyList list of properties
\return the new created model node

*/
ModelNode ModelNode::addChildNode(const QString &nodeTypeString, int majorVersion, int minorVersion, const QString &propertyName, const PropertyListType &propertyList)
{
    Internal::WriteLocker locker(m_model.data());
    if (!isValid()) {
        Q_ASSERT_X(isValid(), Q_FUNC_INFO, "model node is invalid");
        throw InvalidModelNodeException(__LINE__, __FUNCTION__, __FILE__);
    }

    ModelNode newNode(view()->createModelNode(nodeTypeString, majorVersion, minorVersion, propertyList));

    newNode.setParentProperty(nodeListProperty(propertyName));

    return newNode;
}

/*! \brief removes this node from the node tree
*/

static QList<ModelNode> descendantNodes(const ModelNode &parent)
{
    QList<ModelNode> descendants(parent.allDirectSubModelNodes());
    foreach (const ModelNode &child, parent.allDirectSubModelNodes()) {
        descendants += descendantNodes(child);
    }
    return descendants;
}

static void removeModelNodeFromSelection(const ModelNode &node)
{
    { // remove nodes from the active selection:
        QList<ModelNode> selectedList = node.view()->selectedModelNodes();

        foreach (const ModelNode &childModelNode, descendantNodes(node))
            selectedList.removeAll(childModelNode);
        selectedList.removeAll(node);

        node.view()->setSelectedModelNodes(selectedList);
    }
}


/*! \brief complete removes this ModelNode from the Model

*/
void ModelNode::destroy()
{
    if (!isValid()) {
        Q_ASSERT_X(isValid(), Q_FUNC_INFO, "model node is invalid");
        throw InvalidModelNodeException(__LINE__, __FUNCTION__, __FILE__);
    }

    if (isRootNode()) {
        throw InvalidArgumentException(__LINE__, __FUNCTION__, __FILE__, "rootNode");
    }

    removeModelNodeFromSelection(*this);
    model()->m_d->removeNode(internalNode());
}
//\}

/*! \name Property Manipulation
 *  This methodes interact with properties.
 */


/*!
  \brief Returns if the the two nodes reference the same entity in the same model
  */
bool operator ==(const ModelNode &firstNode, const ModelNode &secondNode)
{
    if (firstNode.m_internalNode.isNull() || secondNode.m_internalNode.isNull()) {
        Q_ASSERT_X(0, Q_FUNC_INFO, "model node is invalid");
        throw InvalidModelNodeException(__LINE__, __FUNCTION__, __FILE__);
    }

    return firstNode.m_internalNode.data() == secondNode.m_internalNode.data();
}

/*!
  \brief Returns if the the two nodes do not reference the same entity in the same model
  */
bool operator !=(const ModelNode &firstNode, const ModelNode &secondNode)
{
    if (firstNode.m_internalNode.isNull() || secondNode.m_internalNode.isNull()) {
        Q_ASSERT_X(0, Q_FUNC_INFO, "model node is invalid");
        throw InvalidModelNodeException(__LINE__, __FUNCTION__, __FILE__);
    }

    return firstNode.m_internalNode != secondNode.m_internalNode;
}

bool operator <(const ModelNode &firstNode, const ModelNode &secondNode)
{
    if (firstNode.m_internalNode.isNull() || secondNode.m_internalNode.isNull()) {
        Q_ASSERT_X(0, Q_FUNC_INFO, "model node is invalid");
        throw InvalidModelNodeException(__LINE__, __FUNCTION__, __FILE__);
    }

    return firstNode.internalNode().data() < secondNode.internalNode().data();
}


Internal::InternalNodePointer ModelNode::internalNode() const
{
    if (!isValid()) {
        Q_ASSERT_X(isValid(), Q_FUNC_INFO, "model node is invalid");
        throw InvalidModelNodeException(__LINE__, __FUNCTION__, __FILE__);
    }
    return m_internalNode;
}


uint qHash(const ModelNode &node)
{
//    if (!node.isValid()) {
//        Q_ASSERT_X(node.isValid(), Q_FUNC_INFO, "model node is invalid");
//        throw InvalidModelNodeException(__LINE__, __FUNCTION__, __FILE__);
//    }
    return ::qHash(node.m_internalNode.data());
}

/*!
\brief returns the model of the node
\return returns the model of the node
*/
Model *ModelNode::model() const
{
    return m_model.data();
}

/*!
\brief returns the view of the node
Each ModelNode belongs to one specific view.
\return view of the node
*/
AbstractView *ModelNode::view() const
{
    return m_view.data();
}


/*!
\brief returns all ModelNodes that are direct children of this ModelNode
The list contains every ModelNode that belongs to one of this ModelNodes
properties.
\return a list of all ModelNodes that are direct children
*/
const QList<ModelNode> ModelNode::allDirectSubModelNodes() const
{
    return toModelNodeList(internalNode()->allDirectSubNodes(), view());
}


/*!
\brief returns all ModelNodes that are direct or indirect children of this ModelNode
The list contains every ModelNode that is a direct or indirect child of this ModelNode.
All children in this list will be implicitly removed if this ModelNode is destroyed.
\return a list of all ModelNodes that are direct or indirect children
*/

const QList<ModelNode> ModelNode::allSubModelNodes() const
{
    return toModelNodeList(internalNode()->allSubNodes(), view());
}

/*!
\brief returns if this ModelNode has any child ModelNodes.

\return if this ModelNode has any child ModelNodes
*/

bool ModelNode::hasAnySubModelNodes() const
{
    return !nodeAbstractProperties().isEmpty();
}

/*! \brief returns the meta info of the node
\return meta info of the node
*/
const NodeMetaInfo ModelNode::metaInfo() const
{
    if (!isValid()) {
        Q_ASSERT_X(isValid(), Q_FUNC_INFO, "model node is invalid");
        throw InvalidModelNodeException(__LINE__, __FUNCTION__, __FILE__);
    }
    return model()->metaInfo().nodeMetaInfo(type(), majorVersion(), minorVersion());
}

/*! \brief has a node the selection of the model
\return true if the node his selection
*/
bool ModelNode::isSelected() const
{
    if (!isValid()) {
        Q_ASSERT_X(isValid(), Q_FUNC_INFO, "model node is invalid");
        throw InvalidModelNodeException(__LINE__, __FUNCTION__, __FILE__);
    }
    return view()->selectedModelNodes().contains(ModelNode(m_internalNode, m_model.data(), view()));
}

/*! \briefis this node the root node of the model
\return true if it is the root node
*/
bool ModelNode::isRootNode() const
{
    if (!isValid()) {
        Q_ASSERT_X(isValid(), Q_FUNC_INFO, "model node is invalid");
        throw InvalidModelNodeException(__LINE__, __FUNCTION__, __FILE__);
    }
    return view()->rootModelNode() == *this;
}

/*! \brief returns the list of all property names
\return list of all property names set in this state.

The list of properties set in this state.

\see addProperty property changePropertyValue removeProperty hasProperties
*/
QStringList ModelNode::propertyNames() const
{
    if (!isValid())
        throw InvalidModelNodeException(__LINE__, __FUNCTION__, __FILE__);
    return internalNode()->propertyNameList();
}

/*! \brief test a if a property is set for this node
\return true if property a property ins this or a ancestor state exists
*/
bool ModelNode::hasProperty(const QString &name) const
{
    if (!isValid())
        throw InvalidModelNodeException(__LINE__, __FUNCTION__, __FILE__);

    return internalNode()->hasProperty(name);
}

bool ModelNode::hasVariantProperty(const QString &name) const
{
    return hasProperty(name) && internalNode()->property(name)->isVariantProperty();
}

bool ModelNode::hasBindingProperty(const QString &name) const
{
    return hasProperty(name) && internalNode()->property(name)->isBindingProperty();
}

bool ModelNode::hasNodeAbstracProperty(const QString &name) const
{
    return hasProperty(name) && internalNode()->property(name)->isNodeAbstractProperty();
}

bool ModelNode::hasNodeProperty(const QString &name) const
{
    return hasProperty(name) && internalNode()->property(name)->isNodeProperty();
}

bool ModelNode::hasNodeListProperty(const QString &name) const
{
    return hasProperty(name) && internalNode()->property(name)->isNodeListProperty();
}

static bool recursiveAncestor(const ModelNode &possibleAncestor, const ModelNode &node)
{
    if (node.hasParentProperty()) {
        if (node.parentProperty().parentModelNode() == possibleAncestor)
           return true;
        return recursiveAncestor(possibleAncestor, node.parentProperty().parentModelNode());
    }

    return false;
}

bool ModelNode::isAncestorOf(const ModelNode &node) const
{
    return recursiveAncestor(*this, node);
}

QDebug operator<<(QDebug debug, const ModelNode &modelNode)
{
    if (modelNode.isValid()) {
        debug.nospace() << "ModelNode("
                << modelNode.internalNode().data() << ", "
                << modelNode.type() << ", "
                << modelNode.id() <<")";
    } else {
        debug.nospace() << "ModelNode(invalid)";
    }

    return debug.space();
}

QTextStream& operator<<(QTextStream &stream, const ModelNode &modelNode)
{
    if (modelNode.isValid()) {
        stream << "ModelNode("
                << "type: " << modelNode.type() << ", "
                << "id: " << modelNode.id() <<")";
    } else {
        stream << "ModelNode(invalid)";
    }

    return stream;
}

void ModelNode::selectNode()
{
    if (!isValid()) {
            throw InvalidModelNodeException(__LINE__, __FUNCTION__, __FILE__);
    }

    QList<ModelNode> selectedNodeList;
    selectedNodeList.append(*this);

    view()->setSelectedModelNodes(selectedNodeList);
}

void ModelNode::deselectNode()
{
    if (!isValid()) {
            throw InvalidModelNodeException(__LINE__, __FUNCTION__, __FILE__);
    }

    QList<ModelNode> selectedNodeList(view()->selectedModelNodes());
    selectedNodeList.removeAll(*this);

    view()->setSelectedModelNodes(selectedNodeList);
}

int ModelNode::variantUserType()
{
    return qMetaTypeId<ModelNode>();
}

QVariant ModelNode::toVariant() const
{
    return QVariant::fromValue(*this);
}

QVariant ModelNode::auxiliaryData(const QString &name) const
{
    if (!isValid()) {
        throw InvalidModelNodeException(__LINE__, __FUNCTION__, __FILE__);
    }

    return internalNode()->auxiliaryData(name);
}

void ModelNode::setAuxiliaryData(const QString &name, const QVariant &data)
{
    Internal::WriteLocker locker(m_model.data());
    m_model.data()->m_d->setAuxiliaryData(internalNode(), name, data);
}

bool ModelNode::hasAuxiliaryData(const QString &name) const
{
    if (!isValid()) {
        throw InvalidModelNodeException(__LINE__, __FUNCTION__, __FILE__);
    }

    return internalNode()->hasAuxiliaryData(name);
}

}
