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

#include "clangtranslationunitupdater.h"

#include "clangtranslationunitcore.h"

#include <utf8stringvector.h>

#include <clang-c/Index.h>

#include <QtGlobal>

#include <chrono>
#include <memory>
#include <QSet>

class Utf8String;

namespace ClangBackEnd {

class TranslationUnitCore;
class TranslationUnitData;
class TranslationUnitUpdateResult;
class CodeCompleter;
class UnsavedFile;
class UnsavedFiles;
class ProjectPart;
class DiagnosticContainer;
class DiagnosticSet;
class FileContainer;
class HighlightingMarks;
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
    void parse() const;
    void reparse() const;

    bool isIntact() const;

    CXIndex &index() const;

    CXTranslationUnit &cxTranslationUnit() const;

    UnsavedFile unsavedFile() const;
    UnsavedFiles unsavedFiles() const;
    uint unsavedFilesCount() const;

    Utf8String filePath() const;
    Utf8StringVector fileArguments() const;
    Utf8String projectPartId() const;
    FileContainer fileContainer() const;
    const ProjectPart &projectPart() const;

    void setDocumentRevision(uint revision);
    uint documentRevision() const;

    const time_point &lastProjectPartChangeTimePoint() const;

    bool isNeedingReparse() const;

    // TODO: Remove the following two
    bool hasNewDiagnostics() const;
    bool hasNewHighlightingMarks() const;

    // TODO: Remove the following two
    DiagnosticSet diagnostics() const;
    QVector<DiagnosticContainer> mainFileDiagnostics() const;

    const QSet<Utf8String> &dependedFilePaths() const;

    void setDirtyIfProjectPartIsOutdated();
    void setDirtyIfDependencyIsMet(const Utf8String &filePath);

    CommandLineArguments commandLineArguments() const;

    // TODO: Remove
    HighlightingMarks highlightingMarks() const;

    TranslationUnitCore translationUnitCore() const;

    bool projectPartIsOutdated() const;

private:
    void setDirty();
    void checkIfNull() const;
    void checkIfFileExists() const;
    bool isMainFileAndExistsOrIsOtherFile(const Utf8String &filePath) const;
    void checkParseErrorCode() const;
    bool parseWasSuccessful() const;
    bool reparseWasSuccessful() const;
    bool fileExists() const;

    TranslationUnitUpdateInput createUpdateInput() const;
    TranslationUnitUpdater createUpdater() const;
    void incorporateUpdaterResult(const TranslationUnitUpdateResult &result) const;

private:
    mutable std::shared_ptr<TranslationUnitData> d;
};

bool operator==(const TranslationUnit &first, const TranslationUnit &second);
void PrintTo(const TranslationUnit &translationUnit, ::std::ostream *os);
} // namespace ClangBackEnd
