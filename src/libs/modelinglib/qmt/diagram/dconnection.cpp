// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "dconnection.h"

#include "dvisitor.h"
#include "dconstvisitor.h"

namespace qmt {

DConnectionEnd::DConnectionEnd()
{
}

DConnectionEnd::~DConnectionEnd()
{
}

void DConnectionEnd::setName(const QString &name)
{
    m_name = name;
}

void DConnectionEnd::setCardinality(const QString &cardinality)
{
    m_cardinality = cardinality;
}

void DConnectionEnd::setNavigable(bool navigable)
{
    m_isNavigable = navigable;
}

bool operator==(const DConnectionEnd &lhs, const DConnectionEnd &rhs)
{
    if (&lhs == &rhs)
        return true;
    return lhs.name() == rhs.name()
            && lhs.cardinality() == rhs.cardinality()
            && lhs.isNavigable() == rhs.isNavigable();
}

bool operator!=(const DConnectionEnd &lhs, const DConnectionEnd &rhs)
{
    return !operator==(lhs, rhs);
}

DConnection::DConnection()
{
}

DConnection::~DConnection()
{
}

void DConnection::setCustomRelationId(const QString &customRelationId)
{
    m_customRelationId = customRelationId;
}

void DConnection::setEndA(const DConnectionEnd &endA)
{
    m_endA = endA;
}

void DConnection::setEndB(const DConnectionEnd &endB)
{
    m_endB = endB;
}

void DConnection::accept(DVisitor *visitor)
{
    visitor->visitDConnection(this);
}

void DConnection::accept(DConstVisitor *visitor) const
{
    visitor->visitDConnection(this);
}

} // namespace qmt
