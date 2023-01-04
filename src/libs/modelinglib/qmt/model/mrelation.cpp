// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "mrelation.h"

#include "mvisitor.h"
#include "mconstvisitor.h"

namespace qmt {

MRelation::MRelation()
    : MElement(),
      m_endAUid(Uid::invalidUid()),
      m_endBUid(Uid::invalidUid())
{
}

MRelation::MRelation(const MRelation &rhs)
    : MElement(rhs),
      m_name(rhs.m_name),
      m_endAUid(rhs.m_endAUid),
      m_endBUid(rhs.m_endBUid)
{
}

MRelation::~MRelation()
{
}

MRelation &MRelation::operator =(const MRelation &rhs)
{
    if (this != &rhs) {
        MElement::operator=(rhs);
        m_name = rhs.m_name;
        m_endAUid = rhs.m_endAUid;
        m_endBUid = rhs.m_endBUid;
    }
    return *this;
}

void MRelation::setName(const QString &name)
{
    m_name = name;
}

void MRelation::setEndAUid(const Uid &uid)
{
    m_endAUid = uid;
}

void MRelation::setEndBUid(const Uid &uid)
{
    m_endBUid = uid;
}

void MRelation::accept(MVisitor *visitor)
{
    visitor->visitMRelation(this);
}

void MRelation::accept(MConstVisitor *visitor) const
{
    visitor->visitMRelation(this);
}

} // namespace qmt
