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

#include "qmlprojectnodes.h"
#include "qmlproject.h"

#include <coreplugin/idocument.h>
#include <coreplugin/fileiconprovider.h>
#include <projectexplorer/projectexplorer.h>

#include <QFileInfo>
#include <QStyle>

namespace QmlProjectManager {
namespace Internal {

QmlProjectNode::QmlProjectNode(QmlProject *project, Core::IDocument *projectFile)
    : ProjectExplorer::ProjectNode(projectFile->filePath()),
      m_project(project),
      m_projectFile(projectFile)
{
    setDisplayName(projectFile->filePath().toFileInfo().completeBaseName());
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
{ return m_projectFile->filePath().toString(); }

void QmlProjectNode::refresh()
{
    using namespace ProjectExplorer;

    // remove the existing nodes.
    removeFileNodes(fileNodes());
    removeFolderNodes(subFolderNodes());

    //ProjectExplorerPlugin::instance()->setCurrentNode(0); // ### remove me

    FileNode *projectFilesNode = new FileNode(m_project->filesFileName(),
                                              ProjectFileType,
                                              /* generated = */ false);

    QStringList files = m_project->files();
    files.removeAll(m_project->filesFileName().toString());

    addFileNodes(QList<FileNode *>()
                 << projectFilesNode);

    QHash<QString, QStringList> filesInDirectory;

    foreach (const QString &fileName, files) {
        QFileInfo fileInfo(fileName);

        QString absoluteFilePath;
        QString relativeDirectory;

        if (fileInfo.isAbsolute()) {
            // plain old file format
            absoluteFilePath = fileInfo.filePath();
            relativeDirectory = m_project->projectDir().relativeFilePath(fileInfo.path());
            if (relativeDirectory == QLatin1String("."))
                relativeDirectory.clear();
        } else {
            absoluteFilePath = m_project->projectDir().absoluteFilePath(fileInfo.filePath());
            relativeDirectory = fileInfo.path();
            if (relativeDirectory == QLatin1String("."))
                relativeDirectory.clear();
        }

        filesInDirectory[relativeDirectory].append(absoluteFilePath);
    }

    const QHash<QString, QStringList>::ConstIterator cend = filesInDirectory.constEnd();
    for (QHash<QString, QStringList>::ConstIterator it = filesInDirectory.constBegin(); it != cend; ++it) {
        FolderNode *folder = findOrCreateFolderByName(it.key());

        QList<FileNode *> fileNodes;
        foreach (const QString &file, it.value()) {
            FileType fileType = SourceType; // ### FIXME
            FileNode *fileNode = new FileNode(Utils::FileName::fromString(file),
                                              fileType, /*generated = */ false);
            fileNodes.append(fileNode);
        }

        folder->addFileNodes(fileNodes);
    }

    m_folderByName.clear();
}

ProjectExplorer::FolderNode *QmlProjectNode::findOrCreateFolderByName(const QStringList &components, int end)
{
    if (! end)
        return 0;

    Utils::FileName folderPath = path().parentDir();

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

    folderPath.appendPath(folderName);
    FolderNode *folder = new FolderNode(folderPath);
    folder->setDisplayName(component);

    m_folderByName.insert(folderName, folder);

    FolderNode *parent = findOrCreateFolderByName(components, end - 1);
    if (! parent)
        parent = this;

    parent->addFolderNodes(QList<FolderNode*>() << folder);

    return folder;
}

ProjectExplorer::FolderNode *QmlProjectNode::findOrCreateFolderByName(const QString &filePath)
{
    QStringList components = filePath.split(QLatin1Char('/'));
    return findOrCreateFolderByName(components, components.length());
}

bool QmlProjectNode::showInSimpleTree() const
{
    return true;
}

QList<ProjectExplorer::ProjectAction> QmlProjectNode::supportedActions(Node *node) const
{
    Q_UNUSED(node);
    QList<ProjectExplorer::ProjectAction> actions;
    actions.append(ProjectExplorer::AddNewFile);
    actions.append(ProjectExplorer::EraseFile);
    if (node->nodeType() == ProjectExplorer::FileNodeType) {
        ProjectExplorer::FileNode *fileNode = static_cast<ProjectExplorer::FileNode *>(node);
        if (fileNode->fileType() != ProjectExplorer::ProjectFileType)
            actions.append(ProjectExplorer::Rename);
    }
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

bool QmlProjectNode::addFiles(const QStringList &filePaths, QStringList * /*notAdded*/)
{
    return m_project->addFiles(filePaths);
}

bool QmlProjectNode::removeFiles(const QStringList & /*filePaths*/, QStringList * /*notRemoved*/)
{
    return false;
}

bool QmlProjectNode::deleteFiles(const QStringList & /*filePaths*/)
{
    return true;
}

bool QmlProjectNode::renameFile(const QString & /*filePath*/, const QString & /*newFilePath*/)
{
    return true;
}

} // namespace Internal
} // namespace QmlProjectManager
