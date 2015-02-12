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

#include "nodelistproperty.h"
#include "internalproperty.h"
#include "internalnodelistproperty.h"
#include "invalidpropertyexception.h"
#include "internalnode_p.h"
#include "model.h"
#include "model_p.h"
#include <qmlobjectnode.h>



namespace QmlDesigner {

NodeListProperty::NodeListProperty()
{}

NodeListProperty::NodeListProperty(const NodeListProperty &property, AbstractView *view)
    : NodeAbstractProperty(property.name(), property.internalNode(), property.model(), view)
{

}

NodeListProperty::NodeListProperty(const PropertyName &propertyName, const Internal::InternalNodePointer &internalNode, Model* model, AbstractView *view) :
        NodeAbstractProperty(propertyName, internalNode, model, view)
{
}

NodeListProperty::NodeListProperty(const Internal::InternalNodeListProperty::Pointer &internalNodeListProperty, Model* model, AbstractView *view)
    : NodeAbstractProperty(internalNodeListProperty, model, view)
{
}

static QList<ModelNode> internalNodesToModelNodes(const QList<Internal::InternalNode::Pointer> &inputList, Model* model, AbstractView *view)
{
    QList<ModelNode> modelNodeList;
    foreach (const Internal::InternalNode::Pointer &internalNode, inputList) {
        modelNodeList.append(ModelNode(internalNode, model, view));
    }
    return modelNodeList;
}

const QList<ModelNode> NodeListProperty::toModelNodeList() const
{
    if (!isValid())
        throw InvalidPropertyException(__LINE__, __FUNCTION__, __FILE__, "<invalid node list property>");

    if (internalNode()->hasProperty(name())) {
        Internal::InternalProperty::Pointer internalProperty = internalNode()->property(name());
        if (internalProperty->isNodeListProperty())
            return internalNodesToModelNodes(internalProperty->toNodeListProperty()->nodeList(), model(), view());
    }

    return QList<ModelNode>();
}

const QList<QmlObjectNode> NodeListProperty::toQmlObjectNodeList() const
{
    if (model()->nodeInstanceView())
        return QList<QmlObjectNode>();

    QList<QmlObjectNode> qmlObjectNodeList;

    foreach (const ModelNode &modelNode, toModelNodeList())
        qmlObjectNodeList.append(QmlObjectNode(modelNode));

    return qmlObjectNodeList;
}

void NodeListProperty::slide(int from, int to) const
{
    Internal::WriteLocker locker(model());
    if (!isValid())
        throw InvalidPropertyException(__LINE__, __FUNCTION__, __FILE__, "<invalid node list property>");
    if (to > count() - 1)
        throw InvalidPropertyException(__LINE__, __FUNCTION__, __FILE__, "<invalid node list sliding>");

     model()->d->changeNodeOrder(internalNode(), name(), from, to);
}

void NodeListProperty::reparentHere(const ModelNode &modelNode)
{
    NodeAbstractProperty::reparentHere(modelNode, true);
}

ModelNode NodeListProperty::at(int index) const
{
    if (!isValid())
        throw InvalidPropertyException(__LINE__, __FUNCTION__, __FILE__, "<invalid node list property>");

    Internal::InternalNodeListProperty::Pointer internalProperty = internalNode()->nodeListProperty(name());
    if (internalProperty)
        return ModelNode(internalProperty->at(index), model(), view());


    return ModelNode();
}

} // namespace QmlDesigner
