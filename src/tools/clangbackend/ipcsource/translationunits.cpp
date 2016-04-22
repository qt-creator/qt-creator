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

#include "translationunits.h"

#include <diagnosticschangedmessage.h>
#include <diagnosticset.h>
#include <highlightingchangedmessage.h>
#include <highlightinginformations.h>
#include <projectpartsdonotexistexception.h>
#include <projects.h>
#include <skippedsourceranges.h>
#include <translationunitalreadyexistsexception.h>
#include <translationunitdoesnotexistexception.h>

#include <QDebug>

#include <algorithm>

namespace ClangBackEnd {

bool operator==(const FileContainer &fileContainer, const TranslationUnit &translationUnit)
{
    return fileContainer.filePath() == translationUnit.filePath() && fileContainer.projectPartId() == translationUnit.projectPartId();
}

bool operator==(const TranslationUnit &translationUnit, const FileContainer &fileContainer)
{
    return fileContainer == translationUnit;
}

TranslationUnits::TranslationUnits(ProjectParts &projects, UnsavedFiles &unsavedFiles)
    : fileSystemWatcher(*this),
      projectParts(projects),
      unsavedFiles_(unsavedFiles)
{
}

std::vector<TranslationUnit> TranslationUnits::create(const QVector<FileContainer> &fileContainers)
{
    checkIfTranslationUnitsDoesNotExists(fileContainers);

    std::vector<TranslationUnit> createdTranslationUnits;

    for (const FileContainer &fileContainer : fileContainers)
        createdTranslationUnits.push_back(createTranslationUnit(fileContainer));

    return createdTranslationUnits;
}

void TranslationUnits::update(const QVector<FileContainer> &fileContainers)
{
    checkIfTranslationUnitsForFilePathsDoesExists(fileContainers);

    for (const FileContainer &fileContainer : fileContainers) {
        updateTranslationUnit(fileContainer);
        updateTranslationUnitsWithChangedDependency(fileContainer.filePath());
    }
}

static bool removeFromFileContainer(QVector<FileContainer> &fileContainers, const TranslationUnit &translationUnit)
{
    auto position = std::remove(fileContainers.begin(), fileContainers.end(), translationUnit);

    bool entryIsRemoved = position != fileContainers.end();

    fileContainers.erase(position, fileContainers.end());

    return entryIsRemoved;
}

void TranslationUnits::remove(const QVector<FileContainer> &fileContainers)
{
    checkIfProjectPartsExists(fileContainers);

    removeTranslationUnits(fileContainers);
    updateTranslationUnitsWithChangedDependencies(fileContainers);
}

void TranslationUnits::setUsedByCurrentEditor(const Utf8String &filePath)
{
    for (TranslationUnit &translationUnit : translationUnits_)
        translationUnit.setIsUsedByCurrentEditor(translationUnit.filePath() == filePath);
}

void TranslationUnits::setVisibleInEditors(const Utf8StringVector &filePaths)
{
    for (TranslationUnit &translationUnit : translationUnits_)
        translationUnit.setIsVisibleInEditor(filePaths.contains(translationUnit.filePath()));
}

const TranslationUnit &TranslationUnits::translationUnit(const Utf8String &filePath, const Utf8String &projectPartId) const
{
    checkIfProjectPartExists(projectPartId);

    auto findIterator = findTranslationUnit(filePath, projectPartId);

    if (findIterator == translationUnits_.end())
        throw TranslationUnitDoesNotExistException(FileContainer(filePath, projectPartId));

    return *findIterator;
}

const TranslationUnit &TranslationUnits::translationUnit(const FileContainer &fileContainer) const
{
    return translationUnit(fileContainer.filePath(), fileContainer.projectPartId());
}

bool TranslationUnits::hasTranslationUnit(const Utf8String &filePath) const
{
    return std::any_of(translationUnits_.cbegin(),
                       translationUnits_.cend(),
                       [&filePath] (const TranslationUnit &translationUnit)
    {
        return translationUnit.filePath() == filePath;
    });
}

const std::vector<TranslationUnit> &TranslationUnits::translationUnits() const
{
    return translationUnits_;
}

UnsavedFiles &TranslationUnits::unsavedFiles() const
{
    return unsavedFiles_;
}

void TranslationUnits::addWatchedFiles(QSet<Utf8String> &filePaths)
{
    fileSystemWatcher.addFiles(filePaths);
}

void TranslationUnits::updateTranslationUnitsWithChangedDependency(const Utf8String &filePath)
{
    for (auto &translationUnit : translationUnits_)
        translationUnit.setDirtyIfDependencyIsMet(filePath);
}

void TranslationUnits::updateTranslationUnitsWithChangedDependencies(const QVector<FileContainer> &fileContainers)
{
    for (const FileContainer &fileContainer : fileContainers)
        updateTranslationUnitsWithChangedDependency(fileContainer.filePath());
}

void TranslationUnits::setTranslationUnitsDirtyIfProjectPartChanged()
{
    for (auto &translationUnit : translationUnits_)
        translationUnit.setDirtyIfProjectPartIsOutdated();
}

DocumentAnnotationsSendState TranslationUnits::sendDocumentAnnotations()
{
    auto documentAnnotationsSendState = sendDocumentAnnotationsForCurrentEditor();
    if (documentAnnotationsSendState == DocumentAnnotationsSendState::NoDocumentAnnotationsSent)
        documentAnnotationsSendState = sendDocumentAnnotationsForVisibleEditors();

    return documentAnnotationsSendState;
}

template<class Predicate>
DocumentAnnotationsSendState TranslationUnits::sendDocumentAnnotations(Predicate predicate)
{
    auto foundTranslationUnit = std::find_if(translationUnits_.begin(),
                                             translationUnits_.end(),
                                             predicate);

    if (foundTranslationUnit != translationUnits().end()) {
        sendDocumentAnnotations(*foundTranslationUnit);
        return DocumentAnnotationsSendState::MaybeThereAreDocumentAnnotations;
    }

    return DocumentAnnotationsSendState::NoDocumentAnnotationsSent;
}

namespace {

bool translationUnitHasNewDocumentAnnotations(const TranslationUnit &translationUnit)
{
    return translationUnit.isIntact()
        && (translationUnit.hasNewDiagnostics()
            || translationUnit.hasNewHighlightingInformations());
}

}

DocumentAnnotationsSendState TranslationUnits::sendDocumentAnnotationsForCurrentEditor()
{
    auto hasDocumentAnnotationsForCurrentEditor = [] (const TranslationUnit &translationUnit) {
        return translationUnit.isUsedByCurrentEditor()
            && translationUnitHasNewDocumentAnnotations(translationUnit);
    };

    return sendDocumentAnnotations(hasDocumentAnnotationsForCurrentEditor);
}

DocumentAnnotationsSendState TranslationUnits::sendDocumentAnnotationsForVisibleEditors()
{
    auto hasDocumentAnnotationsForVisibleEditor = [] (const TranslationUnit &translationUnit) {
        return translationUnit.isVisibleInEditor()
            && translationUnitHasNewDocumentAnnotations(translationUnit);
    };

    return sendDocumentAnnotations(hasDocumentAnnotationsForVisibleEditor);
}

DocumentAnnotationsSendState TranslationUnits::sendDocumentAnnotationsForAll()
{
    auto hasDocumentAnnotations = [] (const TranslationUnit &translationUnit) {
        return translationUnitHasNewDocumentAnnotations(translationUnit);
    };

    return sendDocumentAnnotations(hasDocumentAnnotations);
}

void TranslationUnits::setSendDocumentAnnotationsCallback(SendDocumentAnnotationsCallback &&callback)
{
    sendDocumentAnnotationsCallback = std::move(callback);
}

QVector<FileContainer> TranslationUnits::newerFileContainers(const QVector<FileContainer> &fileContainers) const
{
    QVector<FileContainer> newerContainers;

    auto translationUnitIsNewer = [this] (const FileContainer &fileContainer) {
        try {
            return translationUnit(fileContainer).documentRevision() != fileContainer.documentRevision();
        } catch (const TranslationUnitDoesNotExistException &) {
            return true;
        }
    };

    std::copy_if(fileContainers.cbegin(),
                 fileContainers.cend(),
                 std::back_inserter(newerContainers),
                 translationUnitIsNewer);

    return newerContainers;
}

const ClangFileSystemWatcher *TranslationUnits::clangFileSystemWatcher() const
{
    return &fileSystemWatcher;
}

TranslationUnit TranslationUnits::createTranslationUnit(const FileContainer &fileContainer)
{
    TranslationUnit::FileExistsCheck checkIfFileExists = fileContainer.hasUnsavedFileContent() ? TranslationUnit::DoNotCheckIfFileExists : TranslationUnit::CheckIfFileExists;

    translationUnits_.emplace_back(fileContainer.filePath(),
                                   projectParts.project(fileContainer.projectPartId()),
                                   fileContainer.fileArguments(),
                                   *this,
                                   checkIfFileExists);

    translationUnits_.back().setDocumentRevision(fileContainer.documentRevision());

    return translationUnits_.back();
}

void TranslationUnits::updateTranslationUnit(const FileContainer &fileContainer)
{
    auto findIterator = findAllTranslationUnitWithFilePath(fileContainer.filePath());
    if (findIterator != translationUnits_.end())
        findIterator->setDocumentRevision(fileContainer.documentRevision());
}

std::vector<TranslationUnit>::iterator TranslationUnits::findTranslationUnit(const FileContainer &fileContainer)
{
    return std::find(translationUnits_.begin(), translationUnits_.end(), fileContainer);
}

std::vector<TranslationUnit>::iterator TranslationUnits::findAllTranslationUnitWithFilePath(const Utf8String &filePath)
{
    auto filePathCompare = [&filePath] (const TranslationUnit &translationUnit) {
        return translationUnit.filePath() == filePath;
    };

    return std::find_if(translationUnits_.begin(), translationUnits_.end(), filePathCompare);
}

std::vector<TranslationUnit>::const_iterator TranslationUnits::findTranslationUnit(const Utf8String &filePath, const Utf8String &projectPartId) const
{
    FileContainer fileContainer(filePath, projectPartId);
    return std::find(translationUnits_.begin(), translationUnits_.end(), fileContainer);
}

bool TranslationUnits::hasTranslationUnit(const FileContainer &fileContainer) const
{
    auto findIterator = std::find(translationUnits_.begin(), translationUnits_.end(), fileContainer);

    return findIterator != translationUnits_.end();
}

bool TranslationUnits::hasTranslationUnitWithFilePath(const Utf8String &filePath) const
{
    auto filePathCompare = [&filePath] (const TranslationUnit &translationUnit) {
        return translationUnit.filePath() == filePath;
    };

    auto findIterator = std::find_if(translationUnits_.begin(), translationUnits_.end(), filePathCompare);

    return findIterator != translationUnits_.end();
}

void TranslationUnits::checkIfProjectPartExists(const Utf8String &projectFileName) const
{
    projectParts.project(projectFileName);
}

void TranslationUnits::checkIfProjectPartsExists(const QVector<FileContainer> &fileContainers) const
{
    Utf8StringVector notExistingProjectParts;

    for (const FileContainer &fileContainer : fileContainers) {
        if (!projectParts.hasProjectPart(fileContainer.projectPartId()))
            notExistingProjectParts.push_back(fileContainer.projectPartId());
    }

    if (!notExistingProjectParts.isEmpty())
        throw ProjectPartDoNotExistException(notExistingProjectParts);

}

void TranslationUnits::checkIfTranslationUnitsDoesNotExists(const QVector<FileContainer> &fileContainers) const
{
    for (const FileContainer &fileContainer : fileContainers) {
        if (hasTranslationUnit(fileContainer))
            throw  TranslationUnitAlreadyExistsException(fileContainer);
    }
}

void TranslationUnits::checkIfTranslationUnitsForFilePathsDoesExists(const QVector<FileContainer> &fileContainers) const
{
    for (const FileContainer &fileContainer : fileContainers) {
        if (!hasTranslationUnitWithFilePath(fileContainer.filePath()))
            throw  TranslationUnitDoesNotExistException(fileContainer);
    }
}

void TranslationUnits::sendDocumentAnnotations(const TranslationUnit &translationUnit)
{
    if (sendDocumentAnnotationsCallback) {
        const auto fileContainer = translationUnit.fileContainer();
        DiagnosticsChangedMessage diagnosticsMessage(fileContainer,
                                                     translationUnit.mainFileDiagnostics());
        HighlightingChangedMessage highlightingsMessage(fileContainer,
                                                        translationUnit.highlightingInformations().toHighlightingMarksContainers(),
                                                        translationUnit.skippedSourceRanges().toSourceRangeContainers());

        sendDocumentAnnotationsCallback(std::move(diagnosticsMessage),
                                               std::move(highlightingsMessage));
    }
}

void TranslationUnits::removeTranslationUnits(const QVector<FileContainer> &fileContainers)
{
    QVector<FileContainer> processedFileContainers = fileContainers;

    auto removeBeginIterator = std::remove_if(translationUnits_.begin(), translationUnits_.end(), [&processedFileContainers] (const TranslationUnit &translationUnit) {
        return removeFromFileContainer(processedFileContainers, translationUnit);
    });

    translationUnits_.erase(removeBeginIterator, translationUnits_.end());

    if (!processedFileContainers.isEmpty())
        throw TranslationUnitDoesNotExistException(processedFileContainers.first());
}

} // namespace ClangBackEnd
