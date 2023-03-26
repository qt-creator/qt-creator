// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "ditem.h"

#include "dvisitor.h"
#include "dconstvisitor.h"

namespace qmt {

DItem::DItem()
{
}

DItem::~DItem()
{
}

void DItem::setVariety(const QString &variety)
{
    m_variety = variety;
}

void DItem::setShape(const QString &shape)
{
    m_shape = shape;
}

void DItem::setShapeEditable(bool shapeEditable)
{
    m_isShapeEditable = shapeEditable;
}

void DItem::accept(DVisitor *visitor)
{
    visitor->visitDItem(this);
}

void DItem::accept(DConstVisitor *visitor) const
{
    visitor->visitDItem(this);
}

} // namespace qmt
