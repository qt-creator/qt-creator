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

#include "optional.h"

#include <QMetaType>

namespace Utils {

class LineColumn
{
public:
    constexpr LineColumn() = default;
    constexpr
    LineColumn(int line, int column)
        : line(line),
          column(column)
    {}

    bool isValid() const
    {
        return line >= 0 && column >= 0;
    }

    friend bool operator==(LineColumn first, LineColumn second)
    {
        return first.isValid() && first.line == second.line && first.column == second.column;
    }

    friend bool operator!=(LineColumn first, LineColumn second)
    {
        return !(first == second);
    }

public:
    int line = -1;
    int column = -1;
};

using OptionalLineColumn = optional<LineColumn>;

} // namespace Utils

Q_DECLARE_METATYPE(Utils::LineColumn)
