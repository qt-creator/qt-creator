/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
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

    CXIndex index() const;
    CXTranslationUnit cxTranslationUnit() const;
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

    DiagnosticSet diagnostics() const;
    QVector<DiagnosticContainer> mainFileDiagnostics() const;

    const QSet<Utf8String> &dependedFilePaths() const;

    void setDirtyIfDependencyIsMet(const Utf8String &filePath);

    CommandLineArguments commandLineArguments() const;

    SourceLocation sourceLocationAt(uint line, uint column) const;
    SourceLocation sourceLocationAt(const Utf8String &filePath, uint line, uint column) const;

    SourceRange sourceRange(uint fromLine, uint fromColumn, uint toLine, uint toColumn) const;

    Cursor cursorAt(uint line, uint column) const;
    Cursor cursorAt(const Utf8String &filePath, uint line, uint column) const;
    Cursor cursor() const;

    HighlightingInformations highlightingInformationsInRange(const SourceRange &range) const;

    SkippedSourceRanges skippedSourceRanges() const;

private:
    void checkIfNull() const;
    void checkIfFileExists() const;
    void updateLastProjectPartChangeTimePoint() const;
    void removeTranslationUnitIfProjectPartWasChanged() const;
    bool projectPartIsOutdated() const;
    bool isMainFileAndExistsOrIsOtherFile(const Utf8String &filePath) const;
    void createTranslationUnitIfNeeded() const;
    void checkTranslationUnitErrorCode(CXErrorCode errorCode) const;
    void reparseTranslationUnit() const;
    void reparseTranslationUnitIfFilesAreChanged() const;
    void updateIncludeFilePaths() const;
    static uint defaultOptions();
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
