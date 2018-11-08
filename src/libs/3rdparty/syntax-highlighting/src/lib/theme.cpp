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

#include "theme.h"
#include "themedata_p.h"

#include <QCoreApplication>

using namespace KSyntaxHighlighting;

Theme::Theme()
{
}

Theme::Theme(const Theme &copy)
{
    m_data = copy.m_data;
}

Theme::Theme(ThemeData* data)
    : m_data(data)
{
}

Theme::~Theme()
{
}

Theme &Theme::operator=(const Theme &other)
{
    m_data = other.m_data;
    return *this;
}

bool Theme::isValid() const
{
    return m_data.data();
}

QString Theme::name() const
{
    return m_data ? m_data->name() : QString();
}

QString Theme::translatedName() const
{
    return m_data ? QCoreApplication::instance()->translate("Theme", m_data->name().toUtf8().constData())
                  : QString();
}

bool Theme::isReadOnly() const
{
    return m_data ? m_data->isReadOnly() : false;
}

QString Theme::filePath() const
{
    return m_data ? m_data->filePath() : QString();
}

QRgb Theme::textColor(TextStyle style) const
{
    return m_data ? m_data->textColor(style) : 0;
}

QRgb Theme::selectedTextColor(TextStyle style) const
{
    return m_data ? m_data->selectedTextColor(style) : 0;
}

QRgb Theme::backgroundColor(TextStyle style) const
{
    return m_data ? m_data->backgroundColor(style) : 0;
}

QRgb Theme::selectedBackgroundColor(TextStyle style) const
{
    return m_data ? m_data->selectedBackgroundColor(style) : 0;
}

bool Theme::isBold(TextStyle style) const
{
    return m_data ? m_data->isBold(style) : false;
}

bool Theme::isItalic(TextStyle style) const
{
    return m_data ? m_data->isItalic(style) : false;
}

bool Theme::isUnderline(TextStyle style) const
{
    return m_data ? m_data->isUnderline(style) : false;
}

bool Theme::isStrikeThrough(TextStyle style) const
{
    return m_data ? m_data->isStrikeThrough(style) : false;
}

QRgb Theme::editorColor(EditorColorRole role) const
{
    return m_data ? m_data->editorColor(role) : 0;
}
