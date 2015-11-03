/***************************************************************************
**
** Copyright (C) 2015 Jochen Becher
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "stereotypeicon.h"

namespace qmt {

StereotypeIcon::StereotypeIcon()
    : m_width(100.0),
      m_height(100.0),
      m_minWidth(-1),
      m_minHeight(-1),
      m_sizeLock(LOCK_NONE),
      m_display(DISPLAY_SMART),
      m_textAlignment(TEXTALIGN_BELOW)
{
}

StereotypeIcon::~StereotypeIcon()
{
}

void StereotypeIcon::setId(const QString &id)
{
    m_id = id;
}

QString StereotypeIcon::getTitle() const
{
    if (m_title.isEmpty()) {
        return m_id;
    }
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

void StereotypeIcon::setMinWidth(qreal min_width)
{
    m_minWidth = min_width;
}

void StereotypeIcon::setMinHeight(qreal min_height)
{
    m_minHeight = min_height;
}

void StereotypeIcon::setSizeLock(StereotypeIcon::SizeLock size_lock)
{
    m_sizeLock = size_lock;
}

void StereotypeIcon::setDisplay(StereotypeIcon::Display display)
{
    m_display = display;
}

void StereotypeIcon::setTextAlignment(StereotypeIcon::TextAlignment text_alignment)
{
    m_textAlignment = text_alignment;
}

void StereotypeIcon::setBaseColor(const QColor &base_color)
{
    m_baseColor = base_color;
}

void StereotypeIcon::setIconShape(const IconShape &icon_shape)
{
    m_iconShape = icon_shape;
}

} // namespace qmt
