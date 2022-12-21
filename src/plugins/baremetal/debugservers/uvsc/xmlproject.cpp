// Copyright (C) 2020 Denis Shienkov <denis.shienkov@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "xmlnodevisitor.h"
#include "xmlproject.h"

namespace BareMetal::Gen::Xml {

// Project

void Project::accept(INodeVisitor *visitor) const
{
    visitor->visitProjectStart(this);

    for (const auto &child : children())
        child->accept(visitor);

    visitor->visitProjectEnd(this);
}

// ProjectOptions

void ProjectOptions::accept(INodeVisitor *visitor) const
{
    visitor->visitProjectOptionsStart(this);

    for (const auto &child : children())
        child->accept(visitor);

    visitor->visitProjectOptionsEnd(this);
}

} // BareMetal::Gen::Xml
