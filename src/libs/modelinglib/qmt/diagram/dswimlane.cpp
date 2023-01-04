// Copyright (C) 2017 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "dswimlane.h"

#include "dvisitor.h"
#include "dconstvisitor.h"

namespace qmt {

DSwimlane::DSwimlane()
    : DElement()
{
}

DSwimlane::DSwimlane(const DSwimlane &rhs)
    : DElement(rhs),
      m_text(rhs.m_text),
      m_horizontal(rhs.m_horizontal),
      m_pos(rhs.m_pos)
{
}

DSwimlane::~DSwimlane()
{
}

DSwimlane &DSwimlane::operator=(const DSwimlane &rhs)
{
    if (this != &rhs) {
        DElement::operator=(rhs);
        m_text = rhs.m_text;
        m_horizontal = rhs.m_horizontal;
        m_pos = rhs.m_pos;
    }
    return *this;
}

void DSwimlane::setText(const QString &text)
{
    m_text = text;
}

void DSwimlane::setHorizontal(bool horizontal)
{
    m_horizontal = horizontal;
}

void DSwimlane::setPos(qreal pos)
{
    m_pos = pos;
}

void DSwimlane::accept(DVisitor *visitor)
{
    visitor->visitDSwimlane(this);
}

void DSwimlane::accept(DConstVisitor *visitor) const
{
    visitor->visitDSwimlane(this);
}

} // namespoace qmt
