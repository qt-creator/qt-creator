/*
    SPDX-FileCopyrightText: 2016 Volker Krause <vkrause@kde.org>
    SPDX-FileCopyrightText: 2020 Jonathan Poelen <jonathan.poelen@gmail.com>

    SPDX-License-Identifier: MIT
*/

#include "format.h"
#include "definition.h"
#include "definitionref_p.h"
#include "format_p.h"
#include "textstyledata_p.h"
#include "themedata_p.h"
#include "xml_p.h"

#include <QColor>
#include <QMetaEnum>
#include <QXmlStreamReader>

using namespace KSyntaxHighlighting;

static Theme::TextStyle stringToDefaultFormat(QStringView str)
{
    if (!str.startsWith(QLatin1String("ds"))) {
        return Theme::Normal;
    }

    const auto metaEnum = QMetaEnum::fromType<Theme::TextStyle>();

    bool ok = false;
    const auto value = metaEnum.keyToValue(str.mid(2).toLatin1().constData(), &ok);
    if (!ok || value < 0) {
        return Theme::Normal;
    }
    return static_cast<Theme::TextStyle>(value);
}

FormatPrivate *FormatPrivate::detachAndGet(Format &format)
{
    format.d.detach();
    return format.d.data();
}

TextStyleData FormatPrivate::styleOverride(const Theme &theme) const
{
    return ThemeData::get(theme)->textStyleOverride(definitionName, name);
}

QColor FormatPrivate::color(const Theme &theme, StyleColor styleColor, ThemeColor themeColor) const
{
    const auto overrideStyle = styleOverride(theme);
    if (overrideStyle.*styleColor) {
        return QColor::fromRgb(overrideStyle.*styleColor);
    }
    // use QColor::fromRgba for QRgb => QColor conversion to avoid unset colors == black!
    return QColor::fromRgba(style.*styleColor ? style.*styleColor : (theme.*themeColor)(defaultStyle));
}

bool FormatPrivate::hasColor(const Theme &theme, StyleColor styleColor, ThemeColor themeColor) const
{
    // use QColor::fromRgba for background QRgb => QColor conversion to avoid unset colors == black!
    return color(theme, styleColor, themeColor) != QColor::fromRgba((theme.*themeColor)(Theme::Normal)) && (style.*styleColor || (theme.*themeColor)(defaultStyle) || styleOverride(theme).*styleColor);
}

static QExplicitlySharedDataPointer<FormatPrivate> &sharedDefaultPrivate()
{
    static QExplicitlySharedDataPointer<FormatPrivate> def(new FormatPrivate);
    return def;
}

Format::Format()
    : d(sharedDefaultPrivate())
{
}

Format::Format(const Format &other)
    : d(other.d)
{
}

Format::~Format()
{
}

Format &Format::operator=(const Format &other)
{
    d = other.d;
    return *this;
}

bool Format::isValid() const
{
    return !d->name.isEmpty();
}

QString Format::name() const
{
    return d->name;
}

quint16 Format::id() const
{
    return d->id;
}

Theme::TextStyle Format::textStyle() const
{
    return d->defaultStyle;
}

bool Format::isDefaultTextStyle(const Theme &theme) const
{
    // use QColor::fromRgba for background QRgb => QColor conversion to avoid unset colors == black!
    return (!hasTextColor(theme)) && (!hasBackgroundColor(theme)) && (selectedTextColor(theme).rgba() == theme.selectedTextColor(Theme::Normal))
        && (selectedBackgroundColor(theme).rgba() == (theme.selectedBackgroundColor(Theme::Normal))) && (isBold(theme) == theme.isBold(Theme::Normal))
        && (isItalic(theme) == theme.isItalic(Theme::Normal)) && (isUnderline(theme) == theme.isUnderline(Theme::Normal))
        && (isStrikeThrough(theme) == theme.isStrikeThrough(Theme::Normal));
}

bool Format::hasTextColor(const Theme &theme) const
{
    return d->hasColor(theme, &TextStyleData::textColor, &Theme::textColor);
}

QColor Format::textColor(const Theme &theme) const
{
    return d->color(theme, &TextStyleData::textColor, &Theme::textColor);
}

QColor Format::selectedTextColor(const Theme &theme) const
{
    return d->color(theme, &TextStyleData::selectedTextColor, &Theme::selectedTextColor);
}

bool Format::hasBackgroundColor(const Theme &theme) const
{
    return d->hasColor(theme, &TextStyleData::backgroundColor, &Theme::backgroundColor);
}

QColor Format::backgroundColor(const Theme &theme) const
{
    return d->color(theme, &TextStyleData::backgroundColor, &Theme::backgroundColor);
}

QColor Format::selectedBackgroundColor(const Theme &theme) const
{
    return d->color(theme, &TextStyleData::selectedBackgroundColor, &Theme::selectedBackgroundColor);
}

bool Format::isBold(const Theme &theme) const
{
    const auto overrideStyle = d->styleOverride(theme);
    if (overrideStyle.hasBold) {
        return overrideStyle.bold;
    }
    return d->style.hasBold ? d->style.bold : theme.isBold(d->defaultStyle);
}

bool Format::isItalic(const Theme &theme) const
{
    const auto overrideStyle = d->styleOverride(theme);
    if (overrideStyle.hasItalic) {
        return overrideStyle.italic;
    }
    return d->style.hasItalic ? d->style.italic : theme.isItalic(d->defaultStyle);
}

bool Format::isUnderline(const Theme &theme) const
{
    const auto overrideStyle = d->styleOverride(theme);
    if (overrideStyle.hasUnderline) {
        return overrideStyle.underline;
    }
    return d->style.hasUnderline ? d->style.underline : theme.isUnderline(d->defaultStyle);
}

bool Format::isStrikeThrough(const Theme &theme) const
{
    const auto overrideStyle = d->styleOverride(theme);
    if (overrideStyle.hasStrikeThrough) {
        return overrideStyle.strikeThrough;
    }
    return d->style.hasStrikeThrough ? d->style.strikeThrough : theme.isStrikeThrough(d->defaultStyle);
}

bool Format::spellCheck() const
{
    return d->spellCheck;
}

bool Format::hasBoldOverride() const
{
    return d->style.hasBold;
}

bool Format::hasItalicOverride() const
{
    return d->style.hasItalic;
}

bool Format::hasUnderlineOverride() const
{
    return d->style.hasUnderline;
}

bool Format::hasStrikeThroughOverride() const
{
    return d->style.hasStrikeThrough;
}

bool Format::hasTextColorOverride() const
{
    return d->style.textColor;
}

bool Format::hasBackgroundColorOverride() const
{
    return d->style.backgroundColor;
}

bool Format::hasSelectedTextColorOverride() const
{
    return d->style.selectedTextColor;
}

bool Format::hasSelectedBackgroundColorOverride() const
{
    return d->style.selectedBackgroundColor;
}

void FormatPrivate::load(QXmlStreamReader &reader)
{
    name = reader.attributes().value(QLatin1String("name")).toString();
    defaultStyle = stringToDefaultFormat(reader.attributes().value(QLatin1String("defStyleNum")));

    QStringView attribute = reader.attributes().value(QLatin1String("color"));
    if (!attribute.isEmpty()) {
        style.textColor = QColor(attribute).rgba();
    }

    attribute = reader.attributes().value(QLatin1String("selColor"));
    if (!attribute.isEmpty()) {
        style.selectedTextColor = QColor(attribute).rgba();
    }

    attribute = reader.attributes().value(QLatin1String("backgroundColor"));
    if (!attribute.isEmpty()) {
        style.backgroundColor = QColor(attribute).rgba();
    }

    attribute = reader.attributes().value(QLatin1String("selBackgroundColor"));
    if (!attribute.isEmpty()) {
        style.selectedBackgroundColor = QColor(attribute).rgba();
    }

    attribute = reader.attributes().value(QLatin1String("italic"));
    if (!attribute.isEmpty()) {
        style.hasItalic = true;
        style.italic = Xml::attrToBool(attribute);
    }

    attribute = reader.attributes().value(QLatin1String("bold"));
    if (!attribute.isEmpty()) {
        style.hasBold = true;
        style.bold = Xml::attrToBool(attribute);
    }

    attribute = reader.attributes().value(QLatin1String("underline"));
    if (!attribute.isEmpty()) {
        style.hasUnderline = true;
        style.underline = Xml::attrToBool(attribute);
    }

    attribute = reader.attributes().value(QLatin1String("strikeOut"));
    if (!attribute.isEmpty()) {
        style.hasStrikeThrough = true;
        style.strikeThrough = Xml::attrToBool(attribute);
    }

    attribute = reader.attributes().value(QLatin1String("spellChecking"));
    if (!attribute.isEmpty()) {
        spellCheck = Xml::attrToBool(attribute);
    }
}
