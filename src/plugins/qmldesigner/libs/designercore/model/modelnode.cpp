// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "modelnode.h"

#include "annotation.h"
#include "designercoretr.h"
#include "internalnode_p.h"
#include "model_p.h"
#include "nodelistproperty.h"
#include "nodeproperty.h"
#include "signalhandlerproperty.h"
#include "variantproperty.h"

#include <auxiliarydataproperties.h>
#include <modelutils.h>
#include <rewriterview.h>

#include <utils/algorithm.h>

namespace QmlDesigner {
using namespace QmlDesigner::Internal;

auto category = ModelTracing::category;

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
\endcode

You can add a property to a node:
\code
childNode.addProperty("pos", QPoint(2, 12));
\endcode

All the manipulation functions are generating undo commands internally.
*/

/*! \brief internal constructor

*/
ModelNode::ModelNode(const InternalNodePointer &internalNode, Model *model, const AbstractView *view)
    : m_internalNode(internalNode)
    , m_model(model)
    , m_view(const_cast<AbstractView *>(view))
{}

ModelNode::ModelNode(const ModelNode &modelNode, AbstractView *view)
    : m_internalNode(modelNode.m_internalNode)
    , m_model(modelNode.model())
    , m_view(view)
{}

/*! \brief returns the name of node which is a short cut to a property like objectName
\return name of the node
*/
QString ModelNode::id(SL sl) const
{
    if (!isValid())
        return {};

    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"model node id",
                               category(),
                               keyValue("type id", m_internalNode->typeId),
                               keyValue("caller location", sl)};

    return m_internalNode->id;
}

void ModelNode::ensureIdExists(SL sl) const
{
    if (!hasId())
        setIdWithoutRefactoring(model()->generateNewId(simplifiedTypeName()));

    NanotraceHR::Tracer tracer{"model node ensure id exists",
                               category(),
                               keyValue("type id", m_internalNode->typeId),
                               keyValue("caller location", sl)};
}

QString ModelNode::validId(SL sl) const
{
    NanotraceHR::Tracer tracer{"model node valid id",
                               category(),
                               keyValue("type id", m_internalNode->typeId),
                               keyValue("caller location", sl)};

    ensureIdExists(sl);

    return id();
}

bool ModelNode::isValidId(const QString &id)
{
    using namespace ModelUtils;
    return isValidQmlIdentifier(id) && !isBannedQmlId(id);
}

QString ModelNode::getIdValidityErrorMessage(const QString &id)
{
    if (isValidId(id))
        return {}; // valid

    if (id.at(0).isUpper())
        return DesignerCore::Tr::tr("ID cannot start with an uppercase character (%1).").arg(id);

    if (id.at(0).isDigit())
        return DesignerCore::Tr::tr("ID cannot start with a number (%1).").arg(id);

    if (id.contains(' '))
        return DesignerCore::Tr::tr("ID cannot include whitespace (%1).").arg(id);

    if (ModelUtils::isQmlKeyword(id))
        return DesignerCore::Tr::tr("%1 is a reserved QML keyword.").arg(id);

    if (ModelUtils::isQmlBuiltinType(id))
        return DesignerCore::Tr::tr("%1 is a reserved Qml type.").arg(id);

    if (ModelUtils::isDiscouragedQmlId(id))
        return DesignerCore::Tr::tr("%1 is a reserved property keyword.").arg(id);

    return DesignerCore::Tr::tr("ID includes invalid characters (%1).").arg(id);
}

bool ModelNode::hasId(SL sl) const
{
    if (!isValid())
        return false;

    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"model node has id",
                               category(),
                               keyValue("type id", m_internalNode->typeId),
                               keyValue("caller location", sl)};

    return !m_internalNode->id.isEmpty();
}

void ModelNode::setIdWithRefactoring(const QString &id, SL sl) const
{
    if (!isValid())
        return;

    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"model node set id with refactoring",
                               category(),
                               keyValue("type id", m_internalNode->typeId),
                               keyValue("caller location", sl)};

    if (model()->rewriterView() && !id.isEmpty()
        && !m_internalNode->id.isEmpty()) { // refactor the id if they are not empty
        model()->rewriterView()->renameId(m_internalNode->id, id);
    } else {
        setIdWithoutRefactoring(id);
    }
}

void ModelNode::setIdWithoutRefactoring(const QString &id, SL sl) const
{
    Internal::WriteLocker locker(m_model.data());
    if (!isValid())
        return;

    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"model node set id without refactoring",
                               category(),
                               keyValue("type id", m_internalNode->typeId),
                               keyValue("caller location", sl)};

    if (!isValidId(id))
        return;

    if (id == m_internalNode->id)
        return;

    if (model()->hasId(id))
        return;

    m_model.data()->d->changeNodeId(m_internalNode, id);
}

/*! \brief the fully-qualified type name of the node is represented as string
\return type of the node as a string
*/
TypeName ModelNode::type(SL sl) const
{
    if (!isValid())
        return {};

    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"model node type",
                               category(),
                               keyValue("type id", m_internalNode->typeId),
                               keyValue("caller location", sl)};

    return m_internalNode->typeName;
}

/*! \brief minor number of the QML type
\return minor number
*/
int ModelNode::minorVersion(SL sl) const
{
    if (!isValid())
        return {};

    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"model node minor version",
                               category(),
                               keyValue("type id", m_internalNode->typeId),
                               keyValue("caller location", sl)};

    return m_internalNode->minorVersion;
}

/*! \brief major number of the QML type
\return major number
*/
int ModelNode::majorVersion(SL sl) const
{
    if (!isValid())
        return {};

    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"model node major version",
                               category(),
                               keyValue("type id", m_internalNode->typeId),
                               keyValue("caller location", sl)};

    return m_internalNode->majorVersion;
}

/*! \return the short-hand type name of the node. */
QString ModelNode::simplifiedTypeName(SL sl) const
{
    if (!isValid())
        return {};

    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"model node simplified type name",
                               category(),
                               keyValue("type id", m_internalNode->typeId),
                               keyValue("caller location", sl)};

    return QString::fromUtf8(type().split('.').constLast());
}

QString ModelNode::displayName(SL sl) const
{
    NanotraceHR::Tracer tracer{"model node display name",
                               category(),
                               keyValue("type id", m_internalNode->typeId),
                               keyValue("caller location", sl)};

    return hasId() ? id() : simplifiedTypeName();
}

/*! \brief Returns whether the node is valid

A node is valid if its model still exists, and contains this node.
Also, the current state must be a valid one.

A node might become invalid if e.g. it or one of its ancestors is deleted.

\return is a node valid(true) or invalid(false)
*/
bool ModelNode::isValid() const
{
    return !m_model.isNull() && m_internalNode && m_internalNode->isValid;
}

/*!
  \brief Returns whether the root node of the model is one of the anchestors of this node.

  Will return true also for the root node itself.
  */
bool ModelNode::isInHierarchy(SL sl) const
{
    if (!isValid())
        return false;

    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"model node is in hierarchy",
                               category(),
                               keyValue("type id", m_internalNode->typeId),
                               keyValue("caller location", sl)};

    if (isRootNode())
        return true;
    if (!hasParentProperty())
        return false;
    return parentProperty().parentModelNode().isInHierarchy(sl);
}

/*!
  \brief Returns the property containing this node

  The NodeAbstractProperty is invalid if this ModelNode has no parent.
  NodeAbstractProperty can be a NodeProperty containing a single ModelNode, or
  a NodeListProperty.

  \return the property containing this ModelNode
  */
NodeAbstractProperty ModelNode::parentProperty(SL sl) const
{
    if (!isValid())
        return {};

    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"model node parent property",
                               category(),
                               keyValue("type id", m_internalNode->typeId),
                               keyValue("caller location", sl)};

    if (!m_internalNode->parentProperty())
        return {};

    return NodeAbstractProperty(m_internalNode->parentProperty()->name(),
                                m_internalNode->parentProperty()->propertyOwner(),
                                m_model.data(),
                                view());
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

\see parentNode childNodes hasChildNodes Model::undo

*/

void ModelNode::setParentProperty(NodeAbstractProperty parent, SL sl)
{
    if (!isValid())
        return;

    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"model node set parent property",
                               category(),
                               keyValue("type id", m_internalNode->typeId),
                               keyValue("caller location", sl)};

    if (!parent.parentModelNode().isValid())
        return;

    if (*this == parent.parentModelNode())
        return;

    if (hasParentProperty() && parent == parentProperty())
        return;

    parent.reparentHere(*this);
}

void ModelNode::changeType(const TypeName &typeName, int majorVersion, int minorVersion, SL sl)
{
    if (!isValid())
        return;

    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"model node change type",
                               category(),
                               keyValue("type id", m_internalNode->typeId),
                               keyValue("caller location", sl)};

    model()->d->changeNodeType(m_internalNode, typeName, majorVersion, minorVersion);
}

void ModelNode::setParentProperty(const ModelNode &newParentNode, const PropertyName &propertyName, SL sl)
{
    NanotraceHR::Tracer tracer{"model node set parent property",
                               category(),
                               keyValue("type id", m_internalNode->typeId),
                               keyValue("caller location", sl)};

    setParentProperty(newParentNode.nodeAbstractProperty(propertyName), sl);
}

/*! \brief test if there is a parent for this node
\return true is this node has a parent
\see childNodes parentNode setParentNode hasChildNodes Model::undo
*/
bool ModelNode::hasParentProperty(SL sl) const
{
    if (!isValid())
        return false;

    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"model node has parent property",
                               category(),
                               keyValue("type id", m_internalNode->typeId),
                               keyValue("caller location", sl)};

    if (!m_internalNode->parentProperty())
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

BindingProperty ModelNode::bindingProperty(PropertyNameView name, SL sl) const
{
    if (!isValid())
        return {};

    NanotraceHR::Tracer tracer{"model node binding property",
                               category(),
                               keyValue("type id", m_internalNode->typeId),
                               keyValue("caller location", sl)};

    return BindingProperty(name, m_internalNode, model(), view());
}

SignalHandlerProperty ModelNode::signalHandlerProperty(PropertyNameView name, SL sl) const
{
    if (!isValid())
        return {};

    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"model node signal handler property",
                               category(),
                               keyValue("type id", m_internalNode->typeId),
                               keyValue("caller location", sl)};

    return SignalHandlerProperty(name, m_internalNode, model(), view());
}

SignalDeclarationProperty ModelNode::signalDeclarationProperty(PropertyNameView name, SL sl) const
{
    if (!isValid())
        return {};

    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"model node signal declaration property",
                               category(),
                               keyValue("type id", m_internalNode->typeId),
                               keyValue("caller location", sl)};

    return SignalDeclarationProperty(name, m_internalNode, model(), view());
}

/*!
  \brief Returns a NodeProperty

  Note that a valid NodeProperty is returned, if the ModelNode is valid,
  even if this property does not exist or is not a NodeProperty.
  Assigning a ModelNode to this NodeProperty will create the property.

  \return NodeProperty named name
  */

NodeProperty ModelNode::nodeProperty(PropertyNameView name, SL sl) const
{
    if (!isValid())
        return {};

    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"model node node property",
                               category(),
                               keyValue("type id", m_internalNode->typeId),
                               keyValue("caller location", sl)};

    return NodeProperty(name, m_internalNode, model(), view());
}

/*!
  \brief Returns a NodeListProperty

  Note that a valid NodeListProperty is returned, if the ModelNode is valid,
  even if this property does not exist or is not a NodeListProperty.
  Assigning a ModelNode to this NodeListProperty will create the property.

  \return NodeListProperty named name
  */

NodeListProperty ModelNode::nodeListProperty(PropertyNameView name, SL sl) const
{
    if (!isValid())
        return {};

    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"model node node list property",
                               category(),
                               keyValue("type id", m_internalNode->typeId),
                               keyValue("caller location", sl)};

    return NodeListProperty(name, m_internalNode, model(), view());
}

NodeAbstractProperty ModelNode::nodeAbstractProperty(PropertyNameView name, SL sl) const
{
    if (!isValid())
        return {};

    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"model node node abstract property",
                               category(),
                               keyValue("type id", m_internalNode->typeId),
                               keyValue("caller location", sl)};

    return NodeAbstractProperty(name, m_internalNode, model(), view());
}

NodeAbstractProperty ModelNode::defaultNodeAbstractProperty(SL sl) const
{
    NanotraceHR::Tracer tracer{"model node default node abstract property",
                               category(),
                               keyValue("type id", m_internalNode->typeId),
                               keyValue("caller location", sl)};

    return nodeAbstractProperty(metaInfo().defaultPropertyName());
}

NodeListProperty ModelNode::defaultNodeListProperty(SL sl) const
{
    NanotraceHR::Tracer tracer{"model node default node list property",
                               category(),
                               keyValue("type id", m_internalNode->typeId),
                               keyValue("caller location", sl)};

    return nodeListProperty(metaInfo().defaultPropertyName());
}

NodeProperty ModelNode::defaultNodeProperty(SL sl) const
{
    NanotraceHR::Tracer tracer{"model node default node property",
                               category(),
                               keyValue("type id", m_internalNode->typeId),
                               keyValue("caller location", sl)};

    return nodeProperty(metaInfo().defaultPropertyName());
}

/*!
  \brief Returns a VariantProperty

  Note that a valid VariantProperty is returned, if the ModelNode is valid,
  even if this property does not exist or is not a VariantProperty.
  Assigning a value to this VariantProperty will create the property.

  \return VariantProperty named name
  */

VariantProperty ModelNode::variantProperty(PropertyNameView name, SL sl) const
{
    if (!isValid())
        return {};

    NanotraceHR::Tracer tracer{"model node variant property",
                               category(),
                               keyValue("type id", m_internalNode->typeId),
                               keyValue("caller location", sl)};

    return VariantProperty(name, m_internalNode, model(), view());
}

AbstractProperty ModelNode::property(PropertyNameView name, SL sl) const
{
    if (!isValid())
        return {};

    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"model node property",
                               category(),
                               keyValue("type id", m_internalNode->typeId),
                               keyValue("caller location", sl)};

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
QList<AbstractProperty> ModelNode::properties(SL sl) const
{
    if (!isValid())
        return {};

    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"model node properties",
                               category(),
                               keyValue("type id", m_internalNode->typeId),
                               keyValue("caller location", sl)};

    QList<AbstractProperty> propertyList;

    const QList<PropertyName> propertyNames = m_internalNode->propertyNameList();
    for (const PropertyName &propertyName : propertyNames) {
        AbstractProperty property(propertyName, m_internalNode, model(), view());
        propertyList.append(property);
    }

    return propertyList;
}

/*! \brief returns a list of all VariantProperties
\return list of all VariantProperties

The list of all properties containing just an atomic value.

*/
QList<VariantProperty> ModelNode::variantProperties(SL sl) const
{
    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"model node variant property",
                               category(),
                               keyValue("type id", m_internalNode->typeId),
                               keyValue("caller location", sl)};

    return properties<VariantProperty>(PropertyType::Variant);
}

QList<NodeAbstractProperty> ModelNode::nodeAbstractProperties(SL sl) const
{
    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"model node node abstract property",
                               category(),
                               keyValue("type id", m_internalNode->typeId),
                               keyValue("caller location", sl)};

    return properties<NodeAbstractProperty>(PropertyType::Node, PropertyType::NodeList);
}

QList<NodeProperty> ModelNode::nodeProperties(SL sl) const
{
    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"model node node property",
                               category(),
                               keyValue("type id", m_internalNode->typeId),
                               keyValue("caller location", sl)};

    return properties<NodeProperty>(PropertyType::Node);
}

QList<NodeListProperty> ModelNode::nodeListProperties(SL sl) const
{
    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"model node node list property",
                               category(),
                               keyValue("type id", m_internalNode->typeId),
                               keyValue("caller location", sl)};

    return properties<NodeListProperty>(PropertyType::NodeList);
}

/*! \brief returns a list of all BindingProperties
\return list of all BindingProperties

The list of all properties containing an expression.

*/
QList<BindingProperty> ModelNode::bindingProperties(SL sl) const
{
    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"model node binding property",
                               category(),
                               keyValue("type id", m_internalNode->typeId),
                               keyValue("caller location", sl)};

    return properties<BindingProperty>(PropertyType::Binding);
}

QList<SignalHandlerProperty> ModelNode::signalProperties(SL sl) const
{
    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"model node signal handler property",
                               category(),
                               keyValue("type id", m_internalNode->typeId),
                               keyValue("caller location", sl)};

    return properties<SignalHandlerProperty>(PropertyType::SignalHandler);
}

QList<AbstractProperty> ModelNode::dynamicProperties(SL sl) const
{
    if (!isValid())
        return {};

    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"model node dynamic property",
                               category(),
                               keyValue("type id", m_internalNode->typeId),
                               keyValue("caller location", sl)};

    QList<AbstractProperty> properties;

    for (const auto &propertyEntry : *m_internalNode.get()) {
        auto propertyName = propertyEntry.first;
        auto property = propertyEntry.second;
        if (property->dynamicTypeName().size())
            properties.emplace_back(propertyName, m_internalNode, model(), view());
    }

    return properties;
}

/*!
\brief removes a property from this node
\param name name of the property

Does nothing if the node state does not set this property.

\see addProperty property  properties hasProperties
*/
void ModelNode::removeProperty(PropertyNameView name, SL sl) const
{
    if (!isValid())
        return;

    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"model node remove property",
                               category(),
                               keyValue("type id", m_internalNode->typeId),
                               keyValue("caller location", sl)};

    if (!model()->d->propertyNameIsValid(name))
        return;

    if (auto property = m_internalNode->property(name))
        model()->d->removePropertyAndRelatedResources(property);
}

/*! \brief removes this node from the node tree
*/
static QList<ModelNode> descendantNodes(const ModelNode &node)
{
    const QList<ModelNode> children = node.directSubModelNodes();
    QList<ModelNode> descendants = children;
    for (const ModelNode &child : children)
        descendants += descendantNodes(child);

    return descendants;
}

static void removeModelNodeFromSelection(const ModelNode &node)
{
    // remove nodes from the active selection
    auto model = node.model();
    QList<ModelNode> selectedList = model->selectedNodes(node.view());

    const QList<ModelNode> descendants = descendantNodes(node);
    for (const ModelNode &descendantNode : descendants)
        selectedList.removeAll(descendantNode);

    selectedList.removeAll(node);

    model->setSelectedModelNodes(selectedList);
}

/*! \brief complete removes this ModelNode from the Model
*/
void ModelNode::destroy(SL sl)
{
    if (!isValid())
        return;

    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"model node destroy",
                               category(),
                               keyValue("type id", m_internalNode->typeId),
                               keyValue("caller location", sl)};

    if (isRootNode())
        return;

    removeModelNodeFromSelection(*this);
    model()->d->removeNodeAndRelatedResources(m_internalNode);
}

//\}

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
QList<ModelNode> ModelNode::directSubModelNodes(SL sl) const
{
    if (!isValid())
        return {};

    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"model node direct sub model nodes",
                               category(),
                               keyValue("type id", m_internalNode->typeId),
                               keyValue("caller location", sl)};

    return toModelNodeList(m_internalNode->allDirectSubNodes(), model(), view());
}

QList<ModelNode> ModelNode::directSubModelNodesOfType(const NodeMetaInfo &type, SL sl) const
{
    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"model node direct sub model nodes of type",
                               category(),
                               keyValue("type id", m_internalNode->typeId),
                               keyValue("caller location", sl)};

    return Utils::filtered(directSubModelNodes(), [&](const ModelNode &node) {
        return node.metaInfo().isValid() && node.metaInfo().isBasedOn(type);
    });
}

QList<ModelNode> ModelNode::subModelNodesOfType(const NodeMetaInfo &type, SL sl) const
{
    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"model node sub model nodes of type",
                               category(),
                               keyValue("type id", m_internalNode->typeId),
                               keyValue("caller location", sl)};

    return Utils::filtered(allSubModelNodes(), [&](const ModelNode &node) {
        return node.metaInfo().isValid() && node.metaInfo().isBasedOn(type);
    });
}

/*!
\brief returns all ModelNodes that are direct or indirect children of this ModelNode
The list contains every ModelNode that is a direct or indirect child of this ModelNode.
All children in this list will be implicitly removed if this ModelNode is destroyed.
\return a list of all ModelNodes that are direct or indirect children
*/

QList<ModelNode> ModelNode::allSubModelNodes(SL sl) const
{
    if (!isValid())
        return {};

    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"model node all sub model nodes",
                               category(),
                               keyValue("type id", m_internalNode->typeId),
                               keyValue("caller location", sl)};

    return toModelNodeList(internalNode()->allSubNodes(), model(), view());
}

QList<ModelNode> ModelNode::allSubModelNodesAndThisNode(SL sl) const
{
    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"model node all sub model nodes and this node",
                               category(),
                               keyValue("type id", m_internalNode->typeId),
                               keyValue("caller location", sl)};

    QList<ModelNode> modelNodeList;
    modelNodeList.append(*this);
    modelNodeList.append(allSubModelNodes());

    return modelNodeList;
}

/*!
\brief returns if this ModelNode has any child ModelNodes.

\return if this ModelNode has any child ModelNodes
*/

bool ModelNode::hasAnySubModelNodes(SL sl) const
{
    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"model node has any sub model nodes",
                               category(),
                               keyValue("type id", m_internalNode->typeId),
                               keyValue("caller location", sl)};

    return !nodeAbstractProperties().isEmpty();
}

NodeMetaInfo ModelNode::metaInfo([[maybe_unused]] SL sl) const
{
    if (!isValid())
        return {};

#ifdef QDS_USE_PROJECTSTORAGE
    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"model node meta info",
                               category(),
                               keyValue("type id", m_internalNode->typeId),
                               keyValue("caller location", sl)};

    return NodeMetaInfo(m_internalNode->typeId, m_model->projectStorage());
#else
    return NodeMetaInfo(m_model->metaInfoProxyModel(),
                        m_internalNode->typeName,
                        m_internalNode->majorVersion,
                        m_internalNode->minorVersion);
#endif
}

bool ModelNode::hasMetaInfo(SL sl) const
{
    if (!isValid())
        return false;

    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"model node has meta info",
                               category(),
                               keyValue("type id", m_internalNode->typeId),
                               keyValue("caller location", sl)};

    return model()->hasNodeMetaInfo(type(), majorVersion(), minorVersion());
}

/*! \brief has a node the selection of the model
\return true if the node his selection
*/
bool ModelNode::isSelected(SL sl) const
{
    if (!isValid())
        return false;

    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"model node is selected",
                               category(),
                               keyValue("type id", m_internalNode->typeId),
                               keyValue("caller location", sl)};

    return model()->d->selectedNodes().contains(internalNode());
}

/*! \briefis this node the root node of the model
\return true if it is the root node
*/
bool ModelNode::isRootNode(SL sl) const
{
    if (!isValid())
        return false;

    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"model node is root node",
                               category(),
                               keyValue("type id", m_internalNode->typeId),
                               keyValue("caller location", sl)};

    return m_model->d->rootNode() == m_internalNode;
}

/*! \brief returns the list of all property names
\return list of all property names set in this state.

The list of properties set in this state.

\see addProperty property changePropertyValue removeProperty hasProperties
*/
PropertyNameList ModelNode::propertyNames(SL sl) const
{
    if (!isValid())
        return {};

    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"model node property names",
                               category(),
                               keyValue("type id", m_internalNode->typeId),
                               keyValue("caller location", sl)};

    return m_internalNode->propertyNameList();
}

template<typename Type, typename... PropertyType>
QList<Type> ModelNode::properties(PropertyType... type) const
{
    if (!isValid())
        return {};

    QList<Type> properties;

    for (const auto &propertyEntry : *m_internalNode.get()) {
        auto propertyName = propertyEntry.first;
        auto property = propertyEntry.second;
        auto propertyType = property->type();
        QT_WARNING_PUSH
        QT_WARNING_DISABLE_CLANG("-Wparentheses-equality")
        if (((propertyType == type) || ...))
            properties.emplace_back(propertyName, m_internalNode, model(), view());
        QT_WARNING_POP
    }

    return properties;
}

/*! \brief test if a property is set for this node
\return true if property a property in this or a ancestor state exists
*/
bool ModelNode::hasProperty(PropertyNameView name, SL sl) const
{
    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"model node has property",
                               category(),
                               keyValue("type id", m_internalNode->typeId),
                               keyValue("caller location", sl)};

    return isValid() && m_internalNode->property(name);
}

bool ModelNode::hasVariantProperty(PropertyNameView name, SL sl) const
{
    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"model node has variant property",
                               category(),
                               keyValue("type id", m_internalNode->typeId),
                               keyValue("caller location", sl)};

    return hasProperty(name, PropertyType::Variant);
}

bool ModelNode::hasBindingProperty(PropertyNameView name, SL sl) const
{
    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"model node has binding property",
                               category(),
                               keyValue("type id", m_internalNode->typeId),
                               keyValue("caller location", sl)};

    return hasProperty(name, PropertyType::Binding);
}

bool ModelNode::hasSignalHandlerProperty(PropertyNameView name, SL sl) const
{
    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"model node has signal handler property",
                               category(),
                               keyValue("type id", m_internalNode->typeId),
                               keyValue("caller location", sl)};

    return hasProperty(name, PropertyType::SignalHandler);
}

bool ModelNode::hasNodeAbstractProperty(PropertyNameView name, SL sl) const
{
    if (!isValid())
        return {};

    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"model node has node abstract property",
                               category(),
                               keyValue("type id", m_internalNode->typeId),
                               keyValue("caller location", sl)};

    if (auto property = m_internalNode->property(name))
        return property->isNodeAbstractProperty();

    return false;
}

bool ModelNode::hasDefaultNodeAbstractProperty(SL sl) const
{
    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"model node has default node abstract property",
                               category(),
                               keyValue("type id", m_internalNode->typeId),
                               keyValue("caller location", sl)};

    auto defaultPropertyName = metaInfo().defaultPropertyName();
    return hasNodeAbstractProperty(defaultPropertyName);
}

bool ModelNode::hasDefaultNodeListProperty(SL sl) const
{
    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"model node has default node list property",
                               category(),
                               keyValue("type id", m_internalNode->typeId),
                               keyValue("caller location", sl)};

    auto defaultPropertyName = metaInfo().defaultPropertyName();
    return hasNodeListProperty(defaultPropertyName);
}

bool ModelNode::hasDefaultNodeProperty(SL sl) const
{
    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"model node has default node property",
                               category(),
                               keyValue("type id", m_internalNode->typeId),
                               keyValue("caller location", sl)};

    auto defaultPropertyName = metaInfo().defaultPropertyName();
    return hasNodeProperty(defaultPropertyName);
}

bool ModelNode::hasNodeProperty(PropertyNameView name, SL sl) const
{
    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"model node has node property",
                               category(),
                               keyValue("type id", m_internalNode->typeId),
                               keyValue("caller location", sl)};

    return hasProperty(name, PropertyType::Node);
}

bool ModelNode::hasNodeListProperty(PropertyNameView name, SL sl) const
{
    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"model node has node list property",
                               category(),
                               keyValue("type id", m_internalNode->typeId),
                               keyValue("caller location", sl)};

    return hasProperty(name, PropertyType::NodeList);
}

bool ModelNode::hasProperty(PropertyNameView name, PropertyType propertyType, SL sl) const
{
    if (!isValid())
        return {};

    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"model node has property",
                               category(),
                               keyValue("type id", m_internalNode->typeId),
                               keyValue("caller location", sl)};

    if (auto property = m_internalNode->property(name))
        return property->type() == propertyType;

    return false;
}

static bool recursiveAncestor(const ModelNode &possibleAncestor, const ModelNode &node)
{
    if (!node.isValid())
        return false;

    if (node.hasParentProperty()) {
        if (node.parentProperty().parentModelNode() == possibleAncestor)
            return true;
        return recursiveAncestor(possibleAncestor, node.parentProperty().parentModelNode());
    }

    return false;
}

bool ModelNode::isAncestorOf(const ModelNode &node, SL sl) const
{
    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"model node is ancestor of",
                               category(),
                               keyValue("type id", m_internalNode->typeId),
                               keyValue("caller location", sl)};

    return recursiveAncestor(*this, node);
}

QDebug operator<<(QDebug debug, const ModelNode &modelNode)
{
    if (modelNode.isValid()) {
        debug.nospace() << "ModelNode(" << modelNode.internalId() << ", " << modelNode.type()
                        << ", " << modelNode.id() << ')';
    } else {
        debug.nospace() << "ModelNode(invalid)";
    }

    return debug.space();
}

QTextStream &operator<<(QTextStream &stream, const ModelNode &modelNode)
{
    if (modelNode.isValid()) {
        stream << "ModelNode("
               << "type: " << modelNode.type() << ", "
               << "id: " << modelNode.id() << ')';
    } else {
        stream << "ModelNode(invalid)";
    }

    return stream;
}

void ModelNode::selectNode(SL sl)
{
    if (!isValid())
        return;

    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"model node select node",
                               category(),
                               keyValue("type id", m_internalNode->typeId),
                               keyValue("caller location", sl)};

    QList<ModelNode> selectedNodeList;
    selectedNodeList.append(*this);

    model()->setSelectedModelNodes(selectedNodeList);
}

void ModelNode::deselectNode(SL sl)
{
    if (!isValid())
        return;

    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"model node deselect node",
                               category(),
                               keyValue("type id", m_internalNode->typeId),
                               keyValue("caller location", sl)};

    auto selectedNodes = model()->d->selectedNodes();
    selectedNodes.removeAll(internalNode());
    model()->d->setSelectedNodes(selectedNodes);
}

int ModelNode::variantTypeId()
{
    return qMetaTypeId<ModelNode>();
}

QVariant ModelNode::toVariant(SL sl) const
{
    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"model node to variant",
                               category(),
                               keyValue("type id", m_internalNode->typeId),
                               keyValue("caller location", sl)};

    return QVariant::fromValue(*this);
}

std::optional<QVariant> ModelNode::auxiliaryData(AuxiliaryDataKeyView key, SL sl) const
{
    if (!isValid())
        return {};

    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"model node auxiliary data",
                               category(),
                               keyValue("type id", m_internalNode->typeId),
                               keyValue("caller location", sl)};

    return m_internalNode->auxiliaryData(key);
}

std::optional<QVariant> ModelNode::auxiliaryData(AuxiliaryDataType type,
                                                 Utils::SmallStringView name,
                                                 SL sl) const
{
    if (!isValid())
        return {};

    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"model node auxiliary data with name",
                               category(),
                               keyValue("type id", m_internalNode->typeId),
                               keyValue("caller location", sl)};

    return auxiliaryData({type, name});
}

QVariant ModelNode::auxiliaryDataWithDefault(AuxiliaryDataType type,
                                             Utils::SmallStringView name,
                                             SL sl) const
{
    if (!isValid())
        return {};

    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"model node auxiliary data with default 1",
                               category(),
                               keyValue("type id", m_internalNode->typeId),
                               keyValue("caller location", sl)};

    return auxiliaryDataWithDefault({type, name});
}

QVariant ModelNode::auxiliaryDataWithDefault(AuxiliaryDataKeyView key, SL sl) const
{
    if (!isValid())
        return {};

    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"model node auxiliary data with default 2",
                               category(),
                               keyValue("type id", m_internalNode->typeId),
                               keyValue("caller location", sl)};

    auto data = m_internalNode->auxiliaryData(key);

    if (data)
        return *data;

    return {};
}

QVariant ModelNode::auxiliaryDataWithDefault(AuxiliaryDataKeyDefaultValue key, SL sl) const
{
    if (!isValid())
        return toQVariant(key.defaultValue);

    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"model node auxiliary data with default 3",
                               category(),
                               keyValue("type id", m_internalNode->typeId),
                               keyValue("caller location", sl)};

    auto data = m_internalNode->auxiliaryData(key);

    if (data)
        return *data;

    return toQVariant(key.defaultValue);
}

void ModelNode::setAuxiliaryData(AuxiliaryDataType type,
                                 Utils::SmallStringView name,
                                 const QVariant &data,
                                 SL sl) const
{
    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"model node set auxiliary data with type",
                               category(),
                               keyValue("type id", m_internalNode->typeId),
                               keyValue("caller location", sl)};

    setAuxiliaryData({type, name}, data);
}

void ModelNode::setAuxiliaryData(AuxiliaryDataKeyView key, const QVariant &data, SL sl) const
{
    if (!isValid())
        return;

    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"model node set auxiliary data with key",
                               category(),
                               keyValue("type id", m_internalNode->typeId),
                               keyValue("caller location", sl)};

    if (key.type == AuxiliaryDataType::Persistent)
        ensureIdExists();
    Internal::WriteLocker locker(m_model.data());
    m_model->d->setAuxiliaryData(internalNode(), key, data);
}

void ModelNode::setAuxiliaryDataWithoutLock(AuxiliaryDataKeyView key, const QVariant &data, SL sl) const
{
    if (!isValid())
        return;

    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"model node set auxiliary data without lock with key",
                               category(),
                               keyValue("type id", m_internalNode->typeId),
                               keyValue("caller location", sl)};

    if (key.type == AuxiliaryDataType::Persistent)
        ensureIdExists();

    m_model->d->setAuxiliaryData(internalNode(), key, data);
}

void ModelNode::setAuxiliaryDataWithoutLock(AuxiliaryDataType type,
                                            Utils::SmallStringView name,
                                            const QVariant &data,
                                            SL sl) const
{
    if (!isValid())
        return;

    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"model node set auxiliary data without lock with type",
                               category(),
                               keyValue("type id", m_internalNode->typeId),
                               keyValue("caller location", sl)};

    if (type == AuxiliaryDataType::Persistent)
        ensureIdExists();

    m_model->d->setAuxiliaryData(internalNode(), {type, name}, data);
}

void ModelNode::removeAuxiliaryData(AuxiliaryDataKeyView key, SL sl) const
{
    if (!isValid())
        return;

    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"model node remove auxiliary data with key",
                               category(),
                               keyValue("type id", m_internalNode->typeId),
                               keyValue("caller location", sl)};

    if (key.type == AuxiliaryDataType::Persistent)
        ensureIdExists();

    Internal::WriteLocker locker(m_model.data());
    m_model->d->removeAuxiliaryData(internalNode(), key);
}

void ModelNode::removeAuxiliaryData(AuxiliaryDataType type, Utils::SmallStringView name, SL sl) const
{
    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"model node remove auxiliary data with type",
                               category(),
                               keyValue("type id", m_internalNode->typeId),
                               keyValue("caller location", sl)};

    removeAuxiliaryData({type, name});
}

bool ModelNode::hasAuxiliaryData(AuxiliaryDataKeyView key, SL sl) const
{
    if (!isValid())
        return false;

    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"model node has auxiliary data with key",
                               category(),
                               keyValue("type id", m_internalNode->typeId),
                               keyValue("caller location", sl)};

    return m_internalNode->hasAuxiliaryData(key);
}

bool ModelNode::hasAuxiliaryData(AuxiliaryDataType type, Utils::SmallStringView name, SL sl) const
{
    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"model node has auxiliary data with type",
                               category(),
                               keyValue("type id", m_internalNode->typeId),
                               keyValue("caller location", sl)};

    return hasAuxiliaryData({type, name});
}

bool ModelNode::hasAuxiliaryData(AuxiliaryDataType type) const
{
    if (!isValid())
        return false;

    // using NanotraceHR::keyValue;
    // NanotraceHR::Tracer tracer{"model node has auxiliary data with type",
    //                            category(),
    //                            keyValue("type id", m_internalNode->typeId),
    //                            keyValue("caller location", sl)};

    return m_internalNode->hasAuxiliaryData(type);
}

AuxiliaryDatasForType ModelNode::auxiliaryData(AuxiliaryDataType type, SL sl) const
{
    if (!isValid())
        return {};

    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"model node auxiliary data with type",
                               category(),
                               keyValue("type id", m_internalNode->typeId),
                               keyValue("caller location", sl)};

    return m_internalNode->auxiliaryData(type);
}

AuxiliaryDatasView ModelNode::auxiliaryData(SL sl) const
{
    if (!isValid())
        return {};

    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"model node auxiliary data with sl",
                               category(),
                               keyValue("type id", m_internalNode->typeId),
                               keyValue("caller location", sl)};

    return m_internalNode->auxiliaryData();
}

QString ModelNode::customId(SL sl) const
{
    auto data = auxiliaryData(customIdProperty);

    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"model node custom id",
                               category(),
                               keyValue("type id", m_internalNode->typeId),
                               keyValue("caller location", sl)};

    if (data)
        return data->toString();

    return {};
}

bool ModelNode::hasCustomId(SL sl) const
{
    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"model node has custom id",
                               category(),
                               keyValue("type id", m_internalNode->typeId),
                               keyValue("caller location", sl)};

    return hasAuxiliaryData(customIdProperty);
}

void ModelNode::setCustomId(const QString &str, SL sl)
{
    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"model node set custom id",
                               category(),
                               keyValue("type id", m_internalNode->typeId),
                               keyValue("caller location", sl)};

    setAuxiliaryData(customIdProperty, QVariant::fromValue(str));
}

void ModelNode::removeCustomId(SL sl)
{
    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"model node remove custom id",
                               category(),
                               keyValue("type id", m_internalNode->typeId),
                               keyValue("caller location", sl)};

    removeAuxiliaryData(customIdProperty);
}

QVector<Comment> ModelNode::comments(SL sl) const
{
    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"model node comments",
                               category(),
                               keyValue("type id", m_internalNode->typeId),
                               keyValue("caller location", sl)};

    return annotation().comments();
}

bool ModelNode::hasComments(SL sl) const
{
    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"model node has comments",
                               category(),
                               keyValue("type id", m_internalNode->typeId),
                               keyValue("caller location", sl)};

    return annotation().hasComments();
}

void ModelNode::setComments(const QVector<Comment> &coms, SL sl)
{
    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"model node set comments",
                               category(),
                               keyValue("type id", m_internalNode->typeId),
                               keyValue("caller location", sl)};

    Annotation anno = annotation();
    anno.setComments(coms);

    setAnnotation(anno);
}

void ModelNode::addComment(const Comment &com, SL sl)
{
    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"model node add comment",
                               category(),
                               keyValue("type id", m_internalNode->typeId),
                               keyValue("caller location", sl)};

    Annotation anno = annotation();
    anno.addComment(com);

    setAnnotation(anno);
}

bool ModelNode::updateComment(const Comment &com, int position, SL sl)
{
    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"model node update comment",
                               category(),
                               keyValue("type id", m_internalNode->typeId),
                               keyValue("caller location", sl)};

    bool result = false;
    if (hasAnnotation()) {
        Annotation anno = annotation();

        if (anno.updateComment(com, position)) {
            setAnnotation(anno);
            result = true;
        }
    }

    return result;
}

Annotation ModelNode::annotation(SL sl) const
{
    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"model node annotation",
                               category(),
                               keyValue("type id", m_internalNode->typeId),
                               keyValue("caller location", sl)};

    auto data = auxiliaryData(annotationProperty);

    if (data)
        return Annotation(data->toString());

    return {};
}

bool ModelNode::hasAnnotation(SL sl) const
{
    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"model node has annotation",
                               category(),
                               keyValue("type id", m_internalNode->typeId),
                               keyValue("caller location", sl)};

    return hasAuxiliaryData(annotationProperty);
}

void ModelNode::setAnnotation(const Annotation &annotation, SL sl)
{
    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"model node set annotation",
                               category(),
                               keyValue("type id", m_internalNode->typeId),
                               keyValue("caller location", sl)};

    setAuxiliaryData(annotationProperty, QVariant::fromValue(annotation.toQString()));
}

void ModelNode::removeAnnotation(SL sl)
{
    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"model node remove annotation",
                               category(),
                               keyValue("type id", m_internalNode->typeId),
                               keyValue("caller location", sl)};

    removeAuxiliaryData(annotationProperty);
}

Annotation ModelNode::globalAnnotation(SL sl) const
{
    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"model node global annotation",
                               category(),
                               keyValue("type id", m_internalNode->typeId),
                               keyValue("caller location", sl)};

    Annotation result;
    ModelNode root = m_model->rootModelNode();

    auto data = root.auxiliaryData(globalAnnotationProperty);

    if (data)
        return Annotation(data->toString());

    return {};
}

bool ModelNode::hasGlobalAnnotation(SL sl) const
{
    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"model node has global annotation",
                               category(),
                               keyValue("type id", m_internalNode->typeId),
                               keyValue("caller location", sl)};

    return m_model->rootModelNode().hasAuxiliaryData(globalAnnotationProperty);
}

void ModelNode::setGlobalAnnotation(const Annotation &annotation, SL sl)
{
    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"model node set global annotation",
                               category(),
                               keyValue("type id", m_internalNode->typeId),
                               keyValue("caller location", sl)};
    m_model->rootModelNode().setAuxiliaryData(globalAnnotationProperty,
                                              QVariant::fromValue(annotation.toQString()));
}

void ModelNode::removeGlobalAnnotation(SL sl)
{
    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"model node remove global annotation",
                               category(),
                               keyValue("type id", m_internalNode->typeId),
                               keyValue("caller location", sl)};

    m_model->rootModelNode().removeAuxiliaryData(globalAnnotationProperty);
}

GlobalAnnotationStatus ModelNode::globalStatus(SL sl) const
{
    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"model node global status",
                               category(),
                               keyValue("type id", m_internalNode->typeId),
                               keyValue("caller location", sl)};

    GlobalAnnotationStatus result;
    ModelNode root = m_model->rootModelNode();

    auto data = root.auxiliaryData(globalAnnotationStatus);

    if (data)
        result.fromQString(data->value<QString>());

    return result;
}

bool ModelNode::hasGlobalStatus(SL sl) const
{
    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"model node has global status",
                               category(),
                               keyValue("type id", m_internalNode->typeId),
                               keyValue("caller location", sl)};

    return m_model->rootModelNode().hasAuxiliaryData(globalAnnotationStatus);
}

void ModelNode::setGlobalStatus(const GlobalAnnotationStatus &status, SL sl)
{
    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"model node set global status",
                               category(),
                               keyValue("type id", m_internalNode->typeId),
                               keyValue("caller location", sl)};

    m_model->rootModelNode().setAuxiliaryData(globalAnnotationStatus,
                                              QVariant::fromValue(status.toQString()));
}

void ModelNode::removeGlobalStatus(SL sl)
{
    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"model node remove global status",
                               category(),
                               keyValue("type id", m_internalNode->typeId),
                               keyValue("caller location", sl)};

    if (hasGlobalStatus()) {
        m_model->rootModelNode().removeAuxiliaryData(globalAnnotationStatus);
    }
}

bool ModelNode::locked(SL sl) const
{
    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"model node locked",
                               category(),
                               keyValue("type id", m_internalNode->typeId),
                               keyValue("caller location", sl)};

    auto data = auxiliaryData(lockedProperty);

    if (data)
        return data->toBool();

    return false;
}

bool ModelNode::hasLocked(SL sl) const
{
    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"model node has locked",
                               category(),
                               keyValue("type id", m_internalNode->typeId),
                               keyValue("caller location", sl)};

    return hasAuxiliaryData(lockedProperty, sl);
}

void ModelNode::setLocked(bool value, SL sl)
{
    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"model node set locked",
                               category(),
                               keyValue("type id", m_internalNode->typeId),
                               keyValue("caller location", sl)};

    if (value) {
        setAuxiliaryData(lockedProperty, true);
        // Remove newly locked node and all its descendants from potential selection
        for (ModelNode node : allSubModelNodesAndThisNode()) {
            node.deselectNode();
            node.removeAuxiliaryData(timelineExpandedProperty);
            node.removeAuxiliaryData(transitionExpandedPropery);
        }
    } else {
        removeAuxiliaryData(lockedProperty);
    }
}

void ModelNode::setScriptFunctions(const QStringList &scriptFunctionList, SL sl)
{
    if (!isValid())
        return;

    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"model node set script functions",
                               category(),
                               keyValue("type id", m_internalNode->typeId),
                               keyValue("caller location", sl)};

    model()->d->setScriptFunctions(m_internalNode, scriptFunctionList);
}

QStringList ModelNode::scriptFunctions(SL sl) const
{
    if (!isValid())
        return {};

    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"model node script functions",
                               category(),
                               keyValue("type id", m_internalNode->typeId),
                               keyValue("caller location", sl)};

    return m_internalNode->scriptFunctions;
}

qint32 ModelNode::internalId(SL sl) const
{
    if (!m_internalNode)
        return -1;

    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"model node internal id",
                               category(),
                               keyValue("type id", m_internalNode->typeId),
                               keyValue("caller location", sl)};

    return m_internalNode->internalId;
}

void ModelNode::setNodeSource(const QString &newNodeSource, SL sl)
{
    Internal::WriteLocker locker(m_model.data());

    if (!isValid())
        return;

    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"model node set node source",
                               category(),
                               keyValue("type id", m_internalNode->typeId),
                               keyValue("caller location", sl)};

    if (m_internalNode->nodeSource == newNodeSource)
        return;

    m_model.data()->d->setNodeSource(m_internalNode, newNodeSource);
}

void ModelNode::setNodeSource(const QString &newNodeSource, NodeSourceType type, SL sl)
{
    Internal::WriteLocker locker(m_model.data());

    if (!isValid())
        return;

    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"model node set node source with type",
                               category(),
                               keyValue("type id", m_internalNode->typeId),
                               keyValue("caller location", sl)};

    if (m_internalNode->nodeSourceType == type && m_internalNode->nodeSource == newNodeSource)
        return;

    m_internalNode->nodeSourceType = type; // Set type first as it doesn't trigger any notifies
    m_model.data()->d->setNodeSource(m_internalNode, newNodeSource);
}

QString ModelNode::nodeSource(SL sl) const
{
    if (!isValid())
        return {};

    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"model node node source",
                               category(),
                               keyValue("type id", m_internalNode->typeId),
                               keyValue("caller location", sl)};

    return m_internalNode->nodeSource;
}

QString ModelNode::convertTypeToImportAlias(SL sl) const
{
    if (!isValid())
        return {};

    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"model node convert type to import alias",
                               category(),
                               keyValue("type id", m_internalNode->typeId),
                               keyValue("caller location", sl)};

    if (model()->rewriterView())
        return model()->rewriterView()->convertTypeToImportAlias(QString::fromLatin1(type()));

    return QString::fromLatin1(type());
}

ModelNode::NodeSourceType ModelNode::nodeSourceType(SL sl) const
{
    if (!isValid())
        return {};

    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"model node node source type",
                               category(),
                               keyValue("type id", m_internalNode->typeId),
                               keyValue("caller location", sl)};

    return static_cast<ModelNode::NodeSourceType>(m_internalNode->nodeSourceType);
}

bool ModelNode::isComponent(SL sl) const
{
    if (!isValid())
        return {};

    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"model node is component",
                               category(),
                               keyValue("type id", m_internalNode->typeId),
                               keyValue("caller location", sl)};

    if (!metaInfo().isValid())
        return false;

    if (metaInfo().isFileComponent())
        return true;

    if (nodeSourceType() == ModelNode::NodeWithComponentSource)
        return true;

    if (metaInfo().isView() && hasNodeProperty("delegate")) {
        const ModelNode delegateNode = nodeProperty("delegate").modelNode();
        if (delegateNode.isValid()) {
            if (delegateNode.hasMetaInfo()) {
                const NodeMetaInfo delegateMetaInfo = delegateNode.metaInfo();
                if (delegateMetaInfo.isValid() && delegateMetaInfo.isFileComponent())
                    return true;
            }
            if (delegateNode.nodeSourceType() == ModelNode::NodeWithComponentSource)
                return true;
        }
    }

    if (metaInfo().isQtQuickLoader()) {
        if (hasNodeListProperty("component")) {
            /*
         * The component property should be a NodeProperty, but currently is a NodeListProperty, because
         * the default property is always implcitly a NodeListProperty. This is something that has to be fixed.
         */

            ModelNode componentNode = nodeListProperty("component").toModelNodeList().constFirst();
            if (componentNode.nodeSourceType() == ModelNode::NodeWithComponentSource)
                return true;
            if (componentNode.metaInfo().isFileComponent())
                return true;
        }

        if (hasNodeProperty("sourceComponent")) {
            if (nodeProperty("sourceComponent").modelNode().nodeSourceType()
                == ModelNode::NodeWithComponentSource)
                return true;
            if (nodeProperty("sourceComponent").modelNode().metaInfo().isFileComponent())
                return true;
        }

        if (hasVariantProperty("source"))
            return true;
    }

    return false;
}

QIcon ModelNode::typeIcon([[maybe_unused]] SL sl) const
{
#ifdef QDS_USE_PROJECTSTORAGE
    if (!isValid())
        return QIcon(QStringLiteral(":/ItemLibrary/images/item-invalid-icon.png"));

    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"model node type icon",
                               category(),
                               keyValue("type id", m_internalNode->typeId),
                               keyValue("caller location", sl)};

    if (auto iconPath = metaInfo().iconPath(); iconPath.size())
        return QIcon(iconPath.toQString());
    else
        return QIcon(QStringLiteral(":/ItemLibrary/images/item-default-icon.png"));
#else
    if (isValid()) {
        // if node has no own icon, search for it in the itemlibrary
        const ItemLibraryInfo *libraryInfo = model()->metaInfo().itemLibraryInfo();
        QList<ItemLibraryEntry> itemLibraryEntryList = libraryInfo->entriesForType(type(),
                                                                                   majorVersion(),
                                                                                   minorVersion());
        if (!itemLibraryEntryList.isEmpty())
            return itemLibraryEntryList.constFirst().typeIcon();
        else if (metaInfo().isValid())
            return QIcon(QStringLiteral(":/ItemLibrary/images/item-default-icon.png"));
    }

    return QIcon(QStringLiteral(":/ItemLibrary/images/item-invalid-icon.png"));
#endif
}

QString ModelNode::behaviorPropertyName(SL sl) const
{
    if (!m_internalNode)
        return {};

    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"model node behavior property name",
                               category(),
                               keyValue("type id", m_internalNode->typeId),
                               keyValue("caller location", sl)};

    return m_internalNode->behaviorPropertyName;
}

template<typename Result>
Result toInternalNodeList(const QList<ModelNode> &nodeList)
{
    Result newNodeList;
    for (const ModelNode &node : nodeList)
        newNodeList.append(node.internalNode());

    return newNodeList;
}

template QMLDESIGNERCORE_EXPORT QVarLengthArray<Internal::InternalNodePointer, 1024> toInternalNodeList<
    QVarLengthArray<Internal::InternalNodePointer, 1024>>(const QList<ModelNode> &nodeList);
template QMLDESIGNERCORE_EXPORT QVarLengthArray<Internal::InternalNodePointer, 32> toInternalNodeList<
    QVarLengthArray<Internal::InternalNodePointer, 32>>(const QList<ModelNode> &nodeList);

} // namespace QmlDesigner
