/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "tipcontents.h"

#include <QtCore/QtGlobal>

using namespace TextEditor;

TipContent::TipContent()
{}

TipContent::~TipContent()
{}

ColorContent::ColorContent(const QColor &color) : m_color(color)
{}

ColorContent::~ColorContent()
{}

TipContent *ColorContent::clone() const
{
    return new ColorContent(*this);
}

int ColorContent::typeId() const
{
    return COLOR_CONTENT_ID;
}

bool ColorContent::isValid() const
{
    return m_color.isValid();
}

int ColorContent::showTime() const
{
    return 4000;
}

bool ColorContent::equals(const TipContent &tipContent) const
{
    if (typeId() == tipContent.typeId()) {
        if (m_color == static_cast<const ColorContent &>(tipContent).m_color)
            return true;
    }
    return false;
}

const QColor &ColorContent::color() const
{
    return m_color;
}

TextContent::TextContent(const QString &text) : m_text(text)
{}

TextContent::~TextContent()
{}

TipContent *TextContent::clone() const
{
    return new TextContent(*this);
}

int TextContent::typeId() const
{
    return TEXT_CONTENT_ID;
}

bool TextContent::isValid() const
{
    return !m_text.isEmpty();
}

int TextContent::showTime() const
{
    return 10000 + 40 * qMax(0, m_text.length() - 100);
}

bool TextContent::equals(const TipContent &tipContent) const
{
    if (typeId() == tipContent.typeId()) {
        if (m_text == static_cast<const TextContent &>(tipContent).m_text)
            return true;
    }
    return false;
}

const QString &TextContent::text() const
{
    return m_text;
}
