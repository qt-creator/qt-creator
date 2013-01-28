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

#include "nodeabstractproperty.h"
#include "nodeproperty.h"
#include "internalproperty.h"
#include "internalnodelistproperty.h"
#include "invalidmodelnodeexception.h"
#include "invalidpropertyexception.h"
#include "invalidreparentingexception.h"
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

NodeAbstractProperty::NodeAbstractProperty(const QString &propertyName, const Internal::InternalNodePointer &internalNode, Model *model, AbstractView *view)
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
    if (modelNode.parentProperty() == *this)
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

int NodeAbstractProperty::count() const
{
    Internal::InternalNodeAbstractProperty::Pointer property = internalNode()->nodeAbstractProperty(name());
    if (property.isNull())
        return 0;
    else
        return property->count();
}

QList<ModelNode> NodeAbstractProperty::allSubNodes()
{
    if (!internalNode()
        || !internalNode()->isValid()
        || !internalNode()->hasProperty(name())
        || !internalNode()->property(name())->isNodeAbstractProperty())
        return QList<ModelNode>();

    Internal::InternalNodeAbstractProperty::Pointer property = internalNode()->nodeAbstractProperty(name());
    return toModelNodeList(property->allSubNodes(), view());
}

/*!
  \brief Returns if the the two property handles reference the same property in the same node
*/
bool operator ==(const NodeAbstractProperty &property1, const NodeAbstractProperty &property2)
{
    return AbstractProperty(property1) == AbstractProperty(property2);
}

/*!
  \brief Returns if the the two property handles do not reference the same property in the same node
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
    return debug.nospace() << "NodeAbstractProperty(" << (property.isValid() ? property.name() : QLatin1String("invalid")) << ')';
}

QTextStream& operator<<(QTextStream &stream, const NodeAbstractProperty &property)
{
    stream << "NodeAbstractProperty(" << property.name() << ')';

    return stream;
}
} // namespace QmlDesigner
