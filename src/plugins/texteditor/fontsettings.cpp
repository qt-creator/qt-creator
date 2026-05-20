// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "fontsettings.h"

#include "fontsettingspage.h"
#include "texteditorconstants.h"
#include "texteditorsettings.h"
#include "texteditortr.h"

#include <coreplugin/icore.h>
#include <utils/fileutils.h>
#include <utils/hostosinfo.h>
#include <utils/plaintextedit/plaintextedit.h>
#include <utils/stringutils.h>
#include <utils/theme/theme.h>

#include <QApplication>
#include <QCoreApplication>
#include <QDebug>
#include <QFile>
#include <QFont>
#include <QFontDatabase>
#include <QTextCharFormat>

#include <cmath>

using namespace Utils;

const char fontFamilyKey[] = "FontFamily";
const char fontSizeKey[] = "FontSize";
const char fontZoomKey[] = "FontZoom";
const char lineSpacingKey[] = "LineSpacing";
const char antialiasKey[] = "FontAntialias";
const char schemeFileNamesKey[] = "ColorSchemes";

const bool DEFAULT_ANTIALIAS = true;

const char g_sourceCodePro[] = "Source Code Pro";

namespace TextEditor {

// -- FontSettings
FontSettings::FontSettings() :
    m_family(defaultFixedFontFamily()),
    m_fontSize(defaultFontSize()),
    m_fontZoom(100),
    m_lineSpacing(100),
    m_antialias(DEFAULT_ANTIALIAS)
{
}

void FontSettings::clear()
{
    m_family = defaultFixedFontFamily();
    m_fontSize = defaultFontSize();
    m_fontZoom = 100;
    m_lineSpacing = 100;
    m_antialias = DEFAULT_ANTIALIAS;
    m_scheme.clear();
    clearCaches();
}

static Key settingsGroup()
{
    return keyFromString(Utils::settingsKey(TextEditor::Constants::TEXT_EDITOR_SETTINGS_CATEGORY));
}

void FontSettings::toSettings(QtcSettings *s) const
{
    s->beginGroup(settingsGroup());
    if (m_family != defaultFixedFontFamily() || s->contains(fontFamilyKey))
        s->setValue(fontFamilyKey, m_family);

    if (m_fontSize != defaultFontSize() || s->contains(fontSizeKey))
        s->setValue(fontSizeKey, m_fontSize);

    if (m_fontZoom!= 100 || s->contains(fontZoomKey))
        s->setValue(fontZoomKey, m_fontZoom);

    if (m_lineSpacing != 100 || s->contains(lineSpacingKey))
        s->setValue(lineSpacingKey, m_lineSpacing);

    if (m_antialias != DEFAULT_ANTIALIAS || s->contains(antialiasKey))
        s->setValue(antialiasKey, m_antialias);

    auto schemeFileNames = s->value(schemeFileNamesKey).toMap();
    if (m_schemeFileName != defaultSchemeFileName() || schemeFileNames.contains(Utils::creatorTheme()->id())) {
        schemeFileNames.insert(Utils::creatorTheme()->id(), m_schemeFileName.toSettings());
        s->setValue(schemeFileNamesKey, schemeFileNames);
    }

    s->endGroup();
}

bool FontSettings::fromSettings(const FormatDescriptions &descriptions, const QtcSettings *s)
{
    clear();

    Key group = settingsGroup();
    if (!s->childGroups().contains(stringFromKey(group)))
        return false;

    group = group + '/';

    m_family = s->value(group + fontFamilyKey, defaultFixedFontFamily()).toString();
    m_fontSize = s->value(group + fontSizeKey, m_fontSize).toInt();
    m_fontZoom= s->value(group + fontZoomKey, m_fontZoom).toInt();
    m_lineSpacing = s->value(group + lineSpacingKey, m_lineSpacing).toInt();
    QTC_ASSERT(m_lineSpacing >= 0, m_lineSpacing = 100);
    m_antialias = s->value(group + antialiasKey, DEFAULT_ANTIALIAS).toBool();

    if (s->contains(group + schemeFileNamesKey)) {
        // Load the selected color scheme for the current theme
        auto schemeFileNames = s->value(group + schemeFileNamesKey).toMap();
        if (schemeFileNames.contains(Utils::creatorTheme()->id())) {
            const FilePath scheme = FilePath::fromSettings(schemeFileNames.value(Utils::creatorTheme()->id()));
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
            && m_lineSpacing == f.m_lineSpacing
            && m_fontZoom == f.m_fontZoom
            && m_antialias == f.m_antialias
            && m_scheme == f.m_scheme;
}

size_t qHash(const TextStyle &textStyle)
{
    return ::qHash(quint8(textStyle));
}

static bool isOverlayCategory(TextStyle category)
{
    return category == C_OCCURRENCES
           || category == C_OCCURRENCES_RENAME
           || category == C_SEARCH_RESULT
           || category == C_SEARCH_RESULT_ALT1
           || category == C_SEARCH_RESULT_ALT2
           || category == C_PARENTHESES_MISMATCH;
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
        tf.setFontFamilies({m_family});
        tf.setFontPointSize(m_fontSize * m_fontZoom / 100.);
        tf.setFontStyleStrategy(m_antialias ? QFont::PreferAntialias : QFont::NoAntialias);
    }

    if (category == C_OCCURRENCES_UNUSED)
        tf.setToolTip(Tr::tr("Unused variable"));

    if (f.foreground().isValid() && !isOverlayCategory(category))
        tf.setForeground(f.foreground());
    if (f.background().isValid()) {
        if (category == C_TEXT || f.background() != m_scheme.formatFor(C_TEXT).background())
            tf.setBackground(f.background());
    } else if (isOverlayCategory(category)) {
        // overlays without a background schouldn't get painted
        tf.setBackground(QColor());
    }

    tf.setFontWeight(f.bold() ? QFont::Bold : fontNormalWeight());
    tf.setFontItalic(f.italic());

    tf.setUnderlineColor(f.underlineColor());
    tf.setUnderlineStyle(f.underlineStyle());

    m_formatCache.insert(category, tf);
    return tf;
}

size_t qHash(TextStyles textStyles)
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
    const int normalWeight = fontNormalWeight();
    for (TextStyle mixinStyle : mixinStyles) {
        const Format &format = m_scheme.formatFor(mixinStyle);

        if (format.foreground().isValid()) {
            textCharFormat.setForeground(format.foreground());
        } else {
            if (textCharFormat.hasProperty(QTextFormat::ForegroundBrush)) {
                textCharFormat.setForeground(mixBrush(textCharFormat.foreground(),
                                                      format.relativeForegroundSaturation(),
                                                      format.relativeForegroundLightness()));
            }
        }
        if (format.background().isValid()) {
            textCharFormat.setBackground(format.background());
        } else {
            if (textCharFormat.hasProperty(QTextFormat::BackgroundBrush)) {
                textCharFormat.setBackground(mixBrush(textCharFormat.background(),
                                                      format.relativeBackgroundSaturation(),
                                                      format.relativeBackgroundLightness()));
            }
        }
        if (!textCharFormat.fontItalic())
            textCharFormat.setFontItalic(format.italic());

        if (textCharFormat.fontWeight() == normalWeight)
            textCharFormat.setFontWeight(format.bold() ? QFont::Bold : normalWeight);

        if (textCharFormat.underlineStyle() == QTextCharFormat::NoUnderline) {
            textCharFormat.setUnderlineStyle(format.underlineStyle());
            textCharFormat.setUnderlineColor(format.underlineColor());
        }
    };
}

void FontSettings::clearCaches()
{
    m_formatCache.clear();
    m_textCharFormatCache.clear();
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
QList<QTextCharFormat> FontSettings::toTextCharFormats(const QList<TextStyle> &categories) const
{
    QList<QTextCharFormat> rc;
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
    clearCaches();
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
    clearCaches();
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
    clearCaches();
}

qreal FontSettings::lineSpacing() const
{
    QFont currentFont = font();
    currentFont.setPointSize(std::max(m_fontSize * m_fontZoom / 100, 1));
    qreal spacing = PlainTextDocumentLayout::lineSpacing(currentFont);
    if (QTC_GUARD(m_lineSpacing > 0) && m_lineSpacing != 100)
        spacing *= qreal(m_lineSpacing) / 100;
    return spacing;
}

int FontSettings::relativeLineSpacing() const
{
    return m_lineSpacing;
}

void FontSettings::setRelativeLineSpacing(int relativeLineSpacing)
{
    m_lineSpacing = relativeLineSpacing;
}

QFont FontSettings::font() const
{
    QFont f(family(), fontSize());
    f.setStyleStrategy(m_antialias ? QFont::PreferAntialias : QFont::NoAntialias);
    f.setWeight(fontNormalWeight());
    return f;
}

QFont::Weight FontSettings::fontNormalWeight() const
{
    // TODO: Fix this when we upgrade "Source Code Pro" to a version greater than 2.0.30
    QFont::Weight weight = QFont::Normal;
    if (Utils::HostOsInfo::isMacHost() && m_family == g_sourceCodePro)
        weight = QFont::Medium;
    return weight;
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
    clearCaches();
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
Utils::FilePath FontSettings::colorSchemeFileName() const
{
    return m_schemeFileName;
}

/**
 * Sets the file name of the color scheme. Does not load the scheme from the
 * given file. If you want to load a scheme, use loadColorScheme() instead.
 */
void FontSettings::setColorSchemeFileName(const Utils::FilePath &filePath)
{
    m_schemeFileName = filePath;
}

bool FontSettings::loadColorScheme(const Utils::FilePath &filePath,
                                   const FormatDescriptions &descriptions)
{
    clearCaches();

    bool loaded = true;
    m_schemeFileName = filePath;

    if (!m_scheme.load(m_schemeFileName)) {
        loaded = false;
        m_schemeFileName.clear();
        qWarning() << "Failed to load color scheme:" << filePath;
    }

    // Apply default formats to undefined categories
    for (const FormatDescription &desc : descriptions) {
        const TextStyle id = desc.id();
        if (!m_scheme.contains(id)) {
            if ((id == C_NAMESPACE || id == C_CONCEPT) && m_scheme.contains(C_TYPE)) {
                m_scheme.setFormatFor(id, m_scheme.formatFor(C_TYPE));
                continue;
            }
            if (id == C_MACRO && m_scheme.contains(C_FUNCTION)) {
                m_scheme.setFormatFor(C_MACRO, m_scheme.formatFor(C_FUNCTION));
                continue;
            }
            Format format;
            const Format &descFormat = desc.format();
            // Default fallback for background and foreground is C_TEXT, which is set through
            // the editor's palette, i.e. we leave these as invalid colors in that case
            if (descFormat != format || !m_scheme.contains(C_TEXT)) {
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

bool FontSettings::saveColorScheme(const FilePath &fileName)
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
    clearCaches();
}

static QString defaultFontFamily()
{
    if (Utils::HostOsInfo::isMacHost())
        return QLatin1String("Menlo");

    const QString sourceCodePro(g_sourceCodePro);
    if (QFontDatabase::hasFamily(sourceCodePro))
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
FilePath FontSettings::defaultSchemeFileName(const QString &fileName)
{
    FilePath defaultScheme = Core::ICore::resourcePath("styles");

    if (!fileName.isEmpty() && (defaultScheme / fileName).exists()) {
        defaultScheme = defaultScheme / fileName;
    } else {
        const QString themeScheme = Utils::creatorTheme()->defaultTextEditorColorScheme();
        if (!themeScheme.isEmpty() && (defaultScheme / themeScheme).exists())
            defaultScheme = defaultScheme / themeScheme;
        else
            defaultScheme = defaultScheme / "default.xml";
    }

    return defaultScheme;
}

FontSettingsContainer::FontSettingsContainer() = default;

void FontSettingsContainer::writeSettings() const
{
    m_value.toSettings(Core::ICore::settings());
}

void FontSettingsContainer::apply()
{
    writeSettings();
    emit TextEditorSettings::instance()->fontSettingsChanged(m_value);
}

void FontSettingsContainer::setData(const FontSettings &fs)
{
    m_value = fs;
    syncAspectsFromValue();
}

void FontSettingsContainer::syncAspectsFromValue()
{
    m_family.setValue(m_value.family());
    m_fontSize.setValue(m_value.fontSize());
    m_fontZoom.setValue(m_value.fontZoom());
    m_lineSpacing.setValue(m_value.relativeLineSpacing());
    m_antialias.setValue(m_value.antialias());
}

FontSettingsContainer &globalFontSettings()
{
    static FontSettingsContainer theGlobalFontSettingsContainer;
    return theGlobalFontSettingsContainer;
}

void setupFontSettings(const FontSettings::FormatDescriptions &fd)
{
    FontSettingsContainer &c = globalFontSettings();
    QtcSettings *s = Core::ICore::settings();
    c.m_value.fromSettings(fd, s);
    if (c.m_value.colorSchemeFileName().isEmpty())
        c.m_value.loadColorScheme(FontSettings::defaultSchemeFileName(), fd);
    c.syncAspectsFromValue();
}

namespace Internal {

FormatDescriptions initialFormats()
{
    using namespace TextEditor::Constants;

    FormatDescriptions formatDescr;
    formatDescr.reserve(C_LAST_STYLE_SENTINEL);
    formatDescr.emplace_back(C_TEXT, Tr::tr("Text"),
                             Tr::tr("Generic text and punctuation tokens.\n"
                                    "Applied to text that matched no other rule."),
                             Format{Qt::black, Qt::white});

    // Special categories
    const QPalette p = QApplication::palette();
    formatDescr.emplace_back(C_LINK, Tr::tr("Link"),
                             Tr::tr("Links that follow symbol under cursor."), Qt::blue);
    formatDescr.emplace_back(C_SELECTION, Tr::tr("Selection"), Tr::tr("Selected text."),
                             p.color(QPalette::HighlightedText));
    formatDescr.emplace_back(C_LINE_NUMBER, Tr::tr("Line Number"),
                             Tr::tr("Line numbers located on the left side of the editor."),
                             FormatDescription::ShowAllAbsoluteControlsExceptUnderline);
    formatDescr.emplace_back(C_SEARCH_RESULT, Tr::tr("Search Result"),
                             Tr::tr("Highlighted search results inside the editor."),
                             FormatDescription::ShowBackgroundControl);
    formatDescr.emplace_back(C_SEARCH_RESULT_ALT1, Tr::tr("Search Result (Alternative 1)"),
                             Tr::tr("Highlighted search results inside the editor.\n"
                                    "Used to mark read accesses to C++ symbols."),
                             FormatDescription::ShowBackgroundControl);
    formatDescr.emplace_back(C_SEARCH_RESULT_ALT2, Tr::tr("Search Result (Alternative 2)"),
                             Tr::tr("Highlighted search results inside the editor.\n"
                                    "Used to mark write accesses to C++ symbols."),
                             FormatDescription::ShowBackgroundControl);
    formatDescr.emplace_back(C_SEARCH_RESULT_CONTAINING_FUNCTION,
                             Tr::tr("Search Result Containing function"),
                             Tr::tr("Highlighted search results inside the editor.\n"
                                    "Used to mark containing function of the symbol usage."),
                             FormatDescription::ShowForeAndBackgroundControl);
    formatDescr.emplace_back(C_SEARCH_SCOPE, Tr::tr("Search Scope"),
                             Tr::tr("Section where the pattern is searched in."),
                             FormatDescription::ShowBackgroundControl);
    formatDescr.emplace_back(C_PARENTHESES, Tr::tr("Parentheses"),
                             Tr::tr("Displayed when matching parentheses, square brackets "
                                    "or curly brackets are found."));
    formatDescr.emplace_back(C_PARENTHESES_MISMATCH, Tr::tr("Mismatched Parentheses"),
                             Tr::tr("Displayed when mismatched parentheses, "
                                    "square brackets, or curly brackets are found."));
    formatDescr.emplace_back(C_AUTOCOMPLETE, Tr::tr("Auto Complete"),
                             Tr::tr("Displayed when a character is automatically inserted "
                                    "like brackets or quotes."));
    formatDescr.emplace_back(C_CURRENT_LINE, Tr::tr("Current Line"),
                             Tr::tr("Line where the cursor is placed in."),
                             FormatDescription::ShowBackgroundControl);

    FormatDescription currentLineNumber(C_CURRENT_LINE_NUMBER,
                                        Tr::tr("Current Line Number"),
                                        Tr::tr("Line number located on the left side of the "
                                               "editor where the cursor is placed in."),
                                        Qt::darkGray,
                                        FormatDescription::ShowAllAbsoluteControlsExceptUnderline);
    currentLineNumber.format().setBold(true);
    formatDescr.push_back(std::move(currentLineNumber));

    formatDescr.emplace_back(C_OCCURRENCES, Tr::tr("Occurrences"),
                             Tr::tr("Occurrences of the symbol under the cursor.\n"
                                    "(Only the background will be applied.)"),
                             FormatDescription::ShowBackgroundControl);
    formatDescr.emplace_back(C_OCCURRENCES_UNUSED,
                             Tr::tr("Unused Occurrence"),
                             Tr::tr("Occurrences of unused variables."),
                             Qt::darkYellow,
                             QTextCharFormat::SingleUnderline);
    formatDescr.emplace_back(C_OCCURRENCES_RENAME, Tr::tr("Renaming Occurrence"),
                             Tr::tr("Occurrences of a symbol that will be renamed."),
                             FormatDescription::ShowBackgroundControl);

    // Standard categories
    formatDescr.emplace_back(C_NUMBER, Tr::tr("Number"), Tr::tr("Number literal."),
                             Qt::darkBlue);
    formatDescr.emplace_back(C_STRING, Tr::tr("String"),
                             Tr::tr("Character and string literals."), Qt::darkGreen);
    formatDescr.emplace_back(C_PRIMITIVE_TYPE, Tr::tr("Primitive Type"),
                             Tr::tr("Name of a primitive data type."), Qt::darkYellow);
    formatDescr.emplace_back(C_TYPE, Tr::tr("Type"), Tr::tr("Name of a type."),
                             Qt::darkMagenta);
    formatDescr.emplace_back(C_CONCEPT, Tr::tr("Concept"), Tr::tr("Name of a concept."),
                             Qt::darkMagenta);
    formatDescr.emplace_back(C_NAMESPACE, Tr::tr("Namespace"), Tr::tr("Name of a namespace."),
                             Qt::darkGreen);
    formatDescr.emplace_back(C_LOCAL, Tr::tr("Local"),
                             Tr::tr("Local variables."), QColor(9, 46, 100));
    formatDescr.emplace_back(C_PARAMETER, Tr::tr("Parameter"),
                             Tr::tr("Function or method parameters."), QColor(9, 46, 100));
    formatDescr.emplace_back(C_FIELD, Tr::tr("Field"),
                             Tr::tr("Class' data members."), Qt::darkRed);
    formatDescr.emplace_back(C_GLOBAL, Tr::tr("Global"),
                             Tr::tr("Global variables."), QColor(206, 92, 0));
    formatDescr.emplace_back(C_ENUMERATION, Tr::tr("Enumeration"),
                             Tr::tr("Applied to enumeration items."), Qt::darkMagenta);

    Format functionFormat;
    functionFormat.setForeground(QColor(0, 103, 124));
    formatDescr.emplace_back(C_FUNCTION, Tr::tr("Function"), Tr::tr("Name of a function."),
                             functionFormat);
    Format declarationFormat;
    declarationFormat.setBold(true);
    formatDescr.emplace_back(C_DECLARATION,
                             Tr::tr("Declaration"),
                             Tr::tr("Style adjustments to declarations."),
                             declarationFormat,
                             FormatDescription::ShowAllControls);
    formatDescr.emplace_back(C_FUNCTION_DEFINITION,
                             Tr::tr("Function Definition"),
                             Tr::tr("Name of function at its definition."),
                             FormatDescription::ShowAllControls);
    Format virtualFunctionFormat(functionFormat);
    virtualFunctionFormat.setItalic(true);
    formatDescr.emplace_back(C_VIRTUAL_METHOD, Tr::tr("Virtual Function"),
                             Tr::tr("Name of function declared as virtual."),
                             virtualFunctionFormat);

    formatDescr.emplace_back(C_BINDING, Tr::tr("QML Binding"),
                             Tr::tr("QML item property, that allows a "
                                    "binding to another property."),
                             Qt::darkRed);

    Format qmlLocalNameFormat;
    qmlLocalNameFormat.setItalic(true);
    formatDescr.emplace_back(C_QML_LOCAL_ID, Tr::tr("QML Local Id"),
                             Tr::tr("QML item id within a QML file."), qmlLocalNameFormat);
    formatDescr.emplace_back(C_QML_ROOT_OBJECT_PROPERTY,
                             Tr::tr("QML Root Object Property"),
                             Tr::tr("QML property of a parent item."), qmlLocalNameFormat);
    formatDescr.emplace_back(C_QML_SCOPE_OBJECT_PROPERTY,
                             Tr::tr("QML Scope Object Property"),
                             Tr::tr("Property of the same QML item."), qmlLocalNameFormat);
    formatDescr.emplace_back(C_QML_STATE_NAME, Tr::tr("QML State Name"),
                             Tr::tr("Name of a QML state."), qmlLocalNameFormat);

    formatDescr.emplace_back(C_QML_TYPE_ID, Tr::tr("QML Type Name"),
                             Tr::tr("Name of a QML type."), Qt::darkMagenta);

    Format qmlExternalNameFormat = qmlLocalNameFormat;
    qmlExternalNameFormat.setForeground(Qt::darkBlue);
    formatDescr.emplace_back(C_QML_EXTERNAL_ID, Tr::tr("QML External Id"),
                             Tr::tr("QML id defined in another QML file."),
                             qmlExternalNameFormat);
    formatDescr.emplace_back(C_QML_EXTERNAL_OBJECT_PROPERTY,
                             Tr::tr("QML External Object Property"),
                             Tr::tr("QML property defined in another QML file."),
                             qmlExternalNameFormat);

    Format jsLocalFormat;
    jsLocalFormat.setForeground(QColor(41, 133, 199)); // very light blue
    jsLocalFormat.setItalic(true);
    formatDescr.emplace_back(C_JS_SCOPE_VAR, Tr::tr("JavaScript Scope Var"),
                             Tr::tr("Variables defined inside the JavaScript file."),
                             jsLocalFormat);

    Format jsGlobalFormat;
    jsGlobalFormat.setForeground(QColor(0, 85, 175)); // light blue
    jsGlobalFormat.setItalic(true);
    formatDescr.emplace_back(C_JS_IMPORT_VAR, Tr::tr("JavaScript Import"),
                             Tr::tr("Name of a JavaScript import inside a QML file."),
                             jsGlobalFormat);
    formatDescr.emplace_back(C_JS_GLOBAL_VAR, Tr::tr("JavaScript Global Variable"),
                             Tr::tr("Variables defined outside the script."),
                             jsGlobalFormat);

    formatDescr.emplace_back(C_KEYWORD, Tr::tr("Keyword"),
                             Tr::tr("Reserved keywords of the programming language except "
                                "keywords denoting primitive types."), Qt::darkYellow);
    formatDescr.emplace_back(C_PUNCTUATION, Tr::tr("Punctuation"),
                             Tr::tr("Punctuation excluding operators."));
    formatDescr.emplace_back(C_OPERATOR, Tr::tr("Operator"),
                             Tr::tr("Non user-defined language operators.\n"
                                    "To style user-defined operators, use Overloaded Operator."),
                             FormatDescription::ShowAllControls);
    formatDescr.emplace_back(C_OVERLOADED_OPERATOR,
                             Tr::tr("Overloaded Operators"),
                             Tr::tr("Calls and declarations of overloaded (user-defined) operators."),
                             functionFormat,
                             FormatDescription::ShowAllControls);
    formatDescr.emplace_back(C_PREPROCESSOR, Tr::tr("Preprocessor"),
                             Tr::tr("Preprocessor directives."), Qt::darkBlue);
    formatDescr.emplace_back(C_MACRO, Tr::tr("Macro"),
                             Tr::tr("Macros."), functionFormat);
    formatDescr.emplace_back(C_LABEL, Tr::tr("Label"), Tr::tr("Labels for goto statements."),
                             Qt::darkRed);
    formatDescr.emplace_back(C_ATTRIBUTE, Tr::tr("Attribute"), Tr::tr("Attributes."),
                             Qt::darkYellow);
    formatDescr.emplace_back(C_COMMENT, Tr::tr("Comment"),
                             Tr::tr("All style of comments except Doxygen comments."),
                             Qt::darkGreen);
    formatDescr.emplace_back(C_DOXYGEN_COMMENT, Tr::tr("Doxygen Comment"),
                             Tr::tr("Doxygen comments."), Qt::darkBlue);
    formatDescr.emplace_back(C_DOXYGEN_TAG, Tr::tr("Doxygen Tag"), Tr::tr("Doxygen tags."),
                             Qt::blue);
    formatDescr.emplace_back(C_VISUAL_WHITESPACE, Tr::tr("Visual Whitespace"),
                             Tr::tr("Whitespace.\nWill not be applied to whitespace "
                                    "in comments and strings."), Qt::lightGray);
    formatDescr.emplace_back(C_DISABLED_CODE, Tr::tr("Disabled Code"),
                             Tr::tr("Code disabled by preprocessor directives."));

    // Diff categories
    formatDescr.emplace_back(C_ADDED_LINE, Tr::tr("Added Line"),
                             Tr::tr("Applied to added lines in differences (in diff editor)."),
                             QColor(0, 170, 0));
    formatDescr.emplace_back(C_REMOVED_LINE, Tr::tr("Removed Line"),
                             Tr::tr("Applied to removed lines in differences (in diff editor)."),
                             Qt::red);
    formatDescr.emplace_back(C_DIFF_FILE, Tr::tr("Diff File"),
                             Tr::tr("Compared files (in diff editor)."), Qt::darkBlue);
    formatDescr.emplace_back(C_DIFF_LOCATION, Tr::tr("Diff Location"),
                             Tr::tr("Location in the files where the difference is "
                                    "(in diff editor)."), Qt::blue);

    // New diff categories
    formatDescr.emplace_back(C_DIFF_FILE_LINE, Tr::tr("Diff File Line"),
                             Tr::tr("Applied to lines with file information "
                                    "in differences (in side-by-side diff editor)."),
                             Format(QColor(), QColor(255, 255, 0)));
    formatDescr.emplace_back(C_DIFF_CONTEXT_LINE, Tr::tr("Diff Context Line"),
                             Tr::tr("Applied to lines describing hidden context "
                                    "in differences (in side-by-side diff editor)."),
                             Format(QColor(), QColor(175, 215, 231)));
    formatDescr.emplace_back(C_DIFF_SOURCE_LINE, Tr::tr("Diff Source Line"),
                             Tr::tr("Applied to source lines with changes "
                                    "in differences (in side-by-side diff editor)."),
                             Format(QColor(), QColor(255, 223, 223)));
    formatDescr.emplace_back(C_DIFF_SOURCE_CHAR, Tr::tr("Diff Source Character"),
                             Tr::tr("Applied to removed characters "
                                    "in differences (in side-by-side diff editor)."),
                             Format(QColor(), QColor(255, 175, 175)));
    formatDescr.emplace_back(C_DIFF_DEST_LINE, Tr::tr("Diff Destination Line"),
                             Tr::tr("Applied to destination lines with changes "
                                    "in differences (in side-by-side diff editor)."),
                             Format(QColor(), QColor(223, 255, 223)));
    formatDescr.emplace_back(C_DIFF_DEST_CHAR, Tr::tr("Diff Destination Character"),
                             Tr::tr("Applied to added characters "
                                    "in differences (in side-by-side diff editor)."),
                             Format(QColor(), QColor(175, 255, 175)));

    formatDescr.emplace_back(C_LOG_CHANGE_LINE, Tr::tr("Log Change Line"),
                             Tr::tr("Applied to lines describing changes in VCS log."),
                             Format(QColor(192, 0, 0), QColor()));
    formatDescr.emplace_back(C_LOG_AUTHOR_NAME, Tr::tr("Log Author Name"),
                             Tr::tr("Applied to author names in VCS log."),
                             Format(QColor(0x007af4), QColor()));
    formatDescr.emplace_back(C_LOG_COMMIT_DATE, Tr::tr("Log Commit Date"),
                             Tr::tr("Applied to commit dates in VCS log."),
                             Format(QColor(0x006600), QColor()));
    formatDescr.emplace_back(C_LOG_COMMIT_HASH, Tr::tr("Log Commit Hash"),
                             Tr::tr("Applied to commit hashes in VCS log."),
                             Format(QColor(0xff0000), QColor()));
    formatDescr.emplace_back(C_LOG_DECORATION, Tr::tr("Log Decoration"),
                             Tr::tr("Applied to commit decorations in VCS log."),
                             Format(QColor(0xff00ff), QColor()));
    formatDescr.emplace_back(C_LOG_COMMIT_SUBJECT, Tr::tr("Log Commit Subject"),
                             Tr::tr("Applied to commit subjects in VCS log."),
                             Format{QColor{}, QColor{}});

    // Mixin categories
    formatDescr.emplace_back(C_ERROR,
                             Tr::tr("Error"),
                             Tr::tr("Underline color of error diagnostics."),
                             QColor(255,0, 0),
                             QTextCharFormat::SingleUnderline,
                             FormatDescription::ShowAllControls);
    formatDescr.emplace_back(C_ERROR_CONTEXT,
                             Tr::tr("Error Context"),
                             Tr::tr("Underline color of the contexts of error diagnostics."),
                             QColor(255,0, 0),
                             QTextCharFormat::DotLine,
                             FormatDescription::ShowAllControls);
    formatDescr.emplace_back(C_WARNING,
                             Tr::tr("Warning"),
                             Tr::tr("Underline color of warning diagnostics."),
                             QColor(255, 190, 0),
                             QTextCharFormat::SingleUnderline,
                             FormatDescription::ShowAllControls);
    formatDescr.emplace_back(C_WARNING_CONTEXT,
                             Tr::tr("Warning Context"),
                             Tr::tr("Underline color of the contexts of warning diagnostics."),
                             QColor(255, 190, 0),
                             QTextCharFormat::DotLine,
                             FormatDescription::ShowAllControls);
    formatDescr.emplace_back(C_INFO,
                             Tr::tr("Info"),
                             Tr::tr("Underline color of info diagnostics."),
                             QColor(38, 32, 136),
                             QTextCharFormat::DashUnderline,
                             FormatDescription::ShowAllControls);
    formatDescr.emplace_back(C_INFO_CONTEXT,
                             Tr::tr("Info Context"),
                             Tr::tr("Underline color of the contexts of info diagnostics."),
                             QColor(38, 32, 136),
                             QTextCharFormat::DotLine,
                             FormatDescription::ShowAllControls);
    Format outputArgumentFormat;
    outputArgumentFormat.setItalic(true);
    formatDescr.emplace_back(C_OUTPUT_ARGUMENT,
                             Tr::tr("Output Argument"),
                             Tr::tr("Writable arguments of a function call."),
                             outputArgumentFormat,
                             FormatDescription::ShowAllControls);
    formatDescr.emplace_back(C_STATIC_MEMBER,
                             Tr::tr("Static Member"),
                             Tr::tr("Names of static fields or member functions."),
                             FormatDescription::ShowAllControls);

    const auto cocoControls = FormatDescription::ShowControls(
        FormatDescription::ShowAllAbsoluteControls | FormatDescription::ShowRelativeControls);
    formatDescr.emplace_back(C_COCO_CODE_ADDED,
                             Tr::tr("Code Coverage Added Code"),
                             Tr::tr("New code that was not checked for tests."),
                             cocoControls);
    formatDescr.emplace_back(C_COCO_PARTIALLY_COVERED,
                             Tr::tr("Partially Covered Code"),
                             Tr::tr("Partial branch/condition coverage."),
                             Qt::darkYellow,
                             cocoControls);
    formatDescr.emplace_back(C_COCO_NOT_COVERED,
                             Tr::tr("Uncovered Code"),
                             Tr::tr("Not covered at all."),
                             Qt::red,
                             cocoControls);
    formatDescr.emplace_back(C_COCO_FULLY_COVERED,
                             Tr::tr("Fully Covered Code"),
                             Tr::tr("Fully covered code."),
                             Qt::green,
                             cocoControls);
    formatDescr.emplace_back(C_COCO_MANUALLY_VALIDATED,
                             Tr::tr("Manually Validated Code"),
                             Tr::tr("User added validation."),
                             Qt::blue,
                             cocoControls);
    formatDescr.emplace_back(C_COCO_DEAD_CODE,
                             Tr::tr("Code Coverage Dead Code"),
                             Tr::tr("Unreachable code."),
                             Qt::magenta,
                             cocoControls);
    formatDescr.emplace_back(C_COCO_EXECUTION_COUNT_TOO_LOW,
                             Tr::tr("Code Coverage Execution Count Too Low"),
                             Tr::tr("Minimum count not reached."),
                             Qt::red,
                             cocoControls);
    formatDescr.emplace_back(C_COCO_NOT_COVERED_INFO,
                             Tr::tr("Implicitly Not Covered Code"),
                             Tr::tr("PLACEHOLDER"),
                             Qt::red,
                             cocoControls);
    formatDescr.emplace_back(C_COCO_COVERED_INFO,
                             Tr::tr("Implicitly Covered Code"),
                             Tr::tr("PLACEHOLDER"),
                             Qt::green,
                             cocoControls);
    formatDescr.emplace_back(C_COCO_MANUALLY_VALIDATED_INFO,
                             Tr::tr("Implicit Manual Coverage Validation"),
                             Tr::tr("PLACEHOLDER"),
                             Qt::blue,
                             cocoControls);

    return formatDescr;
}

} // namespace Internal

} // namespace TextEditor
