// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "dpackage.h"

#include "dvisitor.h"
#include "dconstvisitor.h"

namespace qmt {

DPackage::DPackage()
{
}

DPackage::~DPackage()
{
}

void DPackage::accept(DVisitor *visitor)
{
    visitor->visitDPackage(this);
}

void DPackage::accept(DConstVisitor *visitor) const
{
    visitor->visitDPackage(this);
}

} // namespace qmt
