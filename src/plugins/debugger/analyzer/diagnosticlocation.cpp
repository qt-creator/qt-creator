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

#include "diagnosticlocation.h"

namespace Debugger {

DiagnosticLocation::DiagnosticLocation() : line(0), column(0)
{
}

DiagnosticLocation::DiagnosticLocation(const QString &filePath, int line, int column)
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

QDebug operator<<(QDebug dbg, const DiagnosticLocation &location)
{
    dbg.nospace() << "Location(" << location.filePath << ", "
                  << location.line << ", "
                  << location.column << ')';
    return dbg.space();
}

} // namespace Debugger

