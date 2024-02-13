/*
    SPDX-FileCopyrightText: 2016 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: MIT
*/

#ifndef KSYNTAXHIGHLIGHTING_FORMAT_P_H
#define KSYNTAXHIGHLIGHTING_FORMAT_P_H

#include "textstyledata_p.h"
#include "theme.h"

#include <QSharedData>
#include <QString>

QT_BEGIN_NAMESPACE
class QXmlStreamReader;
QT_END_NAMESPACE

namespace KSyntaxHighlighting
{
class FormatPrivate : public QSharedData
{
public:
    FormatPrivate() = default;
    static FormatPrivate *detachAndGet(Format &format);

    static std::intptr_t ptrId(const Format &format)
    {
        return std::intptr_t(format.d.data());
    }

    TextStyleData styleOverride(const Theme &theme) const;
    void load(QXmlStreamReader &reader);

    using StyleColor = QRgb(TextStyleData::*);
    using ThemeColor = QRgb (Theme::*)(Theme::TextStyle) const;
    bool hasColor(const Theme &theme, StyleColor styleColor, ThemeColor themeColor) const;
    QColor color(const Theme &theme, StyleColor styleColor, ThemeColor themeColor) const;

    QString definitionName;
    QString name;
    TextStyleData style;
    Theme::TextStyle defaultStyle = Theme::Normal;
    int id = 0;
    bool spellCheck = true;
};

}

#endif
