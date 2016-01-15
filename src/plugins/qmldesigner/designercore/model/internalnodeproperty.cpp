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
    InternalNodeProperty *newPointer = new InternalNodeProperty(name, propertyOwner);
    InternalNodeProperty::Pointer smartPointer(newPointer);

    newPointer->setInternalWeakPointer(smartPointer);

    return smartPointer;
}

bool InternalNodeProperty::isEmpty() const
{
    return m_node.isNull();
}

int InternalNodeProperty::count() const
{
    if (isEmpty())
        return 0;

    return 1;
}

int InternalNodeProperty::indexOf(const InternalNode::Pointer &node) const
{
    if (!node.isNull() && node == m_node)
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

void InternalNodeProperty::remove(const InternalNode::Pointer &node)
{
    Q_UNUSED(node)
    Q_ASSERT(m_node == node);
    m_node.clear();

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
        nodeList.append(node()->allSubNodes());
        nodeList.append(node());
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
