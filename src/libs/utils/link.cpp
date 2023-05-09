// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "link.h"

#include "textutils.h"

namespace Utils {

/*!
    Returns the Link to \a filePath.
    If \a canContainLineNumber is true the line number, and column number components
    are extracted from the \a filePath's \c path() and the found \a postfix is set.

    The following patterns are supported: \c {filepath.txt:19},
    \c{filepath.txt:19:12}, \c {filepath.txt+19},
    \c {filepath.txt+19+12}, and \c {filepath.txt(19)}.
*/

Link Link::fromString(const QString &filePathWithNumbers, bool canContainLineNumber)
{
    Link link;
    if (!canContainLineNumber) {
        link.targetFilePath = FilePath::fromUserInput(filePathWithNumbers);
    } else {
        int postfixPos = -1;
        const Text::Position pos = Text::Position::fromFileName(filePathWithNumbers, postfixPos);
        link.targetFilePath = FilePath::fromUserInput(filePathWithNumbers.left(postfixPos));
        link.targetLine = pos.line;
        link.targetColumn = pos.column;
    }
    return link;
}

} // namespace Utils
