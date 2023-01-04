// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "mitem.h"

#include "mvisitor.h"
#include "mconstvisitor.h"

namespace qmt {

MItem::MItem()
{
}

MItem::~MItem()
{
}

void MItem::setVariety(const QString &variety)
{
    m_variety = variety;
}

void MItem::setVarietyEditable(bool varietyEditable)
{
    m_isVarietyEditable = varietyEditable;
}

void MItem::setShapeEditable(bool shapeEditable)
{
    m_isShapeEditable = shapeEditable;
}

void MItem::accept(MVisitor *visitor)
{
    visitor->visitMItem(this);
}

void MItem::accept(MConstVisitor *visitor) const
{
    visitor->visitMItem(this);
}

} // namespace qmt
