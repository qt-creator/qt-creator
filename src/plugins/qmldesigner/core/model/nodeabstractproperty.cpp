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
        reparentHere(modelNode, metaInfo().isListProperty()); //we could use the metasystem instead?
}

void NodeAbstractProperty::reparentHere(const ModelNode &modelNode,  bool isNodeList)
{
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
        model()->m_d->removeProperty(internalNode()->property(name()));

    if (modelNode.hasParentProperty()) {
        Internal::InternalNodeAbstractProperty::Pointer oldParentProperty = modelNode.internalNode()->parentProperty();

        model()->m_d->reparentNode(internalNode(), name(), modelNode.internalNode(), isNodeList);

        Q_ASSERT(!oldParentProperty.isNull());


    } else {
        model()->m_d->reparentNode(internalNode(), name(), modelNode.internalNode(), isNodeList);
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

} // namespace QmlDesigner
