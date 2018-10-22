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

#include "clangbackend_global.h"
#include "clangreferencescollector.h"
#include "clangtooltipinfocollector.h"
#include "clangtranslationunitupdater.h"
#include "clangfollowsymbol.h"
#include "clangfollowsymboljob.h"
#include "tokenprocessor.h"

#include <codecompleter.h>
#include <cursor.h>
#include <diagnosticcontainer.h>
#include <diagnosticset.h>
#include <skippedsourceranges.h>
#include <sourcelocation.h>
#include <sourcerange.h>
#include <commandlinearguments.h>

#include <utils/qtcassert.h>

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

bool TranslationUnit::suspend() const
{
    return clang_suspendTranslationUnit(cxTranslationUnit());
}

CodeCompletions TranslationUnit::complete(UnsavedFiles &unsavedFiles, uint line, uint column,
                                          int funcNameStartLine, int funcNameStartColumn) const
{
    return CodeCompleter(*this, unsavedFiles).complete(line, column, funcNameStartLine,
                                                       funcNameStartColumn);
}

void TranslationUnit::extractAnnotations(
        DiagnosticContainer &firstHeaderErrorDiagnostic,
        QVector<DiagnosticContainer> &mainFileDiagnostics,
        QVector<TokenInfoContainer> &tokenInfos,
        QVector<SourceRangeContainer> &skippedSourceRanges) const
{
    extractDiagnostics(firstHeaderErrorDiagnostic, mainFileDiagnostics);
    tokenInfos = this->tokenInfos().toTokenInfoContainers();
    skippedSourceRanges = this->skippedSourceRanges().toSourceRangeContainers();
}

ToolTipInfo TranslationUnit::tooltip(UnsavedFiles &unsavedFiles,
                                     const Utf8String &textCodecName,
                                     uint line,
                                     uint column) const
{
    return collectToolTipInfo(unsavedFiles,
                              textCodecName,
                              filePath(),
                              m_cxTranslationUnit,
                              line,
                              column);
}

ReferencesResult TranslationUnit::references(uint line, uint column, bool localReferences) const
{
    return collectReferences(m_cxTranslationUnit, line, column, localReferences);
}

DiagnosticSet TranslationUnit::diagnostics() const
{
    return DiagnosticSet(m_cxTranslationUnit, clang_getDiagnosticSetFromTU(m_cxTranslationUnit));
}

SourceLocation TranslationUnit::sourceLocationAt(uint line,uint column) const
{
    return SourceLocation(m_cxTranslationUnit,
                          clang_getLocation(m_cxTranslationUnit,
                                            clang_getFile(m_cxTranslationUnit,
                                                          m_filePath.constData()),
                                            line, column));
}

SourceLocation TranslationUnit::sourceLocationAt(const Utf8String &filePath,
                                                 uint line,
                                                 uint column) const
{
    return SourceLocation(m_cxTranslationUnit,
                          clang_getLocation(m_cxTranslationUnit,
                                            clang_getFile(m_cxTranslationUnit,
                                                          filePath.constData()),
                                            line, column));
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

TokenProcessor<TokenInfo> TranslationUnit::tokenInfos() const
{
    return tokenInfosInRange(cursor().sourceRange());
}

TokenProcessor<TokenInfo> TranslationUnit::tokenInfosInRange(const SourceRange &range) const
{
    return TokenProcessor<TokenInfo>(range);
}

TokenProcessor<FullTokenInfo> TranslationUnit::fullTokenInfos() const
{
    return fullTokenInfosInRange(cursor().sourceRange());
}

TokenProcessor<FullTokenInfo> TranslationUnit::fullTokenInfosInRange(const SourceRange &range) const
{
    return TokenProcessor<FullTokenInfo>(range);
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

FollowSymbolResult TranslationUnit::followSymbol(uint line, uint column) const
{
    return FollowSymbol::followSymbol(m_cxTranslationUnit, cursorAt(line, column), line, column);
}

} // namespace ClangBackEnd
