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

#include "projectmodels.h"

#include "project.h"
#include "projectnodes.h"
#include "projectexplorer.h"
#include "projecttree.h"

#include <coreplugin/fileiconprovider.h>
#include <utils/algorithm.h>

#include <QDebug>
#include <QFileInfo>
#include <QFont>
#include <QMimeData>
#include <QLoggingCategory>

namespace ProjectExplorer {

using namespace Internal;

namespace {

// sorting helper function

bool sortNodes(Node *n1, Node *n2)
{
    // Ordering is: project files, project, folder, file

    const NodeType n1Type = n1->nodeType();
    const NodeType n2Type = n2->nodeType();

    // project files
    FileNode *file1 = n1->asFileNode();
    FileNode *file2 = n2->asFileNode();
    if (file1 && file1->fileType() == ProjectFileType) {
        if (file2 && file2->fileType() == ProjectFileType) {
            const QString fileName1 = file1->path().fileName();
            const QString fileName2 = file2->path().fileName();

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

            result = caseFriendlyCompare(project1->path().toString(), project2->path().toString());
            if (result != 0)
                return result < 0;
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
            int result = caseFriendlyCompare(folder1->path().toString(), folder2->path().toString());
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

            int result = caseFriendlyCompare(folder1->path().toString(), folder2->path().toString());
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
        int result = caseFriendlyCompare(n1->displayName(), n2->displayName());
        if (result != 0)
            return result < 0;

        const QString filePath1 = n1->path().toString();
        const QString filePath2 = n2->path().toString();

        const QString fileName1 = Utils::FileName::fromString(filePath1).fileName();
        const QString fileName2 = Utils::FileName::fromString(filePath2).fileName();

        result = caseFriendlyCompare(fileName1, fileName2);
        if (result != 0) {
            return result < 0; // sort by filename
        } else {
            result = caseFriendlyCompare(filePath1, filePath2);
            if (result != 0)
                return result < 0; // sort by filepath

            if (n1->line() != n2->line())
                return n1->line() < n2->line(); // sort by line numbers
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
    ProjectTree *tree = ProjectTree::instance();

    connect(tree, &ProjectTree::aboutToChangeShowInSimpleTree,
            this, &FlatModel::aboutToShowInSimpleTreeChanged);

    connect(tree, &ProjectTree::showInSimpleTreeChanged,
            this, &FlatModel::showInSimpleTreeChanged);

    connect(tree, &ProjectTree::foldersAboutToBeAdded,
            this, &FlatModel::foldersAboutToBeAdded);
    connect(tree, &ProjectTree::foldersAdded,
            this, &FlatModel::foldersAdded);

    connect(tree, &ProjectTree::foldersAboutToBeRemoved,
            this, &FlatModel::foldersAboutToBeRemoved);
    connect(tree, &ProjectTree::foldersRemoved,
            this, &FlatModel::foldersRemoved);

    connect(tree, &ProjectTree::filesAboutToBeAdded,
            this, &FlatModel::filesAboutToBeAdded);
    connect(tree, &ProjectTree::filesAdded,
            this, &FlatModel::filesAdded);

    connect(tree, &ProjectTree::filesAboutToBeRemoved,
            this, &FlatModel::filesAboutToBeRemoved);
    connect(tree, &ProjectTree::filesRemoved,
            this, &FlatModel::filesRemoved);

    connect(tree, &ProjectTree::nodeSortKeyAboutToChange,
            this, &FlatModel::nodeSortKeyAboutToChange);
    connect(tree, &ProjectTree::nodeSortKeyChanged,
            this, &FlatModel::nodeSortKeyChanged);

    connect(tree, &ProjectTree::nodeUpdated,
            this, &FlatModel::nodeUpdated);
}

QModelIndex FlatModel::index(int row, int column, const QModelIndex &parent) const
{
    QModelIndex result;
    if (!parent.isValid() && row == 0 && column == 0) { // session
        result = createIndex(0, 0, m_rootNode);
    } else if (parent.isValid() && column == 0) {
        FolderNode *parentNode = nodeForIndex(parent)->asFolderNode();
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
        if (parentNode)
            return indexForNode(parentNode);
    }
    return parentIndex;
}

QVariant FlatModel::data(const QModelIndex &index, int role) const
{
    QVariant result;

    if (Node *node = nodeForIndex(index)) {
        FolderNode *folderNode = node->asFolderNode();
        switch (role) {
        case Qt::DisplayRole: {
            QString name = node->displayName();

            if (node->nodeType() == ProjectNodeType
                    && node->parentFolderNode()
                    && node->parentFolderNode()->nodeType() == SessionNodeType) {
                const QString vcsTopic = static_cast<ProjectNode *>(node)->vcsTopic();

                if (!vcsTopic.isEmpty())
                    name += QLatin1String(" [") + vcsTopic + QLatin1Char(']');
            }

            result = name;
            break;
        }
        case Qt::EditRole: {
            result = node->path().fileName();
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
                result = Core::FileIconProvider::icon(node->path().toString());
            break;
        }
        case Qt::FontRole: {
            QFont font;
            if (node == m_startupProject)
                font.setBold(true);
            result = font;
            break;
        }
        case Project::FilePathRole: {
            result = node->path().toString();
            break;
        }
        case Project::EnabledRole: {
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
        if (!node->asProjectNode()) {
            // either folder or file node
            if (node->supportedActions(node).contains(Rename))
                f = f | Qt::ItemIsEditable;
            if (node->asFileNode())
                f = f | Qt::ItemIsDragEnabled;
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

    Node *node = nodeForIndex(index);

    Utils::FileName orgFilePath = node->path();
    Utils::FileName newFilePath = orgFilePath.parentDir().appendPath(value.toString());

    ProjectExplorerPlugin::renameFile(node, newFilePath.toString());
    emit renamed(orgFilePath, newFilePath);
    return true;
}

int FlatModel::rowCount(const QModelIndex &parent) const
{
    int rows = 0;
    if (!parent.isValid()) {
        rows = 1;
    } else {
        FolderNode *folderNode = nodeForIndex(parent)->asFolderNode();
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

    FolderNode *folderNode = nodeForIndex(parent)->asFolderNode();
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
        if (FolderNode *folderNode = nodeForIndex(parent)->asFolderNode())
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
    qCDebug(logger()) << "    FlatModel::childNodes for " << parentNode->path();
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
    Utils::sort(nodeList, sortNodes);
    qCDebug(logger()) << "      found" << nodeList.size() << "nodes";
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
    FolderNode *folderNode = nodeForIndex(parent)->asFolderNode();
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

Qt::DropActions FlatModel::supportedDragActions() const
{
    return Qt::MoveAction;
}

QStringList FlatModel::mimeTypes() const
{
    return Utils::FileDropSupport::mimeTypesForFilePaths();
}

QMimeData *FlatModel::mimeData(const QModelIndexList &indexes) const
{
    auto data = new Utils::FileDropMimeData;
    foreach (const QModelIndex &index, indexes) {
        Node *node = nodeForIndex(index);
        if (node->asFileNode())
            data->addFile(node->path().toString());
    }
    return data;
}

QModelIndex FlatModel::indexForNode(const Node *node_) const
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
    if (FolderNode *folderNode = node->asFolderNode()) {
        if (m_filterProjects)
            isHidden = !folderNode->showInSimpleTree();
    } else if (FileNode *fileNode = node->asFileNode()) {
        if (m_filterGeneratedFiles)
            isHidden = fileNode->isGenerated();
    }
    return isHidden;
}

const QLoggingCategory &FlatModel::logger()
{
    static QLoggingCategory logger("qtc.projectexplorer.flatmodel");
    return logger;
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
    qCDebug(logger()) << "FlatModel::added" << parentNode->path() << newNodeList.size() << "nodes";
    QModelIndex parentIndex = indexForNode(parentNode);
    // Old  list

    if (newNodeList.isEmpty()) {
        qCDebug(logger()) << "  newNodeList empty";
        return;
    }

    QHash<FolderNode*, QList<Node*> >::const_iterator it = m_childNodes.constFind(parentNode);
    if (it == m_childNodes.constEnd()) {
        if (!parentIndex.isValid()) {
            qCDebug(logger()) << "  parent not mapped returning";
            return;
        }
        qCDebug(logger()) << "  updated m_childNodes";
        beginInsertRows(parentIndex, 0, newNodeList.size() - 1);
        m_childNodes.insert(parentNode, newNodeList);
        endInsertRows();
        return;
    }
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
        qCDebug(logger()) << "  updated m_childNodes";
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
    qCDebug(logger()) << "  updated m_childNodes";
}

void FlatModel::removed(FolderNode* parentNode, const QList<Node*> &newNodeList)
{
    qCDebug(logger()) << "FlatModel::removed" << parentNode->path() << newNodeList.size() << "nodes";
    QModelIndex parentIndex = indexForNode(parentNode);
    // Old  list
    QHash<FolderNode*, QList<Node*> >::const_iterator it = m_childNodes.constFind(parentNode);
    if (it == m_childNodes.constEnd()) {
        qCDebug(logger()) << "  unmapped node";
        return;
    }

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
        qCDebug(logger()) << "  updated m_childNodes";
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
    qCDebug(logger()) << "  updated m_childNodes";
}

void FlatModel::aboutToShowInSimpleTreeChanged(FolderNode* node)
{
    if (!m_filterProjects)
        return;
    FolderNode *folder = visibleFolderNode(node->parentFolderNode());
    QList<Node *> newNodeList = childNodes(folder, QSet<Node *>() << node);
    removed(folder, newNodeList);

    QList<Node *> staleFolders;
    recursiveAddFolderNodesImpl(node, &staleFolders);
    foreach (Node *n, staleFolders)
        if (FolderNode *fn = n->asFolderNode())
            m_childNodes.remove(fn);
}

void FlatModel::showInSimpleTreeChanged(FolderNode *node)
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
    qCDebug(logger()) << "FlatModel::foldersAboutToBeAdded";
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
    if (!m_childNodes.contains(folderNode))
        return; // The directory was not yet mapped, so there is no need to sort it.

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
    qCDebug(logger()) << "FlatModel::filesAboutToBeAdded";
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

namespace Internal {

int caseFriendlyCompare(const QString &a, const QString &b)
{
    int result = a.compare(b, Qt::CaseInsensitive);
    if (result != 0)
        return result;
    return a.compare(b, Qt::CaseSensitive);
}

}

} // namespace ProjectExplorer

