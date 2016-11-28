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

#include "clangdocuments.h"

#include <diagnosticset.h>
#include <documentannotationschangedmessage.h>
#include <highlightingmarks.h>
#include <clangexceptions.h>
#include <projects.h>
#include <skippedsourceranges.h>
#include <unsavedfiles.h>

#include <utils/algorithm.h>

#include <QDebug>

#include <algorithm>

namespace ClangBackEnd {

bool operator==(const FileContainer &fileContainer, const Document &document)
{
    return fileContainer.filePath() == document.filePath() && fileContainer.projectPartId() == document.projectPartId();
}

bool operator==(const Document &document, const FileContainer &fileContainer)
{
    return fileContainer == document;
}

Documents::Documents(ProjectParts &projects, UnsavedFiles &unsavedFiles)
    : fileSystemWatcher(*this),
      projectParts(projects),
      unsavedFiles_(unsavedFiles)
{
}

std::vector<Document> Documents::create(const QVector<FileContainer> &fileContainers)
{
    checkIfDocumentsDoNotExist(fileContainers);

    std::vector<Document> createdDocuments;

    for (const FileContainer &fileContainer : fileContainers) {
        if (fileContainer.hasUnsavedFileContent())
            updateDocumentsWithChangedDependency(fileContainer.filePath());

        createdDocuments.push_back(createDocument(fileContainer));
    }

    return createdDocuments;
}

std::vector<Document> Documents::update(const QVector<FileContainer> &fileContainers)
{
    checkIfDocumentsForFilePathsExist(fileContainers);

    std::vector<Document> createdDocuments;

    for (const FileContainer &fileContainer : fileContainers) {
        const std::vector<Document> documents = updateDocument(fileContainer);
        createdDocuments.insert(createdDocuments.end(), documents.begin(), documents.end());

        updateDocumentsWithChangedDependency(fileContainer.filePath());
    }

    return createdDocuments;
}

static bool removeFromFileContainer(QVector<FileContainer> &fileContainers, const Document &document)
{
    auto position = std::remove(fileContainers.begin(), fileContainers.end(), document);

    bool entryIsRemoved = position != fileContainers.end();

    fileContainers.erase(position, fileContainers.end());

    return entryIsRemoved;
}

void Documents::remove(const QVector<FileContainer> &fileContainers)
{
    checkIfProjectPartsExists(fileContainers);

    removeDocuments(fileContainers);
    updateDocumentsWithChangedDependencies(fileContainers);
}

void Documents::setUsedByCurrentEditor(const Utf8String &filePath)
{
    for (Document &document : documents_)
        document.setIsUsedByCurrentEditor(document.filePath() == filePath);
}

void Documents::setVisibleInEditors(const Utf8StringVector &filePaths)
{
    for (Document &document : documents_)
        document.setIsVisibleInEditor(filePaths.contains(document.filePath()));
}

const Document &Documents::document(const Utf8String &filePath, const Utf8String &projectPartId) const
{
    checkIfProjectPartExists(projectPartId);

    auto findIterator = findDocument(filePath, projectPartId);

    if (findIterator == documents_.end())
        throw DocumentDoesNotExistException(filePath, projectPartId);

    return *findIterator;
}

const Document &Documents::document(const FileContainer &fileContainer) const
{
    return document(fileContainer.filePath(), fileContainer.projectPartId());
}

bool Documents::hasDocument(const Utf8String &filePath,
                                          const Utf8String &projectPartId) const
{
    return hasDocument(FileContainer(filePath, projectPartId));
}

const std::vector<Document> &Documents::documents() const
{
    return documents_;
}

const std::vector<Document> Documents::filtered(const IsMatchingDocument &isMatchingDocument) const
{
    return Utils::filtered(documents_, isMatchingDocument);
}

std::vector<Document> Documents::dirtyAndVisibleButNotCurrentDocuments() const
{
    return filtered([](const Document &document) {
        return document.isNeedingReparse()
            && document.isVisibleInEditor()
            && !document.isUsedByCurrentEditor();
    });
}

UnsavedFiles Documents::unsavedFiles() const
{
    return unsavedFiles_;
}

void Documents::addWatchedFiles(QSet<Utf8String> &filePaths)
{
    fileSystemWatcher.addFiles(filePaths);
}

void Documents::updateDocumentsWithChangedDependency(const Utf8String &filePath)
{
    for (auto &document : documents_)
        document.setDirtyIfDependencyIsMet(filePath);
}

void Documents::updateDocumentsWithChangedDependencies(const QVector<FileContainer> &fileContainers)
{
    for (const FileContainer &fileContainer : fileContainers)
        updateDocumentsWithChangedDependency(fileContainer.filePath());
}

void Documents::setDocumentsDirtyIfProjectPartChanged()
{
    for (auto &document : documents_)
        document.setDirtyIfProjectPartIsOutdated();
}

QVector<FileContainer> Documents::newerFileContainers(const QVector<FileContainer> &fileContainers) const
{
    QVector<FileContainer> newerContainers;

    auto documentIsNewer = [this] (const FileContainer &fileContainer) {
        try {
            return document(fileContainer).documentRevision() != fileContainer.documentRevision();
        } catch (const DocumentDoesNotExistException &) {
            return true;
        }
    };

    std::copy_if(fileContainers.cbegin(),
                 fileContainers.cend(),
                 std::back_inserter(newerContainers),
                 documentIsNewer);

    return newerContainers;
}

const ClangFileSystemWatcher *Documents::clangFileSystemWatcher() const
{
    return &fileSystemWatcher;
}

Document Documents::createDocument(const FileContainer &fileContainer)
{
    Document::FileExistsCheck checkIfFileExists = fileContainer.hasUnsavedFileContent() ? Document::DoNotCheckIfFileExists : Document::CheckIfFileExists;

    documents_.emplace_back(fileContainer.filePath(),
                                   projectParts.project(fileContainer.projectPartId()),
                                   fileContainer.fileArguments(),
                                   *this,
                                   checkIfFileExists);

    documents_.back().setDocumentRevision(fileContainer.documentRevision());

    return documents_.back();
}

std::vector<Document> Documents::updateDocument(const FileContainer &fileContainer)
{
    const auto documents = findAllDocumentsWithFilePath(fileContainer.filePath());

    for (auto document : documents)
        document.setDocumentRevision(fileContainer.documentRevision());

    return documents;
}

std::vector<Document>::iterator Documents::findDocument(const FileContainer &fileContainer)
{
    return std::find(documents_.begin(), documents_.end(), fileContainer);
}

std::vector<Document> Documents::findAllDocumentsWithFilePath(const Utf8String &filePath)
{
    const auto filePathCompare = [&filePath] (const Document &document) {
        return document.filePath() == filePath;
    };

    std::vector<Document> documents;
    std::copy_if(documents_.begin(),
                 documents_.end(),
                 std::back_inserter(documents),
                 filePathCompare);

    return documents;
}

std::vector<Document>::const_iterator Documents::findDocument(const Utf8String &filePath, const Utf8String &projectPartId) const
{
    FileContainer fileContainer(filePath, projectPartId);
    return std::find(documents_.begin(), documents_.end(), fileContainer);
}

bool Documents::hasDocument(const FileContainer &fileContainer) const
{
    auto findIterator = std::find(documents_.begin(), documents_.end(), fileContainer);

    return findIterator != documents_.end();
}

bool Documents::hasDocumentWithFilePath(const Utf8String &filePath) const
{
    auto filePathCompare = [&filePath] (const Document &document) {
        return document.filePath() == filePath;
    };

    auto findIterator = std::find_if(documents_.begin(), documents_.end(), filePathCompare);

    return findIterator != documents_.end();
}

void Documents::checkIfProjectPartExists(const Utf8String &projectFileName) const
{
    projectParts.project(projectFileName);
}

void Documents::checkIfProjectPartsExists(const QVector<FileContainer> &fileContainers) const
{
    Utf8StringVector notExistingProjectParts;

    for (const FileContainer &fileContainer : fileContainers) {
        if (!projectParts.hasProjectPart(fileContainer.projectPartId()))
            notExistingProjectParts.push_back(fileContainer.projectPartId());
    }

    if (!notExistingProjectParts.isEmpty())
        throw ProjectPartDoNotExistException(notExistingProjectParts);

}

void Documents::checkIfDocumentsDoNotExist(const QVector<FileContainer> &fileContainers) const
{
    for (const FileContainer &fileContainer : fileContainers) {
        if (hasDocument(fileContainer)) {
            throw DocumentAlreadyExistsException(fileContainer.filePath(),
                                                 fileContainer.projectPartId());
        }
    }
}

void Documents::checkIfDocumentsForFilePathsExist(const QVector<FileContainer> &fileContainers) const
{
    for (const FileContainer &fileContainer : fileContainers) {
        if (!hasDocumentWithFilePath(fileContainer.filePath())) {
            throw DocumentDoesNotExistException(fileContainer.filePath(),
                                                fileContainer.projectPartId());
        }
    }
}

void Documents::removeDocuments(const QVector<FileContainer> &fileContainers)
{
    QVector<FileContainer> processedFileContainers = fileContainers;

    auto removeBeginIterator = std::remove_if(documents_.begin(), documents_.end(), [&processedFileContainers] (const Document &document) {
        return removeFromFileContainer(processedFileContainers, document);
    });

    documents_.erase(removeBeginIterator, documents_.end());

    if (!processedFileContainers.isEmpty()) {
        const FileContainer fileContainer = processedFileContainers.first();
        throw DocumentDoesNotExistException(fileContainer.filePath(),
                                            fileContainer.projectPartId());
    }
}

} // namespace ClangBackEnd
