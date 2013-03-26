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

#include "projectmodels.h"

#include "project.h"
#include "projectnodes.h"
#include "projectexplorer.h"

#include <coreplugin/fileiconprovider.h>

#include <QDebug>
#include <QFileInfo>

#include <QFont>

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

            int result = caseFriendlyCompare(fileName1, fileName2);
            if (result != 0)
                return result < 0;
            else
                return file1 < file2;
        } else {
            return true; // project file is before everything else
        }
    } else {
        if (file2 && file2->fileType() == ProjectFileType)
            return false;
    }

    // projects
    if (n1Type == ProjectNodeType) {
        if (n2Type == ProjectNodeType) {
            ProjectNode *project1 = static_cast<ProjectNode*>(n1);
            ProjectNode *project2 = static_cast<ProjectNode*>(n2);

            int result = caseFriendlyCompare(project1->displayName(), project2->displayName());
            if (result != 0)
                return result < 0;
            else
                return project1 < project2; // sort by pointer value
        } else {
           return true; // project is before folder & file
       }
    }
    if (n2Type == ProjectNodeType)
        return false;

    if (n1Type == VirtualFolderNodeType) {
        if (n2Type == VirtualFolderNodeType) {
            VirtualFolderNode *folder1 = static_cast<VirtualFolderNode *>(n1);
            VirtualFolderNode *folder2 = static_cast<VirtualFolderNode *>(n2);

            if (folder1->priority() > folder2->priority())
                return true;
            if (folder1->priority() < folder2->priority())
                return false;
            int result = caseFriendlyCompare(folder1->path(), folder2->path());
            if (result != 0)
                return result < 0;
            else
                return folder1 < folder2;
        } else {
            return true; // virtual folder is before folder
        }
    }

    if (n2Type == VirtualFolderNodeType)
        return false;


    if (n1Type == FolderNodeType) {
        if (n2Type == FolderNodeType) {
            FolderNode *folder1 = static_cast<FolderNode*>(n1);
            FolderNode *folder2 = static_cast<FolderNode*>(n2);

            int result = caseFriendlyCompare(folder1->path(), folder2->path());
            if (result != 0)
                return result < 0;
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

        int result = caseFriendlyCompare(fileName1, fileName2);
        if (result != 0) {
            return result < 0; // sort by filename
        } else {
            result = caseFriendlyCompare(filePath1, filePath2);
            if (result != 0)
                return result < 0; // sort by filepath
            else
                return n1 < n2; // sort by pointer value
        }
    }
    return false;
}

} // namespace anon

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

    connect(watcher, SIGNAL(aboutToChangeHasBuildTargets(ProjectExplorer::ProjectNode*)),
            this, SLOT(aboutToHasBuildTargetsChanged(ProjectExplorer::ProjectNode*)));

    connect(watcher, SIGNAL(hasBuildTargetsChanged(ProjectExplorer::ProjectNode*)),
            this, SLOT(hasBuildTargetsChanged(ProjectExplorer::ProjectNode*)));

    connect(watcher, SIGNAL(foldersAboutToBeAdded(FolderNode*,QList<FolderNode*>)),
            this, SLOT(foldersAboutToBeAdded(FolderNode*,QList<FolderNode*>)));
    connect(watcher, SIGNAL(foldersAdded()),
            this, SLOT(foldersAdded()));

    connect(watcher, SIGNAL(foldersAboutToBeRemoved(FolderNode*,QList<FolderNode*>)),
            this, SLOT(foldersAboutToBeRemoved(FolderNode*,QList<FolderNode*>)));
    connect(watcher, SIGNAL(foldersRemoved()),
            this, SLOT(foldersRemoved()));

    connect(watcher, SIGNAL(filesAboutToBeAdded(FolderNode*,QList<FileNode*>)),
            this, SLOT(filesAboutToBeAdded(FolderNode*,QList<FileNode*>)));
    connect(watcher, SIGNAL(filesAdded()),
            this, SLOT(filesAdded()));

    connect(watcher, SIGNAL(filesAboutToBeRemoved(FolderNode*,QList<FileNode*>)),
            this, SLOT(filesAboutToBeRemoved(FolderNode*,QList<FileNode*>)));
    connect(watcher, SIGNAL(filesRemoved()),
            this, SLOT(filesRemoved()));

    connect(watcher, SIGNAL(nodeSortKeyAboutToChange(Node*)),
            this, SLOT(nodeSortKeyAboutToChange(Node*)));
    connect(watcher, SIGNAL(nodeSortKeyChanged()),
            this, SLOT(nodeSortKeyChanged()));

    connect(watcher, SIGNAL(nodeUpdated(ProjectExplorer::Node*)),
            this, SLOT(nodeUpdated(ProjectExplorer::Node*)));
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
        case Qt::DisplayRole: {
            QString name = node->displayName();

            if (node->parentFolderNode()
                    && node->parentFolderNode()->nodeType() == SessionNodeType) {
                const QString vcsTopic = node->vcsTopic();

                if (!vcsTopic.isEmpty())
                    name += QLatin1String(" (") + vcsTopic + QLatin1Char(')');
            }

            result = name;
            break;
        }
        case Qt::EditRole: {
            result = node->displayName();
            break;
        }
        case Qt::ToolTipRole: {
            result = node->tooltip();
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
        case ProjectExplorer::Project::EnabledRole: {
            result = node->isEnabled();
            break;
        }
        }
    }

    return result;
}

Qt::ItemFlags FlatModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return 0;
    // We claim that everything is editable
    // That's slightly wrong
    // We control the only view, and that one does the checks
    Qt::ItemFlags f = Qt::ItemIsSelectable|Qt::ItemIsEnabled;
    if (Node *node = nodeForIndex(index)) {
        if (node == m_rootNode)
            return 0; // no flags for session node...
        if (!qobject_cast<ProjectNode *>(node)) {
            // either folder or file node
            if (node->projectNode()->supportedActions(node).contains(ProjectNode::Rename))
                f = f | Qt::ItemIsEditable;
        }
    }
    return f;
}

bool FlatModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid())
        return false;
    if (role != Qt::EditRole)
        return false;

    ProjectExplorerPlugin::instance()->renameFile(nodeForIndex(index), value.toString());
    return true;
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
    beginResetModel();
    m_childNodes.clear();
    endResetModel();
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
    if (node->nodeType() == SessionNodeType) {
        isHidden = false;
    } else if (ProjectNode *projectNode = qobject_cast<ProjectNode*>(node)) {
        if (m_filterProjects && projectNode->parentFolderNode() != m_rootNode)
            isHidden = !projectNode->hasBuildTargets();
    } else if (node->nodeType() == FolderNodeType || node->nodeType() == VirtualFolderNodeType) {
        if (m_filterProjects)
            isHidden = true;
    } else if (FileNode *fileNode = qobject_cast<FileNode*>(node)) {
        if (m_filterGeneratedFiles)
            isHidden = fileNode->isGenerated();
    }
    return isHidden;
}

bool isSorted(const QList<Node *> &nodes)
{
    int size = nodes.size();
    for (int i = 0; i < size -1; ++i) {
        if (!sortNodes(nodes.at(i), nodes.at(i+1)))
            return false;
    }
    return true;
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

    Q_ASSERT(isSorted(oldNodeList));
    Q_ASSERT(isSorted(newNodeList));

    QSet<Node *> emptyDifference;
    emptyDifference = oldNodeList.toSet();
    emptyDifference.subtract(newNodeList.toSet());
    if (!emptyDifference.isEmpty()) {
        // This should not happen...
        qDebug() << "FlatModel::added, old Node list should be subset of newNode list, found files in old list which were not part of new list";
        foreach (Node *n, emptyDifference) {
            qDebug()<<n->path();
        }
        Q_ASSERT(false);
    }

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

    Q_ASSERT(isSorted(newNodeList));

    QSet<Node *> emptyDifference;
    emptyDifference = newNodeList.toSet();
    emptyDifference.subtract(oldNodeList.toSet());
    if (!emptyDifference.isEmpty()) {
        // This should not happen...
        qDebug() << "FlatModel::removed, new Node list should be subset of oldNode list, found files in new list which were not part of old list";
        foreach (Node *n, emptyDifference) {
            qDebug()<<n->path();
        }
        Q_ASSERT(false);
    }

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

void FlatModel::aboutToHasBuildTargetsChanged(ProjectExplorer::ProjectNode* node)
{
    if (!m_filterProjects)
        return;
    FolderNode *folder = visibleFolderNode(node->parentFolderNode());
    QList<Node *> newNodeList = childNodes(folder, QSet<Node *>() << node);
    removed(folder, newNodeList);

    QList<Node *> staleFolders;
    recursiveAddFolderNodesImpl(node, &staleFolders);
    foreach (Node *n, staleFolders)
        if (FolderNode *fn = qobject_cast<FolderNode *>(n))
            m_childNodes.remove(fn);
}

void FlatModel::hasBuildTargetsChanged(ProjectExplorer::ProjectNode *node)
{
    if (!m_filterProjects)
        return;
    // we are only interested if we filter
    FolderNode *folder = visibleFolderNode(node->parentFolderNode());
    QList<Node *> newNodeList = childNodes(folder);
    added(folder, newNodeList);
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

void FlatModel::changedSortKey(FolderNode *folderNode, Node *node)
{
    QList<Node *> nodes = m_childNodes.value(folderNode);
    int oldIndex = nodes.indexOf(node);

    nodes.removeAt(oldIndex);
    QList<Node *>::iterator newPosIt = qLowerBound(nodes.begin(), nodes.end(), node, sortNodes);
    int newIndex = newPosIt - nodes.begin();

    if (newIndex == oldIndex)
        return;

    nodes.insert(newPosIt, node);

    QModelIndex parentIndex = indexForNode(folderNode);
    if (newIndex > oldIndex)
        ++newIndex; // see QAbstractItemModel::beginMoveRows
    beginMoveRows(parentIndex, oldIndex, oldIndex, parentIndex, newIndex);
    m_childNodes[folderNode] = nodes;
    endMoveRows();
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
    foreach (Node *node, staleFiles)
        blackList.insert(node);

    // Now get the new List for that folder
    QList<Node *> newNodeList = childNodes(folderNode, blackList);
    removed(folderNode, newNodeList);
}

void FlatModel::filesRemoved()
{
    // Do nothing
}

void FlatModel::nodeSortKeyAboutToChange(Node *node)
{
    m_nodeForSortKeyChange = node;
}

void FlatModel::nodeSortKeyChanged()
{
    FolderNode *folderNode = visibleFolderNode(m_nodeForSortKeyChange->parentFolderNode());
    changedSortKey(folderNode, m_nodeForSortKeyChange);
}

void FlatModel::nodeUpdated(Node *node)
{
    QModelIndex idx = indexForNode(node);
    emit dataChanged(idx, idx);
}

namespace ProjectExplorer {
namespace Internal {

int caseFriendlyCompare(const QString &a, const QString &b)
{
    int result = a.compare(b, Qt::CaseInsensitive);
    if (result != 0)
        return result;
    return a.compare(b, Qt::CaseSensitive);
}

}
}

