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

#include "clangdocument.h"

#include "clangdocuments.h"
#include "clangstring.h"
#include "clangunsavedfilesshallowarguments.h"
#include "codecompleter.h"
#include "projectpart.h"
#include "clangexceptions.h"
#include "clangtranslationunit.h"
#include "clangtranslationunits.h"
#include "clangtranslationunitupdater.h"
#include "unsavedfiles.h"
#include "unsavedfile.h"

#include <utf8string.h>

#include <QDebug>
#include <QFileInfo>
#include <QLoggingCategory>

#include <ostream>

namespace ClangBackEnd {

class DocumentData
{
public:
    DocumentData(const Utf8String &filePath,
                 const ProjectPart &projectPart,
                 const Utf8StringVector &fileArguments,
                 Documents &documents);
    ~DocumentData();

public:
    Documents &documents;

    const Utf8String filePath;
    const Utf8StringVector fileArguments;

    ProjectPart projectPart;
    TimePoint lastProjectPartChangeTimePoint;

    TranslationUnits translationUnits;

    QSet<Utf8String> dependedFilePaths;

    uint documentRevision = 0;

    TimePoint visibleTimePoint;

    TimePoint isDirtyChangeTimePoint;
    bool isDirty = false;

    bool hasParseOrReparseFailed = false;
    bool isUsedByCurrentEditor = false;
    bool isVisibleInEditor = false;
    bool increaseResponsiveness = false;
    bool isSuspended = false;
};

DocumentData::DocumentData(const Utf8String &filePath,
                           const ProjectPart &projectPart,
                           const Utf8StringVector &fileArguments,
                           Documents &documents)
    : documents(documents),
      filePath(filePath),
      fileArguments(fileArguments),
      projectPart(projectPart),
      lastProjectPartChangeTimePoint(Clock::now()),
      translationUnits(filePath),
      isDirtyChangeTimePoint(lastProjectPartChangeTimePoint)
{
    dependedFilePaths.insert(filePath);
    translationUnits.createAndAppend();
}

DocumentData::~DocumentData()
{
}

Document::Document(const Utf8String &filePath,
                   const ProjectPart &projectPart,
                   const Utf8StringVector &fileArguments,
                   Documents &documents,
                   FileExistsCheck fileExistsCheck)
    : d(std::make_shared<DocumentData>(filePath,
                                       projectPart,
                                       fileArguments,
                                       documents))
{
    if (fileExistsCheck == FileExistsCheck::Check)
        checkIfFileExists();
}

Document::~Document() = default;
Document::Document(const Document &) = default;
Document &Document::operator=(const Document &) = default;

Document::Document(Document &&other)
    : d(std::move(other.d))
{
}

Document &Document::operator=(Document &&other)
{
    d = std::move(other.d);

    return *this;
}

void Document::reset()
{
    d.reset();
}

bool Document::isNull() const
{
    return !d;
}

bool Document::isIntact() const
{
    return !isNull()
        && fileExists()
        && !d->hasParseOrReparseFailed;
}

bool Document::isParsed() const
{
    return d->translationUnits.areAllTranslationUnitsParsed();
}

Utf8String Document::filePath() const
{
    checkIfNull();

    return d->filePath;
}

Utf8StringVector Document::fileArguments() const
{
    checkIfNull();

    return d->fileArguments;
}

FileContainer Document::fileContainer() const
{
    checkIfNull();

    return FileContainer(d->filePath,
                         d->projectPart.id(),
                         Utf8String(),
                         false,
                         d->documentRevision);
}

const ProjectPart &Document::projectPart() const
{
    checkIfNull();

    return d->projectPart;
}

const TimePoint Document::lastProjectPartChangeTimePoint() const
{
    checkIfNull();

    return d->lastProjectPartChangeTimePoint;
}

bool Document::isProjectPartOutdated() const
{
    checkIfNull();

    return d->projectPart.lastChangeTimePoint() >= d->lastProjectPartChangeTimePoint;
}

uint Document::documentRevision() const
{
    checkIfNull();

    return d->documentRevision;
}

void Document::setDocumentRevision(uint revision)
{
    checkIfNull();

    d->documentRevision = revision;
}

bool Document::isResponsivenessIncreased() const
{
    return d->translationUnits.size() > 1;
}

bool Document::isResponsivenessIncreaseNeeded() const
{
    checkIfNull();

    return d->increaseResponsiveness;
}

void Document::setResponsivenessIncreaseNeeded(bool responsivenessIncreaseNeeded)
{
    d->increaseResponsiveness = responsivenessIncreaseNeeded;
}

bool Document::isSuspended() const
{
    checkIfNull();

    return d->isSuspended;
}

void Document::setIsSuspended(bool isSuspended)
{
    checkIfNull();

    d->isSuspended = isSuspended;
}

bool Document::isUsedByCurrentEditor() const
{
    checkIfNull();

    return d->isUsedByCurrentEditor;
}

void Document::setIsUsedByCurrentEditor(bool isUsedByCurrentEditor)
{
    checkIfNull();

    d->isUsedByCurrentEditor = isUsedByCurrentEditor;
}

bool Document::isVisibleInEditor() const
{
    checkIfNull();

    return d->isVisibleInEditor;
}

void Document::setIsVisibleInEditor(bool isVisibleInEditor, const TimePoint &timePoint)
{
    checkIfNull();

    if (isVisibleInEditor)
        d->visibleTimePoint = timePoint;
    d->isVisibleInEditor = isVisibleInEditor;
}

TimePoint Document::visibleTimePoint() const
{
    checkIfNull();

    return d->visibleTimePoint;;
}

bool Document::isDirty() const
{
    checkIfNull();

    return d->isDirty;
}

TimePoint Document::isDirtyTimeChangePoint() const
{
    checkIfNull();

    return d->isDirtyChangeTimePoint;
}

bool Document::setDirtyIfProjectPartIsOutdated()
{
    if (isProjectPartOutdated()) {
        setDirty();
        return true;
    }

    return false;
}

void Document::setDirtyIfDependencyIsMet(const Utf8String &filePath)
{
    if (d->dependedFilePaths.contains(filePath) && isMainFileAndExistsOrIsOtherFile(filePath))
        setDirty();
}

TranslationUnitUpdateInput Document::createUpdateInput() const
{
    TranslationUnitUpdateInput updateInput;
    updateInput.parseNeeded = isProjectPartOutdated();
    updateInput.reparseNeeded = d->isDirty;
    updateInput.needsToBeReparsedChangeTimePoint = d->isDirtyChangeTimePoint;
    updateInput.filePath = filePath();
    updateInput.fileArguments = fileArguments();
    updateInput.unsavedFiles = d->documents.unsavedFiles();
    updateInput.projectId = projectPart().id();
    updateInput.projectArguments = projectPart().arguments();

    return updateInput;
}

TranslationUnitUpdater Document::createUpdater() const
{
    TranslationUnit unit = translationUnit();

    const TranslationUnitUpdateInput updateInput = createUpdateInput();
    TranslationUnitUpdater updater(unit.id(),
                                   unit.cxIndex(),
                                   unit.cxTranslationUnit(),
                                   updateInput);

    return updater;
}

void Document::setHasParseOrReparseFailed(bool hasFailed)
{
    d->hasParseOrReparseFailed = hasFailed;
}

void Document::incorporateUpdaterResult(const TranslationUnitUpdateResult &result) const
{
    d->hasParseOrReparseFailed = result.hasParseOrReparseFailed;
    if (d->hasParseOrReparseFailed) {
        d->isDirty = false;
        return;
    }

    if (result.hasParsed())
        d->lastProjectPartChangeTimePoint = result.parseTimePoint;

    if (result.hasParsed() || result.hasReparsed()) {
        d->dependedFilePaths = result.dependedOnFilePaths;

        const TimePoint timePoint = qMax(result.parseTimePoint, result.reparseTimePoint);
        d->translationUnits.updateParseTimePoint(result.translationUnitId, timePoint);
    }

    d->documents.addWatchedFiles(d->dependedFilePaths);

    if (result.hasReparsed()
            && result.needsToBeReparsedChangeTimePoint == d->isDirtyChangeTimePoint) {
        d->isDirty = false;
    }
}

TranslationUnit Document::translationUnit(PreferredTranslationUnit preferredTranslationUnit) const
{
    checkIfNull();

    return d->translationUnits.get(preferredTranslationUnit);
}

TranslationUnits &Document::translationUnits() const
{
    return d->translationUnits;
}

void Document::parse() const
{
    checkIfNull();

    const TranslationUnitUpdateInput updateInput = createUpdateInput();
    TranslationUnitUpdateResult result = translationUnit().parse(updateInput);

    incorporateUpdaterResult(result);
}

void Document::reparse() const
{
    checkIfNull();

    const TranslationUnitUpdateInput updateInput = createUpdateInput();
    TranslationUnitUpdateResult result = translationUnit().reparse(updateInput);

    incorporateUpdaterResult(result);
}

const QSet<Utf8String> Document::dependedFilePaths() const
{
    checkIfNull();
    checkIfFileExists();

    return d->dependedFilePaths;
}

void Document::setDependedFilePaths(const QSet<Utf8String> &filePaths)
{
    d->dependedFilePaths = filePaths;
}

void Document::setDirty()
{
    d->isDirtyChangeTimePoint = Clock::now();
    d->isDirty = true;
}

void Document::checkIfNull() const
{
    if (isNull())
        throw DocumentIsNullException();
}

void Document::checkIfFileExists() const
{
    if (!fileExists())
        throw DocumentFileDoesNotExistException(d->filePath);
}

bool Document::fileExists() const
{
    return QFileInfo::exists(d->filePath.toString());
}

bool Document::isMainFileAndExistsOrIsOtherFile(const Utf8String &filePath) const
{
    if (filePath == d->filePath)
        return QFileInfo::exists(d->filePath);

    return true;
}

bool operator==(const Document &first, const Document &second)
{
    return first.filePath() == second.filePath()
        && first.projectPart().id() == second.projectPart().id();
}

std::ostream &operator<<(std::ostream &os, const Document &document)
{
    os << "("
       << document.filePath() << ", "
       << document.projectPart().id() << ", "
       << document.documentRevision()
       << ")";

    return os;
}

} // namespace ClangBackEnd
