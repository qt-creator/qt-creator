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

using namespace ProjectExplorer;

namespace QmlProjectManager {
namespace Internal {

QmlProjectNode::QmlProjectNode(QmlProject *project) : ProjectNode(project->projectDirectory()),
    m_project(project)
{
    setDisplayName(project->projectFilePath().toFileInfo().completeBaseName());

    static QIcon qmlProjectIcon = Core::FileIconProvider::directoryIcon(QLatin1String(":/qmlproject/images/qmlfolder.png"));
    setIcon(qmlProjectIcon);
}

bool QmlProjectNode::showInSimpleTree() const
{
    return true;
}

bool QmlProjectNode::supportsAction(ProjectAction action, const Node *node) const
{
    if (action == AddNewFile || action == EraseFile)
        return true;
    QTC_ASSERT(node, return false);

    if (action == Rename && node->nodeType() == NodeType::File) {
        const FileNode *fileNode = node->asFileNode();
        QTC_ASSERT(fileNode, return false);
        return fileNode->fileType() != FileType::Project;
    }

    return false;
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
