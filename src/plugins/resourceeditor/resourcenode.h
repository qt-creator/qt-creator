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

#ifndef RESOURCENODE_H
#define RESOURCENODE_H

#include "resource_global.h"
#include <projectexplorer/projectnodes.h>
#include <coreplugin/idocument.h>

namespace ProjectExplorer {
class RunConfiguration;
}

namespace ResourceEditor {
namespace Internal { class ResourceFileWatcher; }

class RESOURCE_EXPORT ResourceTopLevelNode : public ProjectExplorer::FolderNode
{
public:
    ResourceTopLevelNode(const Utils::FileName &filePath, FolderNode *parent);
    ~ResourceTopLevelNode();
    void update();

    QList<ProjectExplorer::ProjectAction> supportedActions(Node *node) const;
    bool addFiles(const QStringList &filePaths, QStringList *notAdded);
    bool removeFiles(const QStringList &filePaths, QStringList *notRemoved);

    bool addPrefix(const QString &prefix, const QString &lang);
    bool removePrefix(const QString &prefix, const QString &lang);

    AddNewInformation addNewInformation(const QStringList &files, Node *context) const;
    bool showInSimpleTree() const;
    bool removeNonExistingFiles();

private:
    Internal::ResourceFileWatcher *m_document;
};

namespace Internal {
class ResourceFolderNode : public ProjectExplorer::FolderNode
{
    friend class ResourceEditor::ResourceTopLevelNode; // for updateFiles
public:
    ResourceFolderNode(const QString &prefix, const QString &lang, ResourceTopLevelNode *parent);
    ~ResourceFolderNode();

    QList<ProjectExplorer::ProjectAction> supportedActions(Node *node) const;

    QString displayName() const;

    bool addFiles(const QStringList &filePaths, QStringList *notAdded);
    bool removeFiles(const QStringList &filePaths, QStringList *notRemoved);
    bool renameFile(const QString &filePath, const QString &newFilePath);

    bool renamePrefix(const QString &prefix, const QString &lang);

    AddNewInformation addNewInformation(const QStringList &files, Node *context) const;

    QString prefix() const;
    QString lang() const;
    ResourceTopLevelNode *resourceNode() const;
private:
    void updateFiles(QList<ProjectExplorer::FileNode *> newList);
    ResourceTopLevelNode *m_topLevelNode;
    QString m_prefix;
    QString m_lang;
};

class ResourceFileNode : public ProjectExplorer::FileNode
{
public:
    ResourceFileNode(const Utils::FileName &filePath, const QString &qrcPath, ResourceTopLevelNode *topLevel);

    QString displayName() const;
    QString qrcPath() const;
    QList<ProjectExplorer::ProjectAction> supportedActions(Node *node) const;

private:
    QString m_displayName;
    QString m_qrcPath;
};

class ResourceFileWatcher : public Core::IDocument
{
    Q_OBJECT
public:
    ResourceFileWatcher(ResourceTopLevelNode *node);
    bool save(QString *errorString, const QString &fileName, bool autoSave) override;

    QString defaultPath() const override;
    QString suggestedFileName() const override;

    bool isModified() const override;
    bool isSaveAsAllowed() const override;

    ReloadBehavior reloadBehavior(ChangeTrigger state, ChangeType type) const override;
    bool reload(QString *errorString, ReloadFlag flag, ChangeType type) override;
private:
    ResourceTopLevelNode *m_node;
};
} // namespace Internal
} // namespace ResourceEditor

#endif // RESOUCENODE_H
