/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
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

#include "colorscheme.h"

#include "texteditorconstants.h"

#include <utils/fileutils.h>

#include <QFile>
#include <QCoreApplication>
#include <QMetaEnum>
#include <QXmlStreamWriter>

using namespace TextEditor;

static const char trueString[] = "true";
static const char falseString[] = "false";

// Format


Format::Format(const QColor &foreground, const QColor &background) :
    m_foreground(foreground),
    m_background(background)
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

void Format::setUnderlineColor(const QColor &underlineColor)
{
    m_underlineColor = underlineColor;
}

QColor Format::underlineColor() const
{
    return m_underlineColor;
}

void Format::setUnderlineStyle(QTextCharFormat::UnderlineStyle underlineStyle)
{
    m_underlineStyle = underlineStyle;
}

QTextCharFormat::UnderlineStyle Format::underlineStyle() const
{
    return m_underlineStyle;
}

static QColor stringToColor(const QString &string)
{
    if (string == QLatin1String("invalid"))
        return QColor();
    return QColor(string);
}

static QTextCharFormat::UnderlineStyle stringToUnderlineStyle(const QString &string)
{
    if (string.isEmpty() || string == QStringLiteral("NoUnderline"))
        return QTextCharFormat::NoUnderline;
    else if (string == QStringLiteral("SingleUnderline"))
        return QTextCharFormat::SingleUnderline;
    else if (string == QStringLiteral("DashUnderline"))
        return QTextCharFormat::DashUnderline;
    else if (string == QStringLiteral("DotLine"))
        return QTextCharFormat::DotLine;
    else if (string == QStringLiteral("DashDotLine"))
        return QTextCharFormat::DashDotLine;
    else if (string == QStringLiteral("DashDotDotLine"))
        return QTextCharFormat::DashDotDotLine;
    else if (string == QStringLiteral("WaveUnderline"))
        return QTextCharFormat::WaveUnderline;

    return QTextCharFormat::NoUnderline;
}

static QString underlineStyleToString(QTextCharFormat::UnderlineStyle underlineStyle)
{
    switch (underlineStyle) {
        case QTextCharFormat::NoUnderline: return QStringLiteral("NoUnderline");
        case QTextCharFormat::SingleUnderline: return QStringLiteral("SingleUnderline");
        case QTextCharFormat::DashUnderline: return QStringLiteral("DashUnderline");
        case QTextCharFormat::DotLine: return QStringLiteral("DotLine");
        case QTextCharFormat::DashDotLine: return QStringLiteral("DashDotLine");
        case QTextCharFormat::DashDotDotLine: return QStringLiteral("DashDotDotLine");
        case QTextCharFormat::WaveUnderline: return QStringLiteral("WaveUnderline");
        case QTextCharFormat::SpellCheckUnderline: return QString();
    }

    return QString();
}

bool Format::equals(const Format &other) const
{
    return m_foreground ==  other.m_foreground
        && m_background == other.m_background
        && m_underlineColor == other.m_underlineColor
        && m_underlineStyle == other.m_underlineStyle
        && m_bold == other.m_bold
        && m_italic == other.m_italic;
}

QString Format::toString() const
{
    QStringList text({m_foreground.name(),
                      m_background.name(),
                      m_bold ? QLatin1String(trueString) : QLatin1String(falseString),
                      m_italic ? QLatin1String(trueString) : QLatin1String(falseString),
                      m_underlineColor.name(),
                      underlineStyleToString(m_underlineStyle)});

    return text.join(QLatin1Char(';'));
}

bool Format::fromString(const QString &str)
{
    *this = Format();

    const QStringList lst = str.split(QLatin1Char(';'));
    if (lst.count() != 4 && lst.count() != 6)
        return false;

    m_foreground = stringToColor(lst.at(0));
    m_background = stringToColor(lst.at(1));
    m_bold = lst.at(2) == QLatin1String(trueString);
    m_italic = lst.at(3) == QLatin1String(trueString);
    m_underlineColor = stringToColor(lst.at(4));
    m_underlineStyle = stringToUnderlineStyle(lst.at(5));

    return true;
}


// ColorScheme

bool ColorScheme::contains(TextStyle category) const
{
    return m_formats.contains(category);
}

Format &ColorScheme::formatFor(TextStyle category)
{
    return m_formats[category];
}

Format ColorScheme::formatFor(TextStyle category) const
{
    return m_formats.value(category);
}

void ColorScheme::setFormatFor(TextStyle category, const Format &format)
{
    m_formats[category] = format;
}

void ColorScheme::clear()
{
    m_formats.clear();
}

bool ColorScheme::save(const QString &fileName, QWidget *parent) const
{
    Utils::FileSaver saver(fileName);
    if (!saver.hasError()) {
        QXmlStreamWriter w(saver.file());
        w.setAutoFormatting(true);
        w.setAutoFormattingIndent(2);

        w.writeStartDocument();
        w.writeStartElement(QLatin1String("style-scheme"));
        w.writeAttribute(QLatin1String("version"), QLatin1String("1.0"));
        if (!m_displayName.isEmpty())
            w.writeAttribute(QLatin1String("name"), m_displayName);

        QMapIterator<TextStyle, Format> i(m_formats);
        while (i.hasNext()) {
            const Format &format = i.next().value();
            w.writeStartElement(QLatin1String("style"));
            w.writeAttribute(QLatin1String("name"), QString::fromLatin1(Constants::nameForStyle(i.key())));
            if (format.foreground().isValid())
                w.writeAttribute(QLatin1String("foreground"), format.foreground().name().toLower());
            if (format.background().isValid())
                w.writeAttribute(QLatin1String("background"), format.background().name().toLower());
            if (format.bold())
                w.writeAttribute(QLatin1String("bold"), QLatin1String(trueString));
            if (format.italic())
                w.writeAttribute(QLatin1String("italic"), QLatin1String(trueString));
            if (format.underlineColor().isValid())
                 w.writeAttribute(QStringLiteral("underlineColor"), format.underlineColor().name().toLower());
            if (format.underlineStyle() != QTextCharFormat::NoUnderline)
                 w.writeAttribute(QLatin1String("underlineStyle"), underlineStyleToString(format.underlineStyle()));
            w.writeEndElement();
        }

        w.writeEndElement();
        w.writeEndDocument();

        saver.setResult(&w);
    }
    return saver.finalize(parent);
}

namespace {

class ColorSchemeReader : public QXmlStreamReader
{
public:
    ColorSchemeReader() :
        m_scheme(0)
    {}

    bool read(const QString &fileName, ColorScheme *scheme);
    QString readName(const QString &fileName);

private:
    bool readNextStartElement();
    void skipCurrentElement();
    void readStyleScheme();
    void readStyle();

    ColorScheme *m_scheme;
    QString m_name;
};

bool ColorSchemeReader::read(const QString &fileName, ColorScheme *scheme)
{
    m_scheme = scheme;

    if (m_scheme)
        m_scheme->clear();

    QFile file(fileName);
    if (!file.open(QFile::ReadOnly | QFile::Text))
        return false;

    setDevice(&file);

    if (readNextStartElement() && name() == QLatin1String("style-scheme"))
        readStyleScheme();
    else
        raiseError(QCoreApplication::translate("TextEditor::Internal::ColorScheme", "Not a color scheme file."));

    return true;
}

QString ColorSchemeReader::readName(const QString &fileName)
{
    read(fileName, 0);
    return m_name;
}

bool ColorSchemeReader::readNextStartElement()
{
    while (readNext() != Invalid) {
        if (isStartElement())
            return true;
        else if (isEndElement())
            return false;
    }
    return false;
}

void ColorSchemeReader::skipCurrentElement()
{
    while (readNextStartElement())
        skipCurrentElement();
}

void ColorSchemeReader::readStyleScheme()
{
    Q_ASSERT(isStartElement() && name() == QLatin1String("style-scheme"));

    const QXmlStreamAttributes attr = attributes();
    m_name = attr.value(QLatin1String("name")).toString();
    if (!m_scheme)
        // We're done
        raiseError(QLatin1String("name loaded"));
    else
        m_scheme->setDisplayName(m_name);

    while (readNextStartElement()) {
        if (name() == QLatin1String("style"))
            readStyle();
        else
            skipCurrentElement();
    }
}

void ColorSchemeReader::readStyle()
{
    Q_ASSERT(isStartElement() && name() == QLatin1String("style"));

    const QXmlStreamAttributes attr = attributes();
    QByteArray name = attr.value(QLatin1String("name")).toString().toLatin1();
    QString foreground = attr.value(QLatin1String("foreground")).toString();
    QString background = attr.value(QLatin1String("background")).toString();
    bool bold = attr.value(QLatin1String("bold")) == QLatin1String(trueString);
    bool italic = attr.value(QLatin1String("italic")) == QLatin1String(trueString);
    QString underlineColor = attr.value(QLatin1String("underlineColor")).toString();
    QString underlineStyle = attr.value(QLatin1String("underlineStyle")).toString();

    Format format;

    if (QColor::isValidColor(foreground))
        format.setForeground(QColor(foreground));
    else
        format.setForeground(QColor());

    if (QColor::isValidColor(background))
        format.setBackground(QColor(background));
    else
        format.setBackground(QColor());

    format.setBold(bold);
    format.setItalic(italic);

    if (QColor::isValidColor(underlineColor))
        format.setUnderlineColor(QColor(underlineColor));
    else
        format.setUnderlineColor(QColor());

    format.setUnderlineStyle(stringToUnderlineStyle(underlineStyle));

    m_scheme->setFormatFor(Constants::styleFromName(name), format);

    skipCurrentElement();
}

} // anonymous namespace


bool ColorScheme::load(const QString &fileName)
{
    ColorSchemeReader reader;
    return reader.read(fileName, this) && !reader.hasError();
}

QString ColorScheme::readNameOfScheme(const QString &fileName)
{
    return ColorSchemeReader().readName(fileName);
}
