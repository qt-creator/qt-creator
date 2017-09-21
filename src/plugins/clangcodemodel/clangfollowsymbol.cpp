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

#include "clangeditordocumentprocessor.h"
#include "clangfollowsymbol.h"

#include <texteditor/texteditor.h>

#include <clangsupport/highlightingmarkcontainer.h>

#include <utils/textutils.h>
#include <utils/algorithm.h>

namespace ClangCodeModel {
namespace Internal {

// Returns invalid Mark if it is not found at (line, column)
static bool findMark(const QVector<ClangBackEnd::HighlightingMarkContainer> &marks,
                     uint line,
                     uint column,
                     ClangBackEnd::HighlightingMarkContainer &mark)
{
    mark = Utils::findOrDefault(marks,
        [line, column](const ClangBackEnd::HighlightingMarkContainer &curMark) {
            if (curMark.line() != line)
                return false;
            if (curMark.column() == column)
                return true;
            if (curMark.column() < column && curMark.column() + curMark.length() >= column)
                return true;
            return false;
        });
    if (mark.isInvalid())
        return false;
    return true;
}

static int getMarkPos(QTextCursor cursor, const ClangBackEnd::HighlightingMarkContainer &mark)
{
    cursor.setPosition(0);
    cursor.movePosition(QTextCursor::NextBlock, QTextCursor::MoveAnchor, mark.line() - 1);
    cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::MoveAnchor, mark.column() - 1);
    return cursor.position();
}

static TextEditor::TextEditorWidget::Link linkAtCursor(QTextCursor cursor,
                                                       const QString &filePath,
                                                       uint line,
                                                       uint column,
                                                       ClangEditorDocumentProcessor *processor)
{
    using Link = TextEditor::TextEditorWidget::Link;

    const QVector<ClangBackEnd::HighlightingMarkContainer> &marks
            = processor->highlightingMarks();
    ClangBackEnd::HighlightingMarkContainer mark;
    if (!findMark(marks, line, column, mark))
        return Link();

    cursor.movePosition(QTextCursor::EndOfWord, QTextCursor::KeepAnchor);
    const QString tokenStr = cursor.selectedText();

    Link token(filePath, mark.line(), mark.column());
    token.linkTextStart = getMarkPos(cursor, mark);
    token.linkTextEnd = token.linkTextStart + mark.length();
    if (mark.isIncludeDirectivePath()) {
        if (tokenStr != "include" && tokenStr != "#" && tokenStr != "<")
            return token;
        return Link();
    }
    if (mark.isIdentifier() || tokenStr == "operator")
        return token;
    return Link();
}

TextEditor::TextEditorWidget::Link ClangFollowSymbol::findLink(
        const CppTools::CursorInEditor &data,
        bool resolveTarget,
        const CPlusPlus::Snapshot &,
        const CPlusPlus::Document::Ptr &,
        CppTools::SymbolFinder *,
        bool)
{
    int lineNumber = 0, positionInBlock = 0;
    QTextCursor cursor = Utils::Text::wordStartCursor(data.cursor());
    Utils::Text::convertPosition(cursor.document(), cursor.position(), &lineNumber,
                                 &positionInBlock);

    const uint line = lineNumber;
    const uint column = positionInBlock + 1;

    ClangEditorDocumentProcessor *processor = ClangEditorDocumentProcessor::get(
                data.filePath().toString());
    if (!processor)
        return Link();

    if (!resolveTarget)
        return linkAtCursor(cursor, data.filePath().toString(), line, column, processor);

    QFuture<CppTools::SymbolInfo> info
            = processor->requestFollowSymbol(static_cast<int>(line),
                                             static_cast<int>(column));

    if (info.isCanceled())
        return Link();

    while (!info.isFinished()) {
        if (info.isCanceled())
            return Link();
        QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
    }
    CppTools::SymbolInfo result = info.result();

    // We did not fail but the result is empty
    if (result.fileName.isEmpty())
        return Link();

    return Link(result.fileName, result.startLine, result.startColumn - 1);
}

} // namespace Internal
} // namespace ClangCodeModel
