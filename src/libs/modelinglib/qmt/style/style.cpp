// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "style.h"

namespace qmt {

Style::Style(Type type)
    : m_type(type)
{
}

Style::~Style()
{
}

void Style::setLinePen(const QPen &pen)
{
    m_linePen = pen;
}

void Style::setOuterLinePen(const QPen &pen)
{
    m_outerLinePen = pen;
}

void Style::setInnerLinePen(const QPen &pen)
{
    m_innerLinePen = pen;
}

void Style::setExtraLinePen(const QPen &pen)
{
    m_extraLinePen = pen;
}

void Style::setTextBrush(const QBrush &brush)
{
    m_textBrush = brush;
}

void Style::setFillBrush(const QBrush &brush)
{
    m_fillBrush = brush;
}

void Style::setExtraFillBrush(const QBrush &brush)
{
    m_extraFillBrush = brush;
}

void Style::setNormalFont(const QFont &font)
{
    m_normalFont = font;
}

void Style::setSmallFont(const QFont &font)
{
    m_smallFont = font;
}

void Style::setHeaderFont(const QFont &font)
{
    m_headerFont = font;
}

} // namespace qmt
