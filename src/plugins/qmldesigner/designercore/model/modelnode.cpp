// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "modelnode.h"

#include "annotation.h"
#include "bindingproperty.h"
#include "internalnode_p.h"
#include "model_p.h"
#include "nodeabstractproperty.h"
#include "nodelistproperty.h"
#include "nodeproperty.h"
#include "signalhandlerproperty.h"
#include "variantproperty.h"

#include <auxiliarydataproperties.h>
#include <model.h>
#include <nodemetainfo.h>
#include <rewriterview.h>

#include <utils/algorithm.h>

#include <QHash>
#include <QRegularExpression>
#include <QSet>
#include <QTextStream>

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
QString ModelNode::id() const
{
    if (!isValid())
        return {};

    return m_internalNode->id;
}

QString ModelNode::validId()
{
    if (id().isEmpty())
        setIdWithRefactoring(model()->generateNewId(simplifiedTypeName()));

    return id();
}

static bool idIsQmlKeyWord(const QString &id)
{
    static const QSet<QString> keywords = {"as",         "break",    "case",    "catch",
                                           "continue",   "debugger", "default", "delete",
                                           "do",         "else",     "finally", "for",
                                           "function",   "if",       "import",  "in",
                                           "instanceof", "new",      "print",   "return",
                                           "switch",     "this",     "throw",   "try",
                                           "typeof",     "var",      "void",    "while",
                                           "with"};

    return keywords.contains(id);
}

static bool isIdToAvoid(const QString &id)
{
    static const QSet<QString> ids = {"top",       "bottom",     "left",    "right",
                                      "width",     "height",     "x",       "y",
                                      "opacity",   "parent",     "item",    "flow",
                                      "color",     "margin",     "padding", "border",
                                      "font",      "text",       "source",  "state",
                                      "visible",   "focus",      "data",    "clip",
                                      "layer",     "scale",      "enabled", "anchors",
                                      "texture",   "shaderInfo", "sprite",  "spriteSequence",
                                      "baseState", "rect"};

    return ids.contains(id);
}

static bool idContainsWrongLetter(const QString &id)
{
    static QRegularExpression idExpr(QStringLiteral("^[a-z_][a-zA-Z0-9_]*$"));
    return !id.contains(idExpr);
}

bool ModelNode::isValidId(const QString &id)
{
    return id.isEmpty() || (!idContainsWrongLetter(id) && !idIsQmlKeyWord(id) && !isIdToAvoid(id));
}

QString ModelNode::getIdValidityErrorMessage(const QString &id)
{
    if (isValidId(id))
        return {}; // valid

    if (id.at(0).isUpper())
        return QObject::tr("ID cannot start with an uppercase character (%1).").arg(id);

    if (id.at(0).isDigit())
        return QObject::tr("ID cannot start with a number (%1).").arg(id);

    if (id.contains(' '))
        return QObject::tr("ID cannot include whitespace (%1).").arg(id);

    if (idIsQmlKeyWord(id))
        return QObject::tr("%1 is a reserved QML keyword.").arg(id);

    if (isIdToAvoid(id))
        return QObject::tr("%1 is a reserved property keyword.").arg(id);

    return QObject::tr("ID includes invalid characters (%1).").arg(id);
}

bool ModelNode::hasId() const
{
    if (!isValid())
        return false;

    return !m_internalNode->id.isEmpty();
}

void ModelNode::setIdWithRefactoring(const QString &id)
{
    if (isValid()) {
        if (model()->rewriterView() && !id.isEmpty()
            && !m_internalNode->id.isEmpty()) { // refactor the id if they are not empty
            model()->rewriterView()->renameId(m_internalNode->id, id);
        } else {
            setIdWithoutRefactoring(id);
        }
    }
}

void ModelNode::setIdWithoutRefactoring(const QString &id)
{
    Internal::WriteLocker locker(m_model.data());
    if (!isValid())
        return;

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
TypeName ModelNode::type() const
{
    if (!isValid())
        return {};

    return m_internalNode->typeName;
}

/*! \brief minor number of the QML type
\return minor number
*/
int ModelNode::minorVersion() const
{
    if (!isValid())
        return {};

    return m_internalNode->minorVersion;
}

/*! \brief major number of the QML type
\return major number
*/
int ModelNode::majorVersion() const
{
    if (!isValid())
        return {};

    return m_internalNode->majorVersion;
}

/*! \return the short-hand type name of the node. */
QString ModelNode::simplifiedTypeName() const
{
    if (!isValid())
        return {};

    return QString::fromUtf8(type().split('.').constLast());
}

QString ModelNode::displayName() const
{
    if (hasId())
        return id();
    return simplifiedTypeName();
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
bool ModelNode::isInHierarchy() const
{
    if (!isValid())
        return false;
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
    if (!isValid())
        return {};

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

void ModelNode::setParentProperty(NodeAbstractProperty parent)
{
    if (!isValid())
        return;

    if (!parent.parentModelNode().isValid())
        return;

    if (*this == parent.parentModelNode())
        return;

    if (hasParentProperty() && parent == parentProperty())
        return;

    parent.reparentHere(*this);
}

void ModelNode::changeType(const TypeName &typeName, int majorVersion, int minorVersion)
{
    if (!isValid())
        return;

    model()->d->changeNodeType(m_internalNode, typeName, majorVersion, minorVersion);
}

void ModelNode::setParentProperty(const ModelNode &newParentNode, const PropertyName &propertyName)
{
    setParentProperty(newParentNode.nodeAbstractProperty(propertyName));
}

/*! \brief test if there is a parent for this node
\return true is this node has a parent
\see childNodes parentNode setParentNode hasChildNodes Model::undo
*/
bool ModelNode::hasParentProperty() const
{
    if (!isValid())
        return false;

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

BindingProperty ModelNode::bindingProperty(const PropertyName &name) const
{
    if (!isValid())
        return {};

    return BindingProperty(name, m_internalNode, model(), view());
}

SignalHandlerProperty ModelNode::signalHandlerProperty(const PropertyName &name) const
{
    if (!isValid())
        return {};

    return SignalHandlerProperty(name, m_internalNode, model(), view());
}

SignalDeclarationProperty ModelNode::signalDeclarationProperty(const PropertyName &name) const
{
    if (!isValid())
        return {};

    return SignalDeclarationProperty(name, m_internalNode, model(), view());
}

/*!
  \brief Returns a NodeProperty

  Note that a valid NodeProperty is returned, if the ModelNode is valid,
  even if this property does not exist or is not a NodeProperty.
  Assigning a ModelNode to this NodeProperty will create the property.

  \return NodeProperty named name
  */

NodeProperty ModelNode::nodeProperty(const PropertyName &name) const
{
    if (!isValid())
        return {};

    return NodeProperty(name, m_internalNode, model(), view());
}

/*!
  \brief Returns a NodeListProperty

  Note that a valid NodeListProperty is returned, if the ModelNode is valid,
  even if this property does not exist or is not a NodeListProperty.
  Assigning a ModelNode to this NodeListProperty will create the property.

  \return NodeListProperty named name
  */

NodeListProperty ModelNode::nodeListProperty(const PropertyName &name) const
{
    if (!isValid())
        return {};

    return NodeListProperty(name, m_internalNode, model(), view());
}

NodeAbstractProperty ModelNode::nodeAbstractProperty(const PropertyName &name) const
{
    if (!isValid())
        return {};

    return NodeAbstractProperty(name, m_internalNode, model(), view());
}

NodeAbstractProperty ModelNode::defaultNodeAbstractProperty() const
{
    return nodeAbstractProperty(metaInfo().defaultPropertyName());
}

NodeListProperty ModelNode::defaultNodeListProperty() const
{
    return nodeListProperty(metaInfo().defaultPropertyName());
}

NodeProperty ModelNode::defaultNodeProperty() const
{
    return nodeProperty(metaInfo().defaultPropertyName());
}

/*!
  \brief Returns a VariantProperty

  Note that a valid VariantProperty is returned, if the ModelNode is valid,
  even if this property does not exist or is not a VariantProperty.
  Assigning a value to this VariantProperty will create the property.

  \return VariantProperty named name
  */

VariantProperty ModelNode::variantProperty(const PropertyName &name) const
{
    if (!isValid())
        return {};

    return VariantProperty(name, m_internalNode, model(), view());
}

AbstractProperty ModelNode::property(const PropertyName &name) const
{
    if (!isValid())
        return {};

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
        return {};

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
QList<VariantProperty> ModelNode::variantProperties() const
{
    return properties<VariantProperty>(PropertyType::Variant);
}

QList<NodeAbstractProperty> ModelNode::nodeAbstractProperties() const
{
    return properties<NodeAbstractProperty>(PropertyType::Node, PropertyType::NodeList);
}

QList<NodeProperty> ModelNode::nodeProperties() const
{
    return properties<NodeProperty>(PropertyType::Node);
}

QList<NodeListProperty> ModelNode::nodeListProperties() const
{
    return properties<NodeListProperty>(PropertyType::NodeList);
}

/*! \brief returns a list of all BindingProperties
\return list of all BindingProperties

The list of all properties containing an expression.

*/
QList<BindingProperty> ModelNode::bindingProperties() const
{
    return properties<BindingProperty>(PropertyType::Binding);
}

QList<SignalHandlerProperty> ModelNode::signalProperties() const
{
    return properties<SignalHandlerProperty>(PropertyType::SignalHandler);
}

QList<AbstractProperty> ModelNode::dynamicProperties() const
{
    if (!isValid())
        return {};

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
void ModelNode::removeProperty(PropertyNameView name) const
{
    if (!isValid())
        return;

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
    QList<ModelNode> selectedList = node.view()->selectedModelNodes();

    const QList<ModelNode> descendants = descendantNodes(node);
    for (const ModelNode &descendantNode : descendants)
        selectedList.removeAll(descendantNode);

    selectedList.removeAll(node);

    node.view()->setSelectedModelNodes(selectedList);
}

/*! \brief complete removes this ModelNode from the Model
*/
void ModelNode::destroy()
{
    if (!isValid())
        return;

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
QList<ModelNode> ModelNode::directSubModelNodes() const
{
    if (!isValid())
        return {};

    return toModelNodeList(m_internalNode->allDirectSubNodes(), model(), view());
}

QList<ModelNode> ModelNode::directSubModelNodesOfType(const NodeMetaInfo &type) const
{
    return Utils::filtered(directSubModelNodes(), [&](const ModelNode &node) {
        return node.metaInfo().isValid() && node.metaInfo().isBasedOn(type);
    });
}

QList<ModelNode> ModelNode::subModelNodesOfType(const NodeMetaInfo &type) const
{
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

QList<ModelNode> ModelNode::allSubModelNodes() const
{
    if (!isValid())
        return {};

    return toModelNodeList(internalNode()->allSubNodes(), model(), view());
}

QList<ModelNode> ModelNode::allSubModelNodesAndThisNode() const
{
    QList<ModelNode> modelNodeList;
    modelNodeList.append(*this);
    modelNodeList.append(allSubModelNodes());

    return modelNodeList;
}

/*!
\brief returns if this ModelNode has any child ModelNodes.

\return if this ModelNode has any child ModelNodes
*/

bool ModelNode::hasAnySubModelNodes() const
{
    return !nodeAbstractProperties().isEmpty();
}

NodeMetaInfo ModelNode::metaInfo() const
{
    if (!isValid())
        return {};

    if constexpr (useProjectStorage()) {
        return NodeMetaInfo(m_internalNode->typeId, m_model->projectStorage());
    } else {
        return NodeMetaInfo(m_model->metaInfoProxyModel(),
                            m_internalNode->typeName,
                            m_internalNode->majorVersion,
                            m_internalNode->minorVersion);
    }
}

bool ModelNode::hasMetaInfo() const
{
    if (!isValid())
        return false;

    return model()->hasNodeMetaInfo(type(), majorVersion(), minorVersion());
}

/*! \brief has a node the selection of the model
\return true if the node his selection
*/
bool ModelNode::isSelected() const
{
    if (!isValid())
        return false;

    return view()->selectedModelNodes().contains(ModelNode(m_internalNode, m_model.data(), view()));
}

/*! \briefis this node the root node of the model
\return true if it is the root node
*/
bool ModelNode::isRootNode() const
{
    if (!isValid())
        return false;

    return m_model->d->rootNode() == m_internalNode;
}

/*! \brief returns the list of all property names
\return list of all property names set in this state.

The list of properties set in this state.

\see addProperty property changePropertyValue removeProperty hasProperties
*/
PropertyNameList ModelNode::propertyNames() const
{
    if (!isValid())
        return {};

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

/*! \brief test a if a property is set for this node
\return true if property a property ins this or a ancestor state exists
*/
bool ModelNode::hasProperty(PropertyNameView name) const
{
    return isValid() && m_internalNode->property(name);
}

bool ModelNode::hasVariantProperty(PropertyNameView name) const
{
    return hasProperty(name, PropertyType::Variant);
}

bool ModelNode::hasBindingProperty(PropertyNameView name) const
{
    return hasProperty(name, PropertyType::Binding);
}

bool ModelNode::hasSignalHandlerProperty(PropertyNameView name) const
{
    return hasProperty(name, PropertyType::SignalHandler);
}

bool ModelNode::hasNodeAbstractProperty(PropertyNameView name) const
{
    if (!isValid())
        return false;

    if (auto property = m_internalNode->property(name))
        return property->isNodeAbstractProperty();

    return false;
}

bool ModelNode::hasDefaultNodeAbstractProperty() const
{
    auto defaultPropertyName = metaInfo().defaultPropertyName();
    return hasNodeAbstractProperty(defaultPropertyName);
}

bool ModelNode::hasDefaultNodeListProperty() const
{
    auto defaultPropertyName = metaInfo().defaultPropertyName();
    return hasNodeListProperty(defaultPropertyName);
}

bool ModelNode::hasDefaultNodeProperty() const
{
    auto defaultPropertyName = metaInfo().defaultPropertyName();
    return hasNodeProperty(defaultPropertyName);
}

bool ModelNode::hasNodeProperty(PropertyNameView name) const
{
    return hasProperty(name, PropertyType::Node);
}

bool ModelNode::hasNodeListProperty(PropertyNameView name) const
{
    return hasProperty(name, PropertyType::NodeList);
}

bool ModelNode::hasProperty(PropertyNameView name, PropertyType propertyType) const
{
    if (!isValid())
        return false;

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

bool ModelNode::isAncestorOf(const ModelNode &node) const
{
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

void ModelNode::selectNode()
{
    if (!isValid())
        return;

    QList<ModelNode> selectedNodeList;
    selectedNodeList.append(*this);

    view()->setSelectedModelNodes(selectedNodeList);
}

void ModelNode::deselectNode()
{
    if (!isValid())
        return;

    QList<ModelNode> selectedNodeList(view()->selectedModelNodes());
    selectedNodeList.removeAll(*this);

    view()->setSelectedModelNodes(selectedNodeList);
}

int ModelNode::variantTypeId()
{
    return qMetaTypeId<ModelNode>();
}

QVariant ModelNode::toVariant() const
{
    return QVariant::fromValue(*this);
}

std::optional<QVariant> ModelNode::auxiliaryData(AuxiliaryDataKeyView key) const
{
    if (!isValid())
        return {};

    return m_internalNode->auxiliaryData(key);
}

std::optional<QVariant> ModelNode::auxiliaryData(AuxiliaryDataType type, Utils::SmallStringView name) const
{
    return auxiliaryData({type, name});
}

QVariant ModelNode::auxiliaryDataWithDefault(AuxiliaryDataType type, Utils::SmallStringView name) const
{
    return auxiliaryDataWithDefault({type, name});
}

QVariant ModelNode::auxiliaryDataWithDefault(AuxiliaryDataKeyView key) const
{
    if (!isValid())
        return {};

    auto data = m_internalNode->auxiliaryData(key);

    if (data)
        return *data;

    return {};
}

QVariant ModelNode::auxiliaryDataWithDefault(AuxiliaryDataKeyDefaultValue key) const
{
    if (!isValid())
        return toQVariant(key.defaultValue);

    auto data = m_internalNode->auxiliaryData(key);

    if (data)
        return *data;

    return toQVariant(key.defaultValue);
}

void ModelNode::setAuxiliaryData(AuxiliaryDataType type,
                                 Utils::SmallStringView name,
                                 const QVariant &data) const
{
    setAuxiliaryData({type, name}, data);
}

void ModelNode::setAuxiliaryData(AuxiliaryDataKeyView key, const QVariant &data) const
{
    if (isValid()) {
        Internal::WriteLocker locker(m_model.data());
        m_model->d->setAuxiliaryData(internalNode(), key, data);
    }
}

void ModelNode::setAuxiliaryDataWithoutLock(AuxiliaryDataKeyView key, const QVariant &data) const
{
    if (isValid())
        m_model->d->setAuxiliaryData(internalNode(), key, data);
}

void ModelNode::setAuxiliaryDataWithoutLock(AuxiliaryDataType type,
                                            Utils::SmallStringView name,
                                            const QVariant &data) const
{
    if (isValid())
        m_model->d->setAuxiliaryData(internalNode(), {type, name}, data);
}

void ModelNode::removeAuxiliaryData(AuxiliaryDataKeyView key) const
{
    if (isValid()) {
        Internal::WriteLocker locker(m_model.data());
        m_model->d->removeAuxiliaryData(internalNode(), key);
    }
}

void ModelNode::removeAuxiliaryData(AuxiliaryDataType type, Utils::SmallStringView name) const
{
    removeAuxiliaryData({type, name});
}

bool ModelNode::hasAuxiliaryData(AuxiliaryDataKeyView key) const
{
    if (!isValid())
        return false;

    return m_internalNode->hasAuxiliaryData(key);
}

bool ModelNode::hasAuxiliaryData(AuxiliaryDataType type, Utils::SmallStringView name) const
{
    return hasAuxiliaryData({type, name});
}

AuxiliaryDatasForType ModelNode::auxiliaryData(AuxiliaryDataType type) const
{
    if (!isValid())
        return {};

    return m_internalNode->auxiliaryData(type);
}

AuxiliaryDatasView ModelNode::auxiliaryData() const
{
    if (!isValid())
        return {};

    return m_internalNode->auxiliaryData();
}

QString ModelNode::customId() const
{
    auto data = auxiliaryData(customIdProperty);

    if (data)
        return data->toString();

    return {};
}

bool ModelNode::hasCustomId() const
{
    return hasAuxiliaryData(customIdProperty);
}

void ModelNode::setCustomId(const QString &str)
{
    setAuxiliaryData(customIdProperty, QVariant::fromValue(str));
}

void ModelNode::removeCustomId()
{
    removeAuxiliaryData(customIdProperty);
}

QVector<Comment> ModelNode::comments() const
{
    return annotation().comments();
}

bool ModelNode::hasComments() const
{
    return annotation().hasComments();
}

void ModelNode::setComments(const QVector<Comment> &coms)
{
    Annotation anno = annotation();
    anno.setComments(coms);

    setAnnotation(anno);
}

void ModelNode::addComment(const Comment &com)
{
    Annotation anno = annotation();
    anno.addComment(com);

    setAnnotation(anno);
}

bool ModelNode::updateComment(const Comment &com, int position)
{
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

Annotation ModelNode::annotation() const
{
    auto data = auxiliaryData(annotationProperty);

    if (data)
        return Annotation(data->toString());

    return {};
}

bool ModelNode::hasAnnotation() const
{
    return hasAuxiliaryData(annotationProperty);
}

void ModelNode::setAnnotation(const Annotation &annotation)
{
    setAuxiliaryData(annotationProperty, QVariant::fromValue(annotation.toQString()));
}

void ModelNode::removeAnnotation()
{
    removeAuxiliaryData(annotationProperty);
}

Annotation ModelNode::globalAnnotation() const
{
    Annotation result;
    ModelNode root = m_model->rootModelNode();

    auto data = root.auxiliaryData(globalAnnotationProperty);

    if (data)
        return Annotation(data->toString());

    return {};
}

bool ModelNode::hasGlobalAnnotation() const
{
    return m_model->rootModelNode().hasAuxiliaryData(globalAnnotationProperty);
}

void ModelNode::setGlobalAnnotation(const Annotation &annotation)
{
    m_model->rootModelNode().setAuxiliaryData(globalAnnotationProperty,
                                              QVariant::fromValue(annotation.toQString()));
}

void ModelNode::removeGlobalAnnotation()
{
    m_model->rootModelNode().removeAuxiliaryData(globalAnnotationProperty);
}

GlobalAnnotationStatus ModelNode::globalStatus() const
{
    GlobalAnnotationStatus result;
    ModelNode root = m_model->rootModelNode();

    auto data = root.auxiliaryData(globalAnnotationStatus);

    if (data)
        result.fromQString(data->value<QString>());

    return result;
}

bool ModelNode::hasGlobalStatus() const
{
    return m_model->rootModelNode().hasAuxiliaryData(globalAnnotationStatus);
}

void ModelNode::setGlobalStatus(const GlobalAnnotationStatus &status)
{
    m_model->rootModelNode().setAuxiliaryData(globalAnnotationStatus,
                                              QVariant::fromValue(status.toQString()));
}

void ModelNode::removeGlobalStatus()
{
    if (hasGlobalStatus()) {
        m_model->rootModelNode().removeAuxiliaryData(globalAnnotationStatus);
    }
}

bool ModelNode::locked() const
{
    auto data = auxiliaryData(lockedProperty);

    if (data)
        return data->toBool();

    return false;
}

bool ModelNode::hasLocked() const
{
    return hasAuxiliaryData(lockedProperty);
}

void ModelNode::setLocked(bool value)
{
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

void ModelNode::setScriptFunctions(const QStringList &scriptFunctionList)
{
    if (!isValid())
        return;

    model()->d->setScriptFunctions(m_internalNode, scriptFunctionList);
}

QStringList ModelNode::scriptFunctions() const
{
    if (!isValid())
        return {};

    return m_internalNode->scriptFunctions;
}

qint32 ModelNode::internalId() const
{
    if (!m_internalNode)
        return -1;

    return m_internalNode->internalId;
}

void ModelNode::setNodeSource(const QString &newNodeSource)
{
    Internal::WriteLocker locker(m_model.data());

    if (!isValid())
        return;

    if (m_internalNode->nodeSource == newNodeSource)
        return;

    m_model.data()->d->setNodeSource(m_internalNode, newNodeSource);
}

void ModelNode::setNodeSource(const QString &newNodeSource, NodeSourceType type)
{
    Internal::WriteLocker locker(m_model.data());

    if (!isValid())
        return;

    if (m_internalNode->nodeSourceType == type && m_internalNode->nodeSource == newNodeSource)
        return;

    m_internalNode->nodeSourceType = type; // Set type first as it doesn't trigger any notifies
    m_model.data()->d->setNodeSource(m_internalNode, newNodeSource);
}

QString ModelNode::nodeSource() const
{
    if (!isValid())
        return {};

    return m_internalNode->nodeSource;
}

QString ModelNode::convertTypeToImportAlias() const
{
    if (!isValid())
        return {};

    if (model()->rewriterView())
        return model()->rewriterView()->convertTypeToImportAlias(QString::fromLatin1(type()));

    return QString::fromLatin1(type());
}

ModelNode::NodeSourceType ModelNode::nodeSourceType() const
{
    if (!isValid())
        return {};

    return static_cast<ModelNode::NodeSourceType>(m_internalNode->nodeSourceType);
}

bool ModelNode::isComponent() const
{
    if (!isValid())
        return false;

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

QIcon ModelNode::typeIcon() const
{
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
}

QString ModelNode::behaviorPropertyName() const
{
    if (!m_internalNode)
        return {};

    return m_internalNode->behaviorPropertyName;
}

} // namespace QmlDesigner
