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

#include <utils/smallstring.h>

#include <filepathid.h>

#include <vector>

namespace ClangRefactoring {

class SourceLocation
{
public:
    SourceLocation(ClangBackEnd::FilePathId filePathId, int line, int column)
        : filePathId(filePathId), line(line), column(column)
    {}
    SourceLocation(int directoryId, int sourceId, int line, int column)
        : filePathId{directoryId, sourceId}, line(line), column(column)
    {}

    friend bool operator==(SourceLocation first, SourceLocation second)
    {
        return first.filePathId == second.filePathId
            && first.line == second.line
            && first.column == second.column;
    }

public:
    ClangBackEnd::FilePathId filePathId;
    int line;
    int column;
};

using SourceLocations = std::vector<SourceLocation>;

} // namespace ClangRefactoring
