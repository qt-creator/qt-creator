// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "dboundary.h"

#include "dvisitor.h"
#include "dconstvisitor.h"

namespace qmt {

DBoundary::DBoundary()
    : DElement()
{
}

DBoundary::DBoundary(const DBoundary &rhs)
    : DElement(rhs),
      m_text(rhs.m_text),
      m_pos(rhs.m_pos),
      m_rect(rhs.m_rect)
{
}

DBoundary::~DBoundary()
{
}

DBoundary &DBoundary::operator=(const DBoundary &rhs)
{
    if (this != &rhs) {
        DElement::operator=(rhs);
        m_text = rhs.m_text;
        m_pos = rhs.m_pos;
        m_rect = rhs.m_rect;
    }
    return *this;
}

void DBoundary::setText(const QString &text)
{
    m_text = text;
}

void DBoundary::setPos(const QPointF &pos)
{
    m_pos = pos;
}

void DBoundary::setRect(const QRectF &rect)
{
    m_rect = rect;
}

void DBoundary::accept(DVisitor *visitor)
{
    visitor->visitDBoundary(this);
}

void DBoundary::accept(DConstVisitor *visitor) const
{
    visitor->visitDBoundary(this);
}

} // namespace qmt
