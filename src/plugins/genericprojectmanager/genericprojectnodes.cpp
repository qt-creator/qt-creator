/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#include "genericprojectnodes.h"
#include "genericproject.h"

#include <projectexplorer/projectexplorer.h>

#include <QDir>
#include <QFileInfo>
#include <QSettings>
#include <QtDebug>

using namespace GenericProjectManager;
using namespace GenericProjectManager::Internal;

GenericProjectNode::GenericProjectNode(GenericProject *project, Core::IFile *projectFile)
    : ProjectExplorer::ProjectNode(QFileInfo(projectFile->fileName()).absolutePath()),
      _project(project),
      _projectFile(projectFile)
{}

GenericProjectNode::~GenericProjectNode()
{ }

Core::IFile *GenericProjectNode::projectFile() const
{ return _projectFile; }

QString GenericProjectNode::projectFilePath() const
{ return _projectFile->fileName(); }

void GenericProjectNode::refresh()
{
    using namespace ProjectExplorer;

    // remove the existing nodes.
    removeFileNodes(fileNodes(), this);
    removeFolderNodes(subFolderNodes(), this);

    ProjectExplorerPlugin::instance()->setCurrentNode(0); // ### remove me

    FileNode *projectFilesNode = new FileNode(_project->filesFileName(),
                                              ProjectFileType,
                                              /* generated = */ false);

    FileNode *projectIncludesNode = new FileNode(_project->includesFileName(),
                                                 ProjectFileType,
                                                 /* generated = */ false);

    FileNode *projectConfigNode = new FileNode(_project->configFileName(),
                                               ProjectFileType,
                                               /* generated = */ false);

    QStringList files = _project->files();
    files.removeAll(_project->filesFileName());
    files.removeAll(_project->includesFileName());
    files.removeAll(_project->configFileName());

    addFileNodes(QList<FileNode *>()
                 << projectFilesNode
                 << projectIncludesNode
                 << projectConfigNode,
                 this);

    QStringList filePaths;
    QHash<QString, QStringList> filesInPath;

    foreach (const QString &absoluteFileName, files) {
        QFileInfo fileInfo(absoluteFileName);
        const QString absoluteFilePath = fileInfo.path();

        if (! absoluteFilePath.startsWith(path()))
            continue; // `file' is not part of the project.

        const QString relativeFilePath = absoluteFilePath.mid(path().length() + 1);

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

    _folderByName.clear();
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

    else if (FolderNode *folder = _folderByName.value(folderName))
        return folder;

    FolderNode *folder = new FolderNode(component);
    _folderByName.insert(folderName, folder);

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

bool GenericProjectNode::hasTargets() const
{
    return true;
}

QList<ProjectExplorer::ProjectNode::ProjectAction> GenericProjectNode::supportedActions() const
{
    return QList<ProjectAction>();
}

bool GenericProjectNode::addSubProjects(const QStringList &proFilePaths)
{
    Q_UNUSED(proFilePaths);
    return false;
}

bool GenericProjectNode::removeSubProjects(const QStringList &proFilePaths)
{
    Q_UNUSED(proFilePaths);
    return false;
}

bool GenericProjectNode::addFiles(const ProjectExplorer::FileType fileType,
                                  const QStringList &filePaths, QStringList *notAdded)
{
    Q_UNUSED(fileType);
    Q_UNUSED(filePaths);
    Q_UNUSED(notAdded);
    return false;
}

bool GenericProjectNode::removeFiles(const ProjectExplorer::FileType fileType,
                                     const QStringList &filePaths, QStringList *notRemoved)
{
    Q_UNUSED(fileType);
    Q_UNUSED(filePaths);
    Q_UNUSED(notRemoved);
    return false;
}

bool GenericProjectNode::renameFile(const ProjectExplorer::FileType fileType,
                                    const QString &filePath, const QString &newFilePath)
{
    Q_UNUSED(fileType);
    Q_UNUSED(filePath);
    Q_UNUSED(newFilePath);
    return false;
}
