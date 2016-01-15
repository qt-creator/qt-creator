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
