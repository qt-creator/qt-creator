/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#include "fontsettings.h"
#include "fontsettingspage.h"

#include <utils/qtcassert.h>

#include <QtCore/QSettings>
#include <QtGui/QTextCharFormat>

static const char *fontFamilyKey = "FontFamily";
static const char *fontSizeKey = "FontSize";
static const char *trueString = "true";
static const char *falseString = "false";

namespace {
#ifdef Q_WS_MAC
    enum { DEFAULT_FONT_SIZE = 12 };
    static const char *DEFAULT_FONT_FAMILY = "Monaco";
#else
#ifdef Q_OS_UNIX
    enum { DEFAULT_FONT_SIZE = 9 };
    static const char *DEFAULT_FONT_FAMILY = "Monospace";
#else
    enum { DEFAULT_FONT_SIZE = 10 };
    static const char *DEFAULT_FONT_FAMILY = "Courier";
#endif
#endif
} // anonymous namespace

namespace TextEditor {

// Format --
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

static QString colorToString(const QColor &color) {
    if (color.isValid())
        return color.name();
    return QLatin1String("invalid");
}

static QColor stringToColor(const QString &string) {
    if (string == QLatin1String("invalid"))
        return QColor();
    return QColor(string);
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

bool Format::equals(const Format &f) const
{
    return m_foreground ==  f.m_foreground && m_background == f.m_background &&
           m_bold == f.m_bold && m_italic == f.m_italic;
}

// -- FontSettings
FontSettings::FontSettings(const FormatDescriptions &fd) :
    m_family(defaultFixedFontFamily()),
    m_fontSize(DEFAULT_FONT_SIZE)
{
    Q_UNUSED(fd);
}

void FontSettings::clear()
{
    m_family = defaultFixedFontFamily();
    m_fontSize = DEFAULT_FONT_SIZE;
    qFill(m_formats.begin(), m_formats.end(), Format());
}

void FontSettings::toSettings(const QString &category,
                              const FormatDescriptions &descriptions,
                              QSettings *s) const
{
    const int numFormats = m_formats.size();
    QTC_ASSERT(descriptions.size() == numFormats, /**/);
    s->beginGroup(category);
    if (m_family != defaultFixedFontFamily() || s->contains(QLatin1String(fontFamilyKey)))
        s->setValue(QLatin1String(fontFamilyKey), m_family);

    if (m_fontSize != DEFAULT_FONT_SIZE || s->contains(QLatin1String(fontSizeKey)))
        s->setValue(QLatin1String(fontSizeKey), m_fontSize);

    const Format defaultFormat;

    foreach (const FormatDescription &desc, descriptions) {
        QMap<QString, Format>::const_iterator i = m_formats.find(desc.name());
        if (i != m_formats.end() && ((*i) != defaultFormat || s->contains(desc.name()))) {
            s->setValue(desc.name(), (*i).toString());
        }
    }
    s->endGroup();
}

bool FontSettings::fromSettings(const QString &category,
                                const FormatDescriptions &descriptions,
                                const QSettings *s)
{
    clear();

    if (!s->childGroups().contains(category))
        return false;

    QString group = category;
    group += QLatin1Char('/');

    m_family = s->value(group + QLatin1String(fontFamilyKey), defaultFixedFontFamily()).toString();
    m_fontSize = s->value(group + QLatin1String(QLatin1String(fontSizeKey)), m_fontSize).toInt();

    foreach (const FormatDescription &desc, descriptions) {
        const QString name = desc.name();
        const QString fmt = s->value(group + name, QString()).toString();
        if (fmt.isEmpty()) {
            m_formats[name].setForeground(desc.foreground());
            m_formats[name].setBackground(desc.background());
        } else {
            m_formats[name].fromString(fmt);
        }
    }
    return true;
}

bool FontSettings::equals(const FontSettings &f) const
{
    return m_family == f.m_family
            && m_fontSize == f.m_fontSize
            && m_formats == f.m_formats;
}

QTextCharFormat FontSettings::toTextCharFormat(const QString &category) const
{
    const Format f = m_formats.value(category);
    QTextCharFormat tf;
    if (category == QLatin1String("Text")) {
        tf.setFontFamily(m_family);
        tf.setFontPointSize(m_fontSize);
    }

    if (f.foreground().isValid())
        tf.setForeground(f.foreground());
    if (f.background().isValid() && (category == QLatin1String("Text") || f.background() != m_formats.value(QLatin1String("Text")).background()))
        tf.setBackground(f.background());
    tf.setFontWeight(f.bold() ? QFont::Bold : QFont::Normal);
    tf.setFontItalic(f.italic());
    return tf;
}

QVector<QTextCharFormat> FontSettings::toTextCharFormats(const QVector<QString> &categories) const
{
    QVector<QTextCharFormat> rc;
    const int size = categories.size();
    rc.reserve(size);
    for (int i = 0; i < size; i++)
         rc.push_back(toTextCharFormat(categories.at(i)));
    return rc;
}

QString FontSettings::family() const
{
    return m_family;
}

void FontSettings::setFamily(const QString &family)
{
    m_family = family;
}

int FontSettings::fontSize() const
{
    return m_fontSize;
}

void FontSettings::setFontSize(int size)
{
    m_fontSize = size;
}

Format &FontSettings::formatFor(const QString &category)
{
    return m_formats[category];
}

QString FontSettings::defaultFixedFontFamily()
{
    static QString rc;
    if (rc.isEmpty()) {
        QFont f(DEFAULT_FONT_FAMILY);
        f.setStyleHint(QFont::TypeWriter);
        rc = f.family();
    }
    return rc;
}

int FontSettings::defaultFontSize()
{
    return DEFAULT_FONT_SIZE;
}

} // namespace TextEditor
