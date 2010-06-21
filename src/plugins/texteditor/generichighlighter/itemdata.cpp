/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

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
    m_strikedOutSpecified(false),
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

void ItemData::setStrikedOut(const QString &striked)
{
    if (!striked.isEmpty()) {
        m_strikedOut = toBool(striked);
        m_strikedOutSpecified = true;
        m_isCustomized = true;
    }
}

bool ItemData::isStrikedOut() const
{ return m_strikedOut; }

bool ItemData::isStrikedOutSpecified() const
{ return m_strikedOutSpecified; }

bool ItemData::isCustomized() const
{ return m_isCustomized; }
