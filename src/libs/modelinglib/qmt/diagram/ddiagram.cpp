// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "ddiagram.h"

#include "dvisitor.h"
#include "dconstvisitor.h"

namespace qmt {

DDiagram::DDiagram()
{
}

DDiagram::~DDiagram()
{
}

void DDiagram::accept(DVisitor *visitor)
{
    visitor->visitDDiagram(this);
}

void DDiagram::accept(DConstVisitor *visitor) const
{
    visitor->visitDDiagram(this);
}

} // namespace qmt
