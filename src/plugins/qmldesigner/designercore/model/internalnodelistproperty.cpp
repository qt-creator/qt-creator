// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "internalnodelistproperty.h"
#include "internalnode_p.h"
#include <QList>

namespace QmlDesigner {
namespace Internal {

InternalNodeListProperty::InternalNodeListProperty(const PropertyName &name, const InternalNodePointer &propertyOwner)
        : InternalNodeAbstractProperty(name, propertyOwner)
{
}

InternalNodeListProperty::Pointer InternalNodeListProperty::create(const PropertyName &name, const InternalNodePointer &propertyOwner)
{
    auto newPointer(new InternalNodeListProperty(name, propertyOwner));
    InternalProperty::Pointer smartPointer(newPointer);

    newPointer->setInternalWeakPointer(smartPointer.toWeakRef());

    return smartPointer.staticCast<InternalNodeListProperty>();
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
    return m_nodeList.count();
}

int InternalNodeListProperty::indexOf(const InternalNode::Pointer &node) const
{
    if (!node)
        return -1;

    return m_nodeList.indexOf(node);
}

bool InternalNodeListProperty::isNodeListProperty() const
{
    return true;
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

QList<InternalNode::Pointer> InternalNodeListProperty::allSubNodes() const
{
    QList<InternalNode::Pointer> nodeList;
    for (const InternalNode::Pointer &childNode : std::as_const(m_nodeList)) {
        nodeList.append(childNode->allSubNodes());
        nodeList.append(childNode);
    }

    return nodeList;
}

QList<InternalNodePointer> InternalNodeListProperty::directSubNodes() const
{
    return nodeList();
}

} // namespace Internal
} // namespace QmlDesigner
