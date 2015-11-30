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

#include "translationunit.h"

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
    CXIndex index = nullptr;
    uint documentRevision = 0;
    bool needsToBeReparsed = false;
    bool hasNewDiagnostics = true;
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
    checkIfNull();
    checkIfFileExists();
    removeTranslationUnitIfProjectPartWasChanged();
    createTranslationUnitIfNeeded();
    reparseTranslationUnitIfFilesAreChanged();

    return d->translationUnit;
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

void TranslationUnit::setDirtyIfDependencyIsMet(const Utf8String &filePath)
{
    if (d->dependedFilePaths.contains(filePath) && isMainFileAndExistsOrIsOtherFile(filePath)) {
        d->needsToBeReparsed = true;
        d->hasNewDiagnostics = true;
    }
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
    if (!QFileInfo::exists(d->filePath.toString()))
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

        CXErrorCode errorCode = clang_parseTranslationUnit2(index(),
                                                            NULL,
                                                            args.data(),
                                                            args.count(),
                                                            unsavedFiles().cxUnsavedFiles(),
                                                            unsavedFiles().count(),
                                                            defaultOptions(),
                                                            &d->translationUnit);

        checkTranslationUnitErrorCode(errorCode);

        updateIncludeFilePaths();

        updateLastProjectPartChangeTimePoint();
    }
}

void TranslationUnit::checkTranslationUnitErrorCode(CXErrorCode errorCode) const
{
    switch (errorCode) {
        case CXError_Success: break;
        default: throw TranslationUnitParseErrorException(d->filePath, d->projectPart.projectPartId());
    }
}

void TranslationUnit::reparseTranslationUnit() const
{
    clang_reparseTranslationUnit(d->translationUnit,
                                 unsavedFiles().count(),
                                 unsavedFiles().cxUnsavedFiles(),
                                 clang_defaultReparseOptions(d->translationUnit));

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

CommandLineArguments TranslationUnit::commandLineArguments() const
{
    return CommandLineArguments(d->filePath.constData(),
                                d->projectPart.arguments(),
                                d->fileArguments,
                                isVerboseModeEnabled());
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
