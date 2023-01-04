// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "massociation.h"

#include "mvisitor.h"
#include "mconstvisitor.h"

namespace qmt {

MAssociationEnd::MAssociationEnd()
{
}

MAssociationEnd::MAssociationEnd(const MAssociationEnd &rhs)
    : m_name(rhs.m_name),
      m_cardinality(rhs.m_cardinality),
      m_kind(rhs.m_kind),
      m_isNavigable(rhs.m_isNavigable)
{
}

MAssociationEnd::~MAssociationEnd()
{
}

MAssociationEnd &MAssociationEnd::operator =(const MAssociationEnd &rhs)
{
    if (this != &rhs) {
        m_name = rhs.m_name;
        m_cardinality = rhs.m_cardinality;
        m_kind = rhs.m_kind;
        m_isNavigable = rhs.m_isNavigable;
    }
    return *this;
}

void MAssociationEnd::setName(const QString &name)
{
    m_name = name;
}

void MAssociationEnd::setCardinality(const QString &cardinality)
{
    m_cardinality = cardinality;
}

void MAssociationEnd::setKind(MAssociationEnd::Kind kind)
{
    m_kind = kind;
}

void MAssociationEnd::setNavigable(bool navigable)
{
    m_isNavigable = navigable;
}

bool operator==(const MAssociationEnd &lhs, const MAssociationEnd &rhs)
{
    return lhs.name() == rhs.name()
            && lhs.cardinality() == rhs.cardinality()
            && lhs.kind() == rhs.kind()
            && lhs.isNavigable() == rhs.isNavigable();
}

MAssociation::MAssociation()
    : MRelation(),
      m_associationClassUid(Uid::invalidUid())
{
}

MAssociation::~MAssociation()
{
}

void MAssociation::setEndA(const MAssociationEnd &end)
{
    m_endA = end;
}

void MAssociation::setEndB(const MAssociationEnd &end)
{
    m_endB = end;
}

void MAssociation::setAssociationClassUid(const Uid &uid)
{
    m_associationClassUid = uid;
}

void MAssociation::accept(MVisitor *visitor)
{
    visitor->visitMAssociation(this);
}

void MAssociation::accept(MConstVisitor *visitor) const
{
    visitor->visitMAssociation(this);
}

} // namespace qmt
