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

namespace QbsProjectManager {
namespace Internal {

class QbsNodeTreeBuilder;
class QbsProject;

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
                  const QString &displayName);

private:
    bool supportsAction(ProjectExplorer::ProjectAction action, const Node *node) const final;
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
};

// --------------------------------------------------------------------
// QbsGroupNode:
// --------------------------------------------------------------------

class QbsGroupNode : public QbsBaseProjectNode
{
public:
    QbsGroupNode(const qbs::GroupData &grp, const QString &productPath);

    bool supportsAction(ProjectExplorer::ProjectAction action, const Node *node) const final;
    bool addFiles(const QStringList &filePaths, QStringList *notAdded = 0) override;
    bool removeFiles(const QStringList &filePaths, QStringList *notRemoved = 0) override;
    bool renameFile(const QString &filePath, const QString &newFilePath) override;

    qbs::GroupData qbsGroupData() const { return m_qbsGroupData; }

private:
    qbs::GroupData m_qbsGroupData;
    QString m_productPath;
};

// --------------------------------------------------------------------
// QbsProductNode:
// --------------------------------------------------------------------

class QbsProductNode : public QbsBaseProjectNode
{
public:
    explicit QbsProductNode(const qbs::ProductData &prd);

    bool showInSimpleTree() const override;
    bool supportsAction(ProjectExplorer::ProjectAction action, const Node *node) const final;
    bool addFiles(const QStringList &filePaths, QStringList *notAdded = 0) override;
    bool removeFiles(const QStringList &filePaths, QStringList *notRemoved = 0) override;
    bool renameFile(const QString &filePath, const QString &newFilePath) override;

    const qbs::ProductData qbsProductData() const { return m_qbsProductData; }

    QList<ProjectExplorer::RunConfiguration *> runConfigurations() const override;

private:
    const qbs::ProductData m_qbsProductData;
};

// ---------------------------------------------------------------------------
// QbsProjectNode:
// ---------------------------------------------------------------------------

class QbsProjectNode : public QbsBaseProjectNode
{
public:
    explicit QbsProjectNode(const Utils::FileName &projectDirectory);

    virtual QbsProject *project() const;
    const qbs::Project qbsProject() const;
    const qbs::ProjectData qbsProjectData() const { return m_projectData; }

    bool showInSimpleTree() const override;
    void setProjectData(const qbs::ProjectData &data); // FIXME: Needed?

private:
    qbs::ProjectData m_projectData;

    friend class QbsNodeTreeBuilder;
};

// --------------------------------------------------------------------
// QbsRootProjectNode:
// --------------------------------------------------------------------

class QbsRootProjectNode : public QbsProjectNode
{
public:
    explicit QbsRootProjectNode(QbsProject *project);

    QbsProject *project() const  override { return m_project; }

private:
    QbsProject *const m_project;
};


} // namespace Internal
} // namespace QbsProjectManager
