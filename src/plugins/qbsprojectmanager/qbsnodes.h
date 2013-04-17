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

#ifndef QBSNODES_H
#define QBSNODES_H

#include <projectexplorer/projectnodes.h>

#include <api/projectdata.h>

#include <QIcon>

namespace qbs { class Project; }

namespace QbsProjectManager {
namespace Internal {

class FileTreeNode;
class QbsProjectFile;

// ----------------------------------------------------------------------
// QbsFileNode:
// ----------------------------------------------------------------------

class QbsFileNode : public ProjectExplorer::FileNode
{
    Q_OBJECT
public:
    QbsFileNode(const QString &filePath, const ProjectExplorer::FileType fileType, bool generated,
                int line);
    int line() const { return m_line; }

    void setLine(int l);

private:
    int m_line;
};

// ---------------------------------------------------------------------------
// QbsBaseProjectNode:
// ---------------------------------------------------------------------------

class QbsGroupNode;

class QbsBaseProjectNode : public ProjectExplorer::ProjectNode
{
    Q_OBJECT

public:
    explicit QbsBaseProjectNode(const QString &path);

    bool hasBuildTargets() const;

    QList<ProjectAction> supportedActions(Node *node) const;

    bool canAddSubProject(const QString &proFilePath) const;

    bool addSubProjects(const QStringList &proFilePaths);

    bool removeSubProjects(const QStringList &proFilePaths);

    bool addFiles(const ProjectExplorer::FileType fileType,
                  const QStringList &filePaths,
                  QStringList *notAdded = 0);
    bool removeFiles(const ProjectExplorer::FileType fileType,
                     const QStringList &filePaths,
                     QStringList *notRemoved = 0);
    bool deleteFiles(const ProjectExplorer::FileType fileType,
                     const QStringList &filePaths);
    bool renameFile(const ProjectExplorer::FileType fileType,
                     const QString &filePath,
                     const QString &newFilePath);

    QList<ProjectExplorer::RunConfiguration *> runConfigurationsFor(Node *node);

private:
    friend class QbsGroupNode;
};

// --------------------------------------------------------------------
// QbsGroupNode:
// --------------------------------------------------------------------

class QbsGroupNode : public QbsBaseProjectNode
{
    Q_OBJECT

public:
    QbsGroupNode(const qbs::GroupData *grp, const QString &productPath);

    bool isEnabled() const;
    void setGroup(const qbs::GroupData *group, const QString &productPath);
    const qbs::GroupData *group() const { return m_group; }

    QString productPath() const;

    static void setGroup(QbsBaseProjectNode *root, const qbs::GroupData *group,
                         const QString &productPath, QList<Node *> keepers);

private:
    static void setupFolders(QbsBaseProjectNode *topLevel, FolderNode *root, FileTreeNode *node,
                             const QString &baseDirPath,
                             QList<ProjectExplorer::Node *> keepers = QList<ProjectExplorer::Node *>());

    const qbs::GroupData *m_group;
    QString m_productPath;

    static QIcon m_groupIcon;
};

// --------------------------------------------------------------------
// QbsProductNode:
// --------------------------------------------------------------------

class QbsProductNode : public QbsBaseProjectNode
{
    Q_OBJECT

public:
    explicit QbsProductNode(const qbs::ProductData *prd);

    bool isEnabled() const;

    void setProduct(const qbs::ProductData *prd);
    const qbs::ProductData *product() const { return m_product; }

private:
    QbsGroupNode *findGroupNode(const QString &name);

    const qbs::ProductData *m_product;
    static QIcon m_productIcon;
};

// ---------------------------------------------------------------------------
// QbsProjectNode:
// ---------------------------------------------------------------------------

class QbsProjectNode : public QbsBaseProjectNode
{
    Q_OBJECT

public:
    explicit QbsProjectNode(const QString &projectFile);
    ~QbsProjectNode();

    void update(const qbs::Project *prj);

    const qbs::Project *project() const;
    const qbs::ProjectData *projectData() const;

private:
    QbsProductNode *findProductNode(const QString &name);

    const qbs::Project *m_project;
    const qbs::ProjectData *m_projectData;
    static QIcon m_projectIcon;
};
} // namespace Internal
} // namespace QbsProjectManager

#endif // QBSNODES_H
