/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "internalnodeproperty.h"
#include "internalnode_p.h"

namespace QmlDesigner {
namespace Internal {

InternalNodeProperty::InternalNodeProperty(const QString &name, const InternalNode::Pointer &propertyOwner)
    : InternalNodeAbstractProperty(name, propertyOwner)
{
}

InternalNodeProperty::Pointer InternalNodeProperty::create(const QString &name, const InternalNode::Pointer &propertyOwner)
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

QList<InternalNodePointer> InternalNodeProperty::allDirectSubNodes() const
{
    QList<InternalNode::Pointer> nodeList;

    if (node()) {
        nodeList.append(node());
    }

    return nodeList;
}

} // namespace Internal
} // namespace QmlDesigner
