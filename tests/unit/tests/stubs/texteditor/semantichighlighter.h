// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <texteditor/textstyles.h>

namespace TextEditor {

class HighlightingResult {
public:
    int line;
    int column;
    int length;
    TextStyles textStyles;
    int kind;
    bool useTextSyles;

    bool isValid() const
    { return line != 0; }

    bool isInvalid() const
    { return line == 0; }

    HighlightingResult()
        : line(0), column(0), length(0), kind(0)
    {}

    HighlightingResult(int line, int column, int length, int kind)
        : line(line), column(column), length(length), kind(kind), useTextSyles(false)
    {}

    HighlightingResult(int line, int column, int length, TextStyles textStyles)
        : line(line), column(column), length(length), textStyles(textStyles), useTextSyles(true)
    {}

    bool operator==(const HighlightingResult& other) const
    {
        return line == other.line
                && column == other.column
                && length == other.length
                && kind == other.kind;
    }
};

} // namespace TextEditor
