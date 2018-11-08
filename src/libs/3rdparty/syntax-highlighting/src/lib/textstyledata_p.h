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

#ifndef KSYNTAXHIGHLIGHTING_TEXTSTYLEDATA_P_H
#define KSYNTAXHIGHLIGHTING_TEXTSTYLEDATA_P_H

#include <QColor>

namespace KSyntaxHighlighting {

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
    {}

    QRgb textColor = 0x0;
    QRgb backgroundColor = 0x0;
    QRgb selectedTextColor = 0x0;
    QRgb selectedBackgroundColor = 0x0;
    bool bold :1;
    bool italic :1;
    bool underline :1;
    bool strikeThrough :1;

    bool hasBold :1;
    bool hasItalic :1;
    bool hasUnderline :1;
    bool hasStrikeThrough :1;
};

}

#endif
