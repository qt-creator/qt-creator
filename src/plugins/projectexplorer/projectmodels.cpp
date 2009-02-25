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

#include "projectmodels.h"

#include "project.h"
#include "projectexplorerconstants.h"

#include <coreplugin/fileiconprovider.h>
#include <utils/qtcassert.h>

#include <QtCore/QDebug>
#include <QtCore/QFileInfo>

#include <QtGui/QApplication>
#include <QtGui/QIcon>
#include <QtGui/QMessageBox>
#include <QtGui/QSortFilterProxyModel>
#include <QtGui/QStyle>

using namespace ProjectExplorer;
using namespace ProjectExplorer::Internal;
using Core::FileIconProvider;

namespace {

// sorting helper function
bool sortNodes(Node *n1, Node *n2)
{
    // Ordering is: project files, project, folder, file

    const NodeType n1Type = n1->nodeType();
    const NodeType n2Type = n2->nodeType();

    // project files
    FileNode *file1 = qobject_cast<FileNode*>(n1);
    FileNode *file2 = qobject_cast<FileNode*>(n2);
    if (file1 && file1->fileType() == ProjectFileType) {
        if (file2 && file2->fileType() == ProjectFileType) {
            const QString fileName1 = QFileInfo(file1->path()).fileName();
            const QString fileName2 = QFileInfo(file2->path()).fileName();

            if (fileName1 != fileName2)
                return fileName1 < fileName2;
            else
                return file1 < file2;
        } else {
            return true; // project file is before everything else
        }
    } else {
        if (file2 && file2->fileType() == ProjectFileType) {
            return false;
        }
    }

    // projects
    if (n1Type == ProjectNodeType) {
        if (n2Type == ProjectNodeType) {
            ProjectNode *project1 = static_cast<ProjectNode*>(n1);
            ProjectNode *project2 = static_cast<ProjectNode*>(n2);

            if (project1->name() != project2->name())
                return project1->name() < project2->name(); // sort by name
            else
                return project1 < project2; // sort by pointer value
        } else {
           return true; // project is before folder & file
       }
    }
    if (n2Type == ProjectNodeType)
        return false;

    if (n1Type == FolderNodeType) {
        if (n2Type == FolderNodeType) {
            FolderNode *folder1 = static_cast<FolderNode*>(n1);
            FolderNode *folder2 = static_cast<FolderNode*>(n2);

            if (folder1->name() != folder2->name())
                return folder1->name() < folder2->name();
            else
                return folder1 < folder2;
        } else {
            return true; // folder is before file
        }
    }
    if (n2Type == FolderNodeType)
        return false;

    // must be file nodes
    {
        const QString filePath1 = n1->path();
        const QString filePath2 = n2->path();

        const QString fileName1 = QFileInfo(filePath1).fileName();
        const QString fileName2 = QFileInfo(filePath2).fileName();

        if (fileName1 != fileName2) {
            return fileName1 < fileName2; // sort by file names
        } else {
            if (filePath1 != filePath2) {
                return filePath1 < filePath2; // sort by path names
            } else {
                return n1 < n2; // sort by pointer value
            }
        }
    }
    return false;
}

} // namespace anon

/*!
  \class DetailedModel

  A 1:1 mapping of the internal node tree.

  The detailed model shows the complete project file hierarchy and all containing files.
  */

DetailedModel::DetailedModel(SessionNode *rootNode, QObject *parent)
        : QAbstractItemModel(parent),
          m_rootNode(rootNode),
          //m_startupProject(0),
          m_folderToAddTo(0)
{
    NodesWatcher *watcher = new NodesWatcher(this);
    m_rootNode->registerWatcher(watcher);
    connect(watcher, SIGNAL(foldersAboutToBeAdded(FolderNode*, const QList<FolderNode*> &)),
            this, SLOT(foldersAboutToBeAdded(FolderNode*, const QList<FolderNode*> &)));
    connect(watcher, SIGNAL(foldersAdded()),
            this, SLOT(foldersAdded()));
    connect(watcher, SIGNAL(foldersAboutToBeRemoved(FolderNode*, const QList<FolderNode*> &)),
            this, SLOT(foldersAboutToBeRemoved(FolderNode*, const QList<FolderNode*> &)));
    connect(watcher, SIGNAL(filesAboutToBeAdded(FolderNode*, const QList<FileNode*> &)),
            this, SLOT(filesAboutToBeAdded(FolderNode*, const QList<FileNode*> &)));
    connect(watcher, SIGNAL(filesAdded()),
            this, SLOT(filesAdded()));
    connect(watcher, SIGNAL(filesAboutToBeRemoved(FolderNode*, const QList<FileNode*> &)),
            this, SLOT(filesAboutToBeRemoved(FolderNode*, const QList<FileNode*> &)));
}

QModelIndex DetailedModel::index(int row, int column, const QModelIndex &parent) const
{
    QModelIndex result;
    if (!parent.isValid() && row == 0 && column == 0) {
        result = createIndex(0, 0, m_rootNode);
    } else if (column == 0) {
        FolderNode *parentNode = qobject_cast<FolderNode*>(nodeForIndex(parent));
        Q_ASSERT(parentNode);
        result = createIndex(row, 0, m_childNodes.value(parentNode).at(row));
    }
    return result;
}

QModelIndex DetailedModel::parent(const QModelIndex &index) const
{
    QModelIndex parentIndex;

    if (Node *node = nodeForIndex(index)) {
        if (FolderNode *parentFolderNode = node->parentFolderNode()) {
            if (FolderNode *grandParentFolderNode = parentFolderNode->parentFolderNode()) {
                Q_ASSERT(m_childNodes.contains(grandParentFolderNode));
                int row = m_childNodes.value(grandParentFolderNode).indexOf(parentFolderNode);
                parentIndex = createIndex(row, 0, parentFolderNode);
            } else {
                parentIndex = createIndex(0, 0, parentFolderNode);
            }
        }
    }

    return parentIndex;
}

Qt::ItemFlags DetailedModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags result;
    if (index.isValid()) {
        result = Qt::ItemIsEnabled | Qt::ItemIsSelectable;

        if (Node *node = nodeForIndex(index)) {
            if (node->nodeType() == FileNodeType)
                result |= Qt::ItemIsEditable;
        }
    }
    return result;
}

QVariant DetailedModel::data(const QModelIndex &index, int role) const
{
    QVariant result;

    if (Node *node = nodeForIndex(index)) {
        FolderNode *folderNode = qobject_cast<FolderNode*>(node);
        switch (role) {
        case Qt::DisplayRole:
        case Qt::EditRole: {
            if (folderNode)
                result = folderNode->name();
            else
                result = QFileInfo(node->path()).fileName(); //TODO cache that?
            break;
        }
        case Qt::ToolTipRole: {
            if (folderNode && (folderNode->nodeType() != ProjectNodeType))
                result = tr("%1 of project %2").arg(folderNode->name()).arg(folderNode->projectNode()->name());
            else
                result = node->path();
            break;
        }
        case Qt::DecorationRole: {
            if (folderNode)
                result = folderNode->icon();
            else
                result = FileIconProvider::instance()->icon(QFileInfo(node->path()));
            break;
        }
        case Qt::FontRole: {
            QFont font;
            if (qobject_cast<ProjectNode*>(folderNode)) {
                if (index == this->index(0,0) && m_isStartupProject)
                    font.setBold(true);
            }
            result = font;
            break;
        }
        case ProjectExplorer::Project::FilePathRole: {
            result = node->path();
            break;
        }
        }
    }

    return result;
}

bool DetailedModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    bool result = false;

    if (Node *node = nodeForIndex(index)) {
        FileNode *fileNode = qobject_cast<FileNode*>(node);
        if (fileNode && role == Qt::EditRole && !value.toString().isEmpty()) {
            ProjectNode *projectNode = node->projectNode();
            QString newPath = QFileInfo(fileNode->path()).dir().absoluteFilePath(value.toString());
            if (!projectNode->renameFile(fileNode->fileType(), fileNode->path(), newPath))
                QMessageBox::warning(0, tr("Could not rename file"),
                                     tr("Renaming file %1 to %2 failed.").arg(fileNode->path())
                                                                         .arg(value.toString()));
        }
    }

    return result;
}

int DetailedModel::rowCount(const QModelIndex & parent) const
{
    int count = 0;

    if (!parent.isValid()) { // root item
        count = 1;
    } else {
        // we can be sure that internal cache
        // has been updated by fetchMore()
        FolderNode *folderNode = qobject_cast<FolderNode*>(nodeForIndex(parent));
        if (folderNode && m_childNodes.contains(folderNode))
            count = m_childNodes.value(folderNode).size();
    }
    return count;
}

int DetailedModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 1;
}

bool DetailedModel::hasChildren(const QModelIndex &parent) const
{
    bool hasChilds = false;

    if (!parent.isValid()) {// root index
        hasChilds = true;
    } else {
        if (FolderNode *folderNode = qobject_cast<FolderNode*>(nodeForIndex(parent))) {
            if (m_childNodes.contains(folderNode)) // internal cache
                hasChilds = !m_childNodes.value(folderNode).isEmpty();
            else {
                if (!folderNode->subFolderNodes().isEmpty()
                    || !folderNode->fileNodes().isEmpty()) // underlying data
                    hasChilds = true;
                else {
                    // Make sure add/remove signals are emitted
                    // even for empty nodes (i.e. where canFetchMore
                    // returns false)
                    m_childNodes.insert(folderNode, QList<Node*>());
                }
            }
        }
    }

    return hasChilds;
}

/*!
  Returns true if internal cache has not been built up yet
  */
bool DetailedModel::canFetchMore(const QModelIndex & parent) const
{
    bool canFetch = false;
    if (parent.isValid()) {
        if (FolderNode *folderNode = qobject_cast<FolderNode*>(nodeForIndex(parent))) {
            canFetch = !m_childNodes.contains(folderNode);
        }
    }
    return canFetch;
}

/*!
  Updates internal cache
  */
void DetailedModel::fetchMore(const QModelIndex & parent)
{
    FolderNode *folderNode = qobject_cast<FolderNode*>(nodeForIndex(parent));
    Q_ASSERT(folderNode);
    Q_ASSERT(!m_childNodes.contains(folderNode));

    m_childNodes.insert(folderNode, childNodeList(folderNode));
}

void DetailedModel::reset()
{
    // todo:How to implement this correctly?????
    m_childNodes.clear();
    QAbstractItemModel::reset();
}

void DetailedModel::foldersAboutToBeAdded(FolderNode *parentFolder,
                           const QList<FolderNode*> &newFolders)
{
    Q_UNUSED(newFolders);
    Q_ASSERT(parentFolder);

    if (m_childNodes.contains(parentFolder))
        m_folderToAddTo = parentFolder;
}

void DetailedModel::foldersAdded()
{
    if (m_folderToAddTo) {
        QList<Node*> newChildNodes = childNodeList(m_folderToAddTo);
        addToChildNodes(m_folderToAddTo, newChildNodes);
        m_folderToAddTo = 0;
    }
}

void DetailedModel::foldersAboutToBeRemoved(FolderNode *parentFolder,
                           const QList<FolderNode*> &staleFolders)
{
    Q_ASSERT(parentFolder);

    if (m_childNodes.contains(parentFolder)) {
        QList<Node*> newChildNodes = m_childNodes.value(parentFolder);
        QList<FolderNode*> nodesToRemove = staleFolders;
        qSort(nodesToRemove.begin(), nodesToRemove.end(), sortNodes);

        QList<Node*>::iterator newListIter = newChildNodes.begin();
        QList<FolderNode*>::const_iterator toRemoveIter = nodesToRemove.constBegin();
        for (; toRemoveIter != nodesToRemove.constEnd(); ++toRemoveIter) {
            while (*newListIter != *toRemoveIter)
                ++newListIter;
            newListIter = newChildNodes.erase(newListIter);
        }

        removeFromChildNodes(parentFolder, newChildNodes);

        // Clear cache for all folders beneath the current folder
        foreach (FolderNode *folder, staleFolders) {
            foreach (FolderNode *subFolder, recursiveSubFolders(folder)) {
                m_childNodes.remove(subFolder);
            }
        }
    }
}

void DetailedModel::filesAboutToBeAdded(FolderNode *parentFolder,
                           const QList<FileNode*> &newFiles)
{
    Q_UNUSED(newFiles);
    Q_ASSERT(parentFolder);

    if (m_childNodes.contains(parentFolder))
        m_folderToAddTo = parentFolder;
}

void DetailedModel::filesAdded()
{
    if (m_folderToAddTo) {
        QList<Node*> newChildNodes = childNodeList(m_folderToAddTo);
        addToChildNodes(m_folderToAddTo, newChildNodes);
        m_folderToAddTo = 0;
    }
}

void DetailedModel::filesAboutToBeRemoved(FolderNode *parentFolder,
                           const QList<FileNode*> &staleFiles)
{
    Q_ASSERT(parentFolder);

    if (m_childNodes.contains(parentFolder)) {
        QList<Node*> newChildNodes = m_childNodes.value(parentFolder);
        QList<FileNode*> nodesToRemove = staleFiles;
        qSort(nodesToRemove.begin(), nodesToRemove.end(), sortNodes);

        QList<Node*>::iterator newListIter = newChildNodes.begin();
        QList<FileNode*>::const_iterator toRemoveIter = nodesToRemove.constBegin();
        for (; toRemoveIter != nodesToRemove.constEnd(); ++toRemoveIter) {
            while (*newListIter != *toRemoveIter)
                ++newListIter;
            newListIter = newChildNodes.erase(newListIter);
        }

        removeFromChildNodes(parentFolder, newChildNodes);
    }
}

Node *DetailedModel::nodeForIndex(const QModelIndex &index) const
{
    return (Node*)index.internalPointer();
}

/*!
  Returns the index corresponding to a node.
 */
QModelIndex DetailedModel::indexForNode(const Node *node)
{
    if (!node)
        return QModelIndex();

    if (FolderNode *parentFolder = node->parentFolderNode()) {
        // iterate recursively
        QModelIndex parentIndex = indexForNode(parentFolder);

        // update internal cache
        if (canFetchMore(parentIndex))
            fetchMore(parentIndex);
        Q_ASSERT(m_childNodes.contains(parentFolder));

        int row = m_childNodes.value(parentFolder).indexOf(const_cast<Node*>(node));
        if (row >= 0)
            return index(row, 0, parentIndex);
        else
            return QModelIndex();
    } else {
        // root node
        return index(0, 0);
    }
}

QList<Node*> DetailedModel::childNodeList(FolderNode *folderNode) const
{
    QList<FolderNode*> folderNodes = folderNode->subFolderNodes();
    QList<FileNode*> fileNodes = folderNode->fileNodes();

    QList<Node*> nodes;
    foreach (FolderNode *folderNode, folderNodes)
        nodes << folderNode;
    foreach (FileNode *fileNode, fileNodes)
        nodes << fileNode;

    qSort(nodes.begin(), nodes.end(), sortNodes);

    return nodes;
}

void DetailedModel::addToChildNodes(FolderNode *parentFolder, QList<Node*> newChildNodes)
{
    QList<Node*> childNodes = m_childNodes.value(parentFolder);
    QModelIndex parentIndex = indexForNode(parentFolder);
    Q_ASSERT(parentIndex.isValid());

    // position -> nodes, with positions in decreasing order
    QList<QPair<int, QList<Node*> > > insertions;

    // add nodes that should be added at the end or in between
    int newIndex = newChildNodes.size() - 1;
    for (int oldIndex = childNodes.size() - 1;
         oldIndex >= 0; --oldIndex, --newIndex) {
        QList<Node*> nodesPerIndex;
        Node* oldNode = childNodes.at(oldIndex);
        while (newChildNodes.at(newIndex) != oldNode) {
            nodesPerIndex.append(newChildNodes.at(newIndex));
            --newIndex;
        }
        if (!nodesPerIndex.isEmpty())
            insertions.append(QPair<int, QList<Node*> >(oldIndex + 1, nodesPerIndex));
    }
    { // add nodes that should be added to the beginning
        QList<Node*> insertAtStart;
        while (newIndex >= 0) {
            insertAtStart.append(newChildNodes.at(newIndex--));
        }
        if (!insertAtStart.isEmpty())
            insertions.append(QPair<int, QList<Node*> >(0, insertAtStart));
    }

    for (QList<QPair<int, QList<Node*> > >::const_iterator iter = insertions.constBegin();
         iter != insertions.constEnd(); ++iter) {

        const int key = iter->first;
        const QList<Node*> nodesToInsert = iter->second;

        beginInsertRows(parentIndex, key, key + nodesToInsert.size() - 1);

        // update internal cache
        for (QList<Node*>::const_iterator nodeIterator = nodesToInsert.constBegin();
             nodeIterator != nodesToInsert.constEnd(); ++nodeIterator)
            childNodes.insert(key, *nodeIterator);
        m_childNodes.insert(parentFolder, childNodes);

        endInsertRows();
    }

    Q_ASSERT(childNodes == newChildNodes);
}

void DetailedModel::removeFromChildNodes(FolderNode *parentFolder, QList<Node*> newChildNodes)
{
    QList<Node*> childNodes = m_childNodes.value(parentFolder);
    QModelIndex parentIndex = indexForNode(parentFolder);
    Q_ASSERT(parentIndex.isValid());

    // position -> nodes, with positions in decreasing order
    QList<QPair<int, QList<Node*> > > deletions;

    // add nodes that should be removed at the end or in between
    int oldIndex = childNodes.size() - 1;
    for (int newIndex = newChildNodes.size() - 1;
         newIndex >= 0; --oldIndex, --newIndex) {
        QList<Node*> nodesPerIndex;
        Node* newNode = newChildNodes.at(newIndex);
        while (childNodes.at(oldIndex) != newNode) {
            nodesPerIndex.append(childNodes.at(oldIndex));
            --oldIndex;
        }
        if (!nodesPerIndex.isEmpty())
            deletions.append(QPair<int, QList<Node*> >(oldIndex + 1, nodesPerIndex));
    }
    { // add nodes that should be removed to the beginning
        QList<Node*> deleteAtStart;
        while (oldIndex >= 0) {
            deleteAtStart.append(childNodes.at(oldIndex--));
        }
        if (!deleteAtStart.isEmpty())
            deletions.append(QPair<int, QList<Node*> >(0, deleteAtStart));
    }

    for (QList<QPair<int, QList<Node*> > >::const_iterator iter = deletions.constBegin();
         iter != deletions.constEnd(); ++iter) {

        const int key = iter->first;
        const QList<Node*> nodes = iter->second;

        beginRemoveRows(parentIndex, key, key + nodes.size() - 1);

        // update internal cache
        for (int i = nodes.size(); i > 0; --i)
            childNodes.removeAt(key);
        m_childNodes.insert(parentFolder, childNodes);

        endRemoveRows();
    }

    Q_ASSERT(childNodes == newChildNodes);
}

QList<FolderNode*> DetailedModel::recursiveSubFolders(FolderNode *parentFolder)
{
    QList<FolderNode*> subFolders;
    foreach (FolderNode *subFolder, parentFolder->subFolderNodes()) {
        subFolders.append(subFolder);
        subFolders != recursiveSubFolders(subFolder);
    }
    return subFolders;
}


/*!
  \class FlatModel

  The flat model shows only application/library projects.


  Shows all application/library projects directly unter the root project
  This results in a "flattened" view (max 3 hierachies).
  */

FlatModel::FlatModel(SessionNode *rootNode, QObject *parent)
        : QAbstractItemModel(parent),
          m_filterProjects(false),
          m_filterGeneratedFiles(true),
          m_rootNode(rootNode),
          m_startupProject(0),
          m_parentFolderForChange(0)
{
    NodesWatcher *watcher = new NodesWatcher(this);
    m_rootNode->registerWatcher(watcher);

    connect(watcher, SIGNAL(foldersAboutToBeAdded(FolderNode *, const QList<FolderNode*> &)),
            this, SLOT(foldersAboutToBeAdded(FolderNode *, const QList<FolderNode*> &)));
    connect(watcher, SIGNAL(foldersAdded()),
            this, SLOT(foldersAdded()));

    connect(watcher, SIGNAL(foldersAboutToBeRemoved(FolderNode *, const QList<FolderNode*> &)),
            this, SLOT(foldersAboutToBeRemoved(FolderNode *, const QList<FolderNode*> &)));
    connect(watcher, SIGNAL(foldersRemoved()),
            this, SLOT(foldersRemoved()));

    connect(watcher, SIGNAL(filesAboutToBeAdded(FolderNode *,const QList<FileNode*> &)),
            this, SLOT(filesAboutToBeAdded(FolderNode *, const QList<FileNode *> &)));
    connect(watcher, SIGNAL(filesAdded()),
            this, SLOT(filesAdded()));

    connect(watcher, SIGNAL(filesAboutToBeRemoved(FolderNode *, const QList<FileNode*> &)),
            this, SLOT(filesAboutToBeRemoved(FolderNode *, const QList<FileNode*> &)));
    connect(watcher, SIGNAL(filesRemoved()),
            this, SLOT(filesRemoved()));
}

QModelIndex FlatModel::index(int row, int column, const QModelIndex &parent) const
{
    QModelIndex result;
    if (!parent.isValid() && row == 0 && column == 0) { // session
        result = createIndex(0, 0, m_rootNode);
    } else if (parent.isValid() && column == 0) {
        FolderNode *parentNode = qobject_cast<FolderNode*>(nodeForIndex(parent));
        Q_ASSERT(parentNode);
        QHash<FolderNode*, QList<Node*> >::const_iterator it = m_childNodes.constFind(parentNode);
        if (it == m_childNodes.constEnd()) {
            fetchMore(parentNode);
            it = m_childNodes.constFind(parentNode);
        }

        if (row < it.value().size())
            result = createIndex(row, 0, it.value().at(row));
    }
//    qDebug() << "index of " << row << column << parent.data(Project::FilePathRole) << " is " << result.data(Project::FilePathRole);
    return result;
}

QModelIndex FlatModel::parent(const QModelIndex &idx) const
{
    QModelIndex parentIndex;
    if (Node *node = nodeForIndex(idx)) {
        FolderNode *parentNode = visibleFolderNode(node->parentFolderNode());
        if (parentNode) {
            FolderNode *grandParentNode = visibleFolderNode(parentNode->parentFolderNode());
            if (grandParentNode) {
                QHash<FolderNode*, QList<Node*> >::const_iterator it = m_childNodes.constFind(grandParentNode);
                if (it == m_childNodes.constEnd()) {
                    fetchMore(grandParentNode);
                    it = m_childNodes.constFind(grandParentNode);
                }
                Q_ASSERT(it != m_childNodes.constEnd());
                const int row = it.value().indexOf(parentNode);
                Q_ASSERT(row >= 0);
                parentIndex = createIndex(row, 0, parentNode);
            } else {
                // top level node, parent is session
                parentIndex = index(0, 0, QModelIndex());
            }
        }
    }

//    qDebug() << "parent of " << idx.data(Project::FilePathRole) << " is " << parentIndex.data(Project::FilePathRole);

    return parentIndex;
}

QVariant FlatModel::data(const QModelIndex &index, int role) const
{
    QVariant result;

    if (Node *node = nodeForIndex(index)) {
        FolderNode *folderNode = qobject_cast<FolderNode*>(node);
        switch (role) {
        case Qt::DisplayRole:
        case Qt::EditRole: {
            if (folderNode)
                result = folderNode->name();
            else
                result = QFileInfo(node->path()).fileName(); //TODO cache that?
            break;
        }
        case Qt::ToolTipRole: {
            result = node->path();
            break;
        }
        case Qt::DecorationRole: {
            if (folderNode)
                result = folderNode->icon();
            else
                result = FileIconProvider::instance()->icon(QFileInfo(node->path()));
            break;
        }
        case Qt::FontRole: {
            QFont font;
            if (node == m_startupProject)
                font.setBold(true);
            result = font;
            break;
        }
        case ProjectExplorer::Project::FilePathRole: {
            result = node->path();
            break;
        }
        }
    }

    return result;
}

int FlatModel::rowCount(const QModelIndex &parent) const
{
    int rows = 0;
    if (!parent.isValid()) {
        rows = 1;
    } else {
        FolderNode *folderNode = qobject_cast<FolderNode*>(nodeForIndex(parent));
        if (folderNode && m_childNodes.contains(folderNode))
            rows = m_childNodes.value(folderNode).size();
    }
    return rows;
}

int FlatModel::columnCount(const QModelIndex &/*parent*/) const
{
    return 1;
}

bool FlatModel::hasChildren(const QModelIndex &parent) const
{
    if (!parent.isValid())
        return true;

    FolderNode *folderNode = qobject_cast<FolderNode*>(nodeForIndex(parent));
    if (!folderNode)
        return false;

    QHash<FolderNode*, QList<Node*> >::const_iterator it = m_childNodes.constFind(folderNode);
    if (it == m_childNodes.constEnd()) {
        fetchMore(folderNode);
        it = m_childNodes.constFind(folderNode);
    }
    return !it.value().isEmpty();
}

bool FlatModel::canFetchMore(const QModelIndex & parent) const
{
    if (!parent.isValid()) {
        return false;
    } else {
        if (FolderNode *folderNode = qobject_cast<FolderNode*>(nodeForIndex(parent)))
            return !m_childNodes.contains(folderNode);
        else
            return false;
    }
}

void FlatModel::recursiveAddFolderNodes(FolderNode *startNode, QList<Node *> *list, const QSet<Node *> &blackList) const
{
    foreach (FolderNode *folderNode, startNode->subFolderNodes()) {
        if (folderNode && !blackList.contains(folderNode))
            recursiveAddFolderNodesImpl(folderNode, list, blackList);
    }
}

void FlatModel::recursiveAddFolderNodesImpl(FolderNode *startNode, QList<Node *> *list, const QSet<Node *> &blackList) const
{
    if (!filter(startNode)) {
        if (!blackList.contains(startNode))
            list->append(startNode);
    } else {
        foreach (FolderNode *folderNode, startNode->subFolderNodes()) {
            if (folderNode && !blackList.contains(folderNode))
                recursiveAddFolderNodesImpl(folderNode, list, blackList);
        }
    }
}

void FlatModel::recursiveAddFileNodes(FolderNode *startNode, QList<Node *> *list, const QSet<Node *> &blackList) const
{
    foreach (FolderNode *subFolderNode, startNode->subFolderNodes()) {
        if (!blackList.contains(subFolderNode))
            recursiveAddFileNodes(subFolderNode, list, blackList);
    }
    foreach (Node *node, startNode->fileNodes()) {
        if (!blackList.contains(node) && !filter(node))
            list->append(node);
    }
}

QList<Node*> FlatModel::childNodes(FolderNode *parentNode, const QSet<Node*> &blackList) const
{
    QList<Node*> nodeList;

    if (parentNode->nodeType() == SessionNodeType) {
        SessionNode *sessionNode = static_cast<SessionNode*>(parentNode);
        QList<ProjectNode*> projectList = sessionNode->projectNodes();
        for (int i = 0; i < projectList.size(); ++i) {
            if (!blackList.contains(projectList.at(i)))
                nodeList << projectList.at(i);
        }
    } else {
        recursiveAddFolderNodes(parentNode, &nodeList, blackList);
        recursiveAddFileNodes(parentNode, &nodeList, blackList + nodeList.toSet());
    }
    qSort(nodeList.begin(), nodeList.end(), sortNodes);
    return nodeList;
}

void FlatModel::fetchMore(FolderNode *folderNode) const
{
    Q_ASSERT(folderNode);
    Q_ASSERT(!m_childNodes.contains(folderNode));

    QList<Node*> nodeList = childNodes(folderNode);
    m_childNodes.insert(folderNode, nodeList);
}

void FlatModel::fetchMore(const QModelIndex &parent)
{
    FolderNode *folderNode = qobject_cast<FolderNode*>(nodeForIndex(parent));
    Q_ASSERT(folderNode);

    fetchMore(folderNode);
}

void FlatModel::setStartupProject(ProjectNode *projectNode)
{
    if (m_startupProject != projectNode) {
        QModelIndex oldIndex = (m_startupProject ? indexForNode(m_startupProject) : QModelIndex());
        QModelIndex newIndex = (projectNode ? indexForNode(projectNode) : QModelIndex());
        m_startupProject = projectNode;
        if (oldIndex.isValid())
            emit dataChanged(oldIndex, oldIndex);
        if (newIndex.isValid())
            emit dataChanged(newIndex, newIndex);
    }
}

void FlatModel::reset()
{
    m_childNodes.clear();
    QAbstractItemModel::reset();
}

QModelIndex FlatModel::indexForNode(const Node *node_)
{
    // We assume that we are only called for nodes that are represented

    // we use non-const pointers internally
    Node *node = const_cast<Node*>(node_);
    if (!node)
        return QModelIndex();

    if (node == m_rootNode)
        return createIndex(0, 0, m_rootNode);

    FolderNode *parentNode = visibleFolderNode(node->parentFolderNode());

    // Do we have the parent mapped?
    QHash<FolderNode*, QList<Node*> >::const_iterator it = m_childNodes.constFind(parentNode);
    if (it == m_childNodes.constEnd()) {
        fetchMore(parentNode);
        it = m_childNodes.constFind(parentNode);
    }
    if (it != m_childNodes.constEnd()) {
        const int row = it.value().indexOf(node);
        if (row != -1)
            return createIndex(row, 0, node);
    }
    return QModelIndex();
}

void FlatModel::setProjectFilterEnabled(bool filter)
{
    if (filter == m_filterProjects)
        return;
    m_filterProjects = filter;
    reset();
}

void FlatModel::setGeneratedFilesFilterEnabled(bool filter)
{
    m_filterGeneratedFiles = filter;
    reset();
}

bool FlatModel::projectFilterEnabled()
{
    return m_filterProjects;
}

bool FlatModel::generatedFilesFilterEnabled()
{
    return m_filterGeneratedFiles;
}

Node *FlatModel::nodeForIndex(const QModelIndex &index) const
{
    if (index.isValid())
        return (Node*)index.internalPointer();
    return 0;
}

/*
  Returns the first folder node in the ancestors
  for the given node that is not filtered
  out by the Flat Model.
*/
FolderNode *FlatModel::visibleFolderNode(FolderNode *node) const
{
    if (!node)
        return 0;

    for (FolderNode *folderNode = node;
         folderNode;
         folderNode = folderNode->parentFolderNode()) {
        if (!filter(folderNode))
            return folderNode;
    }
    return 0;
}

bool FlatModel::filter(Node *node) const
{
    bool isHidden = false;
    if (ProjectNode *projectNode = qobject_cast<ProjectNode*>(node)) {
        if (m_filterProjects && projectNode->parentFolderNode() != m_rootNode)
            isHidden = !projectNode->hasTargets();
    }
    if (FileNode *fileNode = qobject_cast<FileNode*>(node)) {
        if (m_filterGeneratedFiles)
            isHidden = fileNode->isGenerated();
    }

    return isHidden;
}

/// slots and all the fun
void FlatModel::added(FolderNode* parentNode, const QList<Node*> &newNodeList)
{
    QModelIndex parentIndex = indexForNode(parentNode);
    // Old  list
    QHash<FolderNode*, QList<Node*> >::const_iterator it = m_childNodes.constFind(parentNode);
    if (it == m_childNodes.constEnd())
        return;
    QList<Node *> oldNodeList = it.value();

    // Compare lists and emit signals, and modify m_childNodes on the fly
    QList<Node *>::const_iterator oldIter = oldNodeList.constBegin();
    QList<Node *>::const_iterator newIter = newNodeList.constBegin();

    // optimization, check for old list is empty
    if (oldIter == oldNodeList.constEnd()) {
        // New Node List is empty, nothing added which intrest us
        if (newIter == newNodeList.constEnd())
            return;
        // So all we need to do is easy
        beginInsertRows(parentIndex, 0, newNodeList.size() - 1);
        m_childNodes.insert(parentNode, newNodeList);
        endInsertRows();
        return;
    }

    while (true) {
        // Skip all that are the same
        while (*oldIter == *newIter) {
            ++oldIter;
            ++newIter;
            if (oldIter == oldNodeList.constEnd()) {
                // At end of oldNodeList, sweep up rest of newNodeList
                QList<Node *>::const_iterator startOfBlock = newIter;
                newIter = newNodeList.constEnd();
                int pos = oldIter - oldNodeList.constBegin();
                int count = newIter - startOfBlock;
                if (count > 0) {
                    beginInsertRows(parentIndex, pos, pos+count-1);
                    while (startOfBlock != newIter) {
                        oldNodeList.insert(pos, *startOfBlock);
                        ++pos;
                        ++startOfBlock;
                    }
                    m_childNodes.insert(parentNode, oldNodeList);
                    endInsertRows();
                }
                return; // Done with the lists, leave the function
            }
        }

        QList<Node *>::const_iterator startOfBlock = newIter;
        while (*oldIter != *newIter)
            ++newIter;
        // startOfBlock is the first that was diffrent
        // newIter points to the new position of oldIter
        // newIter - startOfBlock is number of new items
        // oldIter is the position where those are...
        int pos = oldIter - oldNodeList.constBegin();
        int count = newIter - startOfBlock;
        beginInsertRows(parentIndex, pos, pos + count - 1);
        while (startOfBlock != newIter) {
            oldNodeList.insert(pos, *startOfBlock);
            ++pos;
            ++startOfBlock;
        }
        m_childNodes.insert(parentNode, oldNodeList);
        endInsertRows();
        oldIter = oldNodeList.constBegin() + pos;
    }
}

void FlatModel::removed(FolderNode* parentNode, const QList<Node*> &newNodeList)
{
    QModelIndex parentIndex = indexForNode(parentNode);
    // Old  list
    QHash<FolderNode*, QList<Node*> >::const_iterator it = m_childNodes.constFind(parentNode);
    if (it == m_childNodes.constEnd())
        return;
    QList<Node *> oldNodeList = it.value();
    // Compare lists and emit signals, and modify m_childNodes on the fly
    QList<Node *>::const_iterator oldIter = oldNodeList.constBegin();
    QList<Node *>::const_iterator newIter = newNodeList.constBegin();

    // optimization, check for new list is empty
    if (newIter == newNodeList.constEnd()) {
        // New Node List is empty, everything removed
        if (oldIter == oldNodeList.constEnd())
            return;
        // So all we need to do is easy
        beginRemoveRows(parentIndex, 0, oldNodeList.size() - 1);
        m_childNodes.insert(parentNode, newNodeList);
        endRemoveRows();
        return;
    }

    while (true) {
        // Skip all that are the same
        while (*oldIter == *newIter) {
            ++oldIter;
            ++newIter;
            if (newIter == newNodeList.constEnd()) {
                // At end of newNodeList, sweep up rest of oldNodeList
                QList<Node *>::const_iterator startOfBlock = oldIter;
                oldIter = oldNodeList.constEnd();
                int pos = startOfBlock - oldNodeList.constBegin();
                int count = oldIter - startOfBlock;
                if (count > 0) {
                    beginRemoveRows(parentIndex, pos, pos+count-1);
                    while (startOfBlock != oldIter) {
                        ++startOfBlock;
                        oldNodeList.removeAt(pos);
                    }

                    m_childNodes.insert(parentNode, oldNodeList);
                    endRemoveRows();
                }
                return; // Done with the lists, leave the function
            }
        }

        QList<Node *>::const_iterator startOfBlock = oldIter;
        while (*oldIter != *newIter)
            ++oldIter;
        // startOfBlock is the first that was diffrent
        // oldIter points to the new position of newIter
        // oldIter - startOfBlock is number of new items
        // newIter is the position where those are...
        int pos = startOfBlock - oldNodeList.constBegin();
        int count = oldIter - startOfBlock;
        beginRemoveRows(parentIndex, pos, pos + count - 1);
        while (startOfBlock != oldIter) {
            ++startOfBlock;
            oldNodeList.removeAt(pos);
        }
        m_childNodes.insert(parentNode, oldNodeList);
        endRemoveRows();
        oldIter = oldNodeList.constBegin() + pos;
    }
}

void FlatModel::foldersAboutToBeAdded(FolderNode *parentFolder, const QList<FolderNode*> &newFolders)
{
    Q_UNUSED(newFolders)
    m_parentFolderForChange = parentFolder;
}

void FlatModel::foldersAdded()
{
    // First found out what the folder is that we are adding the files to
    FolderNode *folderNode = visibleFolderNode(m_parentFolderForChange);

    // Now get the new list for that folder
    QList<Node *> newNodeList = childNodes(folderNode);

    added(folderNode, newNodeList);
}

void FlatModel::foldersAboutToBeRemoved(FolderNode *parentFolder, const QList<FolderNode*> &staleFolders)
{
    QSet<Node *> blackList;
    foreach (FolderNode *node, staleFolders)
        blackList.insert(node);

    FolderNode *folderNode = visibleFolderNode(parentFolder);
    QList<Node *> newNodeList = childNodes(folderNode, blackList);

    removed(folderNode, newNodeList);
    removeFromCache(staleFolders);
}

void FlatModel::removeFromCache(QList<FolderNode *> list)
{
    foreach (FolderNode *fn, list) {
        removeFromCache(fn->subFolderNodes());
        m_childNodes.remove(fn);
    }
}

void FlatModel::foldersRemoved()
{
    // Do nothing
}

void FlatModel::filesAboutToBeAdded(FolderNode *folder, const QList<FileNode*> &newFiles)
{
    Q_UNUSED(newFiles)
    m_parentFolderForChange = folder;
}

void FlatModel::filesAdded()
{
    // First find out what the folder is that we are adding the files to
    FolderNode *folderNode = visibleFolderNode(m_parentFolderForChange);

    // Now get the new List for that folder
    QList<Node *> newNodeList = childNodes(folderNode);
    added(folderNode, newNodeList);
}

void FlatModel::filesAboutToBeRemoved(FolderNode *folder, const QList<FileNode*> &staleFiles)
{
    // First found out what the folder (that is the project) is that we are adding the files to
    FolderNode *folderNode = visibleFolderNode(folder);

    QSet<Node *> blackList;
    foreach(Node *node, staleFiles)
        blackList.insert(node);

    // Now get the new List for that folder
    QList<Node *> newNodeList = childNodes(folderNode, blackList);
    removed(folderNode, newNodeList);
}

void FlatModel::filesRemoved()
{
    // Do nothing
}
