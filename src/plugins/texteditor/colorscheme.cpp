/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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
** contact the sales department at http://www.qtsoftware.com/contact.
**
**************************************************************************/

#include "colorscheme.h"

#include <QtCore/QFile>
#include <QtXml/QXmlStreamWriter>

using namespace TextEditor;

static const char *trueString = "true";
static const char *falseString = "false";

// Format

Format::Format() :
    m_foreground(Qt::black),
    m_background(Qt::white),
    m_bold(false),
    m_italic(false)
{
}

void Format::setForeground(const QColor &foreground)
{
    m_foreground = foreground;
}

void Format::setBackground(const QColor &background)
{
    m_background = background;
}

void Format::setBold(bool bold)
{
    m_bold = bold;
}

void Format::setItalic(bool italic)
{
    m_italic = italic;
}

static QString colorToString(const QColor &color)
{
    if (color.isValid())
        return color.name();
    return QLatin1String("invalid");
}

static QColor stringToColor(const QString &string)
{
    if (string == QLatin1String("invalid"))
        return QColor();
    return QColor(string);
}

bool Format::equals(const Format &f) const
{
    return m_foreground ==  f.m_foreground && m_background == f.m_background &&
           m_bold == f.m_bold && m_italic == f.m_italic;
}

QString Format::toString() const
{
    const QChar delimiter = QLatin1Char(';');
    QString s =  colorToString(m_foreground);
    s += delimiter;
    s += colorToString(m_background);
    s += delimiter;
    s += m_bold   ? QLatin1String(trueString) : QLatin1String(falseString);
    s += delimiter;
    s += m_italic ? QLatin1String(trueString) : QLatin1String(falseString);
    return s;
}

bool Format::fromString(const QString &str)
{
    *this = Format();

    const QStringList lst = str.split(QLatin1Char(';'));
    if (lst.count() != 4)
        return false;

    m_foreground = stringToColor(lst.at(0));
    m_background = stringToColor(lst.at(1));
    m_bold = lst.at(2) == QLatin1String(trueString);
    m_italic = lst.at(3) == QLatin1String(trueString);
    return true;
}


// ColorScheme

ColorScheme::ColorScheme()
{
}

bool ColorScheme::contains(const QString &category) const
{
    return m_formats.contains(category);
}

Format &ColorScheme::formatFor(const QString &category)
{
    return m_formats[category];
}

Format ColorScheme::formatFor(const QString &category) const
{
    return m_formats.value(category);
}

void ColorScheme::setFormatFor(const QString &category, const Format &format)
{
    m_formats[category] = format;
}

void ColorScheme::clear()
{
    m_formats.clear();
}

bool ColorScheme::save(const QString &fileName)
{
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly))
        return false;

    QXmlStreamWriter w(&file);
    w.setAutoFormatting(true);
    w.setAutoFormattingIndent(2);

    w.writeStartDocument();
    w.writeStartElement(QLatin1String("style-scheme"));
    w.writeAttribute(QLatin1String("version"), QLatin1String("1.0"));

    QMapIterator<QString, Format> i(m_formats);
    while (i.hasNext()) {
        const Format &format = i.next().value();
        w.writeStartElement(QLatin1String("style"));
        w.writeAttribute(QLatin1String("name"), i.key());
        w.writeAttribute(QLatin1String("foreground"), format.foreground().name().toLower());
        if (format.background().isValid())
            w.writeAttribute(QLatin1String("background"), format.background().name().toLower());
        if (format.bold())
            w.writeAttribute(QLatin1String("bold"), QLatin1String(trueString));
        if (format.italic())
            w.writeAttribute(QLatin1String("italic"), QLatin1String(trueString));
        w.writeEndElement();
    }

    w.writeEndElement();
    w.writeEndDocument();

    return true;
}

bool ColorScheme::load(const QString &fileName)
{
    Q_UNUSED(fileName)
    return false;
}
