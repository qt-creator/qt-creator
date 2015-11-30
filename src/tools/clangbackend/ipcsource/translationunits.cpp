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
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://www.qt.io/licensing.  For further information
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
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "translationunits.h"

#include <diagnosticschangedmessage.h>
#include <diagnosticset.h>
#include <projectpartsdonotexistexception.h>
#include <projects.h>
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

void TranslationUnits::create(const QVector<FileContainer> &fileContainers)
{
    checkIfTranslationUnitsDoesNotExists(fileContainers);

    for (const FileContainer &fileContainer : fileContainers)
        createTranslationUnit(fileContainer);
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

void TranslationUnits::setCurrentEditor(const Utf8String &filePath)
{
    for (TranslationUnit &translationUnit : translationUnits_)
        translationUnit.setIsUsedByCurrentEditor(translationUnit.filePath() == filePath);
}

void TranslationUnits::setVisibleEditors(const Utf8StringVector &filePaths)
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

DiagnosticSendState TranslationUnits::sendChangedDiagnostics()
{
    auto diagnosticSendState = sendChangedDiagnosticsForCurrentEditor();
    if (diagnosticSendState == DiagnosticSendState::NoDiagnosticSend)
        diagnosticSendState = sendChangedDiagnosticsForVisibleEditors();
    if (diagnosticSendState == DiagnosticSendState::NoDiagnosticSend)
        diagnosticSendState = sendChangedDiagnosticsForAll();

    return diagnosticSendState;
}

template<class Predicate>
DiagnosticSendState TranslationUnits::sendChangedDiagnostics(Predicate predicate)
{
    auto foundTranslationUnit = std::find_if(translationUnits_.begin(),
                                             translationUnits_.end(),
                                             predicate);

    if (foundTranslationUnit != translationUnits().end()) {
        sendDiagnosticChangedMessage(*foundTranslationUnit);
        return DiagnosticSendState::MaybeThereAreMoreDiagnostics;
    }

    return DiagnosticSendState::NoDiagnosticSend;
}

DiagnosticSendState TranslationUnits::sendChangedDiagnosticsForCurrentEditor()
{
    auto hasDiagnosticsForCurrentEditor = [] (const TranslationUnit &translationUnit) {
        return translationUnit.isUsedByCurrentEditor() && translationUnit.hasNewDiagnostics();
    };

    return sendChangedDiagnostics(hasDiagnosticsForCurrentEditor);
}

DiagnosticSendState TranslationUnits::sendChangedDiagnosticsForVisibleEditors()
{
    auto hasDiagnosticsForVisibleEditor = [] (const TranslationUnit &translationUnit) {
        return translationUnit.isVisibleInEditor() && translationUnit.hasNewDiagnostics();
    };

    return sendChangedDiagnostics(hasDiagnosticsForVisibleEditor);
}

DiagnosticSendState TranslationUnits::sendChangedDiagnosticsForAll()
{
    auto hasDiagnostics = [] (const TranslationUnit &translationUnit) {
        return translationUnit.hasNewDiagnostics();
    };

    return sendChangedDiagnostics(hasDiagnostics);
}

void TranslationUnits::setSendChangeDiagnosticsCallback(std::function<void(const DiagnosticsChangedMessage &)> &&callback)
{
    sendDiagnosticsChangedCallback = std::move(callback);
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

void TranslationUnits::createTranslationUnit(const FileContainer &fileContainer)
{
    TranslationUnit::FileExistsCheck checkIfFileExists = fileContainer.hasUnsavedFileContent() ? TranslationUnit::DoNotCheckIfFileExists : TranslationUnit::CheckIfFileExists;
    auto findIterator = findTranslationUnit(fileContainer);
    if (findIterator == translationUnits_.end()) {
        translationUnits_.push_back(TranslationUnit(fileContainer.filePath(),
                                                    projectParts.project(fileContainer.projectPartId()),
                                                    fileContainer.fileArguments(),
                                                    *this,
                                                    checkIfFileExists));
        translationUnits_.back().setDocumentRevision(fileContainer.documentRevision());
    }
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

void TranslationUnits::sendDiagnosticChangedMessage(const TranslationUnit &translationUnit)
{
    if (sendDiagnosticsChangedCallback) {
        DiagnosticsChangedMessage message(translationUnit.fileContainer(),
                                          translationUnit.mainFileDiagnostics());

        sendDiagnosticsChangedCallback(std::move(message));
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
