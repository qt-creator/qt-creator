//
// Copyright (C) 2013 Mateusz Łoskot <mateusz@loskot.net>
// Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
//
// This file is part of Qt Creator Boost.Build plugin project.
//
// This is free software; you can redistribute and/or modify it under
// the terms of the  GNU Lesser General Public License, Version 2.1
// as published by the Free Software Foundation.
// See accompanying file LICENSE.txt or copy at
// http://www.gnu.org/licenses/lgpl-2.1-standalone.html.
//
#include "b2projectnode.h"
#include "b2project.h"
#include "b2utility.h"
// Qt Creator
#include <coreplugin/idocument.h>
#include <projectexplorer/projectnodes.h>
#include <utils/qtcassert.h>
// Qt
#include <QFutureInterface>
#include <QHash>
#include <QList>
#include <QSet>
#include <QString>
#include <QStringList>

namespace BoostBuildProjectManager {
namespace Internal {

ProjectNode::ProjectNode(Project* project, Core::IDocument* projectFile)
    : ProjectExplorer::ProjectNode(projectFile->filePath())
    , project_(project)
    , projectFile_(projectFile)
{
    // TODO: setDisplayName(QFileInfo(projectFile->filePath()).completeBaseName());
}

bool ProjectNode::hasBuildTargets() const
{
    return false;
}

QList<ProjectExplorer::ProjectAction>
ProjectNode::supportedActions(Node* node) const
{
    Q_UNUSED(node);

    // TODO: Jamfiles (auto)editing not supported, does it make sense to manage files?
    return QList<ProjectExplorer::ProjectAction>();
}

bool ProjectNode::canAddSubProject(QString const& filePath) const
{
    Q_UNUSED(filePath)
    return false;
}

bool ProjectNode::addSubProjects(QStringList const& filePaths)
{
    Q_UNUSED(filePaths)
    return false;
}

bool ProjectNode::removeSubProjects(QStringList const& filePath)
{
    Q_UNUSED(filePath)
    return false;
}

bool ProjectNode::addFiles(QStringList const& filePaths, QStringList* notAdded = 0)
{
    Q_UNUSED(filePaths);
    Q_UNUSED(notAdded);
    return false;
}

bool ProjectNode::removeFiles(QStringList const& filePaths, QStringList* notRemoved = 0)
{
    Q_UNUSED(filePaths);
    Q_UNUSED(notRemoved);
    return false;
}

bool ProjectNode::deleteFiles(QStringList const& filePaths)
{
    Q_UNUSED(filePaths);
    return false;
}

bool ProjectNode::renameFile(QString const& filePath, QString const& newFilePath)
{
    Q_UNUSED(filePath);
    Q_UNUSED(newFilePath);
    return false;
}

QList<ProjectExplorer::RunConfiguration*> ProjectNode::runConfigurationsFor(Node* node)
{
    Q_UNUSED(node);
    return QList<ProjectExplorer::RunConfiguration*>();
}

void ProjectNode::refresh(QSet<QString> oldFileList)
{
    // The idea of refreshing files in project explorer taken from GenericProjectManager

    // Only do this once, at first run.
    if (oldFileList.isEmpty()) {
        using ProjectExplorer::FileNode;
        FileNode* projectFileNode = new FileNode(project_->projectFilePath()
                                               , ProjectExplorer::ProjectFileType
                                               , Constants::FileNotGenerated);

        FileNode* filesFileNode = new FileNode(Utils::FileName::fromString(project_->filesFilePath())
                                             , ProjectExplorer::ProjectFileType
                                             , Constants::FileNotGenerated);

        FileNode* includesFileNode = new FileNode(Utils::FileName::fromString(project_->includesFilePath())
                                             , ProjectExplorer::ProjectFileType
                                             , Constants::FileNotGenerated);

        addFileNodes(QList<FileNode*>()
            << projectFileNode << filesFileNode << includesFileNode);
    }

    oldFileList.remove(project_->filesFilePath());
    oldFileList.remove(project_->includesFilePath());

    QSet<QString> newFileList = project_->files().toSet();
    newFileList.remove(project_->filesFilePath());
    newFileList.remove(project_->includesFilePath());

    // Calculate set of added and removed files
    QSet<QString> removed = oldFileList;
    removed.subtract(newFileList);
    QSet<QString> added = newFileList;
    added.subtract(oldFileList);

    typedef QHash<QString, QStringList> FilesInPaths;
    typedef FilesInPaths::ConstIterator FilesInPathsIterator;
    using ProjectExplorer::FileNode;
    using ProjectExplorer::FileType;
    QString const baseDir = QFileInfo(path().toString()).absolutePath();

    // Process all added files
    FilesInPaths filesInPaths = Utility::sortFilesIntoPaths(baseDir, added);
    for (FilesInPathsIterator it = filesInPaths.constBegin(),
         cend = filesInPaths.constEnd(); it != cend; ++it) {
        // Create node
        QString const& filePath = it.key();
        QStringList parts;
        if (!filePath.isEmpty())
            parts = filePath.split(QLatin1Char('/'));

        FolderNode* folder = findFolderByName(parts, parts.size());
        if (!folder)
            folder = createFolderByName(parts, parts.size());

        // Attach content to node
        QList<FileNode*> fileNodes;
        foreach (QString const& file, it.value()) {
            // TODO: handle various types, mime to FileType
            FileType fileType = ProjectExplorer::SourceType;
            FileNode* fileNode = new FileNode(Utils::FileName::fromString(file), fileType, Constants::FileNotGenerated);
            fileNodes.append(fileNode);
        }

        addFileNodes(fileNodes);
    }

    // Process all removed files
    filesInPaths = Utility::sortFilesIntoPaths(baseDir, removed);
    for (FilesInPathsIterator it = filesInPaths.constBegin(),
         cend = filesInPaths.constEnd(); it != cend; ++it) {
        // Create node
        QString const& filePath = it.key();
        QStringList parts;
        if (!filePath.isEmpty())
            parts = filePath.split(QLatin1Char('/'));
        FolderNode* folder = findFolderByName(parts, parts.size());

        QList<FileNode*> fileNodes;
        foreach (QString const& file, it.value()) {
            foreach (FileNode* fn, folder->fileNodes())
                if (fn->path() == Utils::FileName::fromString(file))
                    fileNodes.append(fn);
        }

        removeFileNodes(fileNodes);
    }

    // Clean up
    foreach (FolderNode* fn, subFolderNodes())
        removeEmptySubFolders(this, fn);

}

void ProjectNode::removeEmptySubFolders(FolderNode* parent, FolderNode* subParent)
{
    foreach (FolderNode* fn, subParent->subFolderNodes())
        removeEmptySubFolders(subParent, fn);

    if (subParent->subFolderNodes().isEmpty() && subParent->fileNodes().isEmpty())
        removeFolderNodes(QList<FolderNode*>() << subParent);
}

QString appendPathComponents(QStringList const& components, int const end)
{
    QString folderName;
    for (int i = 0; i < end; ++i) {
        folderName.append(components.at(i));
        folderName += QLatin1Char('/');
    }
    return folderName;
}

ProjectExplorer::FolderNode*
ProjectNode::createFolderByName(QStringList const& components, int const end)
{
    if (end == 0)
        return this;

    using ProjectExplorer::FolderNode;
    QString const baseDir = QFileInfo(path().toString()).path();
    QString const folderName = appendPathComponents(components, end);
    FolderNode* folder = new FolderNode(Utils::FileName::fromString(baseDir + QLatin1Char('/') + folderName));
    folder->setDisplayName(components.at(end - 1));

    FolderNode* parent = findFolderByName(components, end - 1);
    if (!parent)
        parent = createFolderByName(components, end - 1);
    addFolderNodes(QList<FolderNode*>() << folder);

    return folder;
}

ProjectExplorer::FolderNode*
ProjectNode::findFolderByName(QStringList const& components, int const end) const
{
    if (end == 0)
        return const_cast<ProjectNode*>(this);

    using ProjectExplorer::FolderNode;
    FolderNode *parent = findFolderByName(components, end - 1);
    if (!parent)
        return 0;

    QString const folderName = appendPathComponents(components, end);
    QString const baseDir = QFileInfo(path().toString()).path();
    foreach (FolderNode* fn, parent->subFolderNodes()) {
        if (fn->path() == Utils::FileName::fromString(baseDir + QLatin1Char('/') + folderName))
            return fn;
    }

    return 0;
}

} // namespace Internal
} // namespace BoostBuildProjectManager
