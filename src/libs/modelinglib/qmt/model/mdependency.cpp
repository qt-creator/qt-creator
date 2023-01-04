// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "mdependency.h"

#include "mvisitor.h"
#include "mconstvisitor.h"

namespace qmt {

MDependency::MDependency()
{
}

MDependency::MDependency(const MDependency &rhs)
    : MRelation(rhs),
      m_direction(rhs.m_direction)
{
}

MDependency::~MDependency()
{
}

MDependency &MDependency::operator =(const MDependency &rhs)
{
    if (this != &rhs) {
        MRelation::operator=(rhs);
        m_direction = rhs.m_direction;
    }
    return *this;
}

void MDependency::setDirection(MDependency::Direction direction)
{
    m_direction = direction;
}

Uid MDependency::source() const
{
    return m_direction == BToA ? endBUid() : endAUid();
}

void MDependency::setSource(const Uid &source)
{
    if (m_direction == BToA)
        setEndBUid(source);
    else
        setEndAUid(source);
}

Uid MDependency::target() const
{
    return m_direction == BToA ? endAUid() : endBUid();
}

void MDependency::setTarget(const Uid &target)
{
    if (m_direction == BToA)
        setEndAUid(target);
    else
        setEndBUid(target);
}

void MDependency::accept(MVisitor *visitor)
{
    visitor->visitMDependency(this);
}

void MDependency::accept(MConstVisitor *visitor) const
{
    visitor->visitMDependency(this);
}

} // namespace qmt
