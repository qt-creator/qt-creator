// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "utils_global.h"

#include <QMetaType>

#include <optional>

namespace Utils {

class QTCREATOR_UTILS_EXPORT LineColumn
{
public:
    constexpr LineColumn() = default;
    constexpr LineColumn(int line, int column) : line(line), column(column) {}

    bool isValid() const
    {
        return line > 0 && column >= 0;
    }

    friend bool operator==(LineColumn first, LineColumn second)
    {
        return first.isValid() && first.line == second.line && first.column == second.column;
    }

    friend bool operator!=(LineColumn first, LineColumn second)
    {
        return !(first == second);
    }

    static LineColumn extractFromFileName(QStringView fileName, int &postfixPos);

public:
    int line = 0;
    int column = -1;
};

} // namespace Utils

Q_DECLARE_METATYPE(Utils::LineColumn)
