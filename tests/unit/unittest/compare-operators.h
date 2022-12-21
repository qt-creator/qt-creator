// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/find/searchresultitem.h>

namespace Core {
namespace Search {

inline
bool operator==(const TextPosition first, class TextPosition second)
{
    return first.line == second.line
        && first.column == second.column;
}

inline
bool operator==(const TextRange first, class TextRange second)
{
    return first.begin == second.begin
        && first.end == second.end;
}
}
}
