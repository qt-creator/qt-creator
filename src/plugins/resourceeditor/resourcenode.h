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

#include "resource_global.h"
#include <projectexplorer/projectnodes.h>

namespace ResourceEditor {
namespace Internal { class ResourceFileWatcher; }

class RESOURCE_EXPORT ResourceTopLevelNode : public ProjectExplorer::FolderNode
{
public:
    ResourceTopLevelNode(const Utils::FileName &filePath, const QString &contents, FolderNode *parent);
    ~ResourceTopLevelNode() override;

    void addInternalNodes();

    QString addFileFilter() const override;

    QList<ProjectExplorer::ProjectAction> supportedActions(Node *node) const override;
    bool addFiles(const QStringList &filePaths, QStringList *notAdded) override;
    bool removeFiles(const QStringList &filePaths, QStringList *notRemoved) override;

    bool addPrefix(const QString &prefix, const QString &lang);
    bool removePrefix(const QString &prefix, const QString &lang);

    AddNewInformation addNewInformation(const QStringList &files, Node *context) const override;
    bool showInSimpleTree() const override;
    bool removeNonExistingFiles();

    QString contents() const { return m_contents; }

private:
    Internal::ResourceFileWatcher *m_document;
    QString m_contents;
};

namespace Internal {

class ResourceFolderNode : public ProjectExplorer::FolderNode
{
public:
    ResourceFolderNode(const QString &prefix, const QString &lang, ResourceTopLevelNode *parent);
    ~ResourceFolderNode() override;

    QList<ProjectExplorer::ProjectAction> supportedActions(Node *node) const override;

    QString displayName() const override;

    bool addFiles(const QStringList &filePaths, QStringList *notAdded) override;
    bool removeFiles(const QStringList &filePaths, QStringList *notRemoved) override;
    bool canRenameFile(const QString &filePath, const QString &newFilePath) override;
    bool renameFile(const QString &filePath, const QString &newFilePath) override;

    bool renamePrefix(const QString &prefix, const QString &lang);

    AddNewInformation addNewInformation(const QStringList &files, Node *context) const override;

    QString prefix() const;
    QString lang() const;
    ResourceTopLevelNode *resourceNode() const;

private:
    ResourceTopLevelNode *m_topLevelNode;
    QString m_prefix;
    QString m_lang;
};

class ResourceFileNode : public ProjectExplorer::FileNode
{
public:
    ResourceFileNode(const Utils::FileName &filePath, const QString &qrcPath, const QString &displayName);

    QString displayName() const override;
    QString qrcPath() const;
    QList<ProjectExplorer::ProjectAction> supportedActions(Node *node) const override;

private:
    QString m_qrcPath;
    QString m_displayName;
};

} // namespace Internal
} // namespace ResourceEditor
