// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "dassociation.h"

#include "dvisitor.h"
#include "dconstvisitor.h"

namespace qmt {

DAssociationEnd::DAssociationEnd()
{
}

DAssociationEnd::~DAssociationEnd()
{
}

void DAssociationEnd::setName(const QString &name)
{
    m_name = name;
}

void DAssociationEnd::setCardinality(const QString &cardinality)
{
    m_cardinality = cardinality;
}

void DAssociationEnd::setNavigable(bool navigable)
{
    m_isNavigable = navigable;
}

void DAssociationEnd::setKind(MAssociationEnd::Kind kind)
{
    m_kind = kind;
}

bool operator==(const DAssociationEnd &lhs, const DAssociationEnd &rhs)
{
    if (&lhs == &rhs)
        return true;
    return lhs.name() == rhs.name()
            && lhs.cardinality() == rhs.cardinality()
            && lhs.kind() == rhs.kind()
            && lhs.isNavigable() == rhs.isNavigable();
}

bool operator!=(const DAssociationEnd &lhs, const DAssociationEnd &rhs)
{
    return !operator==(lhs, rhs);
}

DAssociation::DAssociation()
    : m_associationClassUid(Uid::invalidUid())
{
}

DAssociation::~DAssociation()
{
}

void DAssociation::setEndA(const DAssociationEnd &endA)
{
    m_endA = endA;
}

void DAssociation::setEndB(const DAssociationEnd &endB)
{
    m_endB = endB;
}

void DAssociation::setAssociationClassUid(const Uid &uid)
{
    m_associationClassUid = uid;
}

void DAssociation::accept(DVisitor *visitor)
{
    visitor->visitDAssociation(this);
}

void DAssociation::accept(DConstVisitor *visitor) const
{
    visitor->visitDAssociation(this);
}

} // namespace qmt
