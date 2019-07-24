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

#include <coreplugin/editormanager/editormanager.h>
#include <cpptools/cppmodelmanager.h>
#include <cpptools/cppfollowsymbolundercursor.h>
#include <texteditor/texteditor.h>

#include <clangsupport/tokeninfocontainer.h>

#include <utils/textutils.h>
#include <utils/algorithm.h>

#include <memory>

namespace ClangCodeModel {
namespace Internal {

// Returns invalid Mark if it is not found at (line, column)
static bool findMark(const QVector<ClangBackEnd::TokenInfoContainer> &marks,
                     int line,
                     int column,
                     ClangBackEnd::TokenInfoContainer &mark)
{
    mark = Utils::findOrDefault(marks,
        [line, column](const ClangBackEnd::TokenInfoContainer &curMark) {
            if (curMark.line != line)
                return false;
            if (curMark.column == column)
                return true;
            if (curMark.column < column && curMark.column + curMark.length > column)
                return true;
            return false;
        });
    if (mark.isInvalid())
        return false;
    return true;
}

static int getMarkPos(QTextCursor cursor, const ClangBackEnd::TokenInfoContainer &mark)
{
    cursor.setPosition(0);
    cursor.movePosition(QTextCursor::NextBlock, QTextCursor::MoveAnchor, mark.line - 1);
    cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::MoveAnchor, mark.column - 1);
    return cursor.position();
}

static bool isValidIncludePathToken(const ClangBackEnd::TokenInfoContainer &token)
{
    if (!token.extraInfo.includeDirectivePath)
        return false;
    const Utf8String &tokenName = token.extraInfo.token;
    return !tokenName.startsWith("include") && tokenName != "<" && tokenName != ">"
            && tokenName != "#";
}

static int includePathStartIndex(const QVector<ClangBackEnd::TokenInfoContainer> &marks,
                                 int currentIndex)
{
    int startIndex = currentIndex - 1;
    while (startIndex >= 0 && isValidIncludePathToken(marks[startIndex]))
        --startIndex;
    return startIndex + 1;
}

static int includePathEndIndex(const QVector<ClangBackEnd::TokenInfoContainer> &marks,
                               int currentIndex)
{
    int endIndex = currentIndex + 1;
    while (endIndex < marks.size() && isValidIncludePathToken(marks[endIndex]))
        ++endIndex;
    return endIndex - 1;
}

static Utils::Link linkAtCursor(const QTextCursor &cursor,
                                const QString &filePath,
                                uint line,
                                uint column,
                                ClangEditorDocumentProcessor *processor)
{
    using Link = Utils::Link;

    const QVector<ClangBackEnd::TokenInfoContainer> &marks
            = processor->tokenInfos();
    ClangBackEnd::TokenInfoContainer mark;
    if (!findMark(marks, line, column, mark))
        return Link();

    if (mark.extraInfo.includeDirectivePath && !isValidIncludePathToken(mark))
        return Link();

    Link token(filePath, mark.line, mark.column);
    token.linkTextStart = getMarkPos(cursor, mark);
    token.linkTextEnd = token.linkTextStart + mark.length;

    if (mark.extraInfo.includeDirectivePath) {
        // Tweak include paths to cover everything between "" or <>.
        if (mark.extraInfo.token.startsWith("\"")) {
            token.linkTextStart++;
            token.linkTextEnd--;
        } else {
            // '#include <path/file.h>' case. Clang gives us a separate token for each part of
            // the path. We want to have the full range instead therefore we search for < and >
            // tokens around the current token.
            const int index = marks.indexOf(mark);
            const int startIndex = includePathStartIndex(marks, index);
            const int endIndex = includePathEndIndex(marks, index);

            if (startIndex != index)
                token.linkTextStart = getMarkPos(cursor, marks[startIndex]);
            if (endIndex != index)
                token.linkTextEnd = getMarkPos(cursor, marks[endIndex]) + marks[endIndex].length;
        }
        return token;
    }

    if (mark.extraInfo.identifier || mark.extraInfo.token == "operator"
            || mark.extraInfo.token == "auto") {
        return token;
    }
    return Link();
}

static ::Utils::ProcessLinkCallback extendedCallback(::Utils::ProcessLinkCallback &&callback,
                                                     const CppTools::SymbolInfo &result)
{
    // If globalFollowSymbol finds nothing follow to the declaration.
    return [original_callback = std::move(callback), result](const ::Utils::Link &link) {
        if (link.linkTextStart < 0 && result.isResultOnlyForFallBack) {
            return original_callback(::Utils::Link(result.fileName, result.startLine,
                                          result.startColumn - 1));
        }
        return original_callback(link);
    };
}

static bool isSameInvocationContext(const Utils::FilePath &filePath)
{
    return TextEditor::BaseTextEditor::currentTextEditor()->editorWidget()->isVisible()
        && Core::EditorManager::currentDocument()->filePath() == filePath;
}

void ClangFollowSymbol::findLink(const CppTools::CursorInEditor &data,
                                 ::Utils::ProcessLinkCallback &&processLinkCallback,
                                 bool resolveTarget,
                                 const CPlusPlus::Snapshot &snapshot,
                                 const CPlusPlus::Document::Ptr &documentFromSemanticInfo,
                                 CppTools::SymbolFinder *symbolFinder,
                                 bool inNextSplit)
{
    int line = 0;
    int column = 0;
    QTextCursor cursor = Utils::Text::wordStartCursor(data.cursor());
    Utils::Text::convertPosition(cursor.document(), cursor.position(), &line, &column);

    ClangEditorDocumentProcessor *processor = ClangEditorDocumentProcessor::get(
                data.filePath().toString());
    if (!processor)
        return processLinkCallback(Utils::Link());

    if (!resolveTarget) {
        Utils::Link link = linkAtCursor(cursor,
                                        data.filePath().toString(),
                                        static_cast<uint>(line),
                                        static_cast<uint>(column),
                                        processor);
        if (link == Utils::Link()) {
            CppTools::FollowSymbolUnderCursor followSymbol;
            return followSymbol.findLink(data,
                                         std::move(processLinkCallback),
                                         false,
                                         snapshot,
                                         documentFromSemanticInfo,
                                         symbolFinder,
                                         inNextSplit);
        }
        return processLinkCallback(link);
    }

    QFuture<CppTools::SymbolInfo> infoFuture
            = processor->requestFollowSymbol(static_cast<int>(line),
                                             static_cast<int>(column));

    if (infoFuture.isCanceled())
        return processLinkCallback(Utils::Link());

    if (m_watcher)
        m_watcher->cancel();

    m_watcher = std::make_unique<FutureSymbolWatcher>();

    QObject::connect(m_watcher.get(), &FutureSymbolWatcher::finished, [=, filePath=data.filePath(),
                     callback=std::move(processLinkCallback)]() mutable {
        if (m_watcher->isCanceled() || !isSameInvocationContext(filePath))
            return callback(Utils::Link());
        CppTools::SymbolInfo result = m_watcher->result();
        // We did not fail but the result is empty
        if (result.fileName.isEmpty() || result.isResultOnlyForFallBack) {
            const CppTools::RefactoringEngineInterface &refactoringEngine
                    = *CppTools::CppModelManager::instance();
            refactoringEngine.globalFollowSymbol(data,
                                                 extendedCallback(std::move(callback), result),
                                                 snapshot,
                                                 documentFromSemanticInfo,
                                                 symbolFinder,
                                                 inNextSplit);
        } else {
            callback(Link(result.fileName, result.startLine, result.startColumn - 1));
        }
    });

    m_watcher->setFuture(infoFuture);
}

} // namespace Internal
} // namespace ClangCodeModel
