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

#include "clangfollowsymbol.h"
#include "clangeditordocumentprocessor.h"
#include "texteditor/texteditor.h"
#include "texteditor/convenience.h"

namespace ClangCodeModel {
namespace Internal {

TextEditor::TextEditorWidget::Link ClangFollowSymbol::findLink(
        const CppTools::CursorInEditor &data,
        bool resolveTarget,
        const CPlusPlus::Snapshot &,
        const CPlusPlus::Document::Ptr &,
        CppTools::SymbolFinder *,
        bool)
{
    Link link;

    int lineNumber = 0, positionInBlock = 0;
    QTextCursor cursor = TextEditor::Convenience::wordStartCursor(data.cursor());
    TextEditor::Convenience::convertPosition(cursor.document(), cursor.position(), &lineNumber,
                                             &positionInBlock);
    const unsigned line = lineNumber;
    const unsigned column = positionInBlock + 1;

    if (!resolveTarget)
        return link;
    ClangEditorDocumentProcessor *processor = ClangEditorDocumentProcessor::get(
                data.filePath().toString());
    if (!processor)
        return link;

    QFuture<CppTools::SymbolInfo> info
            = processor->requestFollowSymbol(static_cast<int>(line),
                                             static_cast<int>(column),
                                             resolveTarget);
    if (info.isCanceled())
        return link;

    while (!info.isFinished()) {
        if (info.isCanceled())
            return link;
        QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
    }
    CppTools::SymbolInfo result = info.result();

    if (result.failedToFollow)
        return link;

    // We did not fail but the result is empty
    if (result.fileName.isEmpty())
        return link;

    return Link(result.fileName, result.startLine, result.startColumn - 1);
}

} // namespace Internal
} // namespace ClangCodeModel
