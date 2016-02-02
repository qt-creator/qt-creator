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

#include <utils/fileutils.h>

#include <QAbstractItemModel>
#include <QSet>

namespace ProjectExplorer {

class Node;
class FileNode;
class FolderNode;
class ProjectNode;
class SessionNode;

namespace Internal {

class FlatModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    FlatModel(SessionNode *rootNode, QObject *parent);

    // QAbstractItemModel
    QModelIndex index(int row, int column, const QModelIndex & parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &index) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role) override;

    int rowCount(const QModelIndex & parent = QModelIndex()) const override;
    int columnCount(const QModelIndex & parent = QModelIndex()) const override;
    bool hasChildren(const QModelIndex & parent = QModelIndex()) const override;

    bool canFetchMore(const QModelIndex & parent) const override;
    void fetchMore(const QModelIndex & parent) override;

    void reset();

    Qt::DropActions supportedDragActions() const override;
    QStringList mimeTypes() const override;
    QMimeData *mimeData(const QModelIndexList &indexes) const override;

    void setStartupProject(ProjectNode *projectNode);

    Node *nodeForIndex(const QModelIndex &index) const;
    QModelIndex indexForNode(const Node *node) const;

    bool projectFilterEnabled();
    bool generatedFilesFilterEnabled();

signals:
    void renamed(const Utils::FileName &oldName, const Utils::FileName &newName);

public slots:
    void setProjectFilterEnabled(bool filter);
    void setGeneratedFilesFilterEnabled(bool filter);

private:
    void aboutToShowInSimpleTreeChanged(ProjectExplorer::FolderNode *node);
    void showInSimpleTreeChanged(ProjectExplorer::FolderNode *node);
    void foldersAboutToBeAdded(FolderNode *parentFolder, const QList<FolderNode*> &newFolders);
    void foldersAdded();

    void foldersAboutToBeRemoved(FolderNode *parentFolder, const QList<FolderNode*> &staleFolders);
    void foldersRemoved();

    // files
    void filesAboutToBeAdded(FolderNode *folder, const QList<FileNode*> &newFiles);
    void filesAdded();

    void filesAboutToBeRemoved(FolderNode *folder, const QList<FileNode*> &staleFiles);
    void filesRemoved();

    void nodeSortKeyAboutToChange(Node *node);
    void nodeSortKeyChanged();

    void nodeUpdated(ProjectExplorer::Node *node);

    void added(FolderNode* folderNode, const QList<Node*> &newNodeList);
    void removed(FolderNode* parentNode, const QList<Node*> &newNodeList);
    void removeFromCache(QList<FolderNode *> list);
    void changedSortKey(FolderNode *folderNode, Node *node);
    void fetchMore(FolderNode *foldernode) const;

    void recursiveAddFolderNodes(FolderNode *startNode, QList<Node *> *list, const QSet<Node *> &blackList = QSet<Node*>()) const;
    void recursiveAddFolderNodesImpl(FolderNode *startNode, QList<Node *> *list, const QSet<Node *> &blackList = QSet<Node*>()) const;
    void recursiveAddFileNodes(FolderNode *startNode, QList<Node *> *list, const QSet<Node *> &blackList = QSet<Node*>()) const;
    QList<Node*> childNodes(FolderNode *parentNode, const QSet<Node*> &blackList = QSet<Node*>()) const;

    FolderNode *visibleFolderNode(FolderNode *node) const;
    bool filter(Node *node) const;

    bool m_filterProjects;
    bool m_filterGeneratedFiles;

    SessionNode *m_rootNode;
    mutable QHash<FolderNode*, QList<Node*> > m_childNodes;
    ProjectNode *m_startupProject;

    FolderNode *m_parentFolderForChange;
    Node *m_nodeForSortKeyChange;

    static const QLoggingCategory &logger();

    friend class FlatModelManager;
};

int caseFriendlyCompare(const QString &a, const QString &b);

} // namespace Internal
} // namespace ProjectExplorer
