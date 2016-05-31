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

#include "clangstring.h"
#include "clangunsavedfilesshallowarguments.h"
#include "codecompleter.h"
#include "projectpart.h"
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

    const Utf8String filePath;
    const Utf8StringVector fileArguments;

    ProjectPart projectPart;
    time_point lastProjectPartChangeTimePoint;

    CXTranslationUnit translationUnit = nullptr;
    CXIndex index = nullptr;

    QSet<Utf8String> dependedFilePaths;

    uint documentRevision = 0;
    time_point needsToBeReparsedChangeTimePoint;
    bool hasParseOrReparseFailed = false;
    bool needsToBeReparsed = false;
    bool isUsedByCurrentEditor = false;
    bool isVisibleInEditor = false;
};

TranslationUnitData::TranslationUnitData(const Utf8String &filePath,
                                         const ProjectPart &projectPart,
                                         const Utf8StringVector &fileArguments,
                                         TranslationUnits &translationUnits)
    : translationUnits(translationUnits),
      filePath(filePath),
      fileArguments(fileArguments),
      projectPart(projectPart),
      lastProjectPartChangeTimePoint(std::chrono::steady_clock::now()),
      needsToBeReparsedChangeTimePoint(lastProjectPartChangeTimePoint)
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

void TranslationUnit::reset()
{
    d.reset();
}

bool TranslationUnit::isNull() const
{
    return !d;
}

bool TranslationUnit::isIntact() const
{
    return !isNull()
        && fileExists()
        && !d->hasParseOrReparseFailed;
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

FileContainer TranslationUnit::fileContainer() const
{
    checkIfNull();

    return FileContainer(d->filePath,
                         d->projectPart.projectPartId(),
                         Utf8String(),
                         false,
                         d->documentRevision);
}

Utf8String TranslationUnit::projectPartId() const
{
    checkIfNull();

    return d->projectPart.projectPartId();
}

const ProjectPart &TranslationUnit::projectPart() const
{
    checkIfNull();

    return d->projectPart;
}

const time_point TranslationUnit::lastProjectPartChangeTimePoint() const
{
    checkIfNull();

    return d->lastProjectPartChangeTimePoint;
}

bool TranslationUnit::isProjectPartOutdated() const
{
    checkIfNull();

    return d->projectPart.lastChangeTimePoint() >= d->lastProjectPartChangeTimePoint;
}

uint TranslationUnit::documentRevision() const
{
    checkIfNull();

    return d->documentRevision;
}

void TranslationUnit::setDocumentRevision(uint revision)
{
    checkIfNull();

    d->documentRevision = revision;
}

bool TranslationUnit::isUsedByCurrentEditor() const
{
    checkIfNull();

    return d->isUsedByCurrentEditor;
}

void TranslationUnit::setIsUsedByCurrentEditor(bool isUsedByCurrentEditor)
{
    checkIfNull();

    d->isUsedByCurrentEditor = isUsedByCurrentEditor;
}

bool TranslationUnit::isVisibleInEditor() const
{
    checkIfNull();

    return d->isVisibleInEditor;
}

void TranslationUnit::setIsVisibleInEditor(bool isVisibleInEditor)
{
    checkIfNull();

    d->isVisibleInEditor = isVisibleInEditor;
}

time_point TranslationUnit::isNeededReparseChangeTimePoint() const
{
    checkIfNull();

    return d->needsToBeReparsedChangeTimePoint;
}

bool TranslationUnit::isNeedingReparse() const
{
    checkIfNull();

    return d->needsToBeReparsed;
}

void TranslationUnit::setDirtyIfProjectPartIsOutdated()
{
    if (isProjectPartOutdated())
        setDirty();
}

void TranslationUnit::setDirtyIfDependencyIsMet(const Utf8String &filePath)
{
    if (d->dependedFilePaths.contains(filePath) && isMainFileAndExistsOrIsOtherFile(filePath))
        setDirty();
}

TranslationUnitUpdateInput TranslationUnit::createUpdateInput() const
{
    TranslationUnitUpdateInput updateInput;
    updateInput.parseNeeded = isProjectPartOutdated();
    updateInput.reparseNeeded = isNeedingReparse();
    updateInput.needsToBeReparsedChangeTimePoint = d->needsToBeReparsedChangeTimePoint;
    updateInput.filePath = filePath();
    updateInput.fileArguments = fileArguments();
    updateInput.unsavedFiles = d->translationUnits.unsavedFiles();
    updateInput.projectId = projectPart().projectPartId();
    updateInput.projectArguments = projectPart().arguments();

    return updateInput;
}

TranslationUnitUpdater TranslationUnit::createUpdater() const
{
    const TranslationUnitUpdateInput updateInput = createUpdateInput();
    TranslationUnitUpdater updater(d->index, d->translationUnit, updateInput);

    return updater;
}

void TranslationUnit::setHasParseOrReparseFailed(bool hasFailed)
{
    d->hasParseOrReparseFailed = hasFailed;
}

void TranslationUnit::incorporateUpdaterResult(const TranslationUnitUpdateResult &result) const
{
    d->hasParseOrReparseFailed = result.hasParseOrReparseFailed;
    if (d->hasParseOrReparseFailed) {
        d->needsToBeReparsed = false;
        return;
    }

    if (result.parseTimePointIsSet)
        d->lastProjectPartChangeTimePoint = result.parseTimePoint;

    if (result.parseTimePointIsSet || result.reparsed)
        d->dependedFilePaths = result.dependedOnFilePaths;

    d->translationUnits.addWatchedFiles(d->dependedFilePaths);

    if (result.reparsed
            && result.needsToBeReparsedChangeTimePoint == d->needsToBeReparsedChangeTimePoint) {
        d->needsToBeReparsed = false;
    }
}

TranslationUnitCore TranslationUnit::translationUnitCore() const
{
    checkIfNull();

    return TranslationUnitCore(d->filePath, d->index, d->translationUnit);
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
    checkIfNull();

    const TranslationUnitUpdateInput updateInput = createUpdateInput();
    TranslationUnitUpdateResult result = translationUnitCore().reparse(updateInput);

    incorporateUpdaterResult(result);
}

const QSet<Utf8String> TranslationUnit::dependedFilePaths() const
{
    checkIfNull();
    checkIfFileExists();

    return d->dependedFilePaths;
}

void TranslationUnit::setDirty()
{
    d->needsToBeReparsedChangeTimePoint = std::chrono::steady_clock::now();
    d->needsToBeReparsed = true;
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

bool TranslationUnit::fileExists() const
{
    return QFileInfo::exists(d->filePath.toString());
}

bool TranslationUnit::isMainFileAndExistsOrIsOtherFile(const Utf8String &filePath) const
{
    if (filePath == d->filePath)
        return QFileInfo::exists(d->filePath);

    return true;
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
