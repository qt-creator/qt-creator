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

#ifndef CLANGBACKEND_TRANSLATIONUNIT_H
#define CLANGBACKEND_TRANSLATIONUNIT_H

#include <utf8stringvector.h>

#include <clang-c/Index.h>

#include <QtGlobal>

#include <chrono>
#include <memory>
#include <QSet>

class Utf8String;

namespace ClangBackEnd {

class TranslationUnitData;
class CodeCompleter;
class UnsavedFile;
class UnsavedFiles;
class ProjectPart;
class DiagnosticContainer;
class DiagnosticSet;
class FileContainer;
class HighlightingInformations;
class TranslationUnits;
class CommandLineArguments;
class Cursor;
class SourceLocation;
class SourceRange;
class SkippedSourceRanges;

using time_point = std::chrono::steady_clock::time_point;

class TranslationUnit
{
public:
    enum FileExistsCheck {
        CheckIfFileExists,
        DoNotCheckIfFileExists
    };

    TranslationUnit() = default;
    TranslationUnit(const Utf8String &filePath,
                    const ProjectPart &projectPart,
                    const Utf8StringVector &fileArguments,
                    TranslationUnits &translationUnits,
                    FileExistsCheck fileExistsCheck = CheckIfFileExists);
    ~TranslationUnit();

    TranslationUnit(const TranslationUnit &cxTranslationUnit);
    TranslationUnit &operator=(const TranslationUnit &cxTranslationUnit);

    TranslationUnit(TranslationUnit &&cxTranslationUnit);
    TranslationUnit &operator=(TranslationUnit &&cxTranslationUnit);

    bool isNull() const;

    void setIsUsedByCurrentEditor(bool isUsedByCurrentEditor);
    bool isUsedByCurrentEditor() const;

    void setIsVisibleInEditor(bool isVisibleInEditor);
    bool isVisibleInEditor() const;

    void reset();
    void reparse() const;

    bool isIntact() const;

    CXIndex index() const;
    CXTranslationUnit cxTranslationUnit() const;
    CXTranslationUnit cxTranslationUnitWithoutReparsing() const;

    UnsavedFile &unsavedFile() const;
    CXUnsavedFile * cxUnsavedFiles() const;

    uint unsavedFilesCount() const;

    const Utf8String &filePath() const;
    const Utf8String &projectPartId() const;
    FileContainer fileContainer() const;
    const ProjectPart &projectPart() const;

    void setDocumentRevision(uint revision);
    uint documentRevision() const;

    const time_point &lastProjectPartChangeTimePoint() const;

    bool isNeedingReparse() const;
    bool hasNewDiagnostics() const;
    bool hasNewHighlightingInformations() const;

    DiagnosticSet diagnostics() const;
    QVector<DiagnosticContainer> mainFileDiagnostics() const;

    const QSet<Utf8String> &dependedFilePaths() const;

    void setDirtyIfProjectPartIsOutdated();
    void setDirtyIfDependencyIsMet(const Utf8String &filePath);

    CommandLineArguments commandLineArguments() const;

    SourceLocation sourceLocationAtWithoutReparsing(uint line, uint column) const;
    SourceLocation sourceLocationAt(uint line, uint column) const;
    SourceLocation sourceLocationAt(const Utf8String &filePath, uint line, uint column) const;

    SourceRange sourceRange(uint fromLine, uint fromColumn, uint toLine, uint toColumn) const;

    Cursor cursorAt(uint line, uint column) const;
    Cursor cursorAt(const Utf8String &filePath, uint line, uint column) const;
    Cursor cursor() const;

    HighlightingInformations highlightingInformations() const;
    HighlightingInformations highlightingInformationsInRange(const SourceRange &range) const;

    SkippedSourceRanges skippedSourceRanges() const;

    static uint defaultOptions();

private:
    void setDirty();
    void checkIfNull() const;
    void checkIfFileExists() const;
    void updateLastProjectPartChangeTimePoint() const;
    void removeTranslationUnitIfProjectPartWasChanged() const;
    bool projectPartIsOutdated() const;
    bool isMainFileAndExistsOrIsOtherFile(const Utf8String &filePath) const;
    void createTranslationUnitIfNeeded() const;
    void checkParseErrorCode() const;
    void checkReparseErrorCode() const;
    void reparseTranslationUnit() const;
    void reparseTranslationUnitIfFilesAreChanged() const;
    bool parseWasSuccessful() const;
    bool reparseWasSuccessful() const;
    void updateIncludeFilePaths() const;
    bool fileExists() const;
    static void includeCallback(CXFile included_file,
                                CXSourceLocation * /*inclusion_stack*/,
                                unsigned /*include_len*/,
                                CXClientData clientData);
    UnsavedFiles &unsavedFiles() const;

private:
    mutable std::shared_ptr<TranslationUnitData> d;
};

bool operator==(const TranslationUnit &first, const TranslationUnit &second);
void PrintTo(const TranslationUnit &translationUnit, ::std::ostream *os);
} // namespace ClangBackEnd

#endif // CLANGBACKEND_TRANSLATIONUNIT_H
