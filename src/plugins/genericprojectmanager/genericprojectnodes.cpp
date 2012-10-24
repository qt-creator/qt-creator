/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "genericprojectnodes.h"
#include "genericproject.h"

#include <coreplugin/idocument.h>
#include <projectexplorer/projectexplorer.h>

#include <QFileInfo>

using namespace ProjectExplorer;

namespace GenericProjectManager {
namespace Internal {

GenericProjectNode::GenericProjectNode(GenericProject *project, Core::IDocument *projectFile)
    : ProjectNode(projectFile->fileName()), m_project(project), m_projectFile(projectFile)
{
    setDisplayName(QFileInfo(projectFile->fileName()).completeBaseName());
}

Core::IDocument *GenericProjectNode::projectFile() const
{
    return m_projectFile;
}

QString GenericProjectNode::projectFilePath() const
{
    return m_projectFile->fileName();
}

void GenericProjectNode::refresh()
{
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
        Utils::FileName absoluteFilePath = Utils::FileName::fromString(fileInfo.path());
        QString relativeFilePath;

        if (absoluteFilePath.isChildOf(baseDir)) {
            relativeFilePath = absoluteFilePath.relativeChildPath(Utils::FileName::fromString(base)).toString();
        } else {
            // `file' is not part of the project.
            relativeFilePath = baseDir.relativeFilePath(absoluteFilePath.toString());
        }

        if (! filePaths.contains(relativeFilePath))
            filePaths.append(relativeFilePath);

        filesInPath[relativeFilePath].append(absoluteFileName);
    }

    FolderByName folderByName;
    foreach (const QString &filePath, filePaths) {
        QStringList components = filePath.split(QLatin1Char('/'));
        FolderNode *folder = findOrCreateFolderByName(&folderByName, components, components.size());

        QList<FileNode *> fileNodes;
        foreach (const QString &file, filesInPath.value(filePath)) {
            FileType fileType = SourceType; // ### FIXME
            FileNode *fileNode = new FileNode(file, fileType, /*generated = */ false);
            fileNodes.append(fileNode);
        }

        addFileNodes(fileNodes, folder);
    }
}

FolderNode *GenericProjectNode::findOrCreateFolderByName
    (FolderByName *folderByName, const QStringList &components, int end)
{
    if (!end)
        return 0;

    QString folderName;
    for (int i = 0; i < end; ++i) {
        folderName.append(components.at(i));
        folderName += QLatin1Char('/'); // ### FIXME
    }

    const QString component = components.at(end - 1);

    if (component.isEmpty())
        return this;

    else if (FolderNode *folder = folderByName->value(folderName))
        return folder;

    const QString baseDir = QFileInfo(path()).path();
    FolderNode *folder = new FolderNode(baseDir + QLatin1Char('/') + folderName);
    folder->setDisplayName(component);
    folderByName->insert(folderName, folder);

    FolderNode *parent = findOrCreateFolderByName(folderByName, components, end - 1);
    if (!parent)
        parent = this;
    addFolderNodes(QList<FolderNode*>() << folder, parent);

    return folder;
}

bool GenericProjectNode::hasBuildTargets() const
{
    return true;
}

QList<ProjectNode::ProjectAction> GenericProjectNode::supportedActions(Node *node) const
{
    Q_UNUSED(node);
    return QList<ProjectAction>()
        << AddNewFile
        << AddExistingFile
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

bool GenericProjectNode::addFiles(const FileType fileType,
                                  const QStringList &filePaths, QStringList *notAdded)
{
    Q_UNUSED(fileType)
    Q_UNUSED(notAdded)

    return m_project->addFiles(filePaths);
}

bool GenericProjectNode::removeFiles(const FileType fileType,
                                     const QStringList &filePaths, QStringList *notRemoved)
{
    Q_UNUSED(fileType)
    Q_UNUSED(notRemoved)

    return m_project->removeFiles(filePaths);
}

bool GenericProjectNode::deleteFiles(const FileType fileType,
                                     const QStringList &filePaths)
{
    Q_UNUSED(fileType)
    Q_UNUSED(filePaths)
    return false;
}

bool GenericProjectNode::renameFile(const FileType fileType,
                                    const QString &filePath, const QString &newFilePath)
{
    Q_UNUSED(fileType)

    return m_project->renameFile(filePath, newFilePath);
}

QList<RunConfiguration *> GenericProjectNode::runConfigurationsFor(Node *node)
{
    Q_UNUSED(node)
    return QList<RunConfiguration *>();
}

} // namespace Internal
} // namespace GenericProjectManager
