// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "nodelistproperty.h"
#include "internalproperty.h"
#include "internalnodelistproperty.h"
#include "internalnode_p.h"
#include "model.h"
#include "model_p.h"
#include <qmlobjectnode.h>

#include <cmath>

namespace QmlDesigner {

Internal::NodeListPropertyIterator::value_type Internal::NodeListPropertyIterator::operator*() const
{
    if (!m_nodeListProperty)
        return {};

    return {m_nodeListProperty->at(m_currentIndex), m_model, m_view};
}

NodeListProperty::NodeListProperty() = default;

NodeListProperty::NodeListProperty(const PropertyName &propertyName,
                                   const Internal::InternalNodePointer &internalNode,
                                   Model *model,
                                   AbstractView *view)
    : NodeAbstractProperty(propertyName, internalNode, model, view)
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

    auto property = internalNode()->property(name());
    if (property) {
        if (auto nodeListProperty = property->toShared<PropertyType::NodeList>())
            m_internalNodeListProperty = nodeListProperty;
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
        return {};

    if (internalNodeListProperty())
        return internalNodesToModelNodes(m_internalNodeListProperty->nodeList(), model(), view());

    return {};
}

QList<QmlObjectNode> NodeListProperty::toQmlObjectNodeList() const
{
    if (model()->nodeInstanceView())
        return {};

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
        return;

    if (to < 0 || to > count() - 1 || from < 0 || from > count() - 1)
        return;

    privateModel()->changeNodeOrder(internalNodeSharedPointer(), name(), from, to);
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
        return {};

    if (internalNodeListProperty())
        return ModelNode(m_internalNodeListProperty->at(index), model(), view());

    return ModelNode();
}

void NodeListProperty::iterSwap(NodeListProperty::iterator &first, NodeListProperty::iterator &second)
{
    if (!isValid())
        return;

    if (!internalNodeListProperty())
        return;

    std::swap(m_internalNodeListProperty->at(first.m_currentIndex),
              m_internalNodeListProperty->at(second.m_currentIndex));
}

NodeListProperty::iterator NodeListProperty::rotate(NodeListProperty::iterator first,
                                                    NodeListProperty::iterator newFirst,
                                                    NodeListProperty::iterator last)
{
    if (!isValid())
        return {};

    if (!internalNodeListProperty())
        return {};

    auto begin = m_internalNodeListProperty->begin();

    auto iter = std::rotate(std::next(begin, first.m_currentIndex),
                            std::next(begin, newFirst.m_currentIndex),
                            std::next(begin, last.m_currentIndex));

    privateModel()->notifyNodeOrderChanged(m_internalNodeListProperty.get());

    return {iter - begin, internalNodeListProperty().get(), model(), view()};
}

void NodeListProperty::reverse(NodeListProperty::iterator first, NodeListProperty::iterator last)
{
    if (!isValid())
        return;

    if (!internalNodeListProperty())
        return;

    auto begin = m_internalNodeListProperty->begin();

    std::reverse(std::next(begin, first.m_currentIndex), std::next(begin, last.m_currentIndex));

    privateModel()->notifyNodeOrderChanged(m_internalNodeListProperty.get());
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
    if (!isValid())
        return {};

    return {0, internalNodeListProperty().get(), model(), view()};
}

Internal::NodeListPropertyIterator NodeListProperty::end() const
{
    if (!isValid())
        return {};

    auto nodeListProperty = internalNodeListProperty();
    auto size = nodeListProperty ? nodeListProperty->size() : 0;

    return {size, nodeListProperty.get(), model(), view()};
}

} // namespace QmlDesigner
