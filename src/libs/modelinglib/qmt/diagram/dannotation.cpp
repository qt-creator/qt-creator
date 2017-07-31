/****************************************************************************
**
** Copyright (C) 2016 Jochen Becher
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

#include "dannotation.h"

#include "dvisitor.h"
#include "dconstvisitor.h"

namespace qmt {

DAnnotation::DAnnotation()
    : DElement()
{
}

DAnnotation::DAnnotation(const DAnnotation &rhs)
    : DElement(rhs),
      m_text(rhs.m_text),
      m_pos(rhs.m_pos),
      m_rect(rhs.m_rect),
      m_visualRole(rhs.m_visualRole),
      m_isAutoSized(rhs.m_isAutoSized)
{
}

DAnnotation::~DAnnotation()
{
}

DAnnotation &DAnnotation::operator=(const DAnnotation &rhs)
{
    if (this != &rhs) {
        DElement::operator=(rhs);
        m_text = rhs.m_text;
        m_pos = rhs.m_pos;
        m_rect = rhs.m_rect;
        m_visualRole = rhs.m_visualRole;
        m_isAutoSized = rhs.m_isAutoSized;
    }
    return *this;
}

void DAnnotation::setText(const QString &text)
{
    m_text = text;
}

void DAnnotation::setPos(const QPointF &pos)
{
    m_pos = pos;
}

void DAnnotation::setRect(const QRectF &rect)
{
    m_rect = rect;
}

void DAnnotation::setVisualRole(DAnnotation::VisualRole visualRole)
{
    m_visualRole = visualRole;
}

void DAnnotation::setAutoSized(bool autoSized)
{
    m_isAutoSized = autoSized;
}

void DAnnotation::accept(DVisitor *visitor)
{
    visitor->visitDAnnotation(this);
}

void DAnnotation::accept(DConstVisitor *visitor) const
{
    visitor->visitDAnnotation(this);
}

} // namespace qmt
