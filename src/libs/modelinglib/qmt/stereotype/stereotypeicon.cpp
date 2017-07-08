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

#include "stereotypeicon.h"

namespace qmt {

void StereotypeIcon::setId(const QString &id)
{
    m_id = id;
}

QString StereotypeIcon::title() const
{
    if (m_title.isEmpty())
        return m_id;
    return m_title;
}

void StereotypeIcon::setTitle(const QString &title)
{
    m_title = title;
}

void StereotypeIcon::setElements(const QSet<Element> &elements)
{
    m_elements = elements;
}

void StereotypeIcon::setStereotypes(const QSet<QString> &stereotypes)
{
    m_stereotypes = stereotypes;
}

void StereotypeIcon::setWidth(qreal width)
{
    m_width = width;
}

void StereotypeIcon::setHeight(qreal height)
{
    m_height = height;
}

void StereotypeIcon::setMinWidth(qreal minWidth)
{
    m_minWidth = minWidth;
}

void StereotypeIcon::setMinHeight(qreal minHeight)
{
    m_minHeight = minHeight;
}

void StereotypeIcon::setSizeLock(StereotypeIcon::SizeLock sizeLock)
{
    m_sizeLock = sizeLock;
}

void StereotypeIcon::setDisplay(StereotypeIcon::Display display)
{
    m_display = display;
}

void StereotypeIcon::setTextAlignment(StereotypeIcon::TextAlignment textAlignment)
{
    m_textAlignment = textAlignment;
}

void StereotypeIcon::setBaseColor(const QColor &baseColor)
{
    m_baseColor = baseColor;
}

void StereotypeIcon::setIconShape(const IconShape &iconShape)
{
    m_iconShape = iconShape;
}

} // namespace qmt
