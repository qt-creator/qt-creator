/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#include "link.h"

#include <QRegularExpression>

namespace Utils {

/*!
    Returns the Link to \a fileName.
    If \a canContainLineNumber is true the line number, and column number components
    are extracted from \a fileName and the found \a postFix is set.

    The following patterns are supported: \c {filepath.txt:19},
    \c{filepath.txt:19:12}, \c {filepath.txt+19},
    \c {filepath.txt+19+12}, and \c {filepath.txt(19)}.
*/
Link Link::fromString(const QString &fileName, bool canContainLineNumber, QString *postfix)
{
    if (!canContainLineNumber)
        return {Utils::FilePath::fromString(fileName)};
    // :10:2 GCC/Clang-style
    static const auto regexp = QRegularExpression("[:+](\\d+)?([:+](\\d+)?)?$");
    // (10) MSVC-style
    static const auto vsRegexp = QRegularExpression("[(]((\\d+)[)]?)?$");
    const QRegularExpressionMatch match = regexp.match(fileName);
    QString filePath = fileName;
    int line = -1;
    int column = -1;
    if (match.hasMatch()) {
        if (postfix)
            *postfix = match.captured(0);
        filePath = fileName.left(match.capturedStart(0));
        line = 0; // for the case that there's only a : at the end
        if (match.lastCapturedIndex() > 0) {
            line = match.captured(1).toInt();
            if (match.lastCapturedIndex() > 2) // index 2 includes the + or : for the column number
                column = match.captured(3).toInt() - 1; //column is 0 based, despite line being 1 based
        }
    } else {
        const QRegularExpressionMatch vsMatch = vsRegexp.match(fileName);
        if (postfix)
            *postfix = vsMatch.captured(0);
        filePath = fileName.left(vsMatch.capturedStart(0));
        if (vsMatch.lastCapturedIndex() > 1) // index 1 includes closing )
            line = vsMatch.captured(2).toInt();
    }
    return {Utils::FilePath::fromString(filePath), line, column};
}

} // namespace Utils
