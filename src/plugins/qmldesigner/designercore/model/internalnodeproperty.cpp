// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "internalnodeproperty.h"
#include "internalnode_p.h"

namespace QmlDesigner {
namespace Internal {

InternalNodeProperty::InternalNodeProperty(const PropertyName &name, const InternalNode::Pointer &propertyOwner)
    : InternalNodeAbstractProperty(name, propertyOwner)
{
}

InternalNodeProperty::Pointer InternalNodeProperty::create(const PropertyName &name, const InternalNode::Pointer &propertyOwner)
{
    auto newPointer = new InternalNodeProperty(name, propertyOwner);
    InternalNodeProperty::Pointer smartPointer(newPointer);

    newPointer->setInternalWeakPointer(smartPointer);

    return smartPointer;
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

bool InternalNodeProperty::isNodeProperty() const
{
    return true;
}

InternalNode::Pointer InternalNodeProperty::node() const
{
    return m_node;
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
    QList<InternalNode::Pointer> nodeList;

    if (node()) {
        nodeList.append(node());
        nodeList.append(node()->allSubNodes());
    }

    return nodeList;
}

QList<InternalNodePointer> InternalNodeProperty::directSubNodes() const
{
    QList<InternalNode::Pointer> nodeList;

    if (node())
        nodeList.append(node());

    return nodeList;
}

} // namespace Internal
} // namespace QmlDesigner
