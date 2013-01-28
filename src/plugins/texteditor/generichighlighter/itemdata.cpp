/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "itemdata.h"
#include "reuse.h"

using namespace TextEditor;
using namespace Internal;

ItemData::ItemData() :
    m_italic(false),
    m_italicSpecified(false),
    m_bold(false),
    m_boldSpecified(false),
    m_underlined(false),
    m_underlinedSpecified(false),
    m_strikedOut(false),
    m_strikeOutSpecified(false),
    m_isCustomized(false)
{}

void ItemData::setStyle(const QString &style)
{ m_style = style; }

const QString &ItemData::style() const
{ return m_style; }

void ItemData::setColor(const QString &color)
{
    if (!color.isEmpty()) {
        m_color.setNamedColor(color);
        m_isCustomized = true;
    }
}

const QColor &ItemData::color() const
{ return m_color; }

void ItemData::setSelectionColor(const QString &color)
{
    if (!color.isEmpty()) {
        m_selectionColor.setNamedColor(color);
        m_isCustomized = true;
    }
}

const QColor &ItemData::selectionColor() const
{ return m_selectionColor; }

void ItemData::setItalic(const QString &italic)
{
    if (!italic.isEmpty()) {
        m_italic = toBool(italic);
        m_italicSpecified = true;
        m_isCustomized = true;
    }
}

bool ItemData::isItalic() const
{ return m_italic; }

bool ItemData::isItalicSpecified() const
{ return m_italicSpecified; }

void ItemData::setBold(const QString &bold)
{
    if (!bold.isEmpty()) {
        m_bold = toBool(bold);
        m_boldSpecified = true;
        m_isCustomized = true;
    }
}

bool ItemData::isBold() const
{ return m_bold; }

bool ItemData::isBoldSpecified() const
{ return m_boldSpecified; }

void ItemData::setUnderlined(const QString &underlined)
{
    if (!underlined.isEmpty()) {
        m_underlined = toBool(underlined);
        m_underlinedSpecified = true;
        m_isCustomized = true;
    }
}

bool ItemData::isUnderlined() const
{ return m_underlined; }

bool ItemData::isUnderlinedSpecified() const
{ return m_underlinedSpecified; }

void ItemData::setStrikeOut(const QString &strike)
{
    if (!strike.isEmpty()) {
        m_strikedOut = toBool(strike);
        m_strikeOutSpecified = true;
        m_isCustomized = true;
    }
}

bool ItemData::isStrikeOut() const
{ return m_strikedOut; }

bool ItemData::isStrikeOutSpecified() const
{ return m_strikeOutSpecified; }

bool ItemData::isCustomized() const
{ return m_isCustomized; }
