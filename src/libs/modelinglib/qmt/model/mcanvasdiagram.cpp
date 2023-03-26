// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "mcanvasdiagram.h"

#include "mvisitor.h"
#include "mconstvisitor.h"

namespace qmt {

MCanvasDiagram::MCanvasDiagram()
{
}

MCanvasDiagram::~MCanvasDiagram()
{
}

void MCanvasDiagram::accept(MVisitor *visitor)
{
    visitor->visitMCanvasDiagram(this);
}

void MCanvasDiagram::accept(MConstVisitor *visitor) const
{
    visitor->visitMCanvasDiagram(this);
}

} // namespace qmt
