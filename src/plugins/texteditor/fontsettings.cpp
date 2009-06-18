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

#include "fontsettings.h"
#include "fontsettingspage.h"

#include <utils/qtcassert.h>

#include <QtCore/QSettings>
#include <QtGui/QTextCharFormat>

static const char *fontFamilyKey = "FontFamily";
static const char *fontSizeKey = "FontSize";
static const char *antialiasKey = "FontAntialias";

namespace {
static const bool DEFAULT_ANTIALIAS = true;

#ifdef Q_WS_MAC
    enum { DEFAULT_FONT_SIZE = 12 };
    static const char *DEFAULT_FONT_FAMILY = "Monaco";
#else
#ifdef Q_WS_X11
    enum { DEFAULT_FONT_SIZE = 9 };
    static const char *DEFAULT_FONT_FAMILY = "Monospace";
#else
    enum { DEFAULT_FONT_SIZE = 10 };
    static const char *DEFAULT_FONT_FAMILY = "Courier";
#endif
#endif
} // anonymous namespace

namespace TextEditor {

// -- FontSettings
FontSettings::FontSettings() :
    m_family(defaultFixedFontFamily()),
    m_fontSize(DEFAULT_FONT_SIZE),
    m_antialias(DEFAULT_ANTIALIAS)
{
}

void FontSettings::clear()
{
    m_family = defaultFixedFontFamily();
    m_fontSize = DEFAULT_FONT_SIZE;
    m_antialias = DEFAULT_ANTIALIAS;
    m_scheme.clear();
}

void FontSettings::toSettings(const QString &category,
                              const FormatDescriptions &descriptions,
                              QSettings *s) const
{
    s->beginGroup(category);
    if (m_family != defaultFixedFontFamily() || s->contains(QLatin1String(fontFamilyKey)))
        s->setValue(QLatin1String(fontFamilyKey), m_family);

    if (m_fontSize != DEFAULT_FONT_SIZE || s->contains(QLatin1String(fontSizeKey)))
        s->setValue(QLatin1String(fontSizeKey), m_fontSize);

    if (m_antialias != DEFAULT_ANTIALIAS || s->contains(QLatin1String(antialiasKey)))
        s->setValue(QLatin1String(antialiasKey), m_antialias);

    const Format defaultFormat;

    foreach (const FormatDescription &desc, descriptions) {
        const QString name = desc.name();
        if (m_scheme.contains(name)) {
            const Format &f = m_scheme.formatFor(name);
            if (f != defaultFormat || s->contains(name))
                s->setValue(name, f.toString());
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
    m_fontSize = s->value(group + QLatin1String(fontSizeKey), m_fontSize).toInt();
    m_antialias = s->value(group + QLatin1String(antialiasKey), DEFAULT_ANTIALIAS).toBool();

    foreach (const FormatDescription &desc, descriptions) {
        const QString name = desc.name();
        const QString fmt = s->value(group + name, QString()).toString();
        Format format;
        if (fmt.isEmpty()) {
            format.setForeground(desc.foreground());
            format.setBackground(desc.background());
            format.setBold(desc.format().bold());
            format.setItalic(desc.format().italic());
        } else {
            format.fromString(fmt);
        }
        m_scheme.setFormatFor(name, format);
    }
    return true;
}

bool FontSettings::equals(const FontSettings &f) const
{
    return m_family == f.m_family
            && m_fontSize == f.m_fontSize
            && m_antialias == f.m_antialias
            && m_scheme == f.m_scheme;
}

QTextCharFormat FontSettings::toTextCharFormat(const QString &category) const
{
    const Format &f = m_scheme.formatFor(category);
    const QLatin1String textCategory("Text");

    QTextCharFormat tf;

    if (category == textCategory) {
        tf.setFontFamily(m_family);
        tf.setFontPointSize(m_fontSize);
        tf.setFontStyleStrategy(m_antialias ? QFont::PreferAntialias : QFont::NoAntialias);
    }

    if (f.foreground().isValid())
        tf.setForeground(f.foreground());
    if (f.background().isValid() && (category == textCategory || f.background() != m_scheme.formatFor(textCategory).background()))
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

bool FontSettings::antialias() const
{
    return m_antialias;
}

void FontSettings::setAntialias(bool antialias)
{
    m_antialias = antialias;
}


Format &FontSettings::formatFor(const QString &category)
{
    return m_scheme.formatFor(category);
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
