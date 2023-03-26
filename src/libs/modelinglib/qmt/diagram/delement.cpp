// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "delement.h"

#include "dconstvisitor.h"
#include "dvisitor.h"

namespace qmt {

DElement::DElement()
{
}

DElement::~DElement()
{
}

void DElement::setUid(const Uid &uid)
{
    m_uid = uid;
}

void DElement::renewUid()
{
    m_uid.renew();
}

void DElement::accept(DVisitor *visitor)
{
    visitor->visitDElement(this);
}

void DElement::accept(DConstVisitor *visitor) const
{
    visitor->visitDElement(this);
}

} // namespace qmt
