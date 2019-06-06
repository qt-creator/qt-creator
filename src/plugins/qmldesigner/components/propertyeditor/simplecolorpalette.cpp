/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

#include "simplecolorpalette.h"

#include "designersettings.h"

#include <QDebug>

namespace QmlDesigner {

PaletteColor::PaletteColor()
    : m_color(QColor())
    , m_colorCode(QColor().name())
    , m_isFavorite(false)
{}

PaletteColor::PaletteColor(const QString &colorCode)
    : m_color(colorCode)
    , m_colorCode(colorCode)
    , m_isFavorite(false)
{}

PaletteColor::PaletteColor(const QColor &color)
    : m_color(color)
    , m_colorCode(color.name(QColor::HexArgb))
    , m_isFavorite(false)
{}

QVariant PaletteColor::getProperty(Property id) const
{
    QVariant out;

    switch (id) {
    case objectNameRole:
        out.setValue(QString());
        break;
    case colorRole:
        out.setValue(color());
        break;
    case colorCodeRole:
        out.setValue(colorCode());
        break;
    case isFavoriteRole:
        out.setValue(isFavorite());
        break;
    default:
        qWarning() << "PaletteColor Property switch default case";
        break; //replace with assert before switch?
    }

    return out;
}

QColor PaletteColor::color() const
{
    return m_color;
}

void PaletteColor::setColor(const QColor &value)
{
    m_color = value;
    m_colorCode = m_color.name(QColor::HexArgb);
}

QString PaletteColor::colorCode() const
{
    return m_colorCode;
}

bool PaletteColor::isFavorite() const
{
    return m_isFavorite;
}

void PaletteColor::setFavorite(bool favorite)
{
    m_isFavorite = favorite;
}

bool PaletteColor::toggleFavorite()
{
    return m_isFavorite = !m_isFavorite;
}

bool PaletteColor::operator==(const PaletteColor &other) const
{
    return (m_color == other.m_color);
}

} // namespace QmlDesigner
