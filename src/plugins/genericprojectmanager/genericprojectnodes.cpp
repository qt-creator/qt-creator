/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "genericprojectnodes.h"
#include "genericproject.h"

#include <coreplugin/ifile.h>
#include <projectexplorer/projectexplorer.h>

#include <QFileInfo>

using namespace GenericProjectManager;
using namespace GenericProjectManager::Internal;

GenericProjectNode::GenericProjectNode(GenericProject *project, Core::IFile *projectFile)
    : ProjectExplorer::ProjectNode(projectFile->fileName()),
      m_project(project),
      m_projectFile(projectFile)
{
    setDisplayName(QFileInfo(projectFile->fileName()).completeBaseName());
}

GenericProjectNode::~GenericProjectNode()
{ }

Core::IFile *GenericProjectNode::projectFile() const
{ return m_projectFile; }

QString GenericProjectNode::projectFilePath() const
{ return m_projectFile->fileName(); }

void GenericProjectNode::refresh()
{
    using namespace ProjectExplorer;

    // remove the existing nodes.
    removeFileNodes(fileNodes(), this);
    removeFolderNodes(subFolderNodes(), this);

    //ProjectExplorerPlugin::instance()->setCurrentNode(0); // ### remove me

    FileNode *projectFilesNode = new FileNode(m_project->filesFileName(),
                                              ProjectFileType,
                                              /* generated = */ false);

    FileNode *projectIncludesNode = new FileNode(m_project->includesFileName(),
                                                 ProjectFileType,
                                                 /* generated = */ false);

    FileNode *projectConfigNode = new FileNode(m_project->configFileName(),
                                               ProjectFileType,
                                               /* generated = */ false);

    QStringList files = m_project->files();
    files.removeAll(m_project->filesFileName());
    files.removeAll(m_project->includesFileName());
    files.removeAll(m_project->configFileName());

    addFileNodes(QList<FileNode *>()
                 << projectFilesNode
                 << projectIncludesNode
                 << projectConfigNode,
                 this);

    QStringList filePaths;
    QHash<QString, QStringList> filesInPath;
    const QString base = QFileInfo(path()).absolutePath();
    const QDir baseDir(base);

    foreach (const QString &absoluteFileName, files) {
        QFileInfo fileInfo(absoluteFileName);
        const QString absoluteFilePath = fileInfo.path();
        QString relativeFilePath;

        if (absoluteFilePath.startsWith(base)) {
            relativeFilePath = absoluteFilePath.mid(base.length() + 1);
        } else {
            // `file' is not part of the project.
            relativeFilePath = baseDir.relativeFilePath(absoluteFilePath);
        }

        if (! filePaths.contains(relativeFilePath))
            filePaths.append(relativeFilePath);

        filesInPath[relativeFilePath].append(absoluteFileName);
    }

    foreach (const QString &filePath, filePaths) {
        FolderNode *folder = findOrCreateFolderByName(filePath);

        QList<FileNode *> fileNodes;
        foreach (const QString &file, filesInPath.value(filePath)) {
            FileType fileType = SourceType; // ### FIXME
            FileNode *fileNode = new FileNode(file, fileType, /*generated = */ false);
            fileNodes.append(fileNode);
        }

        addFileNodes(fileNodes, folder);
    }

    m_folderByName.clear();
}

ProjectExplorer::FolderNode *GenericProjectNode::findOrCreateFolderByName(const QStringList &components, int end)
{
    if (! end)
        return 0;

    QString folderName;
    for (int i = 0; i < end; ++i) {
        folderName.append(components.at(i));
        folderName += QLatin1Char('/'); // ### FIXME
    }

    const QString component = components.at(end - 1);

    if (component.isEmpty())
        return this;

    else if (FolderNode *folder = m_folderByName.value(folderName))
        return folder;

    const QString baseDir = QFileInfo(path()).path();
    FolderNode *folder = new FolderNode(baseDir + QLatin1Char('/') + folderName);
    folder->setDisplayName(component);
    m_folderByName.insert(folderName, folder);

    FolderNode *parent = findOrCreateFolderByName(components, end - 1);
    if (! parent)
        parent = this;
    addFolderNodes(QList<FolderNode*>() << folder, parent);

    return folder;
}

ProjectExplorer::FolderNode *GenericProjectNode::findOrCreateFolderByName(const QString &filePath)
{
    QStringList components = filePath.split(QLatin1Char('/'));
    return findOrCreateFolderByName(components, components.length());
}

bool GenericProjectNode::hasBuildTargets() const
{
    return true;
}

QList<ProjectExplorer::ProjectNode::ProjectAction> GenericProjectNode::supportedActions() const
{
    return QList<ProjectAction>()
        << AddFile
        << RemoveFile;
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

bool GenericProjectNode::addFiles(const ProjectExplorer::FileType fileType,
                                  const QStringList &filePaths, QStringList *notAdded)
{
    Q_UNUSED(fileType)
    Q_UNUSED(notAdded)

    return m_project->addFiles(filePaths);
}

bool GenericProjectNode::removeFiles(const ProjectExplorer::FileType fileType,
                                     const QStringList &filePaths, QStringList *notRemoved)
{
    Q_UNUSED(fileType)
    Q_UNUSED(notRemoved)

    return m_project->removeFiles(filePaths);
}

bool GenericProjectNode::renameFile(const ProjectExplorer::FileType fileType,
                                    const QString &filePath, const QString &newFilePath)
{
    Q_UNUSED(fileType)
    Q_UNUSED(filePath)
    Q_UNUSED(newFilePath)
    return false;
}
