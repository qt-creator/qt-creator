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
#include "clangfilepath.h"
#include "clangstring.h"
#include "clangunsavedfilesshallowarguments.h"
#include "codecompleter.h"
#include "commandlinearguments.h"
#include "diagnosticcontainer.h"
#include "diagnosticset.h"
#include "projectpart.h"
#include "skippedsourceranges.h"
#include "sourcelocation.h"
#include "sourcerange.h"
#include "highlightingmarks.h"
#include "translationunitfilenotexitexception.h"
#include "translationunitisnullexception.h"
#include "translationunitparseerrorexception.h"
#include "translationunitreparseerrorexception.h"
#include "clangtranslationunitcore.h"
#include "clangtranslationunitupdater.h"
#include "translationunits.h"
#include "unsavedfiles.h"
#include "unsavedfile.h"

#include <utf8string.h>

#include <QDebug>
#include <QFileInfo>
#include <QLoggingCategory>

#include <ostream>

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
    bool hasNewHighlightingMarks = true;
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

void TranslationUnit::parse() const
{
    checkIfNull();

    const TranslationUnitUpdateInput updateInput = createUpdateInput();
    TranslationUnitUpdateResult result = translationUnitCore().parse(updateInput);

    incorporateUpdaterResult(result);
}

void TranslationUnit::reparse() const
{
    parse(); // TODO: Remove

    const TranslationUnitUpdateInput updateInput = createUpdateInput();
    TranslationUnitUpdateResult result = translationUnitCore().reparse(updateInput);

    incorporateUpdaterResult(result);
}

bool TranslationUnit::parseWasSuccessful() const
{
    return d->parseErrorCode == CXError_Success;
}

bool TranslationUnit::reparseWasSuccessful() const
{
    return d->reparseErrorCode == 0;
}

CXIndex &TranslationUnit::index() const
{
    checkIfNull();

    return d->index;
}

CXTranslationUnit &TranslationUnit::cxTranslationUnit() const
{
    checkIfNull();
    checkIfFileExists();

    return d->translationUnit;
}

UnsavedFile TranslationUnit::unsavedFile() const
{
    return unsavedFiles().unsavedFile(filePath());
}

Utf8String TranslationUnit::filePath() const
{
    checkIfNull();

    return d->filePath;
}

Utf8StringVector TranslationUnit::fileArguments() const
{
    checkIfNull();

    return d->fileArguments;
}

Utf8String TranslationUnit::projectPartId() const
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

bool TranslationUnit::hasNewHighlightingMarks() const
{
    return d->hasNewHighlightingMarks;
}

DiagnosticSet TranslationUnit::diagnostics() const
{
    d->hasNewDiagnostics = false;

    return translationUnitCore().diagnostics();
}

QVector<ClangBackEnd::DiagnosticContainer> TranslationUnit::mainFileDiagnostics() const
{
    d->hasNewDiagnostics = false;

    return translationUnitCore().mainFileDiagnostics();
}

const QSet<Utf8String> &TranslationUnit::dependedFilePaths() const
{
    cxTranslationUnit();

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

HighlightingMarks TranslationUnit::highlightingMarks() const
{
    d->hasNewHighlightingMarks = false;

    return translationUnitCore().highlightingMarks();
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

bool TranslationUnit::projectPartIsOutdated() const
{
    return d->projectPart.lastChangeTimePoint() >= d->lastProjectPartChangeTimePoint;
}

void TranslationUnit::setDirty()
{
    d->needsToBeReparsed = true;
    d->hasNewDiagnostics = true;
    d->hasNewHighlightingMarks = true;
}

bool TranslationUnit::isMainFileAndExistsOrIsOtherFile(const Utf8String &filePath) const
{
    if (filePath == d->filePath)
        return QFileInfo::exists(d->filePath);

    return true;
}

void TranslationUnit::checkParseErrorCode() const
{
    if (!parseWasSuccessful()) {
        throw TranslationUnitParseErrorException(d->filePath,
                                                 d->projectPart.projectPartId(),
                                                 d->parseErrorCode);
    }
}

bool TranslationUnit::fileExists() const
{
    return QFileInfo::exists(d->filePath.toString());
}

TranslationUnitUpdateInput TranslationUnit::createUpdateInput() const
{
    TranslationUnitUpdateInput updateInput;
    updateInput.reparseNeeded = isNeedingReparse();
    updateInput.parseNeeded = projectPartIsOutdated();
    updateInput.filePath = filePath();
    updateInput.fileArguments = fileArguments();
    updateInput.unsavedFiles = unsavedFiles();
    updateInput.projectId = projectPart().projectPartId();
    updateInput.projectArguments = projectPart().arguments();

    return updateInput;
}

TranslationUnitUpdater TranslationUnit::createUpdater() const
{
    const TranslationUnitUpdateInput updateInput = createUpdateInput();
    TranslationUnitUpdater updater(index(), d->translationUnit, updateInput);

    return updater;
}

void TranslationUnit::incorporateUpdaterResult(const TranslationUnitUpdateResult &result) const
{
    if (result.parseTimePointIsSet)
        d->lastProjectPartChangeTimePoint = result.parseTimePoint;

    d->dependedFilePaths = result.dependedOnFilePaths;
    d->translationUnits.addWatchedFiles(d->dependedFilePaths);

    if (result.reparsed)
        d->needsToBeReparsed = false;
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
    return createUpdater().commandLineArguments();
}

TranslationUnitCore TranslationUnit::translationUnitCore() const
{
    return TranslationUnitCore(d->filePath, d->index, d->translationUnit);
}

uint TranslationUnit::unsavedFilesCount() const
{
    return unsavedFiles().count();
}

UnsavedFiles TranslationUnit::unsavedFiles() const
{
    return d->translationUnits.unsavedFiles();
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
