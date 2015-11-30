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
    ~ResourceTopLevelNode() override;
    void update();

    QString addFileFilter() const override;

    QList<ProjectExplorer::ProjectAction> supportedActions(Node *node) const override;
    bool addFiles(const QStringList &filePaths, QStringList *notAdded) override;
    bool removeFiles(const QStringList &filePaths, QStringList *notRemoved) override;

    bool addPrefix(const QString &prefix, const QString &lang);
    bool removePrefix(const QString &prefix, const QString &lang);

    AddNewInformation addNewInformation(const QStringList &files, Node *context) const override;
    bool showInSimpleTree() const override;
    bool removeNonExistingFiles();

private:
    Internal::ResourceFileWatcher *m_document;
};

namespace Internal {

class PrefixFolderLang
{
public:
    PrefixFolderLang(QString prefix, QString folder, QString lang)
        : m_prefix(prefix)
        , m_folder(folder)
        , m_lang(lang)
    {}

    bool operator<(const PrefixFolderLang &other) const
    {
        if (m_prefix != other.m_prefix)
            return m_prefix < other.m_prefix;
        if (m_folder != other.m_folder)
            return m_folder < other.m_folder;
        if (m_lang != other.m_lang)
            return m_lang < other.m_lang;
        return false;
    }
private:
    QString m_prefix;
    QString m_folder;
    QString m_lang;
};

class ResourceFolderNode : public ProjectExplorer::FolderNode
{
    friend class ResourceEditor::ResourceTopLevelNode; // for updateFiles
public:
    ResourceFolderNode(const QString &prefix, const QString &lang, ResourceTopLevelNode *parent);
    ~ResourceFolderNode() override;

    QList<ProjectExplorer::ProjectAction> supportedActions(Node *node) const override;

    QString displayName() const override;

    bool addFiles(const QStringList &filePaths, QStringList *notAdded) override;
    bool removeFiles(const QStringList &filePaths, QStringList *notRemoved) override;
    bool renameFile(const QString &filePath, const QString &newFilePath) override;

    bool renamePrefix(const QString &prefix, const QString &lang);

    AddNewInformation addNewInformation(const QStringList &files, Node *context) const override;

    QString prefix() const;
    QString lang() const;
    ResourceTopLevelNode *resourceNode() const;
private:
    void updateFolders(QList<ProjectExplorer::FolderNode *> newList);
    void updateFiles(QList<ProjectExplorer::FileNode *> newList);
    ResourceTopLevelNode *m_topLevelNode;
    QString m_prefix;
    QString m_lang;
};

class SimpleResourceFolderNode : public ProjectExplorer::FolderNode
{
    friend class ResourceEditor::ResourceTopLevelNode;
public:
    QString displayName() const;
    SimpleResourceFolderNode(const QString &afolderName, const QString &displayName,
                     const QString &prefix, const QString &lang, Utils::FileName absolutePath,
                     ResourceTopLevelNode *topLevel, ResourceFolderNode *prefixNode);
    QList<ProjectExplorer::ProjectAction> supportedActions(ProjectExplorer::Node *node) const;
    void addFilesAndSubfolders(QMap<PrefixFolderLang, QList<ProjectExplorer::FileNode *>> filesToAdd,
                               QMap<PrefixFolderLang, QList<ProjectExplorer::FolderNode *>> foldersToAdd,
                               const QString &prefix, const QString &lang);
    bool addFiles(const QStringList &filePaths, QStringList *notAdded);
    bool removeFiles(const QStringList &filePaths, QStringList *notRemoved);
    bool renameFile(const QString &filePath, const QString &newFilePath);

    QString prefix() const;
    ResourceTopLevelNode *resourceNode() const;
    ResourceFolderNode *prefixNode() const;

private:
    void updateFiles(QList<ProjectExplorer::FileNode *> newList);
    void updateFolders(QList<ProjectExplorer::FolderNode *> newList);
    QString m_folderName;
    QString m_displayName;
    QString m_prefix;
    QString m_lang;
    ResourceTopLevelNode *m_topLevelNode;
    ResourceFolderNode *m_prefixNode;
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
