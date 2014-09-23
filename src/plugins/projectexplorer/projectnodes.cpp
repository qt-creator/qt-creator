/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
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

#include "projectnodes.h"

#include "nodesvisitor.h"
#include "projectexplorerconstants.h"

#include <coreplugin/mimedatabase.h>
#include <coreplugin/fileiconprovider.h>
#include <coreplugin/icore.h>
#include <coreplugin/iversioncontrol.h>
#include <coreplugin/vcsmanager.h>
#include <utils/algorithm.h>
#include <utils/qtcassert.h>

#include <QFileInfo>
#include <QDir>
#include <QIcon>
#include <QStyle>

using namespace ProjectExplorer;

/*!
  \class ProjectExplorer::Node

  \brief The Node class is the base class of all nodes in the node hierarchy.

  The nodes are arranged in a tree where leaves are FileNodes and non-leaves are FolderNodes
  A Project is a special Folder that manages the files and normal folders underneath it.

  The Watcher emits signals for structural changes in the hierarchy.
  A Visitor can be used to traverse all Projects and other Folders.

  \sa ProjectExplorer::FileNode, ProjectExplorer::FolderNode, ProjectExplorer::ProjectNode
  \sa ProjectExplorer::NodesWatcher, ProjectExplorer::NodesVisitor
*/

Node::Node(NodeType nodeType,
           const QString &filePath, int line)
        : QObject(),
          m_nodeType(nodeType),
          m_projectNode(0),
          m_folderNode(0),
          m_path(filePath),
          m_line(line)
{

}

void Node::emitNodeSortKeyAboutToChange()
{
    if (ProjectNode *project = projectNode()) {
        foreach (NodesWatcher *watcher, project->watchers())
            emit watcher->nodeSortKeyAboutToChange(this);
    }
}

void Node::emitNodeSortKeyChanged()
{
    if (ProjectNode *project = projectNode()) {
        foreach (NodesWatcher *watcher, project->watchers())
            emit watcher->nodeSortKeyChanged();
    }
}

/*!
 * The path of the file representing this node.
 *
 * This function does not emit any signals. That has to be done by the calling
 * class.
 */
void Node::setPath(const QString &path)
{
    if (m_path == path)
        return;

    emitNodeSortKeyAboutToChange();
    m_path = path;
    emitNodeSortKeyChanged();
    emitNodeUpdated();
}

void Node::setLine(int line)
{
    if (m_line == line)
        return;
    emitNodeSortKeyAboutToChange();
    m_line = line;
    emitNodeSortKeyChanged();
    emitNodeUpdated();
}

void Node::setPathAndLine(const QString &path, int line)
{
    if (m_path == path
            && m_line == line)
        return;
    emitNodeSortKeyAboutToChange();
    m_path = path;
    m_line = line;
    emitNodeSortKeyChanged();
    emitNodeUpdated();
}

NodeType Node::nodeType() const
{
    return m_nodeType;
}

/*!
  The project that owns and manages the node. It is the first project in the list
  of ancestors.
  */
ProjectNode *Node::projectNode() const
{
    return m_projectNode;
}

/*!
  The parent in the node hierarchy.
  */
FolderNode *Node::parentFolderNode() const
{
    return m_folderNode;
}

/*!
  The path of the file or folder in the filesystem the node represents.
  */
QString Node::path() const
{
    return m_path;
}

int Node::line() const
{
    return m_line;
}

QString Node::displayName() const
{
    return QFileInfo(path()).fileName();
}

QString Node::tooltip() const
{
    return QDir::toNativeSeparators(path());
}

bool Node::isEnabled() const
{
    return parentFolderNode()->isEnabled();
}

QList<ProjectAction> Node::supportedActions(Node *node) const
{
    QList<ProjectAction> list = parentFolderNode()->supportedActions(node);
    list.append(ProjectExplorer::InheritedFromParent);
    return list;
}

void Node::setNodeType(NodeType type)
{
    m_nodeType = type;
}

void Node::setProjectNode(ProjectNode *project)
{
    m_projectNode = project;
}

void Node::emitNodeUpdated()
{
    if (ProjectNode *node = projectNode())
        foreach (NodesWatcher *watcher, node->watchers())
            emit watcher->nodeUpdated(this);
}

void Node::setParentFolderNode(FolderNode *parentFolder)
{
    m_folderNode = parentFolder;
}

/*!
  \class ProjectExplorer::FileNode

  \brief The FileNode class is an in-memory presentation of a file.

  All file nodes are leaf nodes.

  \sa ProjectExplorer::FolderNode, ProjectExplorer::ProjectNode
*/

FileNode::FileNode(const QString &filePath,
                   const FileType fileType,
                   bool generated, int line)
        : Node(FileNodeType, filePath, line),
          m_fileType(fileType),
          m_generated(generated)
{
}

FileType FileNode::fileType() const
{
    return m_fileType;
}

/*!
  Returns \c true if the file is automatically generated by a compile step.
  */
bool FileNode::isGenerated() const
{
    return m_generated;
}

/*!
  \class ProjectExplorer::FolderNode

  In-memory presentation of a folder. Note that the node itself + all children (files and folders) are "managed" by the owning project.

  \sa ProjectExplorer::FileNode, ProjectExplorer::ProjectNode
*/
FolderNode::FolderNode(const QString &folderPath, NodeType nodeType, const QString &displayName)  :
    Node(nodeType, folderPath),
    m_displayName(displayName)
{
    if (m_displayName.isEmpty())
        m_displayName = QDir::toNativeSeparators(folderPath);
}

FolderNode::~FolderNode()
{
    qDeleteAll(m_subFolderNodes);
    qDeleteAll(m_fileNodes);
}

/*!
    Contains the display name that should be used in a view.
    \sa setFolderName()
 */

QString FolderNode::displayName() const
{
    return m_displayName;
}

/*!
  Contains the icon that should be used in a view. Default is the directory icon
 (QStyle::S_PDirIcon).
  s\a setIcon()
 */
QIcon FolderNode::icon() const
{
    // Instantiating the Icon provider is expensive.
    if (m_icon.isNull())
        m_icon = Core::FileIconProvider::icon(QFileIconProvider::Folder);
    return m_icon;
}

QList<FileNode*> FolderNode::fileNodes() const
{
    return m_fileNodes;
}

QList<FolderNode*> FolderNode::subFolderNodes() const
{
    return m_subFolderNodes;
}

void FolderNode::accept(NodesVisitor *visitor)
{
    visitor->visitFolderNode(this);
    foreach (FolderNode *subFolder, m_subFolderNodes)
        subFolder->accept(visitor);
}

void FolderNode::setDisplayName(const QString &name)
{
    if (m_displayName == name)
        return;
    emitNodeSortKeyAboutToChange();
    m_displayName = name;
    emitNodeSortKeyChanged();
    emitNodeUpdated();
}

void FolderNode::setIcon(const QIcon &icon)
{
    m_icon = icon;
}

bool FolderNode::addFiles(const QStringList &filePaths, QStringList *notAdded)
{
    if (projectNode())
        return projectNode()->addFiles(filePaths, notAdded);
    return false;
}

bool FolderNode::removeFiles(const QStringList &filePaths, QStringList *notRemoved)
{
    if (projectNode())
        return projectNode()->removeFiles(filePaths, notRemoved);
    return false;
}

bool FolderNode::deleteFiles(const QStringList &filePaths)
{
    if (projectNode())
        return projectNode()->deleteFiles(filePaths);
    return false;
}

bool FolderNode::renameFile(const QString &filePath, const QString &newFilePath)
{
    if (projectNode())
        return projectNode()->renameFile(filePath, newFilePath);
    return false;
}

FolderNode::AddNewInformation FolderNode::addNewInformation(const QStringList &files, Node *context) const
{
    Q_UNUSED(files);
    return AddNewInformation(QFileInfo(path()).fileName(), context == this ? 120 : 100);
}

/*!
  Adds file nodes specified by \a files to the internal list of the folder
  and emits the corresponding signals from the projectNode.

  This function should be called within an implementation of the public function
  addFiles.
*/

void FolderNode::addFileNodes(const QList<FileNode *> &files)
{
    Q_ASSERT(projectNode());
    ProjectNode *pn = projectNode();
    if (files.isEmpty())
        return;

    foreach (NodesWatcher *watcher, pn->watchers())
        emit watcher->filesAboutToBeAdded(this, files);

    foreach (FileNode *file, files) {
        QTC_ASSERT(!file->parentFolderNode(),
                   qDebug("File node has already a parent folder"));

        file->setParentFolderNode(this);
        file->setProjectNode(pn);
        // Now find the correct place to insert file
        if (m_fileNodes.count() == 0
                || m_fileNodes.last() < file) {
            // empty list or greater then last node
            m_fileNodes.append(file);
        } else {
            QList<FileNode *>::iterator it
                    = qLowerBound(m_fileNodes.begin(),
                                  m_fileNodes.end(),
                                  file);
            m_fileNodes.insert(it, file);
        }
    }

    foreach (NodesWatcher *watcher, pn->watchers())
        emit watcher->filesAdded();
}

/*!
  Removes \a files from the internal list and emits the corresponding signals.

  All objects in the \a files list are deleted.
  This function should be called within an implementation of the public function
  removeFiles.
*/

void FolderNode::removeFileNodes(const QList<FileNode *> &files)
{
    Q_ASSERT(projectNode());
    ProjectNode *pn = projectNode();

    if (files.isEmpty())
        return;

    QList<FileNode*> toRemove = files;
    Utils::sort(toRemove);

    foreach (NodesWatcher *watcher, pn->watchers())
        emit watcher->filesAboutToBeRemoved(this, toRemove);

    QList<FileNode*>::const_iterator toRemoveIter = toRemove.constBegin();
    QList<FileNode*>::iterator filesIter = m_fileNodes.begin();
    for (; toRemoveIter != toRemove.constEnd(); ++toRemoveIter) {
        while (*filesIter != *toRemoveIter) {
            ++filesIter;
            QTC_ASSERT(filesIter != m_fileNodes.end(),
                       qDebug("File to remove is not part of specified folder!"));
        }
        delete *filesIter;
        filesIter = m_fileNodes.erase(filesIter);
    }

    foreach (NodesWatcher *watcher, pn->watchers())
        emit watcher->filesRemoved();
}

/*!
  Adds folder nodes specified by \a subFolders to the node hierarchy below
  \a parentFolder and emits the corresponding signals.
*/
void FolderNode::addFolderNodes(const QList<FolderNode*> &subFolders)
{
    Q_ASSERT(projectNode());
    ProjectNode *pn = projectNode();

    if (subFolders.isEmpty())
        return;

    foreach (NodesWatcher *watcher, pn->watchers())
        watcher->foldersAboutToBeAdded(this, subFolders);

    foreach (FolderNode *folder, subFolders) {
        QTC_ASSERT(!folder->parentFolderNode(),
                   qDebug("Project node has already a parent folder"));
        folder->setParentFolderNode(this);
        folder->setProjectNode(pn);

        // Find the correct place to insert
        if (m_subFolderNodes.count() == 0
                || m_subFolderNodes.last() < folder) {
            // empty list or greater then last node
            m_subFolderNodes.append(folder);
        } else {
            // Binary Search for insertion point
            QList<FolderNode*>::iterator it
                    = qLowerBound(m_subFolderNodes.begin(),
                                  m_subFolderNodes.end(),
                                  folder);
            m_subFolderNodes.insert(it, folder);
        }

        // project nodes have to be added via addProjectNodes
        QTC_ASSERT(folder->nodeType() != ProjectNodeType,
                   qDebug("project nodes have to be added via addProjectNodes"));
    }

    foreach (NodesWatcher *watcher, pn->watchers())
        emit watcher->foldersAdded();
}

/*!
  Removes file nodes specified by \a subFolders from the node hierarchy and emits
  the corresponding signals.

  All objects in the \a subFolders list are deleted.
*/
void FolderNode::removeFolderNodes(const QList<FolderNode*> &subFolders)
{
    Q_ASSERT(projectNode());
    ProjectNode *pn = projectNode();

    if (subFolders.isEmpty())
        return;

    QList<FolderNode*> toRemove = subFolders;
    Utils::sort(toRemove);

    foreach (NodesWatcher *watcher, pn->watchers())
        emit watcher->foldersAboutToBeRemoved(this, toRemove);

    QList<FolderNode*>::const_iterator toRemoveIter = toRemove.constBegin();
    QList<FolderNode*>::iterator folderIter = m_subFolderNodes.begin();
    for (; toRemoveIter != toRemove.constEnd(); ++toRemoveIter) {
        QTC_ASSERT((*toRemoveIter)->nodeType() != ProjectNodeType,
                   qDebug("project nodes have to be removed via removeProjectNodes"));
        while (*folderIter != *toRemoveIter) {
            ++folderIter;
            QTC_ASSERT(folderIter != m_subFolderNodes.end(),
                       qDebug("Folder to remove is not part of specified folder!"));
        }
        delete *folderIter;
        folderIter = m_subFolderNodes.erase(folderIter);
    }

    foreach (NodesWatcher *watcher, pn->watchers())
        emit watcher->foldersRemoved();
}

void FolderNode::aboutToChangeShowInSimpleTree()
{
    foreach (NodesWatcher *watcher, projectNode()->watchers())
        emit watcher->aboutToChangeShowInSimpleTree(this);
}

void FolderNode::showInSimpleTreeChanged()
{
    foreach (NodesWatcher *watcher, projectNode()->watchers())
        emit watcher->showInSimpleTreeChanged(this);
}

bool FolderNode::showInSimpleTree() const
{
    return false;
}

/*!
  \class ProjectExplorer::VirtualFolderNode

  In-memory presentation of a virtual folder.
  Note that the node itself + all children (files and folders) are "managed" by the owning project.
  A virtual folder does not correspond to a actual folder on the file system. See for example the
  sources, headers and forms folder the QmakeProjectManager creates
  VirtualFolderNodes are always sorted before FolderNodes and are sorted according to their priority.

  \sa ProjectExplorer::FileNode, ProjectExplorer::ProjectNode
*/
VirtualFolderNode::VirtualFolderNode(const QString &folderPath, int priority)
    : FolderNode(folderPath, VirtualFolderNodeType), m_priority(priority)
{
}

VirtualFolderNode::~VirtualFolderNode()
{
}

int VirtualFolderNode::priority() const
{
    return m_priority;
}

/*!
  \class ProjectExplorer::ProjectNode

  \brief The ProjectNode class is an in-memory presentation of a Project.

  A concrete subclass must implement the persistent data.

  \sa ProjectExplorer::FileNode, ProjectExplorer::FolderNode
*/

/*!
  Creates an uninitialized project node object.
  */
ProjectNode::ProjectNode(const QString &projectFilePath)
        : FolderNode(projectFilePath)
{
    setNodeType(ProjectNodeType);
    // project node "manages" itself
    setProjectNode(this);
    setDisplayName(QFileInfo(projectFilePath).fileName());
}

QString ProjectNode::vcsTopic() const
{
    const QString dir = QFileInfo(path()).absolutePath();

    if (Core::IVersionControl *const vc =
            Core::VcsManager::findVersionControlForDirectory(dir))
        return vc->vcsTopic(dir);

    return QString();
}

QList<ProjectNode*> ProjectNode::subProjectNodes() const
{
    return m_subProjectNodes;
}

/*!
  \function bool ProjectNode::addSubProjects(const QStringList &)
  */

/*!
  \function bool ProjectNode::removeSubProjects(const QStringList &)
  */

/*!
  \function bool ProjectNode::addFiles(const FileType, const QStringList &, QStringList *)
  */

/*!
  \function bool ProjectNode::removeFiles(const FileType, const QStringList &, QStringList *)
  */

/*!
  \function bool ProjectNode::renameFile(const FileType, const QString &, const QString &)
  */

bool ProjectNode::deploysFolder(const QString &folder) const
{
    Q_UNUSED(folder);
    return false;
}

/*!
  \function bool ProjectNode::runConfigurations() const

  Returns a list of \c RunConfiguration suitable for this node.
  */
QList<RunConfiguration *> ProjectNode::runConfigurations() const
{
    return QList<RunConfiguration *>();
}

QList<NodesWatcher*> ProjectNode::watchers() const
{
    return m_watchers;
}

/*!
   Registers \a watcher for the current project and all subprojects.

   It does not take ownership of the watcher.
*/

void ProjectNode::registerWatcher(NodesWatcher *watcher)
{
    if (!watcher)
        return;
    connect(watcher, SIGNAL(destroyed(QObject*)),
            this, SLOT(watcherDestroyed(QObject*)));
    m_watchers.append(watcher);
    foreach (ProjectNode *subProject, m_subProjectNodes)
        subProject->registerWatcher(watcher);
}

/*!
    Removes \a watcher from the current project and all subprojects.
*/
void ProjectNode::unregisterWatcher(NodesWatcher *watcher)
{
    if (!watcher)
        return;
    m_watchers.removeOne(watcher);
    foreach (ProjectNode *subProject, m_subProjectNodes)
        subProject->unregisterWatcher(watcher);
}

void ProjectNode::accept(NodesVisitor *visitor)
{
    visitor->visitProjectNode(this);

    foreach (FolderNode *folder, m_subFolderNodes)
        folder->accept(visitor);
}

/*!
  Adds project nodes specified by \a subProjects to the node hierarchy and
  emits the corresponding signals.
  */
void ProjectNode::addProjectNodes(const QList<ProjectNode*> &subProjects)
{
    if (!subProjects.isEmpty()) {
        QList<FolderNode*> folderNodes;
        foreach (ProjectNode *projectNode, subProjects)
            folderNodes << projectNode;

        foreach (NodesWatcher *watcher, m_watchers)
            emit watcher->foldersAboutToBeAdded(this, folderNodes);

        foreach (ProjectNode *project, subProjects) {
            QTC_ASSERT(!project->parentFolderNode() || project->parentFolderNode() == this,
                       qDebug("Project node has already a parent"));
            project->setParentFolderNode(this);
            foreach (NodesWatcher *watcher, m_watchers)
                project->registerWatcher(watcher);
            m_subFolderNodes.append(project);
            m_subProjectNodes.append(project);
        }
        Utils::sort(m_subFolderNodes);
        Utils::sort(m_subProjectNodes);

        foreach (NodesWatcher *watcher, m_watchers)
            emit watcher->foldersAdded();
    }
}

/*!
  Removes project nodes specified by \a subProjects from the node hierarchy
  and emits the corresponding signals.

  All objects in the \a subProjects list are deleted.
*/

void ProjectNode::removeProjectNodes(const QList<ProjectNode*> &subProjects)
{
    if (!subProjects.isEmpty()) {
        QList<FolderNode*> toRemove;
        foreach (ProjectNode *projectNode, subProjects)
            toRemove << projectNode;
        Utils::sort(toRemove);

        foreach (NodesWatcher *watcher, m_watchers)
            emit watcher->foldersAboutToBeRemoved(this, toRemove);

        QList<FolderNode*>::const_iterator toRemoveIter = toRemove.constBegin();
        QList<FolderNode*>::iterator folderIter = m_subFolderNodes.begin();
        QList<ProjectNode*>::iterator projectIter = m_subProjectNodes.begin();
        for (; toRemoveIter != toRemove.constEnd(); ++toRemoveIter) {
            while (*projectIter != *toRemoveIter) {
                ++projectIter;
                QTC_ASSERT(projectIter != m_subProjectNodes.end(),
                    qDebug("Project to remove is not part of specified folder!"));
            }
            while (*folderIter != *toRemoveIter) {
                ++folderIter;
                QTC_ASSERT(folderIter != m_subFolderNodes.end(),
                    qDebug("Project to remove is not part of specified folder!"));
            }
            delete *projectIter;
            projectIter = m_subProjectNodes.erase(projectIter);
            folderIter = m_subFolderNodes.erase(folderIter);
        }

        foreach (NodesWatcher *watcher, m_watchers)
            emit watcher->foldersRemoved();
    }
}

void ProjectNode::watcherDestroyed(QObject *watcher)
{
    // cannot use qobject_cast here
    unregisterWatcher(static_cast<NodesWatcher*>(watcher));
}


/*!
  \class ProjectExplorer::SessionNode
*/

SessionNode::SessionNode(QObject *parentObject)
    : FolderNode(QLatin1String("session"))
{
    setParent(parentObject);
    setNodeType(SessionNodeType);
}

QList<ProjectAction> SessionNode::supportedActions(Node *node) const
{
    Q_UNUSED(node)
    return QList<ProjectAction>();
}

QList<NodesWatcher*> SessionNode::watchers() const
{
    return m_watchers;
}

/*!
   Registers \a watcher for the complete session tree.
   It does not take ownership of the watcher.
*/

void SessionNode::registerWatcher(NodesWatcher *watcher)
{
    if (!watcher)
        return;
    connect(watcher, SIGNAL(destroyed(QObject*)),
            this, SLOT(watcherDestroyed(QObject*)));
    m_watchers.append(watcher);
    foreach (ProjectNode *project, m_projectNodes)
        project->registerWatcher(watcher);
}

/*!
    Removes \a watcher from the complete session tree.
*/

void SessionNode::unregisterWatcher(NodesWatcher *watcher)
{
    if (!watcher)
        return;
    m_watchers.removeOne(watcher);
    foreach (ProjectNode *project, m_projectNodes)
        project->unregisterWatcher(watcher);
}

void SessionNode::accept(NodesVisitor *visitor)
{
    visitor->visitSessionNode(this);
    foreach (ProjectNode *project, m_projectNodes)
        project->accept(visitor);
}

bool SessionNode::showInSimpleTree() const
{
    return true;
}

void SessionNode::projectDisplayNameChanged(Node *node)
{
    foreach (NodesWatcher *watcher, m_watchers)
        emit watcher->nodeSortKeyAboutToChange(node);
    foreach (NodesWatcher *watcher, m_watchers)
        emit watcher->nodeSortKeyChanged();
}

QList<ProjectNode*> SessionNode::projectNodes() const
{
    return m_projectNodes;
}

void SessionNode::addProjectNodes(const QList<ProjectNode*> &projectNodes)
{
    if (!projectNodes.isEmpty()) {
        QList<FolderNode*> folderNodes;
        foreach (ProjectNode *projectNode, projectNodes)
            folderNodes << projectNode;

        foreach (NodesWatcher *watcher, m_watchers)
            emit watcher->foldersAboutToBeAdded(this, folderNodes);

        foreach (ProjectNode *project, projectNodes) {
            QTC_ASSERT(!project->parentFolderNode(),
                qDebug("Project node has already a parent folder"));
            project->setParentFolderNode(this);
            foreach (NodesWatcher *watcher, m_watchers)
                project->registerWatcher(watcher);
            m_subFolderNodes.append(project);
            m_projectNodes.append(project);
        }

        Utils::sort(m_subFolderNodes);
        Utils::sort(m_projectNodes);

        foreach (NodesWatcher *watcher, m_watchers)
            emit watcher->foldersAdded();
   }
}

void SessionNode::removeProjectNodes(const QList<ProjectNode*> &projectNodes)
{
    if (!projectNodes.isEmpty()) {
        QList<FolderNode*> toRemove;
        foreach (ProjectNode *projectNode, projectNodes)
            toRemove << projectNode;

        Utils::sort(toRemove);

        foreach (NodesWatcher *watcher, m_watchers)
            emit watcher->foldersAboutToBeRemoved(this, toRemove);

        QList<FolderNode*>::const_iterator toRemoveIter = toRemove.constBegin();
        QList<FolderNode*>::iterator folderIter = m_subFolderNodes.begin();
        QList<ProjectNode*>::iterator projectIter = m_projectNodes.begin();
        for (; toRemoveIter != toRemove.constEnd(); ++toRemoveIter) {
            while (*projectIter != *toRemoveIter) {
                ++projectIter;
                QTC_ASSERT(projectIter != m_projectNodes.end(),
                    qDebug("Project to remove is not part of specified folder!"));
            }
            while (*folderIter != *toRemoveIter) {
                ++folderIter;
                QTC_ASSERT(folderIter != m_subFolderNodes.end(),
                    qDebug("Project to remove is not part of specified folder!"));
            }
            projectIter = m_projectNodes.erase(projectIter);
            folderIter = m_subFolderNodes.erase(folderIter);
        }

        foreach (NodesWatcher *watcher, m_watchers)
            emit watcher->foldersRemoved();
    }
}

void SessionNode::watcherDestroyed(QObject *watcher)
{
    // cannot use qobject_cast here
    unregisterWatcher(static_cast<NodesWatcher*>(watcher));
}

/*!
  \class ProjectExplorer::NodesWatcher

  \brief The NodesWatcher class enables you to keep track of changes in the
  tree.

  Add a watcher by calling ProjectNode::registerWatcher() or
  SessionNode::registerWatcher(). Whenever the tree underneath the
  project node or session node changes (for example, nodes are added or removed),
  the corresponding signals of the watcher are emitted.
  Watchers can be removed from the complete tree or a subtree
  by calling ProjectNode::unregisterWatcher and
  SessionNode::unregisterWatcher().

  The NodesWatcher class is similar to the Observer class in the
  well-known Observer pattern (Booch et al).

  \sa ProjectExplorer::Node
*/

NodesWatcher::NodesWatcher(QObject *parent)
        : QObject(parent)
{
}
