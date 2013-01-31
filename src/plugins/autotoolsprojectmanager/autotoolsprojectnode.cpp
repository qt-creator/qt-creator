/**************************************************************************
**
** Copyright (C) 2013 Openismus GmbH.
** Authors: Peter Penz (ppenz@openismus.com)
**          Patricia Santana Cruz (patriciasantanacruz@gmail.com)
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

#include "autotoolsprojectnode.h"
#include "autotoolsproject.h"

#include <coreplugin/idocument.h>

using namespace AutotoolsProjectManager;
using namespace AutotoolsProjectManager::Internal;
using namespace ProjectExplorer;

AutotoolsProjectNode::AutotoolsProjectNode(AutotoolsProject *project, Core::IDocument *projectFile) :
    ProjectNode(projectFile->fileName()),
    m_project(project),
    m_projectFile(projectFile)
{
}

bool AutotoolsProjectNode::hasBuildTargets() const
{
    return true;
}

QList<ProjectNode::ProjectAction> AutotoolsProjectNode::supportedActions(Node *node) const
{
    Q_UNUSED(node);
    return QList<ProjectNode::ProjectAction>();
}

bool AutotoolsProjectNode::canAddSubProject(const QString &proFilePath) const
{
    Q_UNUSED(proFilePath)
    return false;
}

bool AutotoolsProjectNode::addSubProjects(const QStringList &proFilePaths)
{
    Q_UNUSED(proFilePaths);
    return false;
}

bool AutotoolsProjectNode::removeSubProjects(const QStringList &proFilePaths)
{
    Q_UNUSED(proFilePaths);
    return false;
}

bool AutotoolsProjectNode::addFiles(const FileType fileType,
                                    const QStringList &filePaths,
                                    QStringList *notAdded)
{
    Q_UNUSED(fileType);
    Q_UNUSED(filePaths);
    Q_UNUSED(notAdded);
    return false;
}

bool AutotoolsProjectNode::removeFiles(const FileType fileType,
                                       const QStringList &filePaths,
                                       QStringList *notRemoved)
{
    Q_UNUSED(fileType);
    Q_UNUSED(filePaths);
    Q_UNUSED(notRemoved);
    return false;
}

bool AutotoolsProjectNode::deleteFiles(const FileType fileType,
                                       const QStringList &filePaths)
{
    Q_UNUSED(fileType);
    Q_UNUSED(filePaths);
    return false;
}

bool AutotoolsProjectNode::renameFile(const FileType fileType,
                                      const QString &filePath,
                                      const QString &newFilePath)
{
    Q_UNUSED(fileType);
    Q_UNUSED(filePath);
    Q_UNUSED(newFilePath);
    return false;
}

QList<RunConfiguration *> AutotoolsProjectNode::runConfigurationsFor(Node *node)
{
    Q_UNUSED(node);
    return QList<RunConfiguration *>();
}
