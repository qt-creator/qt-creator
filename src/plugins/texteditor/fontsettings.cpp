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
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "fontsettings.h"
#include "fontsettingspage.h"

#include <utils/qtcassert.h>
#include <coreplugin/icore.h>

#include <QtCore/QSettings>
#include <QtCore/QCoreApplication>
#include <QtGui/QTextCharFormat>

static const char *fontFamilyKey = "FontFamily";
static const char *fontSizeKey = "FontSize";
static const char *fontZoomKey= "FontZoom";
static const char *antialiasKey = "FontAntialias";
static const char *schemeFileNameKey = "ColorScheme";

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
    m_fontZoom(100),
    m_antialias(DEFAULT_ANTIALIAS)
{
}

void FontSettings::clear()
{
    m_family = defaultFixedFontFamily();
    m_fontSize = DEFAULT_FONT_SIZE;
    m_fontZoom = 100;
    m_antialias = DEFAULT_ANTIALIAS;
    m_scheme.clear();
}

void FontSettings::toSettings(const QString &category,
                              QSettings *s) const
{
    s->beginGroup(category);
    if (m_family != defaultFixedFontFamily() || s->contains(QLatin1String(fontFamilyKey)))
        s->setValue(QLatin1String(fontFamilyKey), m_family);

    if (m_fontSize != DEFAULT_FONT_SIZE || s->contains(QLatin1String(fontSizeKey)))
        s->setValue(QLatin1String(fontSizeKey), m_fontSize);

    if (m_fontZoom!= 100 || s->contains(QLatin1String(fontZoomKey)))
        s->setValue(QLatin1String(fontZoomKey), m_fontZoom);

    if (m_antialias != DEFAULT_ANTIALIAS || s->contains(QLatin1String(antialiasKey)))
        s->setValue(QLatin1String(antialiasKey), m_antialias);

    if (m_schemeFileName != defaultSchemeFileName() || s->contains(QLatin1String(schemeFileNameKey)))
        s->setValue(QLatin1String(schemeFileNameKey), m_schemeFileName);

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
    m_fontZoom= s->value(group + QLatin1String(fontZoomKey), m_fontZoom).toInt();
    m_antialias = s->value(group + QLatin1String(antialiasKey), DEFAULT_ANTIALIAS).toBool();

    if (s->contains(group + QLatin1String(schemeFileNameKey))) {
        // Load the selected color scheme
        loadColorScheme(s->value(group + QLatin1String(schemeFileNameKey), defaultSchemeFileName()).toString(),
                        descriptions);
    } else {
        // Load color scheme from ini file
        foreach (const FormatDescription &desc, descriptions) {
            const QString id = desc.id();
            const QString fmt = s->value(group + id, QString()).toString();
            Format format;
            if (fmt.isEmpty()) {
                format.setForeground(desc.foreground());
                format.setBackground(desc.background());
                format.setBold(desc.format().bold());
                format.setItalic(desc.format().italic());
            } else {
                format.fromString(fmt);
            }
            m_scheme.setFormatFor(id, format);
        }

        m_scheme.setDisplayName(QCoreApplication::translate("TextEditor::Internal::FontSettings", "Customized"));
    }

    return true;
}

bool FontSettings::equals(const FontSettings &f) const
{
    return m_family == f.m_family
            && m_schemeFileName == f.m_schemeFileName
            && m_fontSize == f.m_fontSize
            && m_fontZoom == f.m_fontZoom
            && m_antialias == f.m_antialias
            && m_scheme == f.m_scheme;
}

/**
 * Returns the QTextCharFormat of the given format category.
 */
QTextCharFormat FontSettings::toTextCharFormat(const QString &category) const
{
    const Format &f = m_scheme.formatFor(category);
    const QLatin1String textCategory("Text");

    QTextCharFormat tf;

    if (category == textCategory) {
        tf.setFontFamily(m_family);
        tf.setFontPointSize(m_fontSize * m_fontZoom / 100);
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

/**
 * Returns the list of QTextCharFormats that corresponds to the list of
 * requested format categories.
 */
QVector<QTextCharFormat> FontSettings::toTextCharFormats(const QVector<QString> &categories) const
{
    QVector<QTextCharFormat> rc;
    const int size = categories.size();
    rc.reserve(size);
    for (int i = 0; i < size; i++)
         rc.append(toTextCharFormat(categories.at(i)));
    return rc;
}

/**
 * Returns the configured font family.
 */
QString FontSettings::family() const
{
    return m_family;
}

void FontSettings::setFamily(const QString &family)
{
    m_family = family;
}

/**
 * Returns the configured font size.
 */
int FontSettings::fontSize() const
{
    return m_fontSize;
}

void FontSettings::setFontSize(int size)
{
    m_fontSize = size;
}

/**
 * Returns the configured font zoom factor in percent.
 */
int FontSettings::fontZoom() const
{
    return m_fontZoom;
}

void FontSettings::setFontZoom(int zoom)
{
    m_fontZoom = zoom;
}

/**
 * Returns the configured antialiasing behavior.
 */
bool FontSettings::antialias() const
{
    return m_antialias;
}

void FontSettings::setAntialias(bool antialias)
{
    m_antialias = antialias;
}

/**
 * Returns the format for the given font category.
 */
Format &FontSettings::formatFor(const QString &category)
{
    return m_scheme.formatFor(category);
}

/**
 * Returns the file name of the currently selected color scheme.
 */
QString FontSettings::colorSchemeFileName() const
{
    return m_schemeFileName;
}

/**
 * Sets the file name of the color scheme. Does not load the scheme from the
 * given file. If you want to load a scheme, use loadColorScheme() instead.
 */
void FontSettings::setColorSchemeFileName(const QString &fileName)
{
    m_schemeFileName = fileName;
}

bool FontSettings::loadColorScheme(const QString &fileName,
                                   const FormatDescriptions &descriptions)
{
    bool loaded = true;
    m_schemeFileName = fileName;

    if (!m_scheme.load(m_schemeFileName)) {
        loaded = false;
        m_schemeFileName.clear();
        qWarning() << "Failed to load color scheme:" << fileName;
    }

    // Apply default formats to undefined categories
    foreach (const FormatDescription &desc, descriptions) {
        const QString id = desc.id();
        if (!m_scheme.contains(id)) {
            Format format;
            format.setForeground(desc.foreground());
            format.setBackground(desc.background());
            format.setBold(desc.format().bold());
            format.setItalic(desc.format().italic());
            m_scheme.setFormatFor(id, format);
        }
    }

    return loaded;
}

bool FontSettings::saveColorScheme(const QString &fileName)
{
    const bool saved = m_scheme.save(fileName);
    if (saved)
        m_schemeFileName = fileName;
    return saved;
}

/**
 * Returns the currently active color scheme.
 */
const ColorScheme &FontSettings::colorScheme() const
{
    return m_scheme;
}

void FontSettings::setColorScheme(const ColorScheme &scheme)
{
    m_scheme = scheme;
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

QString FontSettings::defaultSchemeFileName()
{
    QString fileName = Core::ICore::instance()->resourcePath();
    fileName += QLatin1String("/styles/default.xml");
    return fileName;
}

} // namespace TextEditor
