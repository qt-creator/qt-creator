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

using namespace GenericEditor;
using namespace Internal;

const QLatin1String ItemData::kDsNormal("dsNormal");
const QLatin1String ItemData::kDsKeyword("dsKeyword");
const QLatin1String ItemData::kDsDataType("dsDataType");
const QLatin1String ItemData::kDsDecVal("dsDecVal");
const QLatin1String ItemData::kDsBaseN("dsBaseN");
const QLatin1String ItemData::kDsFloat("dsFloat");
const QLatin1String ItemData::kDsChar("dsChar");
const QLatin1String ItemData::kDsString("dsString");
const QLatin1String ItemData::kDsComment("dsComment");
const QLatin1String ItemData::kDsOthers("dsOthers");
const QLatin1String ItemData::kDsAlert("dsAlert");
const QLatin1String ItemData::kDsFunction("dsFunction");
const QLatin1String ItemData::kDsRegionMarker("dsRegionMarker");
const QLatin1String ItemData::kDsError("dsError");

ItemData::ItemData() :
    m_italicSpecified(false),
    m_boldSpecified(false),
    m_underlinedSpecified(false),
    m_strikedOutSpecified(false)
{}

void ItemData::setStyle(const QString &style)
{ m_style = style; }

const QString &ItemData::style() const
{ return m_style; }

void ItemData::setColor(const QString &color)
{ m_color.setNamedColor(color); }

const QColor &ItemData::color() const
{ return m_color; }

void ItemData::setSelectionColor(const QString &color)
{ m_selectionColor.setNamedColor(color); }

const QColor &ItemData::selectionColor() const
{ return m_selectionColor; }

void ItemData::setItalic(const QString &italic)
{
    if (!italic.isEmpty()) {
        m_italic = toBool(italic);
        m_italicSpecified = true;
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
    }
}

bool ItemData::isStrikedOut() const
{ return m_strikedOut; }

bool ItemData::isStrikedOutSpecified() const
{ return m_strikedOutSpecified; }
