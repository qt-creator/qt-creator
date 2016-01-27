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

#include <clangbackendipc/diagnosticcontainer.h>

namespace ClangCodeModel {
namespace Internal {

bool isWithinRange(const ClangBackEnd::SourceRangeContainer &range,
                   uint line,
                   uint column)
{
    const ClangBackEnd::SourceLocationContainer startLocation = range.start();
    const ClangBackEnd::SourceLocationContainer endLocation = range.end();

    return startLocation.line() <= line
        && startLocation.column() <= column
        && line <= endLocation.line()
        && column <= endLocation.column();
}

bool isWithinOneRange(const QVector<ClangBackEnd::SourceRangeContainer> &ranges,
                      uint line,
                      uint column)
{
    foreach (const ClangBackEnd::SourceRangeContainer &range, ranges) {
        if (isWithinRange(range, line, column))
            return true;
    }

    return false;
}

bool isDiagnosticRelatedToLocation(const ClangBackEnd::DiagnosticContainer &diagnostic,
                                   const QVector<ClangBackEnd::SourceRangeContainer> &additionalRanges,
                                   uint line,
                                   uint column)
{
    const ClangBackEnd::SourceLocationContainer location = diagnostic.location();

    if (location.line() == line && location.column() == column)
        return true;

    if (isWithinOneRange(additionalRanges, line, column))
        return true;

    if (isWithinOneRange(diagnostic.ranges(), line, column))
        return true;

    return false;
}

} // namespace Internal
} // namespace ClangCodeModel
