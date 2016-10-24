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

#include "clangdocument.h"
#include "clangfilesystemwatcher.h"

#include <filecontainer.h>

#include <QVector>

#include <functional>
#include <vector>

namespace ClangBackEnd {

class ProjectParts;
class UnsavedFiles;

class Documents
{
public:
    Documents(ProjectParts &projectParts, UnsavedFiles &unsavedFiles);

    std::vector<Document> create(const QVector<FileContainer> &fileContainers);
    std::vector<Document> update(const QVector<FileContainer> &fileContainers);
    void remove(const QVector<FileContainer> &fileContainers);

    void setUsedByCurrentEditor(const Utf8String &filePath);
    void setVisibleInEditors(const Utf8StringVector &filePaths);

    const Document &document(const Utf8String &filePath, const Utf8String &projectPartId) const;
    const Document &document(const FileContainer &fileContainer) const;
    bool hasDocument(const Utf8String &filePath, const Utf8String &projectPartId) const;
    bool hasDocumentWithFilePath(const Utf8String &filePath) const;

    const std::vector<Document> &documents() const;
    using IsMatchingDocument = std::function<bool(const Document &document)>;
    const std::vector<Document> filtered(const IsMatchingDocument &isMatchingDocument) const;
    std::vector<Document> dirtyAndVisibleButNotCurrentDocuments() const;

    UnsavedFiles unsavedFiles() const;

    void addWatchedFiles(QSet<Utf8String> &filePaths);

    void updateDocumentsWithChangedDependency(const Utf8String &filePath);
    void updateDocumentsWithChangedDependencies(const QVector<FileContainer> &fileContainers);
    void setDocumentsDirtyIfProjectPartChanged();

    QVector<FileContainer> newerFileContainers(const QVector<FileContainer> &fileContainers) const;

    const ClangFileSystemWatcher *clangFileSystemWatcher() const;

private:
    Document createDocument(const FileContainer &fileContainer);
    std::vector<Document> updateDocument(const FileContainer &fileContainer);
    std::vector<Document>::iterator findDocument(const FileContainer &fileContainer);
    std::vector<Document> findAllDocumentsWithFilePath(const Utf8String &filePath);
    std::vector<Document>::const_iterator findDocument(const Utf8String &filePath, const Utf8String &projectPartId) const;
    bool hasDocument(const FileContainer &fileContainer) const;
    void checkIfProjectPartExists(const Utf8String &projectFileName) const;
    void checkIfProjectPartsExists(const QVector<FileContainer> &fileContainers) const;
    void checkIfDocumentsDoNotExist(const QVector<FileContainer> &fileContainers) const;
    void checkIfDocumentsForFilePathsExist(const QVector<FileContainer> &fileContainers) const;

    void removeDocuments(const QVector<FileContainer> &fileContainers);

private:
    ClangFileSystemWatcher fileSystemWatcher;
    std::vector<Document> documents_;
    ProjectParts &projectParts;
    UnsavedFiles &unsavedFiles_;
};

} // namespace ClangBackEnd
