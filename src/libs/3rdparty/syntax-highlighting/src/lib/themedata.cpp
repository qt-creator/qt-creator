/*
    SPDX-FileCopyrightText: 2016 Volker Krause <vkrause@kde.org>
    SPDX-FileCopyrightText: 2016 Dominik Haumann <dhaumann@kde.org>
    SPDX-FileCopyrightText: 2020 Jonathan Poelen <jonathan.poelen@gmail.com>

    SPDX-License-Identifier: MIT
*/

#include "ksyntaxhighlighting_logging.h"
#include "themedata_p.h"

#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QMetaEnum>

using namespace KSyntaxHighlighting;

ThemeData *ThemeData::get(const Theme &theme)
{
    return theme.m_data.data();
}

ThemeData::ThemeData()
{
    memset(m_editorColors, 0, sizeof(m_editorColors));
}

/**
 * Convert QJsonValue @p val into a color, if possible. Valid colors are only
 * in hex format: #aarrggbb. On error, returns 0x00000000.
 */
static inline QRgb readColor(const QJsonValue &val)
{
    const QRgb unsetColor = 0;
    if (!val.isString()) {
        return unsetColor;
    }
    const QString str = val.toString();
    if (str.isEmpty() || str[0] != QLatin1Char('#')) {
        return unsetColor;
    }
    const QColor color(str);
    return color.isValid() ? color.rgba() : unsetColor;
}

static inline TextStyleData readThemeData(const QJsonObject &obj)
{
    TextStyleData td;

    td.textColor = readColor(obj.value(QLatin1String("text-color")));
    td.backgroundColor = readColor(obj.value(QLatin1String("background-color")));
    td.selectedTextColor = readColor(obj.value(QLatin1String("selected-text-color")));
    td.selectedBackgroundColor = readColor(obj.value(QLatin1String("selected-background-color")));

    auto val = obj.value(QLatin1String("bold"));
    if (val.isBool()) {
        td.bold = val.toBool();
        td.hasBold = true;
    }
    val = obj.value(QLatin1String("italic"));
    if (val.isBool()) {
        td.italic = val.toBool();
        td.hasItalic = true;
    }
    val = obj.value(QLatin1String("underline"));
    if (val.isBool()) {
        td.underline = val.toBool();
        td.hasUnderline = true;
    }
    val = obj.value(QLatin1String("strike-through"));
    if (val.isBool()) {
        td.strikeThrough = val.toBool();
        td.hasStrikeThrough = true;
    }

    return td;
}

bool ThemeData::load(const QString &filePath)
{
    QFile loadFile(filePath);
    if (!loadFile.open(QIODevice::ReadOnly)) {
        return false;
    }
    const QByteArray jsonData = loadFile.readAll();

    QJsonParseError parseError;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonData, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        qCWarning(Log) << "Failed to parse theme file" << filePath << ":" << parseError.errorString();
        return false;
    }

    m_filePath = filePath;

    QJsonObject obj = jsonDoc.object();

    // read metadata
    const QJsonObject metadata = obj.value(QLatin1String("metadata")).toObject();
    m_name = metadata.value(QLatin1String("name")).toString();
    m_revision = metadata.value(QLatin1String("revision")).toInt();

    // read text styles
    static const auto styleIdx = Theme::staticMetaObject.indexOfEnumerator("TextStyle");
    Q_ASSERT(styleIdx >= 0);
    const auto metaEnumStyle = Theme::staticMetaObject.enumerator(styleIdx);
    const QJsonObject textStyles = obj.value(QLatin1String("text-styles")).toObject();
    for (int i = 0; i < metaEnumStyle.keyCount(); ++i) {
        Q_ASSERT(i == metaEnumStyle.value(i));
        m_textStyles[i] = readThemeData(textStyles.value(QLatin1String(metaEnumStyle.key(i))).toObject());
    }

    // read editor colors
    static const auto colorIdx = Theme::staticMetaObject.indexOfEnumerator("EditorColorRole");
    Q_ASSERT(colorIdx >= 0);
    const auto metaEnumColor = Theme::staticMetaObject.enumerator(colorIdx);
    const QJsonObject editorColors = obj.value(QLatin1String("editor-colors")).toObject();
    for (int i = 0; i < metaEnumColor.keyCount(); ++i) {
        Q_ASSERT(i == metaEnumColor.value(i));
        m_editorColors[i] = readColor(editorColors.value(QLatin1String(metaEnumColor.key(i))));
    }

    // if we have no new key around for Theme::BackgroundColor => use old variants to be compatible
    if (!editorColors.contains(QLatin1String(metaEnumColor.key(Theme::BackgroundColor)))) {
        m_editorColors[Theme::BackgroundColor] = readColor(editorColors.value(QLatin1String("background-color")));
        m_editorColors[Theme::TextSelection] = readColor(editorColors.value(QLatin1String("selection")));
        m_editorColors[Theme::CurrentLine] = readColor(editorColors.value(QLatin1String("current-line")));
        m_editorColors[Theme::SearchHighlight] = readColor(editorColors.value(QLatin1String("search-highlight")));
        m_editorColors[Theme::ReplaceHighlight] = readColor(editorColors.value(QLatin1String("replace-highlight")));
        m_editorColors[Theme::BracketMatching] = readColor(editorColors.value(QLatin1String("bracket-matching")));
        m_editorColors[Theme::TabMarker] = readColor(editorColors.value(QLatin1String("tab-marker")));
        m_editorColors[Theme::SpellChecking] = readColor(editorColors.value(QLatin1String("spell-checking")));
        m_editorColors[Theme::IndentationLine] = readColor(editorColors.value(QLatin1String("indentation-line")));
        m_editorColors[Theme::IconBorder] = readColor(editorColors.value(QLatin1String("icon-border")));
        m_editorColors[Theme::CodeFolding] = readColor(editorColors.value(QLatin1String("code-folding")));
        m_editorColors[Theme::LineNumbers] = readColor(editorColors.value(QLatin1String("line-numbers")));
        m_editorColors[Theme::CurrentLineNumber] = readColor(editorColors.value(QLatin1String("current-line-number")));
        m_editorColors[Theme::WordWrapMarker] = readColor(editorColors.value(QLatin1String("word-wrap-marker")));
        m_editorColors[Theme::ModifiedLines] = readColor(editorColors.value(QLatin1String("modified-lines")));
        m_editorColors[Theme::SavedLines] = readColor(editorColors.value(QLatin1String("saved-lines")));
        m_editorColors[Theme::Separator] = readColor(editorColors.value(QLatin1String("separator")));
        m_editorColors[Theme::MarkBookmark] = readColor(editorColors.value(QLatin1String("mark-bookmark")));
        m_editorColors[Theme::MarkBreakpointActive] = readColor(editorColors.value(QLatin1String("mark-breakpoint-active")));
        m_editorColors[Theme::MarkBreakpointReached] = readColor(editorColors.value(QLatin1String("mark-breakpoint-reached")));
        m_editorColors[Theme::MarkBreakpointDisabled] = readColor(editorColors.value(QLatin1String("mark-breakpoint-disabled")));
        m_editorColors[Theme::MarkExecution] = readColor(editorColors.value(QLatin1String("mark-execution")));
        m_editorColors[Theme::MarkWarning] = readColor(editorColors.value(QLatin1String("mark-warning")));
        m_editorColors[Theme::MarkError] = readColor(editorColors.value(QLatin1String("mark-error")));
        m_editorColors[Theme::TemplateBackground] = readColor(editorColors.value(QLatin1String("template-background")));
        m_editorColors[Theme::TemplatePlaceholder] = readColor(editorColors.value(QLatin1String("template-placeholder")));
        m_editorColors[Theme::TemplateFocusedPlaceholder] = readColor(editorColors.value(QLatin1String("template-focused-placeholder")));
        m_editorColors[Theme::TemplateReadOnlyPlaceholder] = readColor(editorColors.value(QLatin1String("template-read-only-placeholder")));
    }

    // read per-definition style overrides
    const auto customStyles = obj.value(QLatin1String("custom-styles")).toObject();
    for (auto it = customStyles.begin(); it != customStyles.end(); ++it) {
        const auto obj = it.value().toObject();
        auto &overrideStyle = m_textStyleOverrides[it.key()];
        for (auto it2 = obj.begin(); it2 != obj.end(); ++it2)
            overrideStyle.insert(it2.key(), readThemeData(it2.value().toObject()));
    }

    return true;
}

QString ThemeData::name() const
{
    return m_name;
}

int ThemeData::revision() const
{
    return m_revision;
}

bool ThemeData::isReadOnly() const
{
    return !QFileInfo(m_filePath).isWritable();
}

QString ThemeData::filePath() const
{
    return m_filePath;
}

QRgb ThemeData::textColor(Theme::TextStyle style) const
{
    Q_ASSERT(static_cast<int>(style) >= 0 && static_cast<int>(style) <= static_cast<int>(Theme::Others));
    return m_textStyles[style].textColor;
}

QRgb ThemeData::selectedTextColor(Theme::TextStyle style) const
{
    Q_ASSERT(static_cast<int>(style) >= 0 && static_cast<int>(style) <= static_cast<int>(Theme::Others));
    return m_textStyles[style].selectedTextColor;
}

QRgb ThemeData::backgroundColor(Theme::TextStyle style) const
{
    Q_ASSERT(static_cast<int>(style) >= 0 && static_cast<int>(style) <= static_cast<int>(Theme::Others));
    return m_textStyles[style].backgroundColor;
}

QRgb ThemeData::selectedBackgroundColor(Theme::TextStyle style) const
{
    Q_ASSERT(static_cast<int>(style) >= 0 && static_cast<int>(style) <= static_cast<int>(Theme::Others));
    return m_textStyles[style].selectedBackgroundColor;
}

bool ThemeData::isBold(Theme::TextStyle style) const
{
    Q_ASSERT(static_cast<int>(style) >= 0 && static_cast<int>(style) <= static_cast<int>(Theme::Others));
    return m_textStyles[style].bold;
}

bool ThemeData::isItalic(Theme::TextStyle style) const
{
    Q_ASSERT(static_cast<int>(style) >= 0 && static_cast<int>(style) <= static_cast<int>(Theme::Others));
    return m_textStyles[style].italic;
}

bool ThemeData::isUnderline(Theme::TextStyle style) const
{
    Q_ASSERT(static_cast<int>(style) >= 0 && static_cast<int>(style) <= static_cast<int>(Theme::Others));
    return m_textStyles[style].underline;
}

bool ThemeData::isStrikeThrough(Theme::TextStyle style) const
{
    Q_ASSERT(static_cast<int>(style) >= 0 && static_cast<int>(style) <= static_cast<int>(Theme::Others));
    return m_textStyles[style].strikeThrough;
}

QRgb ThemeData::editorColor(Theme::EditorColorRole role) const
{
    Q_ASSERT(static_cast<int>(role) >= 0 && static_cast<int>(role) <= static_cast<int>(Theme::TemplateReadOnlyPlaceholder));
    return m_editorColors[role];
}

TextStyleData ThemeData::textStyleOverride(const QString &definitionName, const QString &attributeName) const
{
    return m_textStyleOverrides.value(definitionName).value(attributeName);
}
