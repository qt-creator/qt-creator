/*
    SPDX-FileCopyrightText: 2016 Volker Krause <vkrause@kde.org>
    SPDX-FileCopyrightText: 2022 Jonathan Poelen <jonathan.poelen@gmail.com>

    SPDX-License-Identifier: MIT
*/

#include "theme.h"
#include "themedata_p.h"

#include <QCoreApplication>

using namespace KSyntaxHighlighting;

static QExplicitlySharedDataPointer<ThemeData> &sharedDefaultThemeData()
{
    static QExplicitlySharedDataPointer<ThemeData> data(new ThemeData);
    return data;
}

Theme::Theme()
    : m_data(sharedDefaultThemeData())
{
}

Theme::Theme(const Theme &copy) = default;

Theme::Theme(ThemeData *data)
    : m_data(data)
{
}

Theme::~Theme() = default;

Theme &Theme::operator=(const Theme &other) = default;

bool Theme::isValid() const
{
    return m_data.data() != sharedDefaultThemeData().data();
}

QString Theme::name() const
{
    return m_data->name();
}

QString Theme::translatedName() const
{
    return isValid() ? QCoreApplication::instance()->translate("Theme", m_data->name().toUtf8().constData()) : QString();
}

bool Theme::isReadOnly() const
{
    return m_data->isReadOnly();
}

QString Theme::filePath() const
{
    return m_data->filePath();
}

QRgb Theme::textColor(TextStyle style) const
{
    return m_data->textColor(style);
}

QRgb Theme::selectedTextColor(TextStyle style) const
{
    return m_data->selectedTextColor(style);
}

QRgb Theme::backgroundColor(TextStyle style) const
{
    return m_data->backgroundColor(style);
}

QRgb Theme::selectedBackgroundColor(TextStyle style) const
{
    return m_data->selectedBackgroundColor(style);
}

bool Theme::isBold(TextStyle style) const
{
    return m_data->isBold(style);
}

bool Theme::isItalic(TextStyle style) const
{
    return m_data->isItalic(style);
}

bool Theme::isUnderline(TextStyle style) const
{
    return m_data->isUnderline(style);
}

bool Theme::isStrikeThrough(TextStyle style) const
{
    return m_data->isStrikeThrough(style);
}

QRgb Theme::editorColor(EditorColorRole role) const
{
    return m_data->editorColor(role);
}
