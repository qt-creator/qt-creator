/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include <texteditor/textstyles.h>

namespace TextEditor {

class HighlightingResult {
public:
    unsigned line;
    unsigned column;
    unsigned length;
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

    HighlightingResult(unsigned line, unsigned column, unsigned length, int kind)
        : line(line), column(column), length(length), kind(kind), useTextSyles(false)
    {}

    HighlightingResult(unsigned line, unsigned column, unsigned length, TextStyles textStyles)
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
