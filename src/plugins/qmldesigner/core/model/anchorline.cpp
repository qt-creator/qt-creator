/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
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
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "anchorline.h"
#include "model.h"
#include "internalnode_p.h"
#include "modelnode.h"

namespace QmlDesigner {

AnchorLine::AnchorLine()
    : m_anchorType(Invalid)
{
}

AnchorLine::AnchorLine(const Internal::InternalNodeStatePointer &internalNodeState,
                       const Internal::InternalNodePointer  &internalNode,
                       Model* model,
                       Type type)
    : m_internalNodeState(internalNodeState),
    m_internalNode(internalNode),
    m_model(model),
    m_anchorType(type)
{
}

AnchorLine::AnchorLine(const NodeState  &nodeState,
                       Type type)
    : m_internalNodeState(nodeState.internalNodeState()),
    m_internalNode(nodeState.internalNode()),
    m_model(nodeState.model()),
    m_anchorType(type)
{

}

AnchorLine::~AnchorLine()
{
}

AnchorLine::AnchorLine(const AnchorLine &other)
    : m_internalNodeState(other.m_internalNodeState),
    m_internalNode(other.m_internalNode),
    m_model(other.m_model),
    m_anchorType(other.m_anchorType)
{
}

AnchorLine &AnchorLine::operator =(const AnchorLine &other)
{
    m_internalNodeState = other.m_internalNodeState;
    m_internalNode = other.m_internalNode;
    m_model = other.m_model;
    m_anchorType = other.m_anchorType;

    return *this;
}

ModelNode AnchorLine::modelNode() const
{
    if (m_internalNode.isNull() || m_internalNodeState.isNull() || m_model.isNull()) {
        return ModelNode();
    }
    return ModelNode(m_internalNode, m_model.data());
}


AnchorLine::Type AnchorLine::type() const
{
    return m_anchorType;
}

bool AnchorLine::isValid() const
{
    return m_anchorType != Invalid &&
            !m_model.isNull() &&
            !m_internalNode.isNull() &&
            !m_internalNodeState.isNull() &&
            m_internalNode.toStrongRef()->isValid() &&
            m_internalNodeState.toStrongRef()->isValid();
}

QVariant AnchorLine::toVariant() const
{
    return QVariant::fromValue(*this);
}

Internal::InternalNodeStatePointer AnchorLine::internalNodeState() const
{
    if (m_internalNodeState.isNull())
        return Internal::InternalNodeStatePointer(new Internal::InternalNodeState);

    return m_internalNodeState.toStrongRef();
}

Internal::InternalNodePointer AnchorLine::internalNode() const
{
    if (m_internalNode.isNull())
        return Internal::InternalNodePointer(new Internal::InternalNode);

    return m_internalNode.toStrongRef();
}

} // namespace QmlDesigner
