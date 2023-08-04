// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "internalnodeproperty.h"
#include "internalnode_p.h"

namespace QmlDesigner {
namespace Internal {

InternalNodeProperty::InternalNodeProperty(const PropertyName &name,
                                           const InternalNode::Pointer &propertyOwner)
    : InternalNodeAbstractProperty(name, propertyOwner, PropertyType::Node)
{
}

bool InternalNodeProperty::isEmpty() const
{
    return !m_node;
}

int InternalNodeProperty::count() const
{
    if (isEmpty())
        return 0;

    return 1;
}

int InternalNodeProperty::indexOf(const InternalNode::Pointer &node) const
{
    if (node && node == m_node)
        return 0;

    return -1;
}

bool InternalNodeProperty::isValid() const
{
    return InternalProperty::isValid() && isNodeProperty();
}

void InternalNodeProperty::remove([[maybe_unused]] const InternalNode::Pointer &node)
{
    Q_ASSERT(m_node == node);
    m_node.reset();
}

void InternalNodeProperty::add(const InternalNode::Pointer &node)
{
    Q_ASSERT(node);
    Q_ASSERT(node->parentProperty());
    m_node = node;
}

QList<InternalNode::Pointer> InternalNodeProperty::allSubNodes() const
{
    QList<InternalNode::Pointer> nodes;
    nodes.reserve(1024);

    addSubNodes(nodes);

    return nodes;
}

void InternalNodeProperty::addSubNodes(QList<InternalNodePointer> &container) const
{
    container.push_back(m_node);
    m_node->addSubNodes(container);
}

} // namespace Internal
} // namespace QmlDesigner
