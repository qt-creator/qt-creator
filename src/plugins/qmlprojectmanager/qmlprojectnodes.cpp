/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

#include "qmlprojectnodes.h"
#include "qmlprojectmanager.h"
#include "qmlproject.h"

#include <coreplugin/idocument.h>
#include <coreplugin/fileiconprovider.h>
#include <projectexplorer/projectexplorer.h>

#include <QFileInfo>
#include <QDir>
#include <QTextStream>
#include <QStyle>

namespace QmlProjectManager {
namespace Internal {

QmlProjectNode::QmlProjectNode(QmlProject *project, Core::IDocument *projectFile)
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

Core::IDocument *QmlProjectNode::projectFile() const
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
            if (relativeDirectory == QLatin1String("."))
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

    FolderNode *folder = new FolderNode(baseDir + QLatin1Char('/') + folderName);
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

QList<ProjectExplorer::ProjectNode::ProjectAction> QmlProjectNode::supportedActions(Node *node) const
{
    Q_UNUSED(node);
    QList<ProjectAction> actions;
    actions.append(AddNewFile);
    actions.append(EraseFile);
    actions.append(Rename);
    return actions;
}

bool QmlProjectNode::canAddSubProject(const QString &proFilePath) const
{
    Q_UNUSED(proFilePath)
    return false;
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

bool QmlProjectNode::deleteFiles(const ProjectExplorer::FileType /*fileType*/,
                                 const QStringList & /*filePaths*/)
{
    return true;
}

bool QmlProjectNode::renameFile(const ProjectExplorer::FileType /*fileType*/,
                                    const QString & /*filePath*/, const QString & /*newFilePath*/)
{
    return true;
}

QList<ProjectExplorer::RunConfiguration *> QmlProjectNode::runConfigurationsFor(Node *node)
{
    Q_UNUSED(node)
    return QList<ProjectExplorer::RunConfiguration *>();
}

} // namespace Internal
} // namespace QmlProjectManager
