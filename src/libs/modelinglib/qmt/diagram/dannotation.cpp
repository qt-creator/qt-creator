// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
