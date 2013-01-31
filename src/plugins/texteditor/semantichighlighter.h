/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef TEXTEDITOR_SEMANTICHIGHLIGHTER_H
#define TEXTEDITOR_SEMANTICHIGHLIGHTER_H

#include "texteditor_global.h"

#include <QHash>
#include <QMap>
#include <QVector>
#include <QFuture>
#include <QTextCharFormat>

QT_BEGIN_NAMESPACE
class QTextDocument;
QT_END_NAMESPACE

namespace TextEditor {

class SyntaxHighlighter;

namespace SemanticHighlighter {

class TEXTEDITOR_EXPORT Result {
public:
    unsigned line; // 1-based
    unsigned column; // 1-based
    unsigned length;
    int kind;

    bool isValid() const
    { return line != 0; }

    bool isInvalid() const
    { return line == 0; }

    Result()
        : line(0), column(0), length(0), kind(-1) {}
    Result(unsigned line, unsigned column, unsigned length, int kind)
        : line(line), column(column), length(length), kind(kind) {}
};

// Applies the future results [from, to) and applies the extra formats
// indicated by Result::kind and kindToFormat to the correct location using
// SyntaxHighlighter::setExtraAdditionalFormats.
// It is incremental in the sense that it clears the extra additional formats
// from all lines that have no results between the (from-1).line result and
// the (to-1).line result.
// Requires that results of the Future are ordered by line.
void TEXTEDITOR_EXPORT incrementalApplyExtraAdditionalFormats(
        SyntaxHighlighter *highlighter,
        const QFuture<Result> &future,
        int from, int to,
        const QHash<int, QTextCharFormat> &kindToFormat);

// Cleans the extra additional formats after the last result of the Future
// until the end of the document.
// Requires that results of the Future are ordered by line.
void TEXTEDITOR_EXPORT clearExtraAdditionalFormatsUntilEnd(
        SyntaxHighlighter *highlighter,
        const QFuture<Result> &future);


} // namespace SemanticHighlighter
} // namespace TextEditor

#endif // TEXTEDITOR_SEMANTICHIGHLIGHTER_H
