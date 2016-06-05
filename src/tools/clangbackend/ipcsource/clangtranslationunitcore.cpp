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

#include "clangtranslationunitcore.h"
#include "clangtranslationunitupdater.h"

#include <codecompleter.h>
#include <cursor.h>
#include <diagnosticcontainer.h>
#include <diagnosticset.h>
#include <highlightingmark.h>
#include <highlightingmarks.h>
#include <skippedsourceranges.h>
#include <sourcelocation.h>
#include <sourcerange.h>

namespace ClangBackEnd {

TranslationUnitCore::TranslationUnitCore(const Utf8String &filepath,
                                         CXIndex &cxIndex,
                                         CXTranslationUnit &cxTranslationUnit)
    : m_filePath(filepath)
    , m_cxIndex(cxIndex)
    , m_cxTranslationUnit(cxTranslationUnit)
{
}

bool TranslationUnitCore::isNull() const
{
    return !m_cxTranslationUnit || !m_cxIndex || m_filePath.isEmpty();
}

Utf8String TranslationUnitCore::filePath() const
{
    return m_filePath;
}

CXIndex &TranslationUnitCore::cxIndex() const
{
    return m_cxIndex;
}

CXTranslationUnit &TranslationUnitCore::cxTranslationUnit() const
{
    return m_cxTranslationUnit;
}

TranslationUnitUpdateResult TranslationUnitCore::update(
        const TranslationUnitUpdateInput &parseInput) const
{
    TranslationUnitUpdater updater(cxIndex(), cxTranslationUnit(), parseInput);

    return updater.update(TranslationUnitUpdater::UpdateMode::AsNeeded);
}

TranslationUnitUpdateResult TranslationUnitCore::parse(
        const TranslationUnitUpdateInput &parseInput) const
{
    TranslationUnitUpdater updater(cxIndex(), cxTranslationUnit(), parseInput);

    return updater.update(TranslationUnitUpdater::UpdateMode::ParseIfNeeded);
}

TranslationUnitUpdateResult TranslationUnitCore::reparse(
        const TranslationUnitUpdateInput &parseInput) const
{
    TranslationUnitUpdater updater(cxIndex(), cxTranslationUnit(), parseInput);

    return updater.update(TranslationUnitUpdater::UpdateMode::ForceReparse);
}

TranslationUnitCore::CodeCompletionResult TranslationUnitCore::complete(
        UnsavedFiles &unsavedFiles,
        uint line,
        uint column) const
{
    CodeCompleter codeCompleter(*this, unsavedFiles);

    const CodeCompletions completions = codeCompleter.complete(line, column);
    const CompletionCorrection correction = codeCompleter.neededCorrection();

    return CodeCompletionResult{completions, correction};
}

void TranslationUnitCore::extractDocumentAnnotations(
        QVector<DiagnosticContainer> &diagnostics,
        QVector<HighlightingMarkContainer> &highlightingMarks,
        QVector<SourceRangeContainer> &skippedSourceRanges) const
{
    diagnostics = mainFileDiagnostics();
    highlightingMarks = this->highlightingMarks().toHighlightingMarksContainers();
    skippedSourceRanges = this->skippedSourceRanges().toSourceRangeContainers();
}

DiagnosticSet TranslationUnitCore::diagnostics() const
{
    return DiagnosticSet(clang_getDiagnosticSetFromTU(m_cxTranslationUnit));
}

QVector<DiagnosticContainer> TranslationUnitCore::mainFileDiagnostics() const
{
    const auto isMainFileDiagnostic = [this](const Diagnostic &diagnostic) {
        return diagnostic.location().filePath() == m_filePath;
    };

    return diagnostics().toDiagnosticContainers(isMainFileDiagnostic);
}

SourceLocation TranslationUnitCore::sourceLocationAt(uint line,uint column) const
{
    return SourceLocation(m_cxTranslationUnit, m_filePath, line, column);
}

SourceLocation TranslationUnitCore::sourceLocationAt(const Utf8String &filePath,
                                                     uint line,
                                                     uint column) const
{
    return SourceLocation(m_cxTranslationUnit, filePath, line, column);
}

SourceRange TranslationUnitCore::sourceRange(uint fromLine,
                                             uint fromColumn,
                                             uint toLine,
                                             uint toColumn) const
{
    return SourceRange(sourceLocationAt(fromLine, fromColumn),
                       sourceLocationAt(toLine, toColumn));
}

Cursor TranslationUnitCore::cursorAt(uint line, uint column) const
{
    return clang_getCursor(m_cxTranslationUnit, sourceLocationAt(line, column));
}

Cursor TranslationUnitCore::cursorAt(const Utf8String &filePath,
                                     uint line,
                                     uint column) const
{
    return clang_getCursor(m_cxTranslationUnit, sourceLocationAt(filePath, line, column));
}

Cursor TranslationUnitCore::cursor() const
{
    return clang_getTranslationUnitCursor(m_cxTranslationUnit);
}

HighlightingMarks TranslationUnitCore::highlightingMarks() const
{
    return highlightingMarksInRange(cursor().sourceRange());
}

HighlightingMarks TranslationUnitCore::highlightingMarksInRange(const SourceRange &range) const
{
    CXToken *cxTokens = 0;
    uint cxTokensCount = 0;

    clang_tokenize(m_cxTranslationUnit, range, &cxTokens, &cxTokensCount);

    return HighlightingMarks(m_cxTranslationUnit, cxTokens, cxTokensCount);
}

SkippedSourceRanges TranslationUnitCore::skippedSourceRanges() const
{
    return SkippedSourceRanges(m_cxTranslationUnit, m_filePath.constData());
}

} // namespace ClangBackEnd
