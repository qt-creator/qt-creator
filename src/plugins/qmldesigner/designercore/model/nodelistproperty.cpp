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

#include "nodelistproperty.h"
#include "internalproperty.h"
#include "internalnodelistproperty.h"
#include "invalidpropertyexception.h"
#include "internalnode_p.h"
#include "model.h"
#include "model_p.h"
#include <qmlobjectnode.h>

#include <cmath>

namespace QmlDesigner {

Internal::NodeListPropertyIterator::value_type Internal::NodeListPropertyIterator::operator*() const
{
    return {m_nodeListProperty->at(m_currentIndex), m_model, m_view};
}

NodeListProperty::NodeListProperty() = default;

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

Internal::InternalNodeListPropertyPointer &NodeListProperty::internalNodeListProperty() const
{
    if (m_internalNodeListProperty)
        return m_internalNodeListProperty;

    if (internalNode()->hasProperty(name())) {
        Internal::InternalProperty::Pointer internalProperty = internalNode()->property(name());
        if (internalProperty->isNodeListProperty())
            m_internalNodeListProperty = internalProperty->toNodeListProperty();
    }

    return m_internalNodeListProperty;
}

static QList<ModelNode> internalNodesToModelNodes(const QList<Internal::InternalNode::Pointer> &inputList, Model* model, AbstractView *view)
{
    QList<ModelNode> modelNodeList;
    for (const Internal::InternalNode::Pointer &internalNode : inputList) {
        modelNodeList.append(ModelNode(internalNode, model, view));
    }
    return modelNodeList;
}

QList<ModelNode> NodeListProperty::toModelNodeList() const
{
    if (!isValid())
        throw InvalidPropertyException(__LINE__, __FUNCTION__, __FILE__, "<invalid node list property>");

    if (internalNodeListProperty())
        return internalNodesToModelNodes(m_internalNodeListProperty->toNodeListProperty()->nodeList(),
                                         model(),
                                         view());

    return QList<ModelNode>();
}

QList<QmlObjectNode> NodeListProperty::toQmlObjectNodeList() const
{
    if (model()->nodeInstanceView())
        return QList<QmlObjectNode>();

    QList<QmlObjectNode> qmlObjectNodeList;

    const QList<ModelNode> modelNodeList = toModelNodeList();
    for (const ModelNode &modelNode : modelNodeList)
        qmlObjectNodeList.append(QmlObjectNode(modelNode));

    return qmlObjectNodeList;
}

void NodeListProperty::slide(int from, int to) const
{
    Internal::WriteLocker locker(model());
    if (!isValid())
        throw InvalidPropertyException(__LINE__, __FUNCTION__, __FILE__, "<invalid node list property>");
    if (to < 0 || to > count() - 1 || from < 0 || from > count() - 1)
        throw InvalidPropertyException(__LINE__, __FUNCTION__, __FILE__, "<invalid node list sliding>");

     privateModel()->changeNodeOrder(internalNode(), name(), from, to);
}

void NodeListProperty::swap(int from, int to) const
{
    if (from == to)
        return;

    // Prerequisite a < b
    int a = from;
    int b = to;

    if (a > b) {
        a = to;
        b = from;
    }

    slide(b, a);
    slide(a + 1, b);
}

void NodeListProperty::reparentHere(const ModelNode &modelNode)
{
    NodeAbstractProperty::reparentHere(modelNode, true);
}

ModelNode NodeListProperty::at(int index) const
{
    if (!isValid())
        throw InvalidPropertyException(__LINE__, __FUNCTION__, __FILE__, "<invalid node list property>");

    if (internalNodeListProperty())
        return ModelNode(m_internalNodeListProperty->at(index), model(), view());

    return ModelNode();
}

void NodeListProperty::iterSwap(NodeListProperty::iterator &first, NodeListProperty::iterator &second)
{
    if (!internalNodeListProperty())
        return;

    std::swap(m_internalNodeListProperty->at(first.m_currentIndex),
              m_internalNodeListProperty->at(second.m_currentIndex));
}

NodeListProperty::iterator NodeListProperty::rotate(NodeListProperty::iterator first,
                                                    NodeListProperty::iterator newFirst,
                                                    NodeListProperty::iterator last)
{
    if (!internalNodeListProperty())
        return {};

    auto begin = m_internalNodeListProperty->begin();

    auto iter = std::rotate(std::next(begin, first.m_currentIndex),
                            std::next(begin, newFirst.m_currentIndex),
                            std::next(begin, last.m_currentIndex));

    privateModel()->notifyNodeOrderChanged(m_internalNodeListProperty);

    return {iter - begin, internalNodeListProperty().data(), model(), view()};
}

void NodeListProperty::reverse(NodeListProperty::iterator first, NodeListProperty::iterator last)
{
    if (!internalNodeListProperty())
        return;

    auto begin = m_internalNodeListProperty->begin();

    std::reverse(std::next(begin, first.m_currentIndex), std::next(begin, last.m_currentIndex));

    privateModel()->notifyNodeOrderChanged(m_internalNodeListProperty);
}

void NodeListProperty::reverseModelNodes(const QList<ModelNode> &nodes)
{
    ModelNode firstNode = nodes.first();
    if (!firstNode.isValid())
        return;

    NodeListProperty parentProperty = firstNode.parentProperty().toNodeListProperty();
    std::vector<int> selectedNodeIndices;

    for (ModelNode modelNode : nodes)
        selectedNodeIndices.push_back(parentProperty.indexOf(modelNode));

    std::sort(selectedNodeIndices.begin(), selectedNodeIndices.end());

    int mid = std::ceil(selectedNodeIndices.size() / 2);

    for (int i = 0; i != mid; ++i)
        parentProperty.swap(selectedNodeIndices[i], selectedNodeIndices[selectedNodeIndices.size() - 1 - i]);
}

Internal::NodeListPropertyIterator NodeListProperty::begin()
{
    return const_cast<const NodeListProperty *>(this)->begin();
}

Internal::NodeListPropertyIterator NodeListProperty::end()
{
    return const_cast<const NodeListProperty *>(this)->end();
}

Internal::NodeListPropertyIterator NodeListProperty::begin() const
{
    return {0, internalNodeListProperty().data(), model(), view()};
}

Internal::NodeListPropertyIterator NodeListProperty::end() const
{
    auto nodeListProperty = internalNodeListProperty();
    auto size = nodeListProperty ? nodeListProperty->size() : 0;

    return {size, nodeListProperty.data(), model(), view()};
}

} // namespace QmlDesigner
