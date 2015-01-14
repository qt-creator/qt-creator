/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "nodeabstractproperty.h"
#include "nodeproperty.h"
#include "invalidmodelnodeexception.h"
#include "invalidpropertyexception.h"
#include "invalidreparentingexception.h"
#include "internalnodeabstractproperty.h"
#include "internalnode_p.h"
#include "model.h"
#include "model_p.h"

namespace QmlDesigner {

NodeAbstractProperty::NodeAbstractProperty()
{
}

NodeAbstractProperty::NodeAbstractProperty(const NodeAbstractProperty &property, AbstractView *view)
    : AbstractProperty(property.name(), property.internalNode(), property.model(), view)
{
}

NodeAbstractProperty::NodeAbstractProperty(const PropertyName &propertyName, const Internal::InternalNodePointer &internalNode, Model *model, AbstractView *view)
    : AbstractProperty(propertyName, internalNode, model, view)
{
}

NodeAbstractProperty::NodeAbstractProperty(const Internal::InternalNodeAbstractProperty::Pointer &property, Model *model, AbstractView *view)
    : AbstractProperty(property, model, view)
{}

void NodeAbstractProperty::reparentHere(const ModelNode &modelNode)
{
    if (internalNode()->hasProperty(name()) && !internalNode()->property(name())->isNodeAbstractProperty())
        reparentHere(modelNode, isNodeListProperty());
    else
        reparentHere(modelNode, parentModelNode().metaInfo().propertyIsListProperty(name()) || isDefaultProperty()); //we could use the metasystem instead?
}

void NodeAbstractProperty::reparentHere(const ModelNode &modelNode,  bool isNodeList)
{
    if (modelNode.hasParentProperty() && modelNode.parentProperty() == *this)
        return;
    Internal::WriteLocker locker(model());
    if (!isValid())
        throw InvalidModelNodeException(__LINE__, __FUNCTION__, __FILE__);

    if (isNodeProperty()) {
        NodeProperty nodeProperty(toNodeProperty());
        if (nodeProperty.modelNode().isValid())
            throw InvalidReparentingException(__LINE__, __FUNCTION__, __FILE__);
    }

    if (modelNode.isAncestorOf(parentModelNode()))
        throw InvalidReparentingException(__LINE__, __FUNCTION__, __FILE__);

    if (internalNode()->hasProperty(name()) && !internalNode()->property(name())->isNodeAbstractProperty())
        model()->d->removeProperty(internalNode()->property(name()));

    if (modelNode.hasParentProperty()) {
        Internal::InternalNodeAbstractProperty::Pointer oldParentProperty = modelNode.internalNode()->parentProperty();

        model()->d->reparentNode(internalNode(), name(), modelNode.internalNode(), isNodeList);

        Q_ASSERT(!oldParentProperty.isNull());


    } else {
        model()->d->reparentNode(internalNode(), name(), modelNode.internalNode(), isNodeList);
    }
}

bool NodeAbstractProperty::isEmpty() const
{
    Internal::InternalNodeAbstractProperty::Pointer property = internalNode()->nodeAbstractProperty(name());
    if (property.isNull())
        return true;
    else
        return property->isEmpty();
}

int NodeAbstractProperty::indexOf(const ModelNode &node) const
{
    Internal::InternalNodeAbstractProperty::Pointer property = internalNode()->nodeAbstractProperty(name());
    if (property.isNull())
        return 0;

    return property->indexOf(node.internalNode());
}

NodeAbstractProperty NodeAbstractProperty::parentProperty() const
{
    if (!isValid()) {
        Q_ASSERT_X(isValid(), Q_FUNC_INFO, "property is invalid");
        throw InvalidPropertyException(__LINE__, __FUNCTION__, __FILE__, name());
    }

    if (internalNode()->parentProperty().isNull()) {
        Q_ASSERT_X(internalNode()->parentProperty(), Q_FUNC_INFO, "parentProperty is invalid");
        throw InvalidPropertyException(__LINE__, __FUNCTION__, __FILE__, "parent");
    }

    return NodeAbstractProperty(internalNode()->parentProperty()->name(), internalNode()->parentProperty()->propertyOwner(), model(), view());
}

int NodeAbstractProperty::count() const
{
    Internal::InternalNodeAbstractProperty::Pointer property = internalNode()->nodeAbstractProperty(name());
    if (property.isNull())
        return 0;
    else
        return property->count();
}

const QList<ModelNode> NodeAbstractProperty::allSubNodes()
{
    if (!internalNode()
        || !internalNode()->isValid()
        || !internalNode()->hasProperty(name())
        || !internalNode()->property(name())->isNodeAbstractProperty())
        return QList<ModelNode>();

    Internal::InternalNodeAbstractProperty::Pointer property = internalNode()->nodeAbstractProperty(name());
    return QmlDesigner::toModelNodeList(property->allSubNodes(), view());
}

const QList<ModelNode> NodeAbstractProperty::directSubNodes() const
{
    if (!internalNode()
        || !internalNode()->isValid()
        || !internalNode()->hasProperty(name())
        || !internalNode()->property(name())->isNodeAbstractProperty())
        return QList<ModelNode>();

    Internal::InternalNodeAbstractProperty::Pointer property = internalNode()->nodeAbstractProperty(name());
    return QmlDesigner::toModelNodeList(property->directSubNodes(), view());
}

/*!
    Returns whether property handles \a property1 and \a property2 reference
    the same property in the same node.
*/
bool operator ==(const NodeAbstractProperty &property1, const NodeAbstractProperty &property2)
{
    return AbstractProperty(property1) == AbstractProperty(property2);
}

/*!
    Returns whether the property handles \a property1 and \a property2 do not
    reference the same property in the same node.
  */
bool operator !=(const NodeAbstractProperty &property1, const NodeAbstractProperty &property2)
{
    return !(property1 == property2);
}

uint qHash(const NodeAbstractProperty &property)
{
    return qHash(AbstractProperty(property));
}

QDebug operator<<(QDebug debug, const NodeAbstractProperty &property)
{
    return debug.nospace() << "NodeAbstractProperty(" << (property.isValid() ? property.name() : PropertyName("invalid")) << ')';
}

QTextStream& operator<<(QTextStream &stream, const NodeAbstractProperty &property)
{
    stream << "NodeAbstractProperty(" << property.name() << ')';

    return stream;
}
} // namespace QmlDesigner
