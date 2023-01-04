// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "minheritance.h"

#include "mclass.h"
#include "mvisitor.h"
#include "mconstvisitor.h"

namespace qmt {

MInheritance::MInheritance()
    : MRelation()
{
}

MInheritance::MInheritance(const MInheritance &rhs)
    : MRelation(rhs)
{
}

MInheritance::~MInheritance()
{
}

MInheritance MInheritance::operator =(const MInheritance &rhs)
{
    if (this != &rhs)
        MRelation::operator=(rhs);
    return *this;
}

Uid MInheritance::derived() const
{
    return endAUid();
}

void MInheritance::setDerived(const Uid &derived)
{
    setEndAUid(derived);
}

Uid MInheritance::base() const
{
    return endBUid();
}

void MInheritance::setBase(const Uid &base)
{
    setEndBUid(base);
}

void MInheritance::accept(MVisitor *visitor)
{
    visitor->visitMInheritance(this);
}

void MInheritance::accept(MConstVisitor *visitor) const
{
    visitor->visitMInheritance(this);
}

} // namespace qmt
