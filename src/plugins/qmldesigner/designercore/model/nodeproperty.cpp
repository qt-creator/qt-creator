/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "nodeproperty.h"
#include "invalidmodelnodeexception.h"
#include "invalidargumentexception.h"
#include "invalidreparentingexception.h"
#include "internalnode_p.h"
#include "model.h"
#include "model_p.h"

namespace QmlDesigner {

NodeProperty::NodeProperty()
{
}

NodeProperty::NodeProperty(const PropertyName &propertyName, const Internal::InternalNodePointer &internalNode, Model* model, AbstractView *view)
    :  NodeAbstractProperty(propertyName, internalNode, model, view)
{

}

void NodeProperty::setModelNode(const ModelNode &modelNode)
{
    if (!isValid())
        throw InvalidModelNodeException(__LINE__, __FUNCTION__, __FILE__);

    if (!modelNode.isValid())
        throw InvalidArgumentException(__LINE__, __FUNCTION__, __FILE__, name());

    if (internalNode()->hasProperty(name())) { //check if oldValue != value
        Internal::InternalProperty::Pointer internalProperty = internalNode()->property(name());
        if (internalProperty->isNodeProperty()
            && internalProperty->toNodeProperty()->node() == modelNode.internalNode())
            return;
    }

    if (internalNode()->hasProperty(name()) && !internalNode()->property(name())->isNodeProperty())
        model()->d->removeProperty(internalNode()->property(name()));

    model()->d->reparentNode(internalNode(), name(), modelNode.internalNode(), false); //### we have to add a flag that this is not a list
}

ModelNode NodeProperty::modelNode() const
{
     if (!isValid())
        throw InvalidModelNodeException(__LINE__, __FUNCTION__, __FILE__);

     if (internalNode()->hasProperty(name())) { //check if oldValue != value
         Internal::InternalProperty::Pointer internalProperty = internalNode()->property(name());
         if (internalProperty->isNodeProperty())
             return ModelNode(internalProperty->toNodeProperty()->node(), model(), view());
     }

    return ModelNode();
}

void NodeProperty::reparentHere(const ModelNode &modelNode)
{
    NodeAbstractProperty::reparentHere(modelNode, false);
}

void NodeProperty::setDynamicTypeNameAndsetModelNode(const TypeName &typeName, const ModelNode &modelNode)
{
    if (!modelNode.isValid())
       throw InvalidModelNodeException(__LINE__, __FUNCTION__, __FILE__);

    if (modelNode.hasParentProperty()) /* Not supported */
        throw InvalidReparentingException(__LINE__, __FUNCTION__, __FILE__);

    NodeAbstractProperty::reparentHere(modelNode, false, typeName);
}

} // namespace QmlDesigner
