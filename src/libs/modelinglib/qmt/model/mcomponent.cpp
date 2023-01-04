// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "mcomponent.h"

#include "mvisitor.h"
#include "mconstvisitor.h"

namespace qmt {

MComponent::MComponent()
    : MObject()
{
}

MComponent::~MComponent()
{
}

void MComponent::accept(MVisitor *visitor)
{
    visitor->visitMComponent(this);
}

void MComponent::accept(MConstVisitor *visitor) const
{
    visitor->visitMComponent(this);
}

} // namespace qmt
