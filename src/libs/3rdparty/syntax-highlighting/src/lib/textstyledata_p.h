/*
    SPDX-FileCopyrightText: 2016 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: MIT
*/

#ifndef KSYNTAXHIGHLIGHTING_TEXTSTYLEDATA_P_H
#define KSYNTAXHIGHLIGHTING_TEXTSTYLEDATA_P_H

#include <QColor>

namespace KSyntaxHighlighting
{
class TextStyleData
{
public:
    // Constructor initializing all data.
    TextStyleData()
        : bold(false)
        , italic(false)
        , underline(false)
        , strikeThrough(false)
        , hasBold(false)
        , hasItalic(false)
        , hasUnderline(false)
        , hasStrikeThrough(false)
    {
    }

    QRgb textColor = 0x0;
    QRgb backgroundColor = 0x0;
    QRgb selectedTextColor = 0x0;
    QRgb selectedBackgroundColor = 0x0;
    bool bold : 1;
    bool italic : 1;
    bool underline : 1;
    bool strikeThrough : 1;

    bool hasBold : 1;
    bool hasItalic : 1;
    bool hasUnderline : 1;
    bool hasStrikeThrough : 1;
};

}

#endif
