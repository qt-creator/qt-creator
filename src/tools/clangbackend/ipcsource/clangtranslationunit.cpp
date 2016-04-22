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

#include "cursor.h"
#include "clangstring.h"
#include "codecompleter.h"
#include "commandlinearguments.h"
#include "diagnosticcontainer.h"
#include "diagnosticset.h"
#include "projectpart.h"
#include "skippedsourceranges.h"
#include "sourcelocation.h"
#include "sourcerange.h"
#include "highlightinginformations.h"
#include "translationunitfilenotexitexception.h"
#include "translationunitisnullexception.h"
#include "translationunitparseerrorexception.h"
#include "translationunitreparseerrorexception.h"
#include "translationunits.h"
#include "unsavedfiles.h"

#include <utf8string.h>

#include <QDebug>
#include <QFileInfo>
#include <QLoggingCategory>

#include <ostream>

static Q_LOGGING_CATEGORY(verboseLibLog, "qtc.clangbackend.verboselib");

static bool isVerboseModeEnabled()
{
    return verboseLibLog().isDebugEnabled();
}

namespace ClangBackEnd {

class TranslationUnitData
{
public:
    TranslationUnitData(const Utf8String &filePath,
                        const ProjectPart &projectPart,
                        const Utf8StringVector &fileArguments,
                        TranslationUnits &translationUnits);
    ~TranslationUnitData();

public:
    TranslationUnits &translationUnits;
    time_point lastProjectPartChangeTimePoint;
    QSet<Utf8String> dependedFilePaths;
    ProjectPart projectPart;
    Utf8StringVector fileArguments;
    Utf8String filePath;
    CXTranslationUnit translationUnit = nullptr;
    CXErrorCode parseErrorCode = CXError_Success;
    int reparseErrorCode = 0;
    CXIndex index = nullptr;
    uint documentRevision = 0;
    bool needsToBeReparsed = false;
    bool hasNewDiagnostics = true;
    bool hasNewHighlightingInformations = true;
    bool isUsedByCurrentEditor = false;
    bool isVisibleInEditor = false;
};

TranslationUnitData::TranslationUnitData(const Utf8String &filePath,
                                         const ProjectPart &projectPart,
                                         const Utf8StringVector &fileArguments,
                                         TranslationUnits &translationUnits)
    : translationUnits(translationUnits),
      lastProjectPartChangeTimePoint(std::chrono::steady_clock::now()),
      projectPart(projectPart),
      fileArguments(fileArguments),
      filePath(filePath)
{
    dependedFilePaths.insert(filePath);
}

TranslationUnitData::~TranslationUnitData()
{
    clang_disposeTranslationUnit(translationUnit);
    clang_disposeIndex(index);
}

TranslationUnit::TranslationUnit(const Utf8String &filePath,
                                 const ProjectPart &projectPart,
                                 const Utf8StringVector &fileArguments,
                                 TranslationUnits &translationUnits,
                                 FileExistsCheck fileExistsCheck)
    : d(std::make_shared<TranslationUnitData>(filePath,
                                              projectPart,
                                              fileArguments,
                                              translationUnits))
{
    if (fileExistsCheck == CheckIfFileExists)
        checkIfFileExists();
}

bool TranslationUnit::isNull() const
{
    return !d;
}

void TranslationUnit::setIsUsedByCurrentEditor(bool isUsedByCurrentEditor)
{
    d->isUsedByCurrentEditor = isUsedByCurrentEditor;
}

bool TranslationUnit::isUsedByCurrentEditor() const
{
    return d->isUsedByCurrentEditor;
}

void TranslationUnit::setIsVisibleInEditor(bool isVisibleInEditor)
{
    d->isVisibleInEditor = isVisibleInEditor;
}

bool TranslationUnit::isVisibleInEditor() const
{
    return d->isVisibleInEditor;
}

void TranslationUnit::reset()
{
    d.reset();
}

void TranslationUnit::reparse() const
{
    cxTranslationUnit();

    reparseTranslationUnit();
}

bool TranslationUnit::parseWasSuccessful() const
{
    return d->parseErrorCode == CXError_Success;
}

bool TranslationUnit::reparseWasSuccessful() const
{
    return d->reparseErrorCode == 0;
}

CXIndex TranslationUnit::index() const
{
    checkIfNull();

    if (!d->index) {
        const bool displayDiagnostics = isVerboseModeEnabled();
        d->index = clang_createIndex(1, displayDiagnostics);
    }

    return d->index;
}

CXTranslationUnit TranslationUnit::cxTranslationUnit() const
{
    cxTranslationUnitWithoutReparsing();
    reparseTranslationUnitIfFilesAreChanged();

    return d->translationUnit;
}

CXTranslationUnit TranslationUnit::cxTranslationUnitWithoutReparsing() const
{
    checkIfNull();
    checkIfFileExists();
    removeTranslationUnitIfProjectPartWasChanged();
    createTranslationUnitIfNeeded();

    return d->translationUnit;
}

UnsavedFile &TranslationUnit::unsavedFile() const
{
    return unsavedFiles().unsavedFile(filePath());
}

const Utf8String &TranslationUnit::filePath() const
{
    checkIfNull();

    return d->filePath;
}

const Utf8String &TranslationUnit::projectPartId() const
{
    checkIfNull();

    return d->projectPart.projectPartId();
}

FileContainer TranslationUnit::fileContainer() const
{
    checkIfNull();

    return FileContainer(d->filePath,
                         d->projectPart.projectPartId(),
                         Utf8String(),
                         false,
                         d->documentRevision);
}

const ProjectPart &TranslationUnit::projectPart() const
{
    checkIfNull();

    return d->projectPart;
}

void TranslationUnit::setDocumentRevision(uint revision)
{
    d->documentRevision = revision;
}

uint TranslationUnit::documentRevision() const
{
    return d->documentRevision;
}

const time_point &TranslationUnit::lastProjectPartChangeTimePoint() const
{
    return d->lastProjectPartChangeTimePoint;
}

bool TranslationUnit::isNeedingReparse() const
{
    return d->needsToBeReparsed;
}

bool TranslationUnit::hasNewDiagnostics() const
{
    return d->hasNewDiagnostics;
}

bool TranslationUnit::hasNewHighlightingInformations() const
{
    return d->hasNewHighlightingInformations;
}

DiagnosticSet TranslationUnit::diagnostics() const
{
    d->hasNewDiagnostics = false;

    return DiagnosticSet(clang_getDiagnosticSetFromTU(cxTranslationUnit()));
}

QVector<ClangBackEnd::DiagnosticContainer> TranslationUnit::mainFileDiagnostics() const
{
    const auto mainFilePath = filePath();
    const auto isMainFileDiagnostic = [mainFilePath](const Diagnostic &diagnostic) {
        return diagnostic.location().filePath() == mainFilePath;
    };

    return diagnostics().toDiagnosticContainers(isMainFileDiagnostic);
}

const QSet<Utf8String> &TranslationUnit::dependedFilePaths() const
{
    createTranslationUnitIfNeeded();

    return d->dependedFilePaths;
}

void TranslationUnit::setDirtyIfProjectPartIsOutdated()
{
    if (projectPartIsOutdated())
        setDirty();
}

void TranslationUnit::setDirtyIfDependencyIsMet(const Utf8String &filePath)
{
    if (d->dependedFilePaths.contains(filePath) && isMainFileAndExistsOrIsOtherFile(filePath))
        setDirty();
}

SourceLocation TranslationUnit::sourceLocationAt(uint line, uint column) const
{
    return SourceLocation(cxTranslationUnit(), filePath(), line, column);
}

SourceLocation TranslationUnit::sourceLocationAt(const Utf8String &filePath, uint line, uint column) const
{
    return SourceLocation(cxTranslationUnit(), filePath, line, column);
}

SourceRange TranslationUnit::sourceRange(uint fromLine, uint fromColumn, uint toLine, uint toColumn) const
{
    return SourceRange(sourceLocationAt(fromLine, fromColumn),
                       sourceLocationAt(toLine, toColumn));
}

Cursor TranslationUnit::cursorAt(uint line, uint column) const
{
    return clang_getCursor(cxTranslationUnit(), sourceLocationAt(line, column));
}

Cursor TranslationUnit::cursorAt(const Utf8String &filePath, uint line, uint column) const
{
    return clang_getCursor(cxTranslationUnit(), sourceLocationAt(filePath, line, column));
}

Cursor TranslationUnit::cursor() const
{
    return clang_getTranslationUnitCursor(cxTranslationUnit());
}

HighlightingInformations TranslationUnit::highlightingInformations() const
{
    d->hasNewHighlightingInformations = false;

    return highlightingInformationsInRange(cursor().sourceRange());
}

HighlightingInformations TranslationUnit::highlightingInformationsInRange(const SourceRange &range) const
{
    CXToken *cxTokens = 0;
    uint cxTokensCount = 0;
    auto translationUnit = cxTranslationUnit();

    clang_tokenize(translationUnit, range, &cxTokens, &cxTokensCount);

    return HighlightingInformations(translationUnit, cxTokens, cxTokensCount);
}

SkippedSourceRanges TranslationUnit::skippedSourceRanges() const
{
    return SkippedSourceRanges(cxTranslationUnit(), d->filePath.constData());
}

void TranslationUnit::checkIfNull() const
{
    if (isNull())
        throw TranslationUnitIsNullException();
}

void TranslationUnit::checkIfFileExists() const
{
    if (!fileExists())
        throw TranslationUnitFileNotExitsException(d->filePath);
}

void TranslationUnit::updateLastProjectPartChangeTimePoint() const
{
    d->lastProjectPartChangeTimePoint = std::chrono::steady_clock::now();
}

void TranslationUnit::removeTranslationUnitIfProjectPartWasChanged() const
{
    if (projectPartIsOutdated()) {
        clang_disposeTranslationUnit(d->translationUnit);
        d->translationUnit = nullptr;
    }
}

bool TranslationUnit::projectPartIsOutdated() const
{
    return d->projectPart.lastChangeTimePoint() >= d->lastProjectPartChangeTimePoint;
}

void TranslationUnit::setDirty()
{
    d->needsToBeReparsed = true;
    d->hasNewDiagnostics = true;
    d->hasNewHighlightingInformations = true;
}

bool TranslationUnit::isMainFileAndExistsOrIsOtherFile(const Utf8String &filePath) const
{
    if (filePath == d->filePath)
        return QFileInfo::exists(d->filePath);

    return true;
}

void TranslationUnit::createTranslationUnitIfNeeded() const
{
    if (!d->translationUnit) {
        d->translationUnit = CXTranslationUnit();

        const auto args = commandLineArguments();
        if (isVerboseModeEnabled())
            args.print();

        d->parseErrorCode = clang_parseTranslationUnit2(index(),
                                                        NULL,
                                                        args.data(),
                                                        args.count(),
                                                        unsavedFiles().cxUnsavedFiles(),
                                                        unsavedFiles().count(),
                                                        defaultOptions(),
                                                        &d->translationUnit);

        checkParseErrorCode();

        updateIncludeFilePaths();

        updateLastProjectPartChangeTimePoint();
    }
}

void TranslationUnit::checkParseErrorCode() const
{
    if (!parseWasSuccessful()) {
        throw TranslationUnitParseErrorException(d->filePath,
                                                 d->projectPart.projectPartId(),
                                                 d->parseErrorCode);
    }
}

void TranslationUnit::checkReparseErrorCode() const
{
    if (!reparseWasSuccessful()) {
        throw TranslationUnitReparseErrorException(d->filePath,
                                                   d->projectPart.projectPartId(),
                                                   d->reparseErrorCode);
    }
}

void TranslationUnit::reparseTranslationUnit() const
{
    d->reparseErrorCode = clang_reparseTranslationUnit(
                                    d->translationUnit,
                                    unsavedFiles().count(),
                                    unsavedFiles().cxUnsavedFiles(),
                                    clang_defaultReparseOptions(d->translationUnit));

    checkReparseErrorCode();

    updateIncludeFilePaths();

    d->needsToBeReparsed = false;
}

void TranslationUnit::reparseTranslationUnitIfFilesAreChanged() const
{
    if (isNeedingReparse())
        reparseTranslationUnit();
}

void TranslationUnit::includeCallback(CXFile included_file,
                                      CXSourceLocation * /*inclusion_stack*/,
                                      unsigned /*include_len*/,
                                      CXClientData clientData)
{

    ClangString includeFilePath(clang_getFileName(included_file));

    TranslationUnit *translationUnit = static_cast<TranslationUnit*>(clientData);

    translationUnit->d->dependedFilePaths.insert(includeFilePath);
}

UnsavedFiles &TranslationUnit::unsavedFiles() const
{
    return d->translationUnits.unsavedFiles();
}

void TranslationUnit::updateIncludeFilePaths() const
{
    auto oldDependedFilePaths = d->dependedFilePaths;

    d->dependedFilePaths.clear();
    d->dependedFilePaths.insert(filePath());

    clang_getInclusions(d->translationUnit, includeCallback, const_cast<TranslationUnit*>(this));

    if (d->dependedFilePaths.size() == 1)
        d->dependedFilePaths = oldDependedFilePaths;

    d->translationUnits.addWatchedFiles(d->dependedFilePaths);
}

bool TranslationUnit::fileExists() const
{
    return QFileInfo::exists(d->filePath.toString());
}

bool TranslationUnit::isIntact() const
{
    return !isNull()
        && fileExists()
        && parseWasSuccessful()
        && reparseWasSuccessful();
}

CommandLineArguments TranslationUnit::commandLineArguments() const
{
    return CommandLineArguments(d->filePath.constData(),
                                d->projectPart.arguments(),
                                d->fileArguments,
                                isVerboseModeEnabled());
}

SourceLocation TranslationUnit::sourceLocationAtWithoutReparsing(uint line, uint column) const
{
    return SourceLocation(cxTranslationUnitWithoutReparsing(), filePath(), line, column);
}

uint TranslationUnit::defaultOptions()
{
    return CXTranslationUnit_CacheCompletionResults
         | CXTranslationUnit_PrecompiledPreamble
         | CXTranslationUnit_IncludeBriefCommentsInCodeCompletion
         | CXTranslationUnit_DetailedPreprocessingRecord;
}

uint TranslationUnit::unsavedFilesCount() const
{
    return unsavedFiles().count();
}

CXUnsavedFile *TranslationUnit::cxUnsavedFiles() const
{
    return unsavedFiles().cxUnsavedFiles();
}

TranslationUnit::~TranslationUnit() = default;

TranslationUnit::TranslationUnit(const TranslationUnit &) = default;
TranslationUnit &TranslationUnit::operator=(const TranslationUnit &) = default;

TranslationUnit::TranslationUnit(TranslationUnit &&other)
    : d(std::move(other.d))
{
}

TranslationUnit &TranslationUnit::operator=(TranslationUnit &&other)
{
    d = std::move(other.d);

    return *this;
}

bool operator==(const TranslationUnit &first, const TranslationUnit &second)
{
    return first.filePath() == second.filePath() && first.projectPartId() == second.projectPartId();
}

void PrintTo(const TranslationUnit &translationUnit, ::std::ostream *os)
{
    *os << "TranslationUnit("
        << translationUnit.filePath().constData() << ", "
        << translationUnit.projectPartId().constData() << ", "
        << translationUnit.documentRevision() << ")";
}

} // namespace ClangBackEnd
