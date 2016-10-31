/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "qmlprojectnodes.h"
#include "qmlproject.h"

#include <coreplugin/idocument.h>
#include <coreplugin/fileiconprovider.h>
#include <projectexplorer/projectexplorer.h>

#include <utils/algorithm.h>

#include <QFileInfo>
#include <QStyle>

namespace QmlProjectManager {
namespace Internal {

QmlProjectNode::QmlProjectNode(QmlProject *project)
    : ProjectExplorer::ProjectNode(project->projectDirectory()),
      m_project(project)
{
    setDisplayName(project->projectFilePath().toFileInfo().completeBaseName());
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

void QmlProjectNode::refresh()
{
    using namespace ProjectExplorer;

    FileNode *projectFilesNode = new FileNode(m_project->filesFileName(),
                                              FileType::Project,
                                              /* generated = */ false);

    QStringList files = m_project->files();
    files.removeAll(m_project->filesFileName().toString());

    QList<FileNode *> fileNodes = Utils::transform(files, [](const QString &f) {
        FileType fileType = FileType::Source; // ### FIXME
        return new FileNode(Utils::FileName::fromString(f), fileType, false);

    });
    fileNodes.append(projectFilesNode);

    buildTree(fileNodes);
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
    if (node->nodeType() == ProjectExplorer::NodeType::File) {
        ProjectExplorer::FileNode *fileNode = static_cast<ProjectExplorer::FileNode *>(node);
        if (fileNode->fileType() != ProjectExplorer::FileType::Project)
            actions.append(ProjectExplorer::Rename);
    }
    return actions;
}

bool QmlProjectNode::addFiles(const QStringList &filePaths, QStringList * /*notAdded*/)
{
    return m_project->addFiles(filePaths);
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
