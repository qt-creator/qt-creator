// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "dcomponent.h"

#include "dvisitor.h"
#include "dconstvisitor.h"

namespace qmt {

DComponent::DComponent()
{
}

DComponent::~DComponent()
{
}

void DComponent::setPlainShape(bool planeShape)
{
    m_isPlainShape = planeShape;
}

void DComponent::accept(DVisitor *visitor)
{
    visitor->visitDComponent(this);
}

void DComponent::accept(DConstVisitor *visitor) const
{
    visitor->visitDComponent(this);
}

} // namespace qmt
