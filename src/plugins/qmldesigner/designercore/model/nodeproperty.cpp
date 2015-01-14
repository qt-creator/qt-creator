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

#include "nodeproperty.h"
#include "invalidmodelnodeexception.h"
#include "invalidargumentexception.h"
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

} // namespace QmlDesigner
