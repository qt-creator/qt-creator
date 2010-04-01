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

#include "qmlprojectnodes.h"
#include "qmlprojectmanager.h"
#include "qmlproject.h"

#include <coreplugin/ifile.h>
#include <coreplugin/fileiconprovider.h>
#include <projectexplorer/projectexplorer.h>

#include <QFileInfo>
#include <QDir>
#include <QTextStream>
#include <QStyle>

namespace QmlProjectManager {
namespace Internal {

QmlProjectNode::QmlProjectNode(QmlProject *project, Core::IFile *projectFile)
    : ProjectExplorer::ProjectNode(QFileInfo(projectFile->fileName()).absoluteFilePath()),
      m_project(project),
      m_projectFile(projectFile)
{
    setDisplayName(QFileInfo(projectFile->fileName()).completeBaseName());
    // make overlay
    const QSize desiredSize = QSize(16, 16);
    const QIcon projectBaseIcon(QLatin1String(":/qmlproject/images/qmlfolder.png"));
    const QPixmap projectPixmap = Core::FileIconProvider::overlayIcon(QStyle::SP_DirIcon,
                                                                      projectBaseIcon,
                                                                      desiredSize);
    setIcon(QIcon(projectPixmap));
}

QmlProjectNode::~QmlProjectNode()
{ }

Core::IFile *QmlProjectNode::projectFile() const
{ return m_projectFile; }

QString QmlProjectNode::projectFilePath() const
{ return m_projectFile->fileName(); }

void QmlProjectNode::refresh()
{
    using namespace ProjectExplorer;

    // remove the existing nodes.
    removeFileNodes(fileNodes(), this);
    removeFolderNodes(subFolderNodes(), this);

    //ProjectExplorerPlugin::instance()->setCurrentNode(0); // ### remove me

    FileNode *projectFilesNode = new FileNode(m_project->filesFileName(),
                                              ProjectFileType,
                                              /* generated = */ false);

    QStringList files = m_project->files();
    files.removeAll(m_project->filesFileName());

    addFileNodes(QList<FileNode *>()
                 << projectFilesNode,
                 this);

    QHash<QString, QStringList> filesInDirectory;

    foreach (const QString &fileName, files) {
        QFileInfo fileInfo(fileName);

        QString absoluteFilePath;
        QString relativeDirectory;

        if (fileInfo.isAbsolute()) {
            // plain old file format
            absoluteFilePath = fileInfo.filePath();
            relativeDirectory = m_project->projectDir().relativeFilePath(fileInfo.path());
        } else {
            absoluteFilePath = m_project->projectDir().absoluteFilePath(fileInfo.filePath());
            relativeDirectory = fileInfo.path();
            if (relativeDirectory == ".")
                relativeDirectory.clear();
        }

        filesInDirectory[relativeDirectory].append(absoluteFilePath);
    }

    foreach (const QString &directory, filesInDirectory.keys()) {
        FolderNode *folder = findOrCreateFolderByName(directory);

        QList<FileNode *> fileNodes;
        foreach (const QString &file, filesInDirectory.value(directory)) {
            FileType fileType = SourceType; // ### FIXME
            FileNode *fileNode = new FileNode(file, fileType, /*generated = */ false);
            fileNodes.append(fileNode);
        }

        addFileNodes(fileNodes, folder);
    }

    m_folderByName.clear();
}

ProjectExplorer::FolderNode *QmlProjectNode::findOrCreateFolderByName(const QStringList &components, int end)
{
    if (! end)
        return 0;

    QString baseDir = QFileInfo(path()).path();

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

    FolderNode *folder = new FolderNode(baseDir + "/" + folderName);
    folder->setDisplayName(component);

    m_folderByName.insert(folderName, folder);

    FolderNode *parent = findOrCreateFolderByName(components, end - 1);
    if (! parent)
        parent = this;

    addFolderNodes(QList<FolderNode*>() << folder, parent);

    return folder;
}

ProjectExplorer::FolderNode *QmlProjectNode::findOrCreateFolderByName(const QString &filePath)
{
    QStringList components = filePath.split(QLatin1Char('/'));
    return findOrCreateFolderByName(components, components.length());
}

bool QmlProjectNode::hasBuildTargets() const
{
    return true;
}

QList<ProjectExplorer::ProjectNode::ProjectAction> QmlProjectNode::supportedActions() const
{
    QList<ProjectAction> actions;
    actions.append(AddFile);
    return actions;
}

bool QmlProjectNode::addSubProjects(const QStringList &proFilePaths)
{
    Q_UNUSED(proFilePaths)
    return false;
}

bool QmlProjectNode::removeSubProjects(const QStringList &proFilePaths)
{
    Q_UNUSED(proFilePaths)
    return false;
}

bool QmlProjectNode::addFiles(const ProjectExplorer::FileType /*fileType*/,
                              const QStringList &filePaths, QStringList * /*notAdded*/)
{
    return m_project->addFiles(filePaths);
}

bool QmlProjectNode::removeFiles(const ProjectExplorer::FileType /*fileType*/,
                                 const QStringList & /*filePaths*/, QStringList * /*notRemoved*/)
{
    return false;
}

bool QmlProjectNode::renameFile(const ProjectExplorer::FileType /*fileType*/,
                                    const QString & /*filePath*/, const QString & /*newFilePath*/)
{
    return false;
}

} // namespace Internal
} // namespace QmlProjectManager
