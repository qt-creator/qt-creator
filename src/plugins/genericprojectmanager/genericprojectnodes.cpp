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

#include "genericprojectnodes.h"
#include "genericproject.h"

#include <utils/fileutils.h>
#include <coreplugin/idocument.h>
#include <projectexplorer/projectexplorer.h>

#include <QFileInfo>

using namespace ProjectExplorer;

namespace GenericProjectManager {
namespace Internal {

GenericProjectNode::GenericProjectNode(GenericProject *project, Core::IDocument *projectFile)
    : ProjectNode(projectFile->filePath())
    , m_project(project)
    , m_projectFile(projectFile)
{
    setDisplayName(projectFile->filePath().toFileInfo().completeBaseName());
}

Core::IDocument *GenericProjectNode::projectFile() const
{
    return m_projectFile;
}

QString GenericProjectNode::projectFilePath() const
{
    return m_projectFile->filePath().toString();
}

QHash<QString, QStringList> sortFilesIntoPaths(const QString &base, const QSet<QString> &files)
{
    QHash<QString, QStringList> filesInPath;
    const QDir baseDir(base);

    foreach (const QString &absoluteFileName, files) {
        QFileInfo fileInfo(absoluteFileName);
        Utils::FileName absoluteFilePath = Utils::FileName::fromString(fileInfo.path());
        QString relativeFilePath;

        if (absoluteFilePath.isChildOf(baseDir)) {
            relativeFilePath = absoluteFilePath.relativeChildPath(Utils::FileName::fromString(base)).toString();
        } else {
            // `file' is not part of the project.
            relativeFilePath = baseDir.relativeFilePath(absoluteFilePath.toString());
            if (relativeFilePath.endsWith(QLatin1Char('/')))
                relativeFilePath.chop(1);
        }

        filesInPath[relativeFilePath].append(absoluteFileName);
    }
    return filesInPath;
}

void GenericProjectNode::refresh(QSet<QString> oldFileList)
{
    typedef QHash<QString, QStringList> FilesInPathHash;
    typedef FilesInPathHash::ConstIterator FilesInPathHashConstIt;

    // Do those separately
    oldFileList.remove(m_project->filesFileName());
    oldFileList.remove(m_project->includesFileName());
    oldFileList.remove(m_project->configFileName());

    QSet<QString> newFileList = m_project->files().toSet();
    newFileList.remove(m_project->filesFileName());
    newFileList.remove(m_project->includesFileName());
    newFileList.remove(m_project->configFileName());

    QSet<QString> removed = oldFileList;
    removed.subtract(newFileList);
    QSet<QString> added = newFileList;
    added.subtract(oldFileList);

    QString baseDir = path().toFileInfo().absolutePath();
    FilesInPathHash filesInPaths = sortFilesIntoPaths(baseDir, added);

    FilesInPathHashConstIt cend = filesInPaths.constEnd();
    for (FilesInPathHashConstIt it = filesInPaths.constBegin(); it != cend; ++it) {
        const QString &filePath = it.key();
        QStringList components;
        if (!filePath.isEmpty())
            components = filePath.split(QLatin1Char('/'));
        FolderNode *folder = findFolderByName(components, components.size());
        if (!folder)
            folder = createFolderByName(components, components.size());

        QList<FileNode *> fileNodes;
        foreach (const QString &file, it.value()) {
            FileType fileType = SourceType; // ### FIXME
            if (file.endsWith(QLatin1String(".qrc")))
                fileType = ResourceType;
            FileNode *fileNode = new FileNode(Utils::FileName::fromString(file),
                                              fileType, /*generated = */ false);
            fileNodes.append(fileNode);
        }

        folder->addFileNodes(fileNodes);
    }

    filesInPaths = sortFilesIntoPaths(baseDir, removed);
    cend = filesInPaths.constEnd();
    for (FilesInPathHashConstIt it = filesInPaths.constBegin(); it != cend; ++it) {
        const QString &filePath = it.key();
        QStringList components;
        if (!filePath.isEmpty())
            components = filePath.split(QLatin1Char('/'));
        FolderNode *folder = findFolderByName(components, components.size());

        QList<FileNode *> fileNodes;
        foreach (const QString &file, it.value()) {
            foreach (FileNode *fn, folder->fileNodes()) {
                if (fn->path().toString() == file)
                    fileNodes.append(fn);
            }
        }

        folder->removeFileNodes(fileNodes);
    }

    foreach (FolderNode *fn, subFolderNodes())
        removeEmptySubFolders(this, fn);

}

void GenericProjectNode::removeEmptySubFolders(FolderNode *gparent, FolderNode *parent)
{
    foreach (FolderNode *fn, parent->subFolderNodes())
        removeEmptySubFolders(parent, fn);

    if (parent->subFolderNodes().isEmpty() && parent->fileNodes().isEmpty())
        gparent->removeFolderNodes(QList<FolderNode*>() << parent);
}

FolderNode *GenericProjectNode::createFolderByName(const QStringList &components, int end)
{
    if (end == 0)
        return this;

    QString folderName;
    for (int i = 0; i < end; ++i) {
        folderName.append(components.at(i));
        folderName += QLatin1Char('/');
    }

    const QString component = components.at(end - 1);

    const Utils::FileName folderPath = path().parentDir().appendPath(folderName);
    FolderNode *folder = new FolderNode(folderPath);
    folder->setDisplayName(component);

    FolderNode *parent = findFolderByName(components, end - 1);
    if (!parent)
        parent = createFolderByName(components, end - 1);
    parent->addFolderNodes(QList<FolderNode*>() << folder);

    return folder;
}

FolderNode *GenericProjectNode::findFolderByName(const QStringList &components, int end)
{
    if (end == 0)
        return this;

    QString folderName;
    for (int i = 0; i < end; ++i) {
        folderName.append(components.at(i));
        folderName += QLatin1Char('/');
    }

    FolderNode *parent = findFolderByName(components, end - 1);

    if (!parent)
        return 0;

    const QString baseDir = path().toFileInfo().path();
    foreach (FolderNode *fn, parent->subFolderNodes()) {
        if (fn->path().toString() == baseDir + QLatin1Char('/') + folderName)
            return fn;
    }
    return 0;
}

bool GenericProjectNode::showInSimpleTree() const
{
    return true;
}

QList<ProjectAction> GenericProjectNode::supportedActions(Node *node) const
{
    Q_UNUSED(node);
    return QList<ProjectAction>()
        << AddNewFile
        << AddExistingFile
        << AddExistingDirectory
        << RemoveFile
        << Rename;
}

bool GenericProjectNode::canAddSubProject(const QString &proFilePath) const
{
    Q_UNUSED(proFilePath)
    return false;
}

bool GenericProjectNode::addSubProjects(const QStringList &proFilePaths)
{
    Q_UNUSED(proFilePaths)
    return false;
}

bool GenericProjectNode::removeSubProjects(const QStringList &proFilePaths)
{
    Q_UNUSED(proFilePaths)
    return false;
}

bool GenericProjectNode::addFiles(const QStringList &filePaths, QStringList *notAdded)
{
    Q_UNUSED(notAdded)

    return m_project->addFiles(filePaths);
}

bool GenericProjectNode::removeFiles(const QStringList &filePaths, QStringList *notRemoved)
{
    Q_UNUSED(notRemoved)

    return m_project->removeFiles(filePaths);
}

bool GenericProjectNode::deleteFiles(const QStringList &filePaths)
{
    Q_UNUSED(filePaths)
    return false;
}

bool GenericProjectNode::renameFile(const QString &filePath, const QString &newFilePath)
{
    return m_project->renameFile(filePath, newFilePath);
}

} // namespace Internal
} // namespace GenericProjectManager
