// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "mpackage.h"

#include "mvisitor.h"
#include "mconstvisitor.h"

namespace qmt {

MPackage::MPackage()
    : MObject()
{
}

MPackage::~MPackage()
{
}

void MPackage::accept(MVisitor *visitor)
{
    visitor->visitMPackage(this);
}

void MPackage::accept(MConstVisitor *visitor) const
{
    visitor->visitMPackage(this);
}

} // namespace qmt
