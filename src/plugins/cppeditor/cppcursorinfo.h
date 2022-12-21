// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "cppeditor_global.h"

#include "cppsemanticinfo.h"

#include <QTextCursor>
#include <QVector>

namespace CppEditor {

class CPPEDITOR_EXPORT CursorInfoParams
{
public:
    SemanticInfo semanticInfo;
    QTextCursor textCursor;
};

class CPPEDITOR_EXPORT CursorInfo
{
public:
    struct Range {
        Range() = default;
        Range(int line, int column, int length)
            : line(line)
            , column(column)
            , length(length)
        {
        }

        int line = 0; // 1-based
        int column = 0; // 1-based
        int length = 0;
    };
    using Ranges = QVector<Range>;

    Ranges useRanges;
    bool areUseRangesForLocalVariable = false;

    Ranges unusedVariablesRanges;
    SemanticInfo::LocalUseMap localUses;
};

} // namespace CppEditor
