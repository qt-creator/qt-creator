// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "internalnodelistproperty.h"
#include "internalnode_p.h"
#include <QList>

namespace QmlDesigner {
namespace Internal {

InternalNodeListProperty::InternalNodeListProperty(const PropertyName &name,
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
    return m_nodeList.isEmpty();
}

int InternalNodeListProperty::count() const
{
    return m_nodeList.size();
}

int InternalNodeListProperty::indexOf(const InternalNode::Pointer &node) const
{
    if (!node)
        return -1;

    return m_nodeList.indexOf(node);
}

void InternalNodeListProperty::add(const InternalNode::Pointer &internalNode)
{
    Q_ASSERT(!m_nodeList.contains(internalNode));
    m_nodeList.append(internalNode);
}

void InternalNodeListProperty::remove(const InternalNodePointer &internalNode)
{
    Q_ASSERT(m_nodeList.contains(internalNode));
    m_nodeList.removeAll(internalNode);
}

const QList<InternalNode::Pointer> &InternalNodeListProperty::nodeList() const
{
    return m_nodeList;
}

void InternalNodeListProperty::slide(int from, int to)
{
    InternalNode::Pointer internalNode = m_nodeList.takeAt(from);
    m_nodeList.insert(to, internalNode);
}

void InternalNodeListProperty::addSubNodes(QList<InternalNodePointer> &container) const
{
    for (const auto &node : std::as_const(m_nodeList)) {
        container.push_back(node);
        node->addSubNodes(container);
    }
}

QList<InternalNode::Pointer> InternalNodeListProperty::allSubNodes() const
{
    QList<InternalNode::Pointer> nodes;
    nodes.reserve(1024);

    addSubNodes(nodes);

    return nodes;
}

} // namespace Internal
} // namespace QmlDesigner
