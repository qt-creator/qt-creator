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

namespace {
    static const QLatin1String kDsNormal("dsNormal");
    static const QLatin1String kDsKeyword("dsKeyword");
    static const QLatin1String kDsDataType("dsDataType");
    static const QLatin1String kDsDecVal("dsDecVal");
    static const QLatin1String kDsBaseN("dsBaseN");
    static const QLatin1String kDsFloat("dsFloat");
    static const QLatin1String kDsChar("dsChar");
    static const QLatin1String kDsString("dsString");
    static const QLatin1String kDsComment("dsComment");
    static const QLatin1String kDsOthers("dsOthers");
    static const QLatin1String kDsAlert("dsAlert");
    static const QLatin1String kDsFunction("dsFunction");
    static const QLatin1String kDsRegionMarker("dsRegionMarker");
    static const QLatin1String kDsError("dsError");
}

ItemData::ItemData()
{}

void ItemData::setStyle(const QString &style)
{ m_style = style; }

void ItemData::setColor(const QString &color)
{ m_color.setNamedColor(color); }

void ItemData::setSelectionColor(const QString &color)
{ m_selectionColor.setNamedColor(color); }

void ItemData::setItalic(const QString &italic)
{ m_font.setItalic(toBool(italic)); }

void ItemData::setBold(const QString &bold)
{ m_font.setBold(toBool(bold)); }

void ItemData::setUnderlined(const QString &underlined)
{ m_font.setUnderline(toBool(underlined)); }

void ItemData::setStrikedOut(const QString &striked)
{ m_font.setStrikeOut(toBool(striked)); }

void ItemData::configureFormat()
{
    //Todo: Overwrite defaults when true?
    m_format.setFont(m_font);

    if (m_style == kDsNormal) {
        m_format.setForeground(Qt::black);
    } else if (m_style == kDsKeyword) {
        m_format.setForeground(Qt::black);
        m_format.setFontWeight(QFont::Bold);
    } else if (m_style == kDsDataType) {
        m_format.setForeground(Qt::blue);
    } else if (m_style == kDsDecVal) {
        m_format.setForeground(Qt::darkYellow);
    } else if (m_style == kDsBaseN) {
        m_format.setForeground(Qt::darkYellow);
        m_format.setFontWeight(QFont::Bold);
    } else if (m_style == kDsFloat) {
        m_format.setForeground(Qt::darkYellow);
        m_format.setFontUnderline(true);
    } else if (m_style == kDsChar) {
        m_format.setForeground(Qt::magenta);
    } else if (m_style == kDsString) {
        m_format.setForeground(Qt::red);
    } else if (m_style == kDsComment) {
        m_format.setForeground(Qt::darkGray);
        m_format.setFontItalic(true);
        m_format.setFontWeight(QFont::Bold);
    } else if (m_style == kDsOthers) {
        m_format.setForeground(Qt::darkGreen);
    } else if (m_style == kDsAlert) {
        m_format.setForeground(Qt::darkRed);
        m_format.setFontWeight(QFont::Bold);
    } else if (m_style == kDsFunction) {
        m_format.setForeground(Qt::darkBlue);
        m_format.setFontWeight(QFont::Bold);
    } else if (m_style == kDsRegionMarker) {
        m_format.setForeground(Qt::yellow);
    } else if (m_style == kDsError) {
        m_format.setForeground(Qt::darkRed);
        m_format.setFontUnderline(true);
    }

    if (m_color.isValid())
        m_format.setForeground(QColor(m_color));
    //if (m_selectionColor.isValid())
        //m_format.setBackground(QColor(m_selectionColor));
}

const QTextCharFormat &ItemData::format() const
{ return m_format; }
