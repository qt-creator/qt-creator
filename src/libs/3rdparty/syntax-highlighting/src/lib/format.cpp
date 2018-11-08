/*
    Copyright (C) 2016 Volker Krause <vkrause@kde.org>

    Permission is hereby granted, free of charge, to any person obtaining
    a copy of this software and associated documentation files (the
    "Software"), to deal in the Software without restriction, including
    without limitation the rights to use, copy, modify, merge, publish,
    distribute, sublicense, and/or sell copies of the Software, and to
    permit persons to whom the Software is furnished to do so, subject to
    the following conditions:

    The above copyright notice and this permission notice shall be included
    in all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
    EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
    IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
    CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
    TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
    SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include "format.h"
#include "format_p.h"
#include "definition.h"
#include "definitionref_p.h"
#include "textstyledata_p.h"
#include "themedata_p.h"
#include "xml_p.h"

#include <QColor>
#include <QDebug>
#include <QMetaEnum>
#include <QXmlStreamReader>

using namespace KSyntaxHighlighting;

static Theme::TextStyle stringToDefaultFormat(const QStringRef &str)
{
    if (!str.startsWith(QLatin1String("ds")))
        return Theme::Normal;

    static const auto idx = Theme::staticMetaObject.indexOfEnumerator("TextStyle");
    Q_ASSERT(idx >= 0);
    const auto metaEnum = Theme::staticMetaObject.enumerator(idx);

    bool ok = false;
    const auto value = metaEnum.keyToValue(str.mid(2).toLatin1().constData(), &ok);
    if (!ok || value < 0)
        return Theme::Normal;
    return static_cast<Theme::TextStyle>(value);
}

FormatPrivate* FormatPrivate::detachAndGet(Format &format)
{
    format.d.detach();
    return format.d.data();
}

TextStyleData FormatPrivate::styleOverride(const Theme &theme) const
{
    const auto themeData = ThemeData::get(theme);
    if (themeData)
        return themeData->textStyleOverride(definition.definition().name(), name);
    return TextStyleData();
}

static QExplicitlySharedDataPointer<FormatPrivate> &sharedDefaultPrivate()
{
    static QExplicitlySharedDataPointer<FormatPrivate> def(new FormatPrivate);
    return def;
}

Format::Format() : d(sharedDefaultPrivate())
{
}

Format::Format(const Format &other) :
    d(other.d)
{
}

Format::~Format()
{
}

Format& Format::operator=(const Format& other)
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
    return (!hasTextColor(theme))
        && (!hasBackgroundColor(theme))
        && (selectedTextColor(theme) == theme.selectedTextColor(Theme::Normal))
        && (selectedBackgroundColor(theme) == theme.selectedBackgroundColor(Theme::Normal))
        && (isBold(theme) == theme.isBold(Theme::Normal))
        && (isItalic(theme) == theme.isItalic(Theme::Normal))
        && (isUnderline(theme) == theme.isUnderline(Theme::Normal))
        && (isStrikeThrough(theme) == theme.isStrikeThrough(Theme::Normal));
}

bool Format::hasTextColor(const Theme &theme) const
{
    const auto overrideStyle = d->styleOverride(theme);
    return textColor(theme) != theme.textColor(Theme::Normal)
        && (d->style.textColor || theme.textColor(d->defaultStyle) || overrideStyle.textColor);
}

QColor Format::textColor(const Theme &theme) const
{
    const auto overrideStyle = d->styleOverride(theme);
    if (overrideStyle.textColor)
        return overrideStyle.textColor;
    return d->style.textColor ? d->style.textColor : theme.textColor(d->defaultStyle);
}

QColor Format::selectedTextColor(const Theme &theme) const
{
    const auto overrideStyle = d->styleOverride(theme);
    if (overrideStyle.selectedTextColor)
        return overrideStyle.selectedTextColor;
    return d->style.selectedTextColor ? d->style.selectedTextColor : theme.selectedTextColor(d->defaultStyle);
}

bool Format::hasBackgroundColor(const Theme &theme) const
{
    const auto overrideStyle = d->styleOverride(theme);
    return backgroundColor(theme) != theme.backgroundColor(Theme::Normal)
         && (d->style.backgroundColor || theme.backgroundColor(d->defaultStyle) || overrideStyle.backgroundColor);
}

QColor Format::backgroundColor(const Theme &theme) const
{
    const auto overrideStyle = d->styleOverride(theme);
    if (overrideStyle.backgroundColor)
        return overrideStyle.backgroundColor;
    return d->style.backgroundColor ? d->style.backgroundColor : theme.backgroundColor(d->defaultStyle);
}

QColor Format::selectedBackgroundColor(const Theme &theme) const
{
    const auto overrideStyle = d->styleOverride(theme);
    if (overrideStyle.selectedBackgroundColor)
        return overrideStyle.selectedBackgroundColor;
    return d->style.selectedBackgroundColor ? d->style.selectedBackgroundColor
                            : theme.selectedBackgroundColor(d->defaultStyle);
}

bool Format::isBold(const Theme &theme) const
{
    const auto overrideStyle = d->styleOverride(theme);
    if (overrideStyle.hasBold)
        return overrideStyle.bold;
    return d->style.hasBold ? d->style.bold : theme.isBold(d->defaultStyle);
}

bool Format::isItalic(const Theme &theme) const
{
    const auto overrideStyle = d->styleOverride(theme);
    if (overrideStyle.hasItalic)
        return overrideStyle.italic;
    return d->style.hasItalic ? d->style.italic : theme.isItalic(d->defaultStyle);
}

bool Format::isUnderline(const Theme &theme) const
{
    const auto overrideStyle = d->styleOverride(theme);
    if (overrideStyle.hasUnderline)
        return overrideStyle.underline;
    return d->style.hasUnderline ? d->style.underline : theme.isUnderline(d->defaultStyle);
}

bool Format::isStrikeThrough(const Theme &theme) const
{
    const auto overrideStyle = d->styleOverride(theme);
    if (overrideStyle.hasStrikeThrough)
        return overrideStyle.strikeThrough;
    return d->style.hasStrikeThrough ? d->style.strikeThrough : theme.isStrikeThrough(d->defaultStyle);
}

bool Format::spellCheck() const
{
    return d->spellCheck;
}


void FormatPrivate::load(QXmlStreamReader& reader)
{
    name = reader.attributes().value(QStringLiteral("name")).toString();
    defaultStyle = stringToDefaultFormat(reader.attributes().value(QStringLiteral("defStyleNum")));

    QStringRef ref = reader.attributes().value(QStringLiteral("color"));
    if (!ref.isEmpty()) {
        style.textColor = QColor(ref.toString()).rgba();
    }

    ref = reader.attributes().value(QStringLiteral("selColor"));
    if (!ref.isEmpty()) {
        style.selectedTextColor = QColor(ref.toString()).rgba();
    }

    ref = reader.attributes().value(QStringLiteral("backgroundColor"));
    if (!ref.isEmpty()) {
        style.backgroundColor = QColor(ref.toString()).rgba();
    }

    ref = reader.attributes().value(QStringLiteral("selBackgroundColor"));
    if (!ref.isEmpty()) {
        style.selectedBackgroundColor = QColor(ref.toString()).rgba();
    }

    ref = reader.attributes().value(QStringLiteral("italic"));
    if (!ref.isEmpty()) {
        style.hasItalic = true;
        style.italic = Xml::attrToBool(ref);
    }

    ref = reader.attributes().value(QStringLiteral("bold"));
    if (!ref.isEmpty()) {
        style.hasBold = true;
        style.bold = Xml::attrToBool(ref);
    }

    ref = reader.attributes().value(QStringLiteral("underline"));
    if (!ref.isEmpty()) {
        style.hasUnderline = true;
        style.underline = Xml::attrToBool(ref);
    }

    ref = reader.attributes().value(QStringLiteral("strikeOut"));
    if (!ref.isEmpty()) {
        style.hasStrikeThrough = true;
        style.strikeThrough = Xml::attrToBool(ref);
    }

    ref = reader.attributes().value(QStringLiteral("spellChecking"));
    if (!ref.isEmpty()) {
        spellCheck = Xml::attrToBool(ref);
    }
}
