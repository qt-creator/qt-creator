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
#include <QDir>
#include <QFileInfo>
#include <QSettings>
#include <QtDebug>

using namespace GenericProjectManager;
using namespace GenericProjectManager::Internal;

GenericProjectNode::GenericProjectNode(Core::IFile *projectFile)
    : ProjectExplorer::ProjectNode(QFileInfo(projectFile->fileName()).absolutePath()),
      _projectFile(projectFile)
{}

GenericProjectNode::~GenericProjectNode()
{ }

Core::IFile *GenericProjectNode::projectFile() const
{ return _projectFile; }

QString GenericProjectNode::projectFilePath() const
{ return _projectFile->fileName(); }

static QStringList convertToAbsoluteFiles(const QDir &dir, const QStringList &paths)
{
    QStringList absolutePaths;
    foreach (const QString &file, paths) {
        QFileInfo fileInfo(dir, file);
        absolutePaths.append(fileInfo.absoluteFilePath());
    }
    absolutePaths.removeDuplicates();
    return absolutePaths;
}

void GenericProjectNode::refresh()
{
    using namespace ProjectExplorer;

    // remove the existing nodes.
    removeFileNodes(fileNodes(), this);
    removeFolderNodes(subFolderNodes(), this);

    QDir projectPath(path());
    QSettings projectInfo(projectFilePath(), QSettings::IniFormat);

    _files        = convertToAbsoluteFiles(projectPath, projectInfo.value(QLatin1String("files")).toStringList());
    _generated    = convertToAbsoluteFiles(projectPath, projectInfo.value(QLatin1String("generated")).toStringList());
    _includePaths = convertToAbsoluteFiles(projectPath, projectInfo.value(QLatin1String("includes")).toStringList());
    _defines      = projectInfo.value(QLatin1String("defines")).toStringList();
    _toolChainId  = projectInfo.value(QLatin1String("toolchain"), QLatin1String("gcc")).toString().toLower();

    FileNode *projectFileNode = new FileNode(projectFilePath(), ProjectFileType,
                                             /* generated = */ false);

    addFileNodes(QList<FileNode *>() << projectFileNode, this);

    QStringList filePaths;
    QHash<QString, QStringList> filesInPath;

    foreach (const QString &absoluteFileName, _files) {
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

    FolderNode *folder = _folderByName.value(folderName);
    if (! folder) {
        folder = new FolderNode(components.at(end - 1));
        _folderByName.insert(folderName, folder);

        FolderNode *parent = findOrCreateFolderByName(components, end - 1);
        if (! parent)
            parent = this;
        addFolderNodes(QList<FolderNode*>() << folder, parent);
    }

    return folder;
}

ProjectExplorer::FolderNode *GenericProjectNode::findOrCreateFolderByName(const QString &filePath)
{
    QStringList components = filePath.split(QLatin1Char('/'));
    return findOrCreateFolderByName(components, components.length());
}

#if 0
void GenericProjectNode::refresh()
{
    using namespace ProjectExplorer;

    removeFileNodes(fileNodes(), this);
    removeFolderNodes(subFolderNodes(), this);

    QDir projectPath(path());

    QSettings projectInfo(projectFilePath(), QSettings::IniFormat);

    _files = convertToAbsoluteFiles(projectPath, projectInfo.value(QLatin1String("files")).toStringList());
    _generated = convertToAbsoluteFiles(projectPath, projectInfo.value(QLatin1String("generated")).toStringList());
    _includePaths = convertToAbsoluteFiles(projectPath, projectInfo.value(QLatin1String("includes")).toStringList());
    _defines      = projectInfo.value(QLatin1String("defines")).toStringList();
    _toolChainId  = projectInfo.value(QLatin1String("toolchain"), QLatin1String("gcc")).toString().toLower();

    FileNode *projectFileNode = new FileNode(projectFilePath(), ProjectFileType,
                                             /* generated = */ false);
    addFileNodes(QList<FileNode *>() << projectFileNode, this);

    QHash<QString, FolderNode *> folders;
    QHash<FolderNode *, QStringList> filesInFolder;
    QList<FolderNode *> folderNodes;

    foreach (const QString &file, _files) {
        QFileInfo fileInfo(projectPath, file);

        if (! fileInfo.isFile())
            continue; // not a file.

        QString folderPath = fileInfo.canonicalPath();
        if (! folderPath.startsWith(path())) {
            qDebug() << "*** skip:" << folderPath;
            continue;
        }

        folderPath = folderPath.mid(path().length() + 1);

        FolderNode *folder = folders.value(folderPath);
        if (! folder) {
            folder = new FolderNode(folderPath);
            folders.insert(folderPath, folder);

            folderNodes.append(folder);
        }

        filesInFolder[folder].append(fileInfo.canonicalFilePath());
    }

    addFolderNodes(folderNodes, this);

    QHashIterator<FolderNode *, QStringList> it(filesInFolder);
    while (it.hasNext()) {
        it.next();

        FolderNode *folder = it.key();
        QStringList files = it.value();

        QList<FileNode *> fileNodes;
        foreach (const QString &filePath, files) {
            QFileInfo fileInfo(projectPath, filePath);

            FileNode *fileNode = new FileNode(fileInfo.absoluteFilePath(),
                                              SourceType, /*generated = */ false);

            fileNodes.append(fileNode);
        }

        addFileNodes(fileNodes, folder);
    }
}
#endif

QStringList GenericProjectNode::files() const
{ return _files; }

QStringList GenericProjectNode::generated() const
{ return _generated; }

QStringList GenericProjectNode::includePaths() const
{ return _includePaths; }

QStringList GenericProjectNode::defines() const
{ return _defines; }

QString GenericProjectNode::toolChainId() const
{ return _toolChainId; }

bool GenericProjectNode::hasTargets() const
{
    qDebug() << Q_FUNC_INFO;

    return true;
}

QList<ProjectExplorer::ProjectNode::ProjectAction> GenericProjectNode::supportedActions() const
{
    qDebug() << Q_FUNC_INFO;

    return QList<ProjectAction>();
}

bool GenericProjectNode::addSubProjects(const QStringList &proFilePaths)
{
    qDebug() << Q_FUNC_INFO;

    Q_UNUSED(proFilePaths);
    return false;
}

bool GenericProjectNode::removeSubProjects(const QStringList &proFilePaths)
{
    qDebug() << Q_FUNC_INFO;

    Q_UNUSED(proFilePaths);
    return false;
}

bool GenericProjectNode::addFiles(const ProjectExplorer::FileType fileType,
                                  const QStringList &filePaths, QStringList *notAdded)
{
    qDebug() << Q_FUNC_INFO;

    Q_UNUSED(fileType);
    Q_UNUSED(filePaths);
    Q_UNUSED(notAdded);
    return false;
}

bool GenericProjectNode::removeFiles(const ProjectExplorer::FileType fileType,
                                     const QStringList &filePaths, QStringList *notRemoved)
{
    qDebug() << Q_FUNC_INFO;

    Q_UNUSED(fileType);
    Q_UNUSED(filePaths);
    Q_UNUSED(notRemoved);
    return false;
}

bool GenericProjectNode::renameFile(const ProjectExplorer::FileType fileType,
                                    const QString &filePath, const QString &newFilePath)
{
    qDebug() << Q_FUNC_INFO;

    Q_UNUSED(fileType);
    Q_UNUSED(filePath);
    Q_UNUSED(newFilePath);
    return false;
}
