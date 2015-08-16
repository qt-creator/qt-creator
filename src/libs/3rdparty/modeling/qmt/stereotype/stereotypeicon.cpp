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
    : _width(100.0),
      _height(100.0),
      _min_width(-1),
      _min_height(-1),
      _size_lock(LOCK_NONE),
      _display(DISPLAY_SMART),
      _text_alignment(TEXTALIGN_BELOW)
{
}

StereotypeIcon::~StereotypeIcon()
{
}

void StereotypeIcon::setId(const QString &id)
{
    _id = id;
}

QString StereotypeIcon::getTitle() const
{
    if (_title.isEmpty()) {
        return _id;
    }
    return _title;
}

void StereotypeIcon::setTitle(const QString &title)
{
    _title = title;
}

void StereotypeIcon::setElements(const QSet<Element> &elements)
{
    _elements = elements;
}

void StereotypeIcon::setStereotypes(const QSet<QString> &stereotypes)
{
    _stereotypes = stereotypes;
}

void StereotypeIcon::setWidth(qreal width)
{
    _width = width;
}

void StereotypeIcon::setHeight(qreal height)
{
    _height = height;
}

void StereotypeIcon::setMinWidth(qreal min_width)
{
    _min_width = min_width;
}

void StereotypeIcon::setMinHeight(qreal min_height)
{
    _min_height = min_height;
}

void StereotypeIcon::setSizeLock(StereotypeIcon::SizeLock size_lock)
{
    _size_lock = size_lock;
}

void StereotypeIcon::setDisplay(StereotypeIcon::Display display)
{
    _display = display;
}

void StereotypeIcon::setTextAlignment(StereotypeIcon::TextAlignment text_alignment)
{
    _text_alignment = text_alignment;
}

void StereotypeIcon::setBaseColor(const QColor &base_color)
{
    _base_color = base_color;
}

void StereotypeIcon::setIconShape(const IconShape &icon_shape)
{
    _icon_shape = icon_shape;
}

} // namespace qmt
