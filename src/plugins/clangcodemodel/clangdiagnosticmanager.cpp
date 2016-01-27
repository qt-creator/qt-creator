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

#include "clangdiagnosticfilter.h"
#include "clangdiagnosticmanager.h"
#include "clangisdiagnosticrelatedtolocation.h"

#include <texteditor/convenience.h>
#include <texteditor/fontsettings.h>
#include <texteditor/textdocument.h>
#include <texteditor/texteditorsettings.h>

#include <utils/fileutils.h>
#include <utils/qtcassert.h>

#include <QTextBlock>

namespace  {

QTextEdit::ExtraSelection createExtraSelections(const QTextCharFormat &mainformat,
                                                const QTextCursor &cursor)
{
    QTextEdit::ExtraSelection extraSelection;

    extraSelection.format = mainformat;
    extraSelection.cursor = cursor;

    return extraSelection;
}

int positionInText(QTextDocument *textDocument,
                   const ClangBackEnd::SourceLocationContainer &sourceLocationContainer)
{
    auto textBlock = textDocument->findBlockByNumber(int(sourceLocationContainer.line()) - 1);

    return textBlock.position() + int(sourceLocationContainer.column()) - 1;
}

void addRangeSelections(const ClangBackEnd::DiagnosticContainer &diagnostic,
                        QTextDocument *textDocument,
                        const QTextCharFormat &contextFormat,
                        QList<QTextEdit::ExtraSelection> &extraSelections)
{
    for (auto &&range : diagnostic.ranges()) {
        QTextCursor cursor(textDocument);
        cursor.setPosition(positionInText(textDocument, range.start()));
        cursor.setPosition(positionInText(textDocument, range.end()), QTextCursor::KeepAnchor);

        auto extraSelection = createExtraSelections(contextFormat, cursor);

        extraSelections.push_back(std::move(extraSelection));
    }
}

QTextCursor createSelectionCursor(QTextDocument *textDocument,
                                  const ClangBackEnd::SourceLocationContainer &sourceLocationContainer)
{
    QTextCursor cursor(textDocument);
    cursor.setPosition(positionInText(textDocument, sourceLocationContainer));
    cursor.movePosition(QTextCursor::EndOfWord, QTextCursor::KeepAnchor);

    if (!cursor.hasSelection()) {
        cursor.setPosition(positionInText(textDocument, sourceLocationContainer) - 1);
        cursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, 2);
    }

    return cursor;
}

void addSelections(const QVector<ClangBackEnd::DiagnosticContainer> &diagnostics,
                   QTextDocument *textDocument,
                   const QTextCharFormat &mainFormat,
                   const QTextCharFormat &contextFormat,
                   QList<QTextEdit::ExtraSelection> &extraSelections)
{
    for (auto &&diagnostic : diagnostics) {
        auto cursor = createSelectionCursor(textDocument, diagnostic.location());
        auto extraSelection = createExtraSelections(mainFormat, cursor);

        addRangeSelections(diagnostic, textDocument, contextFormat, extraSelections);

        extraSelections.push_back(std::move(extraSelection));
    }
}

void addWarningSelections(const QVector<ClangBackEnd::DiagnosticContainer> &diagnostics,
                          QTextDocument *textDocument,
                          QList<QTextEdit::ExtraSelection> &extraSelections)
{
    const auto fontSettings = TextEditor::TextEditorSettings::instance()->fontSettings();

    QTextCharFormat warningFormat = fontSettings.toTextCharFormat(TextEditor::C_WARNING);

    QTextCharFormat warningContextFormat = fontSettings.toTextCharFormat(TextEditor::C_WARNING_CONTEXT);

    addSelections(diagnostics, textDocument, warningFormat, warningContextFormat, extraSelections);
}

void addErrorSelections(const QVector<ClangBackEnd::DiagnosticContainer> &diagnostics,
                        QTextDocument *textDocument,
                        QList<QTextEdit::ExtraSelection> &extraSelections)
{
    const auto fontSettings = TextEditor::TextEditorSettings::instance()->fontSettings();

    QTextCharFormat errorFormat = fontSettings.toTextCharFormat(TextEditor::C_ERROR);
    QTextCharFormat errorContextFormat = fontSettings.toTextCharFormat(TextEditor::C_ERROR_CONTEXT);

    addSelections(diagnostics, textDocument, errorFormat, errorContextFormat, extraSelections);
}

ClangBackEnd::SourceLocationContainer toSourceLocation(QTextDocument *textDocument, int position)
{
    int line, column;
    if (TextEditor::Convenience::convertPosition(textDocument, position, &line, &column))
        return ClangBackEnd::SourceLocationContainer(Utf8String(), line, column);

    return ClangBackEnd::SourceLocationContainer();
}

ClangBackEnd::SourceRangeContainer toSourceRange(const QTextCursor &cursor)
{
    using namespace ClangBackEnd;

    QTextDocument *textDocument = cursor.document();

    return SourceRangeContainer(toSourceLocation(textDocument, cursor.anchor()),
                                toSourceLocation(textDocument, cursor.position()));
}

bool isDiagnosticAtLocation(const ClangBackEnd::DiagnosticContainer &diagnostic,
                            uint line,
                            uint column,
                            QTextDocument *textDocument)
{
    using namespace ClangCodeModel::Internal;

    const ClangBackEnd::SourceLocationContainer &location = diagnostic.location();
    const QTextCursor cursor = createSelectionCursor(textDocument, location);
    const ClangBackEnd::SourceRangeContainer cursorRange = toSourceRange(cursor);

    return isDiagnosticRelatedToLocation(diagnostic, {cursorRange}, line, column);
}

QVector<ClangBackEnd::DiagnosticContainer>
filteredDiagnosticsAtLocation(const QVector<ClangBackEnd::DiagnosticContainer> &diagnostics,
                              uint line,
                              uint column,
                              QTextDocument *textDocument)
{
    QVector<ClangBackEnd::DiagnosticContainer> filteredDiagnostics;

    foreach (const auto &diagnostic, diagnostics) {
        if (isDiagnosticAtLocation(diagnostic, line, column, textDocument))
            filteredDiagnostics.append(diagnostic);
    }

    return filteredDiagnostics;
}

bool editorDocumentProcessorHasDiagnosticAt(
        const QVector<ClangBackEnd::DiagnosticContainer> &diagnostics,
        uint line,
        uint column,
        QTextDocument *textDocument)
{
    foreach (const auto &diagnostic, diagnostics) {
        if (isDiagnosticAtLocation(diagnostic, line, column, textDocument))
            return true;
    }

    return false;
}

} // anonymous

namespace ClangCodeModel {
namespace Internal {

ClangDiagnosticManager::ClangDiagnosticManager(TextEditor::TextDocument *textDocument)
    : m_textDocument(textDocument)
{
}

void ClangDiagnosticManager::generateTextMarks()
{
    m_clangTextMarks.clear();
    m_clangTextMarks.reserve(m_warningDiagnostics.size() + m_errorDiagnostics.size());

    addClangTextMarks(m_warningDiagnostics);
    addClangTextMarks(m_errorDiagnostics);
}

QList<QTextEdit::ExtraSelection> ClangDiagnosticManager::takeExtraSelections()
{
    auto extraSelections = m_extraSelections;

    m_extraSelections.clear();

    return extraSelections;
}

bool ClangDiagnosticManager::hasDiagnosticsAt(uint line, uint column) const
{
    QTextDocument *textDocument = m_textDocument->document();

    return editorDocumentProcessorHasDiagnosticAt(m_errorDiagnostics, line, column, textDocument)
        || editorDocumentProcessorHasDiagnosticAt(m_warningDiagnostics, line, column, textDocument);
}

QVector<ClangBackEnd::DiagnosticContainer>
ClangDiagnosticManager::diagnosticsAt(uint line, uint column) const
{
    QTextDocument *textDocument = m_textDocument->document();

    QVector<ClangBackEnd::DiagnosticContainer> diagnostics;
    diagnostics += filteredDiagnosticsAtLocation(m_errorDiagnostics, line, column, textDocument);
    diagnostics += filteredDiagnosticsAtLocation(m_warningDiagnostics, line, column, textDocument);

    return diagnostics;
}

void ClangDiagnosticManager::clearDiagnosticsWithFixIts()
{
    m_fixItdiagnostics.clear();
}

void ClangDiagnosticManager::generateEditorSelections()
{
    m_extraSelections.clear();
    m_extraSelections.reserve(int(m_warningDiagnostics.size() + m_errorDiagnostics.size()));

    addWarningSelections(m_warningDiagnostics, m_textDocument->document(), m_extraSelections);
    addErrorSelections(m_errorDiagnostics, m_textDocument->document(), m_extraSelections);
}

void ClangDiagnosticManager::processNewDiagnostics(
        const QVector<ClangBackEnd::DiagnosticContainer> &allDiagnostics)
{
    filterDiagnostics(allDiagnostics);

    generateTextMarks();
    generateEditorSelections();
}

const QVector<ClangBackEnd::DiagnosticContainer> &
ClangDiagnosticManager::diagnosticsWithFixIts() const
{
    return m_fixItdiagnostics;
}

void ClangDiagnosticManager::addClangTextMarks(
        const QVector<ClangBackEnd::DiagnosticContainer> &diagnostics)
{
    QTC_ASSERT(m_clangTextMarks.size() + diagnostics.size() <= m_clangTextMarks.capacity(), return);

    for (auto &&diagnostic : diagnostics) {
        m_clangTextMarks.emplace_back(filePath(),
                                      diagnostic.location().line(),
                                      diagnostic.severity());

        ClangTextMark &textMark = m_clangTextMarks.back();

        textMark.setBaseTextDocument(m_textDocument);

        m_textDocument->addMark(&textMark);
    }
}

QString ClangDiagnosticManager::filePath() const
{
    return m_textDocument->filePath().toString();
}

void ClangDiagnosticManager::filterDiagnostics(
        const QVector<ClangBackEnd::DiagnosticContainer> &diagnostics)
{
    ClangDiagnosticFilter filter(filePath());
    filter.filter(diagnostics);

    m_warningDiagnostics = filter.takeWarnings();
    m_errorDiagnostics = filter.takeErrors();
    m_fixItdiagnostics = filter.takeFixIts();
}

} // namespace Internal
} // namespace ClangCodeModel
