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

#include "linecolumn.h"

namespace Utils {

/*!
    Returns the Link to \a fileName.
    If \a canContainLineNumber is true the line number, and column number components
    are extracted from \a fileName and the found \a postfix is set.

    The following patterns are supported: \c {filepath.txt:19},
    \c{filepath.txt:19:12}, \c {filepath.txt+19},
    \c {filepath.txt+19+12}, and \c {filepath.txt(19)}.
*/
Link Link::fromString(const QString &fileName, bool canContainLineNumber, QString *postfix)
{
    if (!canContainLineNumber)
        return {FilePath::fromString(fileName)};
    int postfixPos = -1;
    const LineColumn lineColumn = LineColumn::extractFromFileName(fileName, postfixPos);
    if (postfix && postfixPos >= 0)
        *postfix = fileName.mid(postfixPos);
    return {FilePath::fromString(fileName.left(postfixPos)),
            lineColumn.line,
            lineColumn.column};
}

Link Link::fromFilePath(const FilePath &filePath, bool canContainLineNumber, QString *postfix)
{
    if (!canContainLineNumber)
        return {filePath};
    int postfixPos = -1;
    QString fileName = filePath.path();
    const LineColumn lineColumn = LineColumn::extractFromFileName(fileName, postfixPos);
    if (postfix && postfixPos >= 0)
        *postfix = fileName.mid(postfixPos);
    return Link{filePath.withNewPath(fileName.left(postfixPos)), lineColumn.line, lineColumn.column};
}

uint qHash(const Link &l)
{
    QString s = l.targetFilePath.toString();
    return qHash(s.append(':').append(QString::number(l.targetLine)).append(':')
                 .append(QString::number(l.targetColumn)));
}

} // namespace Utils
