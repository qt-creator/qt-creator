/****************************************************************************
**
** Copyright (C) 2017 Jochen Becher
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

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
