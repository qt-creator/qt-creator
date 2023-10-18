// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "resource_global.h"
#include <projectexplorer/projectnodes.h>

namespace ResourceEditor {
namespace Internal { class ResourceFileWatcher; }

class RESOURCE_EXPORT ResourceTopLevelNode : public ProjectExplorer::FolderNode
{
public:
    ResourceTopLevelNode(const Utils::FilePath &filePath,
                         const Utils::FilePath &basePath,
                         const QString &contents = {});
    ~ResourceTopLevelNode() override;

    void setupWatcherIfNeeded();
    void addInternalNodes();

    bool supportsAction(ProjectExplorer::ProjectAction action, const Node *node) const override;
    bool addFiles(const Utils::FilePaths &filePaths, Utils::FilePaths *notAdded) override;
    ProjectExplorer::RemovedFilesFromProject removeFiles(const Utils::FilePaths &filePaths,
                                                        Utils::FilePaths *notRemoved) override;

    void compress() override { } // do not compress

    bool addPrefix(const QString &prefix, const QString &lang);
    bool removePrefix(const QString &prefix, const QString &lang);

    AddNewInformation addNewInformation(const Utils::FilePaths &files, Node *context) const override;
    bool showInSimpleTree() const override;
    bool removeNonExistingFiles();

    QString contents() const { return m_contents; }

private:
    Internal::ResourceFileWatcher *m_document = nullptr;
    QString m_contents;
};

class RESOURCE_EXPORT ResourceFolderNode : public ProjectExplorer::FolderNode
{
public:
    ResourceFolderNode(const QString &prefix, const QString &lang, ResourceTopLevelNode *parent);
    ~ResourceFolderNode() override;

    bool supportsAction(ProjectExplorer::ProjectAction action, const Node *node) const override;

    QString displayName() const override;

    bool addFiles(const Utils::FilePaths &filePaths, Utils::FilePaths *notAdded) override;
    ProjectExplorer::RemovedFilesFromProject removeFiles(const Utils::FilePaths &filePaths,
                                                         Utils::FilePaths *notRemoved) override;
    bool canRenameFile(const Utils::FilePath &oldFilePath, const Utils::FilePath &newFilePath) override;
    bool renameFile(const Utils::FilePath &oldFilePath, const Utils::FilePath &newFilePath) override;

    bool renamePrefix(const QString &prefix, const QString &lang);

    AddNewInformation addNewInformation(const Utils::FilePaths &files, Node *context) const override;

    QString prefix() const;
    QString lang() const;
    ResourceTopLevelNode *resourceNode() const;

private:
    ResourceTopLevelNode *m_topLevelNode;
    QString m_prefix;
    QString m_lang;
};

} // namespace ResourceEditor
