// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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

void StereotypeIcon::setHasName(bool hasName)
{
    m_hasName = hasName;
}

void StereotypeIcon::setName(const QString &name)
{
    m_name = name;
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

void StereotypeIcon::setOutlineShape(const IconShape &outlineShape)
{
    m_outlineShape = outlineShape;
}

} // namespace qmt
