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

#include "genericprojectnodes.h"
#include "genericproject.h"

#include <coreplugin/idocument.h>
#include <projectexplorer/projectexplorer.h>

#include <utils/algorithm.h>
#include <utils/fileutils.h>

#include <QFileInfo>

using namespace ProjectExplorer;

namespace GenericProjectManager {
namespace Internal {

GenericProjectNode::GenericProjectNode(GenericProject *project) :
    ProjectNode(project->projectDirectory()),
    m_project(project)
{
    setDisplayName(project->projectFilePath().toFileInfo().completeBaseName());
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
            if (relativeFilePath.endsWith('/'))
                relativeFilePath.chop(1);
        }

        if (relativeFilePath == ".")
            relativeFilePath.clear();

        filesInPath[relativeFilePath].append(absoluteFileName);
    }
    return filesInPath;
}

bool GenericProjectNode::showInSimpleTree() const
{
    return true;
}

QList<ProjectAction> GenericProjectNode::supportedActions(Node *node) const
{
    Q_UNUSED(node);
    return {
        AddNewFile,
        AddExistingFile,
        AddExistingDirectory,
        RemoveFile,
        Rename
    };
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

bool GenericProjectNode::renameFile(const QString &filePath, const QString &newFilePath)
{
    return m_project->renameFile(filePath, newFilePath);
}

} // namespace Internal
} // namespace GenericProjectManager
