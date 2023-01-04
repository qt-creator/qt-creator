// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "dinheritance.h"

#include "dclass.h"
#include "dvisitor.h"
#include "dconstvisitor.h"

namespace qmt {

DInheritance::DInheritance()
    : DRelation()
{
}

DInheritance::~DInheritance()
{
}

Uid DInheritance::derived() const
{
    return endAUid();
}

void DInheritance::setDerived(const Uid &derived)
{
    setEndAUid(derived);
}

Uid DInheritance::base() const
{
    return endBUid();
}

void DInheritance::setBase(const Uid &base)
{
    setEndBUid(base);
}

void DInheritance::accept(DVisitor *visitor)
{
    visitor->visitDInheritance(this);
}

void DInheritance::accept(DConstVisitor *visitor) const
{
    visitor->visitDInheritance(this);
}

} // namespace qmt
