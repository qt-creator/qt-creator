// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "linecolumn.h"

#include <QRegularExpression>

namespace Utils {

/*!
    Returns the line and column of a \a fileName and sets the \a postfixPos if
    it can find a positional postfix.

    The following patterns are supported: \c {filepath.txt:19},
    \c{filepath.txt:19:12}, \c {filepath.txt+19},
    \c {filepath.txt+19+12}, and \c {filepath.txt(19)}.
*/

LineColumn LineColumn::extractFromFileName(QStringView fileName, int &postfixPos)
{
    static const auto regexp = QRegularExpression("[:+](\\d+)?([:+](\\d+)?)?$");
    // (10) MSVC-style
    static const auto vsRegexp = QRegularExpression("[(]((\\d+)[)]?)?$");
    const QRegularExpressionMatch match = regexp.match(fileName);
    LineColumn lineColumn;
    if (match.hasMatch()) {
        postfixPos = match.capturedStart(0);
        lineColumn.line = 0; // for the case that there's only a : at the end
        if (match.lastCapturedIndex() > 0) {
            lineColumn.line = match.captured(1).toInt();
            if (match.lastCapturedIndex() > 2) // index 2 includes the + or : for the column number
                lineColumn.column = match.captured(3).toInt() - 1; //column is 0 based, despite line being 1 based
        }
    } else {
        const QRegularExpressionMatch vsMatch = vsRegexp.match(fileName);
        postfixPos = vsMatch.capturedStart(0);
        if (vsMatch.lastCapturedIndex() > 1) // index 1 includes closing )
            lineColumn.line = vsMatch.captured(2).toInt();
    }
    return lineColumn;
}

} // namespace Utils
