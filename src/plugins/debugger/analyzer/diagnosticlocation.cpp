// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "diagnosticlocation.h"

namespace Debugger {

DiagnosticLocation::DiagnosticLocation() = default;

DiagnosticLocation::DiagnosticLocation(const Utils::FilePath &filePath, int line, int column)
    : filePath(filePath), line(line), column(column)
{
}

bool DiagnosticLocation::isValid() const
{
    return !filePath.isEmpty();
}

bool operator==(const DiagnosticLocation &first, const DiagnosticLocation &second)
{
    return first.filePath == second.filePath
            && first.line == second.line
            && first.column == second.column;
}

bool operator<(const DiagnosticLocation &first, const DiagnosticLocation &second)
{
    return std::tie(first.filePath, first.line, first.column)
           < std::tie(second.filePath, second.line, second.column);
}

QDebug operator<<(QDebug dbg, const DiagnosticLocation &location)
{
    dbg.nospace() << "Location(" << location.filePath << ", "
                  << location.line << ", "
                  << location.column << ')';
    return dbg.space();
}

} // namespace Debugger

