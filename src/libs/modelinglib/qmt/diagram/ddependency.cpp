// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "ddependency.h"

#include "dvisitor.h"
#include "dconstvisitor.h"

namespace qmt {

DDependency::DDependency()
{
}

DDependency::~DDependency()
{
}

Uid DDependency::source() const
{
    return m_direction == MDependency::BToA ? endBUid() : endAUid();
}

void DDependency::setSource(const Uid &source)
{
    m_direction == MDependency::BToA ? setEndBUid(source) : setEndAUid(source);
}

Uid DDependency::target() const
{
    return m_direction == MDependency::BToA ? endAUid() : endBUid();
}

void DDependency::setTarget(const Uid &target)
{
    return m_direction == MDependency::BToA ? setEndAUid(target) : setEndBUid(target);
}

void DDependency::setDirection(MDependency::Direction direction)
{
    m_direction = direction;
}

void DDependency::accept(DVisitor *visitor)
{
    visitor->visitDDependency(this);
}

void DDependency::accept(DConstVisitor *visitor) const
{
    visitor->visitDDependency(this);
}

} // namespace qmt
