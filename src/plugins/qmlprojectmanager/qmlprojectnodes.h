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

#ifndef QMLPROJECTNODES_H
#define QMLPROJECTNODES_H

#include <projectexplorer/projectnodes.h>

#include <QStringList>
#include <QHash>

namespace Core {
class IDocument;
}

namespace QmlProjectManager {

class QmlProject;

namespace Internal {

class QmlProjectNode : public ProjectExplorer::ProjectNode
{
public:
    QmlProjectNode(QmlProject *project, Core::IDocument *projectFile);
    virtual ~QmlProjectNode();

    Core::IDocument *projectFile() const;
    QString projectFilePath() const;

    virtual bool hasBuildTargets() const;

    virtual QList<ProjectExplorer::ProjectNode::ProjectAction> supportedActions(Node *node) const;

    virtual bool canAddSubProject(const QString &proFilePath) const;

    virtual bool addSubProjects(const QStringList &proFilePaths);
    virtual bool removeSubProjects(const QStringList &proFilePaths);

    virtual bool addFiles(const ProjectExplorer::FileType fileType,
                          const QStringList &filePaths,
                          QStringList *notAdded = 0);

    virtual bool removeFiles(const ProjectExplorer::FileType fileType,
                             const QStringList &filePaths,
                             QStringList *notRemoved = 0);

    virtual bool deleteFiles(const ProjectExplorer::FileType fileType,
                             const QStringList &filePaths);

    virtual bool renameFile(const ProjectExplorer::FileType fileType,
                             const QString &filePath,
                             const QString &newFilePath);
    virtual QList<ProjectExplorer::RunConfiguration *> runConfigurationsFor(Node *node);


    void refresh();

private:
    FolderNode *findOrCreateFolderByName(const QString &filePath);
    FolderNode *findOrCreateFolderByName(const QStringList &components, int end);

private:
    QmlProject *m_project;
    Core::IDocument *m_projectFile;
    QHash<QString, FolderNode *> m_folderByName;
};

} // namespace Internal
} // namespace QmlProjectManager

#endif // QMLPROJECTNODES_H
