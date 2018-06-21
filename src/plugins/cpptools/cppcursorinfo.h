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

#include "cpptools_global.h"

#include "cppsemanticinfo.h"

#include <QTextCursor>
#include <QVector>

namespace CppTools {

class CPPTOOLS_EXPORT CursorInfoParams
{
public:
    CppTools::SemanticInfo semanticInfo;
    QTextCursor textCursor;
};

class CPPTOOLS_EXPORT CursorInfo
{
public:
    struct Range {
        Range() = default;
        Range(unsigned line, unsigned column, unsigned length)
            : line(line)
            , column(column)
            , length(length)
        {
        }

        unsigned line = 0; // 1-based
        unsigned column = 0; // 1-based
        unsigned length = 0;
    };
    using Ranges = QVector<Range>;

    Ranges useRanges;
    bool areUseRangesForLocalVariable = false;

    Ranges unusedVariablesRanges;
    SemanticInfo::LocalUseMap localUses;
};

} // namespace CppTools
