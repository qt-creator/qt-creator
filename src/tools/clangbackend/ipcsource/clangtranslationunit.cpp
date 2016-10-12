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

#include "clangtranslationunit.h"
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

TranslationUnit::TranslationUnit(const Utf8String &id,
                                 const Utf8String &filepath,
                                 CXIndex &cxIndex,
                                 CXTranslationUnit &cxTranslationUnit)
    : m_id(id)
    , m_filePath(filepath)
    , m_cxIndex(cxIndex)
    , m_cxTranslationUnit(cxTranslationUnit)
{
}

bool TranslationUnit::isNull() const
{
    return !m_cxTranslationUnit || !m_cxIndex || m_filePath.isEmpty() || m_id.isEmpty();
}

Utf8String TranslationUnit::id() const
{
    return m_id;
}

Utf8String TranslationUnit::filePath() const
{
    return m_filePath;
}

CXIndex &TranslationUnit::cxIndex() const
{
    return m_cxIndex;
}

CXTranslationUnit &TranslationUnit::cxTranslationUnit() const
{
    return m_cxTranslationUnit;
}

TranslationUnitUpdateResult TranslationUnit::update(
        const TranslationUnitUpdateInput &parseInput) const
{
    TranslationUnitUpdater updater(id(), cxIndex(), cxTranslationUnit(), parseInput);

    return updater.update(TranslationUnitUpdater::UpdateMode::AsNeeded);
}

TranslationUnitUpdateResult TranslationUnit::parse(
        const TranslationUnitUpdateInput &parseInput) const
{
    TranslationUnitUpdater updater(id(), cxIndex(), cxTranslationUnit(), parseInput);

    return updater.update(TranslationUnitUpdater::UpdateMode::ParseIfNeeded);
}

TranslationUnitUpdateResult TranslationUnit::reparse(
        const TranslationUnitUpdateInput &parseInput) const
{
    TranslationUnitUpdater updater(id(), cxIndex(), cxTranslationUnit(), parseInput);

    return updater.update(TranslationUnitUpdater::UpdateMode::ForceReparse);
}

TranslationUnit::CodeCompletionResult TranslationUnit::complete(
        UnsavedFiles &unsavedFiles,
        uint line,
        uint column) const
{
    CodeCompleter codeCompleter(*this, unsavedFiles);

    const CodeCompletions completions = codeCompleter.complete(line, column);
    const CompletionCorrection correction = codeCompleter.neededCorrection();

    return CodeCompletionResult{completions, correction};
}

void TranslationUnit::extractDocumentAnnotations(
        DiagnosticContainer &firstHeaderErrorDiagnostic,
        QVector<DiagnosticContainer> &mainFileDiagnostics,
        QVector<HighlightingMarkContainer> &highlightingMarks,
        QVector<SourceRangeContainer> &skippedSourceRanges) const
{
    extractDiagnostics(firstHeaderErrorDiagnostic, mainFileDiagnostics);
    highlightingMarks = this->highlightingMarks().toHighlightingMarksContainers();
    skippedSourceRanges = this->skippedSourceRanges().toSourceRangeContainers();
}

DiagnosticSet TranslationUnit::diagnostics() const
{
    return DiagnosticSet(clang_getDiagnosticSetFromTU(m_cxTranslationUnit));
}

SourceLocation TranslationUnit::sourceLocationAt(uint line,uint column) const
{
    return SourceLocation(m_cxTranslationUnit, m_filePath, line, column);
}

SourceLocation TranslationUnit::sourceLocationAt(const Utf8String &filePath,
                                                 uint line,
                                                 uint column) const
{
    return SourceLocation(m_cxTranslationUnit, filePath, line, column);
}

SourceRange TranslationUnit::sourceRange(uint fromLine,
                                         uint fromColumn,
                                         uint toLine,
                                         uint toColumn) const
{
    return SourceRange(sourceLocationAt(fromLine, fromColumn),
                       sourceLocationAt(toLine, toColumn));
}

Cursor TranslationUnit::cursorAt(uint line, uint column) const
{
    return clang_getCursor(m_cxTranslationUnit, sourceLocationAt(line, column));
}

Cursor TranslationUnit::cursorAt(const Utf8String &filePath,
                                     uint line,
                                     uint column) const
{
    return clang_getCursor(m_cxTranslationUnit, sourceLocationAt(filePath, line, column));
}

Cursor TranslationUnit::cursor() const
{
    return clang_getTranslationUnitCursor(m_cxTranslationUnit);
}

HighlightingMarks TranslationUnit::highlightingMarks() const
{
    return highlightingMarksInRange(cursor().sourceRange());
}

HighlightingMarks TranslationUnit::highlightingMarksInRange(const SourceRange &range) const
{
    CXToken *cxTokens = 0;
    uint cxTokensCount = 0;

    clang_tokenize(m_cxTranslationUnit, range, &cxTokens, &cxTokensCount);

    return HighlightingMarks(m_cxTranslationUnit, cxTokens, cxTokensCount);
}

SkippedSourceRanges TranslationUnit::skippedSourceRanges() const
{
    return SkippedSourceRanges(m_cxTranslationUnit, m_filePath.constData());
}

static bool isMainFileDiagnostic(const Utf8String &mainFilePath, const Diagnostic &diagnostic)
{
    return diagnostic.location().filePath() == mainFilePath;
}

static bool isHeaderErrorDiagnostic(const Utf8String &mainFilePath, const Diagnostic &diagnostic)
{
    const bool isCritical = diagnostic.severity() == DiagnosticSeverity::Error
                         || diagnostic.severity() == DiagnosticSeverity::Fatal;
    return isCritical && diagnostic.location().filePath() != mainFilePath;
}

void TranslationUnit::extractDiagnostics(DiagnosticContainer &firstHeaderErrorDiagnostic,
                                         QVector<DiagnosticContainer> &mainFileDiagnostics) const
{
    mainFileDiagnostics.clear();
    mainFileDiagnostics.reserve(int(diagnostics().size()));

    bool hasFirstHeaderErrorDiagnostic = false;

    for (const Diagnostic &diagnostic : diagnostics()) {
        if (!hasFirstHeaderErrorDiagnostic && isHeaderErrorDiagnostic(m_filePath, diagnostic)) {
            hasFirstHeaderErrorDiagnostic = true;
            firstHeaderErrorDiagnostic = diagnostic.toDiagnosticContainer();
        }

        if (isMainFileDiagnostic(m_filePath, diagnostic))
            mainFileDiagnostics.push_back(diagnostic.toDiagnosticContainer());
    }
}

} // namespace ClangBackEnd
