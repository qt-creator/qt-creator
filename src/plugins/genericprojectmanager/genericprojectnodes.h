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

#ifndef GENERICPROJECTNODE_H
#define GENERICPROJECTNODE_H

#include <projectexplorer/projectnodes.h>

#include <QStringList>
#include <QHash>
#include <QSet>

namespace Core { class IDocument; }

namespace GenericProjectManager {
namespace Internal {

class GenericProject;

class GenericProjectNode : public ProjectExplorer::ProjectNode
{
public:
    GenericProjectNode(GenericProject *project);

    bool showInSimpleTree() const override;

    QList<ProjectExplorer::ProjectAction> supportedActions(Node *node) const override;

    bool addFiles(const QStringList &filePaths, QStringList *notAdded = 0) override;
    bool removeFiles(const QStringList &filePaths, QStringList *notRemoved = 0) override;
    bool renameFile(const QString &filePath, const QString &newFilePath) override;

    void refresh(QSet<QString> oldFileList = QSet<QString>());

private:
    typedef QHash<QString, FolderNode *> FolderByName;
    FolderNode *createFolderByName(const QStringList &components, int end);
    FolderNode *findFolderByName(const QStringList &components, int end);
    void removeEmptySubFolders(FolderNode *gparent, FolderNode *parent);

private:
    GenericProject *m_project;
};

} // namespace Internal
} // namespace GenericProjectManager

#endif // GENERICPROJECTNODE_H
