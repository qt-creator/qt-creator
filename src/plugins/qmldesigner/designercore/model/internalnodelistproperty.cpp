/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "internalnodelistproperty.h"
#include "internalnode_p.h"
#include <QList>

namespace QmlDesigner {
namespace Internal {

InternalNodeListProperty::InternalNodeListProperty(const QString &name, const InternalNodePointer &propertyOwner)
        : InternalNodeAbstractProperty(name, propertyOwner)
{
}

InternalNodeListProperty::Pointer InternalNodeListProperty::create(const QString &name, const InternalNodePointer &propertyOwner)
{
    InternalNodeListProperty *newPointer(new InternalNodeListProperty(name, propertyOwner));
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
    if (node.isNull())
        return -1;

    return m_nodeList.indexOf(node);
}

InternalNode::Pointer InternalNodeListProperty::at(int index) const
{
    Q_ASSERT(index >=0 || index < m_nodeList.count());
    return InternalNode::Pointer(m_nodeList.at(index));
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
    foreach (const InternalNode::Pointer &childNode, m_nodeList) {
        nodeList.append(childNode->allSubNodes());
        nodeList.append(childNode);
    }

    return nodeList;
}

QList<InternalNodePointer> InternalNodeListProperty::allDirectSubNodes() const
{
    return nodeList();
}

} // namespace Internal
} // namespace QmlDesigner
