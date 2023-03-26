// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
