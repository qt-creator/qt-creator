/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#ifndef PROJECTMODELS_H
#define PROJECTMODELS_H

#include <QtCore/QAbstractItemModel>
#include <QtCore/QSet>

namespace ProjectExplorer {

class Node;
class FileNode;
class FolderNode;
class ProjectNode;
class SessionNode;

namespace Internal {

// A 1:1 mapping of the internal data model
class DetailedModel : public QAbstractItemModel {
    Q_OBJECT
public:
    DetailedModel(SessionNode *rootNode, QObject *parent);

    // QAbstractItemModel
    QModelIndex index(int row, int column, const QModelIndex & parent = QModelIndex()) const;
    QModelIndex parent(const QModelIndex &index) const;
    Qt::ItemFlags flags(const QModelIndex & index) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);

    int rowCount(const QModelIndex & parent = QModelIndex()) const;
    int columnCount(const QModelIndex & parent = QModelIndex()) const;
    bool hasChildren(const QModelIndex & parent = QModelIndex()) const;

    bool canFetchMore(const QModelIndex & parent) const;
    void fetchMore(const QModelIndex & parent);

    void reset();

    void setStartupProject(ProjectNode * /* projectNode */) {}

    Node *nodeForIndex(const QModelIndex &index) const;
    QModelIndex indexForNode(const Node *node);

    public slots:
    void setProjectFilterEnabled(bool /* filter */) {}

private slots:
    void foldersAboutToBeAdded(FolderNode *parentFolder,
                               const QList<FolderNode*> &newFolders);
    void foldersAdded();

    void foldersAboutToBeRemoved(FolderNode *parentFolder,
                               const QList<FolderNode*> &staleFolders);

    void filesAboutToBeAdded(FolderNode *folder,
                               const QList<FileNode*> &newFiles);
    void filesAdded();

    void filesAboutToBeRemoved(FolderNode *folder,
                               const QList<FileNode*> &staleFiles);

private:
    QList<Node*> childNodeList(FolderNode *folderNode) const;

    void connectProject(ProjectNode *project);
    void addToChildNodes(FolderNode *parentFolder, QList<Node*> newList);
    void removeFromChildNodes(FolderNode *parentFolder, QList<Node*> newList);
    QList<FolderNode*> recursiveSubFolders(FolderNode *parentFolder);

    SessionNode *m_rootNode;
    mutable QHash<FolderNode*, QList<Node*> > m_childNodes;
    bool m_isStartupProject;

    FolderNode *m_folderToAddTo;

    friend class DetailedModelManager;
};


// displays "flattened" view without pri files, subdirs pro files & virtual folders
// This view is optimized to be used in the edit mode sidebar
class FlatModel : public QAbstractItemModel {
    Q_OBJECT
public:
    FlatModel(SessionNode *rootNode, QObject *parent);

    // QAbstractItemModel
    QModelIndex index(int row, int column, const QModelIndex & parent = QModelIndex()) const;
    QModelIndex parent(const QModelIndex &index) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;

    int rowCount(const QModelIndex & parent = QModelIndex()) const;
    int columnCount(const QModelIndex & parent = QModelIndex()) const;
    bool hasChildren(const QModelIndex & parent = QModelIndex()) const;

    bool canFetchMore(const QModelIndex & parent) const;
    void fetchMore(const QModelIndex & parent);

    void reset();

    void setStartupProject(ProjectNode *projectNode);

    ProjectExplorer::Node *nodeForIndex(const QModelIndex &index) const;
    QModelIndex indexForNode(const Node *node);

    bool projectFilterEnabled();
    bool generatedFilesFilterEnabled();

public slots:
    void setProjectFilterEnabled(bool filter);
    void setGeneratedFilesFilterEnabled(bool filter);

private slots:
    void foldersAboutToBeAdded(FolderNode *parentFolder, const QList<FolderNode*> &newFolders);
    void foldersAdded();

    void foldersAboutToBeRemoved(FolderNode *parentFolder, const QList<FolderNode*> &staleFolders);
    void foldersRemoved();

    // files
    void filesAboutToBeAdded(FolderNode *folder, const QList<FileNode*> &newFiles);
    void filesAdded();

    void filesAboutToBeRemoved(FolderNode *folder, const QList<FileNode*> &staleFiles);
    void filesRemoved();

private:
    void added(FolderNode* folderNode, const QList<Node*> &newNodeList);
    void removed(FolderNode* parentNode, const QList<Node*> &newNodeList);
    void removeFromCache(QList<FolderNode *> list);
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

    friend class FlatModelManager;
};

} // namespace Internal
} // namespace ProjectExplorer


#endif // PROJECTMODELS_H
