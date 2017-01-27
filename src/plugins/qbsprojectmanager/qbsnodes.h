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

#pragma once

#include <projectexplorer/projectnodes.h>

#include <qbs.h>

#include <QIcon>

namespace QbsProjectManager {
namespace Internal {

class FileTreeNode;
class QbsProject;
class QbsProjectFile;

// ----------------------------------------------------------------------
// QbsFileNode:
// ----------------------------------------------------------------------

class QbsFileNode : public ProjectExplorer::FileNode
{
public:
    QbsFileNode(const Utils::FileName &filePath, const ProjectExplorer::FileType fileType, bool generated,
                int line);

    QString displayName() const override;
};

class QbsFolderNode : public ProjectExplorer::FolderNode
{
public:
    QbsFolderNode(const Utils::FileName &folderPath, ProjectExplorer::NodeType nodeType,
                  const QString &displayName, bool isGeneratedFilesFolder);

    bool isGeneratedFilesFolder() const { return m_isGeneratedFilesFolder; }

private:
    QList<ProjectExplorer::ProjectAction> supportedActions(ProjectExplorer::Node *node) const override;

    const bool m_isGeneratedFilesFolder;
};

// ---------------------------------------------------------------------------
// QbsBaseProjectNode:
// ---------------------------------------------------------------------------

class QbsGroupNode;

class QbsBaseProjectNode : public ProjectExplorer::ProjectNode
{
public:
    explicit QbsBaseProjectNode(const Utils::FileName &absoluteFilePath);

    bool showInSimpleTree() const override;

    QList<ProjectExplorer::ProjectAction> supportedActions(Node *node) const override;
private:
    friend class QbsGroupNode;
};

// --------------------------------------------------------------------
// QbsGroupNode:
// --------------------------------------------------------------------

class QbsGroupNode : public QbsBaseProjectNode
{
public:
    QbsGroupNode(const qbs::GroupData &grp, const QString &productPath);

    QList<ProjectExplorer::ProjectAction> supportedActions(Node *node) const override;
    bool addFiles(const QStringList &filePaths, QStringList *notAdded = 0) override;
    bool removeFiles(const QStringList &filePaths, QStringList *notRemoved = 0) override;
    bool renameFile(const QString &filePath, const QString &newFilePath) override;
    void updateQbsGroupData(const qbs::GroupData &grp, const QString &productPath, bool productIsEnabled);

    qbs::GroupData qbsGroupData() const { return m_qbsGroupData; }

    QString productPath() const;

    // group can be invalid
    static void setupFiles(FolderNode *root, const qbs::GroupData &group, const QStringList &files,
                           const QString &productPath, bool generated);

private:
    static void setupFolder(ProjectExplorer::FolderNode *folder,
                            const QHash<QString, ProjectExplorer::FileType> &fileTypeHash,
                            const FileTreeNode *subFileTree, const QString &baseDir,
                            bool generated);
    static ProjectExplorer::FileType fileType(const qbs::ArtifactData &artifact);

    qbs::GroupData m_qbsGroupData;
    QString m_productPath;

    static QIcon m_groupIcon;
};

// --------------------------------------------------------------------
// QbsProductNode:
// --------------------------------------------------------------------

class QbsProductNode : public QbsBaseProjectNode
{
public:
    explicit QbsProductNode(const qbs::Project &project, const qbs::ProductData &prd);

    bool showInSimpleTree() const override;
    QList<ProjectExplorer::ProjectAction> supportedActions(Node *node) const override;
    bool addFiles(const QStringList &filePaths, QStringList *notAdded = 0) override;
    bool removeFiles(const QStringList &filePaths, QStringList *notRemoved = 0) override;
    bool renameFile(const QString &filePath, const QString &newFilePath) override;

    void setQbsProductData(const qbs::Project &project, const qbs::ProductData prd);
    const qbs::ProductData qbsProductData() const { return m_qbsProductData; }

    QList<ProjectExplorer::RunConfiguration *> runConfigurations() const override;

private:
    QbsGroupNode *findGroupNode(const QString &name);

    qbs::ProductData m_qbsProductData;
    ProjectExplorer::FolderNode * const m_generatedFilesNode;
    static QIcon m_productIcon;
};

// ---------------------------------------------------------------------------
// QbsProjectNode:
// ---------------------------------------------------------------------------

class QbsProjectNode : public QbsBaseProjectNode
{
public:
    explicit QbsProjectNode(const Utils::FileName &absoluteFilePath);
    ~QbsProjectNode() override;

    virtual QbsProject *project() const;
    const qbs::Project qbsProject() const;
    const qbs::ProjectData qbsProjectData() const { return m_projectData; }

    bool showInSimpleTree() const override;

protected:
    void update(const qbs::Project &qbsProject, const qbs::ProjectData &prjData);

private:
    void ctor();

    QbsProductNode *findProductNode(const QString &uniqueName);
    QbsProjectNode *findProjectNode(const QString &name);

    static QIcon m_projectIcon;
    qbs::ProjectData m_projectData;
};

// --------------------------------------------------------------------
// QbsRootProjectNode:
// --------------------------------------------------------------------

class QbsRootProjectNode : public QbsProjectNode
{
public:
    explicit QbsRootProjectNode(QbsProject *project);

    using QbsProjectNode::update;
    void update();

    QbsProject *project() const  override { return m_project; }

private:
    QStringList unreferencedBuildSystemFiles(const qbs::Project &p) const;

    QbsProject *const m_project;
    ProjectExplorer::FolderNode *m_buildSystemFiles;
};


} // namespace Internal
} // namespace QbsProjectManager
