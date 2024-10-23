// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "internalnodelistproperty.h"
#include "internalnode_p.h"
#include <QList>

namespace QmlDesigner {
namespace Internal {

InternalNodeListProperty::InternalNodeListProperty(PropertyNameView name,
                                                   const InternalNodePointer &propertyOwner)
    : InternalNodeAbstractProperty(name, propertyOwner, PropertyType::NodeList)
{
}

bool InternalNodeListProperty::isValid() const
{
    return InternalProperty::isValid() && isNodeListProperty();
}

bool InternalNodeListProperty::isEmpty() const
{
    return m_nodes.isEmpty();
}

int InternalNodeListProperty::count() const
{
    return m_nodes.size();
}

int InternalNodeListProperty::indexOf(const InternalNode::Pointer &node) const
{
    if (!node)
        return -1;

    return m_nodes.indexOf(node);
}

void InternalNodeListProperty::add(const InternalNode::Pointer &internalNode)
{
    Q_ASSERT(!m_nodes.contains(internalNode));

    auto flowToken = traceToken.tickWithFlow("add node");
    internalNode->traceToken.tick(flowToken, "node added");

    m_nodes.append(internalNode);
}

void InternalNodeListProperty::remove(const InternalNodePointer &internalNode)
{
    Q_ASSERT(m_nodes.contains(internalNode));

    auto flowToken = traceToken.tickWithFlow("remove node");
    internalNode->traceToken.tick(flowToken, "node removed");

    m_nodes.removeAll(internalNode);
}

const InternalNodeListProperty::FewNodes &InternalNodeListProperty::nodeList() const
{
    return m_nodes;
}

void InternalNodeListProperty::slide(int from, int to)
{
    traceToken.tick("slide", keyValue("from", from), keyValue("to", to));

    InternalNode::Pointer internalNode = m_nodes.at(from);
    m_nodes.remove(from);
    m_nodes.insert(to, internalNode);
}

void InternalNodeListProperty::addSubNodes(ManyNodes &container) const
{
    for (const auto &node : std::as_const(m_nodes)) {
        container.push_back(node);
        node->addSubNodes(container);
    }
}

InternalNodeListProperty::ManyNodes InternalNodeListProperty::allSubNodes() const
{
    ManyNodes nodes;
    nodes.reserve(1024);

    addSubNodes(nodes);

    return nodes;
}

} // namespace Internal
} // namespace QmlDesigner
