/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
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

#include <stringcachefwd.h>

#include <limits>
#include <vector>
#include <iosfwd>

using uint = unsigned int;

namespace ClangBackEnd {

enum class SymbolType
{
    Declaration,
    DeclarationReference
};

class LineColumn
{
public:
    LineColumn(uint line, uint column)
        : line(line),
          column(column)
    {}

    uint line =  0;
    uint column = 0;
};

using SymbolIndex = long long;

class SourceLocationEntry
{
public:
    SourceLocationEntry(SymbolIndex symbolId,
                        FilePathIndex fileId,
                        LineColumn lineColumn,
                        SymbolType symbolType)
        : symbolId(symbolId),
          fileId(fileId),
          line(lineColumn.line),
          column(lineColumn.column),
          symbolType(symbolType)
    {}

    SymbolIndex symbolId = 0;
    FilePathIndex fileId = std::numeric_limits<uint>::max();
    uint line =  0;
    uint column = 0;
    SymbolType symbolType;

    friend bool operator==(const SourceLocationEntry &first, const SourceLocationEntry &second)
    {
        return first.symbolId == second.symbolId
            && first.fileId == second.fileId
            && first.line == second.line
            && first.column == second.column
            && first.symbolType == second.symbolType;
    }
};

using SourceLocationEntries = std::vector<SourceLocationEntry>;

std::ostream &operator<<(std::ostream &out, const SourceLocationEntry &entry);

} // namespace ClangBackEnd
