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

#pragma once

#include <clangbackendipc/codecompletion.h>

#include <utf8string.h>

#include <clang-c/Index.h>

class Utf8String;

namespace ClangBackEnd {

class Cursor;
class DiagnosticContainer;
class DiagnosticSet;
class HighlightingMarkContainer;
class HighlightingMarks;
class SkippedSourceRanges;
class SourceLocation;
class SourceRange;
class SourceRangeContainer;
class TranslationUnitUpdateInput;
class TranslationUnitUpdateResult;
class UnsavedFiles;

class TranslationUnit
{
public:
    struct CodeCompletionResult {
        CodeCompletions completions;
        CompletionCorrection correction;
    };

public:
    TranslationUnit(const Utf8String &id,
                    const Utf8String &filePath,
                    CXIndex &cxIndex,
                    CXTranslationUnit &cxTranslationUnit);

    bool isNull() const;

    Utf8String id() const;

    Utf8String filePath() const;
    CXIndex &cxIndex() const;
    CXTranslationUnit &cxTranslationUnit() const;

    TranslationUnitUpdateResult update(const TranslationUnitUpdateInput &parseInput) const;
    TranslationUnitUpdateResult parse(const TranslationUnitUpdateInput &parseInput) const;
    TranslationUnitUpdateResult reparse(const TranslationUnitUpdateInput &parseInput) const;

    CodeCompletionResult complete(UnsavedFiles &unsavedFiles, uint line, uint column) const;

    void extractDiagnostics(DiagnosticContainer &firstHeaderErrorDiagnostic,
                            QVector<DiagnosticContainer> &mainFileDiagnostics) const;
    void extractDocumentAnnotations(DiagnosticContainer &firstHeaderErrorDiagnostic,
                                    QVector<DiagnosticContainer> &mainFileDiagnostics,
                                    QVector<HighlightingMarkContainer> &highlightingMarks,
                                    QVector<SourceRangeContainer> &skippedSourceRanges) const;

    DiagnosticSet diagnostics() const;

    SourceLocation sourceLocationAt(uint line, uint column) const;
    SourceLocation sourceLocationAt(const Utf8String &filePath, uint line, uint column) const;
    SourceRange sourceRange(uint fromLine, uint fromColumn, uint toLine, uint toColumn) const;

    Cursor cursorAt(uint line, uint column) const;
    Cursor cursorAt(const Utf8String &filePath, uint line, uint column) const;
    Cursor cursor() const;

    HighlightingMarks highlightingMarks() const;
    HighlightingMarks highlightingMarksInRange(const SourceRange &range) const;

    SkippedSourceRanges skippedSourceRanges() const;

private:
    const Utf8String m_id;
    const Utf8String m_filePath;
    CXIndex &m_cxIndex;
    CXTranslationUnit &m_cxTranslationUnit;
};

} // namespace ClangBackEnd
