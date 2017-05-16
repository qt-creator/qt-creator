/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "fontsettings.h"
#include "fontsettingspage.h"

#include <utils/fileutils.h>
#include <utils/hostosinfo.h>
#include <utils/theme/theme.h>
#include <coreplugin/icore.h>

#include <QCoreApplication>
#include <QDebug>
#include <QFile>
#include <QFont>
#include <QFontDatabase>
#include <QSettings>
#include <QTextCharFormat>

#include <cmath>

static const char fontFamilyKey[] = "FontFamily";
static const char fontSizeKey[] = "FontSize";
static const char fontZoomKey[] = "FontZoom";
static const char antialiasKey[] = "FontAntialias";
static const char schemeFileNamesKey[] = "ColorSchemes";

namespace {
static const bool DEFAULT_ANTIALIAS = true;

} // anonymous namespace

namespace TextEditor {

// -- FontSettings
FontSettings::FontSettings() :
    m_family(defaultFixedFontFamily()),
    m_fontSize(defaultFontSize()),
    m_fontZoom(100),
    m_antialias(DEFAULT_ANTIALIAS)
{
}

void FontSettings::clear()
{
    m_family = defaultFixedFontFamily();
    m_fontSize = defaultFontSize();
    m_fontZoom = 100;
    m_antialias = DEFAULT_ANTIALIAS;
    m_scheme.clear();
    m_formatCache.clear();
    m_textCharFormatCache.clear();
}

void FontSettings::toSettings(const QString &category,
                              QSettings *s) const
{
    s->beginGroup(category);
    if (m_family != defaultFixedFontFamily() || s->contains(QLatin1String(fontFamilyKey)))
        s->setValue(QLatin1String(fontFamilyKey), m_family);

    if (m_fontSize != defaultFontSize() || s->contains(QLatin1String(fontSizeKey)))
        s->setValue(QLatin1String(fontSizeKey), m_fontSize);

    if (m_fontZoom!= 100 || s->contains(QLatin1String(fontZoomKey)))
        s->setValue(QLatin1String(fontZoomKey), m_fontZoom);

    if (m_antialias != DEFAULT_ANTIALIAS || s->contains(QLatin1String(antialiasKey)))
        s->setValue(QLatin1String(antialiasKey), m_antialias);

    auto schemeFileNames = s->value(QLatin1String(schemeFileNamesKey)).toMap();
    if (m_schemeFileName != defaultSchemeFileName() || schemeFileNames.contains(Utils::creatorTheme()->id())) {
        schemeFileNames.insert(Utils::creatorTheme()->id(), m_schemeFileName);
        s->setValue(QLatin1String(schemeFileNamesKey), schemeFileNames);
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
    m_fontZoom= s->value(group + QLatin1String(fontZoomKey), m_fontZoom).toInt();
    m_antialias = s->value(group + QLatin1String(antialiasKey), DEFAULT_ANTIALIAS).toBool();

    if (s->contains(group + QLatin1String(schemeFileNamesKey))) {
        // Load the selected color scheme for the current theme
        auto schemeFileNames = s->value(group + QLatin1String(schemeFileNamesKey)).toMap();
        if (schemeFileNames.contains(Utils::creatorTheme()->id())) {
            const QString scheme = schemeFileNames.value(Utils::creatorTheme()->id()).toString();
            loadColorScheme(scheme, descriptions);
        }
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

uint qHash(const TextStyle &textStyle)
{
    return ::qHash(quint8(textStyle));
}

/**
 * Returns the QTextCharFormat of the given format category.
 */
QTextCharFormat FontSettings::toTextCharFormat(TextStyle category) const
{
    auto textCharFormatIterator = m_formatCache.find(category);
    if (textCharFormatIterator != m_formatCache.end())
        return *textCharFormatIterator;

    const Format &f = m_scheme.formatFor(category);
    QTextCharFormat tf;

    if (category == C_TEXT) {
        tf.setFontFamily(m_family);
        tf.setFontPointSize(m_fontSize * m_fontZoom / 100.);
        tf.setFontStyleStrategy(m_antialias ? QFont::PreferAntialias : QFont::NoAntialias);
    }

    if (category == C_OCCURRENCES_UNUSED) {
        tf.setToolTip(QCoreApplication::translate("FontSettings_C_OCCURRENCES_UNUSED",
                                                  "Unused variable"));
    }

    if (f.foreground().isValid()
            && category != C_OCCURRENCES
            && category != C_OCCURRENCES_RENAME
            && category != C_SEARCH_RESULT
            && category != C_PARENTHESES_MISMATCH)
        tf.setForeground(f.foreground());
    if (f.background().isValid() && (category == C_TEXT || f.background() != m_scheme.formatFor(C_TEXT).background()))
        tf.setBackground(f.background());

    // underline does not need to fill without having background color
    if (f.underlineStyle() != QTextCharFormat::NoUnderline && !f.background().isValid())
        tf.setBackground(QBrush(Qt::BrushStyle::NoBrush));

    tf.setFontWeight(f.bold() ? QFont::Bold : QFont::Normal);
    tf.setFontItalic(f.italic());

    tf.setUnderlineColor(f.underlineColor());
    tf.setUnderlineStyle(f.underlineStyle());

    m_formatCache.insert(category, tf);
    return tf;
}

uint qHash(TextStyles textStyles)
{
    return ::qHash(reinterpret_cast<quint64&>(textStyles));
}

bool operator==(const TextStyles &first, const TextStyles &second)
{
    return first.mainStyle == second.mainStyle
        && first.mixinStyles == second.mixinStyles;
}

namespace {

double clamp(double value)
{
    return std::max(0.0, std::min(1.0, value));
}

QBrush mixBrush(const QBrush &original, double relativeSaturation, double relativeLightness)
{
    const QColor originalColor = original.color().toHsl();
    QColor mixedColor(QColor::Hsl);

    double mixedSaturation = clamp(originalColor.hslSaturationF() + relativeSaturation);

    double mixedLightness = clamp(originalColor.lightnessF() + relativeLightness);

    mixedColor.setHslF(originalColor.hslHueF(), mixedSaturation, mixedLightness);

    return mixedColor;
}
}

void FontSettings::addMixinStyle(QTextCharFormat &textCharFormat,
                                 const MixinTextStyles &mixinStyles) const
{
    for (TextStyle mixinStyle : mixinStyles) {
        const Format &format = m_scheme.formatFor(mixinStyle);

        if (textCharFormat.hasProperty(QTextFormat::ForegroundBrush)) {
            textCharFormat.setForeground(mixBrush(textCharFormat.foreground(),
                                                  format.relativeForegroundSaturation(),
                                                  format.relativeForegroundLightness()));
        }

        if (textCharFormat.hasProperty(QTextFormat::BackgroundBrush)) {
            textCharFormat.setBackground(mixBrush(textCharFormat.background(),
                                                  format.relativeBackgroundSaturation(),
                                                  format.relativeBackgroundLightness()));
        }

        if (!textCharFormat.fontItalic())
            textCharFormat.setFontItalic(format.italic());

        if (textCharFormat.fontWeight() == QFont::Normal)
            textCharFormat.setFontWeight(format.bold() ? QFont::Bold : QFont::Normal);

        if (textCharFormat.underlineStyle() == QTextCharFormat::NoUnderline) {
            textCharFormat.setUnderlineStyle(format.underlineStyle());
            textCharFormat.setUnderlineColor(format.underlineColor());
        }
    };
}

QTextCharFormat FontSettings::toTextCharFormat(TextStyles textStyles) const
{
    auto textCharFormatIterator = m_textCharFormatCache.find(textStyles);
    if (textCharFormatIterator != m_textCharFormatCache.end())
        return *textCharFormatIterator;

    QTextCharFormat textCharFormat = toTextCharFormat(textStyles.mainStyle);

    addMixinStyle(textCharFormat, textStyles.mixinStyles);

    m_textCharFormatCache.insert(textStyles, textCharFormat);

    return textCharFormat;
}

/**
 * Returns the list of QTextCharFormats that corresponds to the list of
 * requested format categories.
 */
QVector<QTextCharFormat> FontSettings::toTextCharFormats(const QVector<TextStyle> &categories) const
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
    m_formatCache.clear();
    m_textCharFormatCache.clear();
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
    m_formatCache.clear();
    m_textCharFormatCache.clear();
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
    m_formatCache.clear();
    m_textCharFormatCache.clear();
}

QFont FontSettings::font() const
{
    QFont f(family(), fontSize());
    f.setStyleStrategy(m_antialias ? QFont::PreferAntialias : QFont::NoAntialias);
    return f;
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
    m_formatCache.clear();
    m_textCharFormatCache.clear();
}

/**
 * Returns the format for the given font category.
 */
Format &FontSettings::formatFor(TextStyle category)

{
    return m_scheme.formatFor(category);
}

Format FontSettings::formatFor(TextStyle category) const
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
    m_formatCache.clear();
    m_textCharFormatCache.clear();
    bool loaded = true;
    m_schemeFileName = fileName;

    if (!m_scheme.load(m_schemeFileName)) {
        loaded = false;
        m_schemeFileName.clear();
        qWarning() << "Failed to load color scheme:" << fileName;
    }

    // Apply default formats to undefined categories
    foreach (const FormatDescription &desc, descriptions) {
        const TextStyle id = desc.id();
        if (!m_scheme.contains(id)) {
            Format format;
            const Format &descFormat = desc.format();
            if (descFormat == format && m_scheme.contains(C_TEXT)) {
                // Default format -> Text
                const Format textFormat = m_scheme.formatFor(C_TEXT);
                format.setForeground(textFormat.foreground());
                format.setBackground(textFormat.background());
            } else {
                format.setForeground(descFormat.foreground());
                format.setBackground(descFormat.background());
            }
            format.setRelativeForegroundSaturation(descFormat.relativeForegroundSaturation());
            format.setRelativeForegroundLightness(descFormat.relativeForegroundLightness());
            format.setRelativeBackgroundSaturation(descFormat.relativeBackgroundSaturation());
            format.setRelativeBackgroundLightness(descFormat.relativeBackgroundLightness());
            format.setBold(descFormat.bold());
            format.setItalic(descFormat.italic());
            format.setUnderlineColor(descFormat.underlineColor());
            format.setUnderlineStyle(descFormat.underlineStyle());
            m_scheme.setFormatFor(id, format);
        }
    }

    return loaded;
}

bool FontSettings::saveColorScheme(const QString &fileName)
{
    const bool saved = m_scheme.save(fileName, Core::ICore::mainWindow());
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
    m_formatCache.clear();
    m_textCharFormatCache.clear();
}

static QString defaultFontFamily()
{
    if (Utils::HostOsInfo::isMacHost())
        return QLatin1String("Monaco");

    const QString sourceCodePro("Source Code Pro");
    const QFontDatabase dataBase;
    if (dataBase.hasFamily(sourceCodePro))
        return sourceCodePro;

    if (Utils::HostOsInfo::isAnyUnixHost())
        return QLatin1String("Monospace");
    return QLatin1String("Courier");
}

QString FontSettings::defaultFixedFontFamily()
{
    static QString rc;
    if (rc.isEmpty()) {
        QFont f = QFont(defaultFontFamily());
        f.setStyleHint(QFont::TypeWriter);
        rc = f.family();
    }
    return rc;
}

int FontSettings::defaultFontSize()
{
    if (Utils::HostOsInfo::isMacHost())
        return 12;
    if (Utils::HostOsInfo::isAnyUnixHost())
        return 9;
    return 10;
}

/**
 * Returns the default scheme file name, or the path to a shipped scheme when
 * one exists with the given \a fileName.
 */
QString FontSettings::defaultSchemeFileName(const QString &fileName)
{
    QString defaultScheme = Core::ICore::resourcePath();
    defaultScheme += QLatin1String("/styles/");

    if (!fileName.isEmpty() && QFile::exists(defaultScheme + fileName)) {
        defaultScheme += fileName;
    } else {
        const QString themeScheme = Utils::creatorTheme()->defaultTextEditorColorScheme();
        if (!themeScheme.isEmpty() && QFile::exists(defaultScheme + themeScheme))
            defaultScheme += themeScheme;
        else
            defaultScheme += QLatin1String("default.xml");
    }

    return defaultScheme;
}

} // namespace TextEditor
