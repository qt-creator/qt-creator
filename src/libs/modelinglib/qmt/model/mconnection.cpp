/****************************************************************************
**
** Copyright (C) 2016 Jochen Becher
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

#include "mconnection.h"

#include "mvisitor.h"
#include "mconstvisitor.h"

namespace qmt {

MConnectionEnd::MConnectionEnd()
{
}

MConnectionEnd::MConnectionEnd(const MConnectionEnd &rhs)
    : m_name(rhs.name()),
      m_cardinality(rhs.m_cardinality),
      m_isNavigable(rhs.m_isNavigable)
{
}

MConnectionEnd::~MConnectionEnd()
{
}

MConnectionEnd &MConnectionEnd::operator=(const MConnectionEnd &rhs)
{
    if (this != &rhs) {
        m_name = rhs.m_name;
        m_cardinality = rhs.m_cardinality;
        m_isNavigable = rhs.m_isNavigable;
    }
    return *this;
}

void MConnectionEnd::setName(const QString &name)
{
    m_name = name;
}

void MConnectionEnd::setCardinality(const QString &cardinality)
{
    m_cardinality = cardinality;
}

void MConnectionEnd::setNavigable(bool navigable)
{
    m_isNavigable = navigable;
}

bool operator==(const MConnectionEnd &lhs, const MConnectionEnd &rhs)
{
    return lhs.name() == rhs.name()
            && lhs.cardinality() == rhs.cardinality()
            && lhs.isNavigable() == rhs.isNavigable();
}

MConnection::MConnection()
    : MRelation()
{
}

MConnection::~MConnection()
{
}

void MConnection::setCustomRelationId(const QString &customRelationId)
{
    m_customRelationId = customRelationId;
}

void MConnection::setEndA(const MConnectionEnd &end)
{
    m_endA = end;
}

void MConnection::setEndB(const MConnectionEnd &end)
{
    m_endB = end;
}

void MConnection::accept(MVisitor *visitor)
{
    visitor->visitMConnection(this);
}

void MConnection::accept(MConstVisitor *visitor) const
{
    visitor->visitMConnection(this);
}

} // namespace qmt
