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

#include "projectnodes.h"

#include "nodesvisitor.h"
#include "projectexplorerconstants.h"
#include "projecttree.h"

#include <coreplugin/fileiconprovider.h>
#include <coreplugin/icore.h>
#include <coreplugin/iversioncontrol.h>
#include <coreplugin/vcsmanager.h>
#include <utils/algorithm.h>
#include <utils/fileutils.h>
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

Node::Node(NodeType nodeType, const Utils::FileName &filePath, int line)
        : m_nodeType(nodeType),
          m_line(line),
          m_projectNode(0),
          m_folderNode(0),
          m_filePath(filePath)
{

}

Node::~Node()
{

}

void Node::emitNodeSortKeyAboutToChange()
{
    if (parentFolderNode())
        ProjectTree::instance()->emitNodeSortKeyAboutToChange(this);
}

void Node::emitNodeSortKeyChanged()
{
    if (parentFolderNode())
        ProjectTree::instance()->emitNodeSortKeyChanged(this);
}

void Node::setAbsoluteFilePathAndLine(const Utils::FileName &path, int line)
{
    if (m_filePath == path && m_line == line)
        return;

    emitNodeSortKeyAboutToChange();
    m_filePath = path;
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
const Utils::FileName &Node::filePath() const
{
    return m_filePath;
}

int Node::line() const
{
    return m_line;
}

QString Node::displayName() const
{
    return filePath().fileName();
}

QString Node::tooltip() const
{
    return filePath().toUserOutput();
}

bool Node::isEnabled() const
{
    return parentFolderNode()->isEnabled();
}

QList<ProjectAction> Node::supportedActions(Node *node) const
{
    QList<ProjectAction> list = parentFolderNode()->supportedActions(node);
    list.append(InheritedFromParent);
    return list;
}

void Node::setProjectNode(ProjectNode *project)
{
    m_projectNode = project;
}

void Node::emitNodeUpdated()
{
    if (parentFolderNode())
        ProjectTree::instance()->emitNodeUpdated(this);
}

FileNode *Node::asFileNode()
{
    return 0;
}

FolderNode *Node::asFolderNode()
{
    return 0;
}

ProjectNode *Node::asProjectNode()
{
    return 0;
}

SessionNode *Node::asSessionNode()
{
    return 0;
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

FileNode::FileNode(const Utils::FileName &filePath,
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

FileNode *FileNode::asFileNode()
{
    return this;
}

/*!
  \class ProjectExplorer::FolderNode

  In-memory presentation of a folder. Note that the node itself + all children (files and folders) are "managed" by the owning project.

  \sa ProjectExplorer::FileNode, ProjectExplorer::ProjectNode
*/
FolderNode::FolderNode(const Utils::FileName &folderPath, NodeType nodeType, const QString &displayName) :
    Node(nodeType, folderPath),
    m_displayName(displayName)
{
    if (m_displayName.isEmpty())
        m_displayName = folderPath.toUserOutput();
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

QString FolderNode::addFileFilter() const
{
    return parentFolderNode()->addFileFilter();
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

bool FolderNode::canRenameFile(const QString &filePath, const QString &newFilePath)
{
    if (projectNode())
        return projectNode()->canRenameFile(filePath, newFilePath);
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
    return AddNewInformation(displayName(), context == this ? 120 : 100);
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
    if (files.isEmpty())
        return;

    ProjectTree::instance()->emitFilesAboutToBeAdded(this, files);

    foreach (FileNode *file, files) {
        QTC_ASSERT(!file->parentFolderNode(),
                   qDebug("File node has already a parent folder"));

        file->setParentFolderNode(this);
        file->setProjectNode(projectNode());
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

    ProjectTree::instance()->emitFilesAdded(this);
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

    if (files.isEmpty())
        return;

    QList<FileNode*> toRemove = files;
    Utils::sort(toRemove);

    ProjectTree::instance()->emitFilesAboutToBeRemoved(this, toRemove);

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

    ProjectTree::instance()->emitFilesRemoved(this);
}

/*!
  Adds folder nodes specified by \a subFolders to the node hierarchy below
  \a parentFolder and emits the corresponding signals.
*/
void FolderNode::addFolderNodes(const QList<FolderNode*> &subFolders)
{
    Q_ASSERT(projectNode());

    if (subFolders.isEmpty())
        return;

    ProjectTree::instance()->emitFoldersAboutToBeAdded(this, subFolders);
    foreach (FolderNode *folder, subFolders) {
        QTC_ASSERT(!folder->parentFolderNode(),
                   qDebug("Project node has already a parent folder"));
        folder->setParentFolderNode(this);
        folder->setProjectNode(projectNode());

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

    ProjectTree::instance()->emitFoldersAdded(this);
}

/*!
  Removes file nodes specified by \a subFolders from the node hierarchy and emits
  the corresponding signals.

  All objects in the \a subFolders list are deleted.
*/
void FolderNode::removeFolderNodes(const QList<FolderNode*> &subFolders)
{
    Q_ASSERT(projectNode());

    if (subFolders.isEmpty())
        return;

    QList<FolderNode*> toRemove = subFolders;
    Utils::sort(toRemove);

    ProjectTree::instance()->emitFoldersAboutToBeRemoved(this, toRemove);

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

    ProjectTree::instance()->emitFoldersRemoved(this);
}

FolderNode *FolderNode::asFolderNode()
{
    return this;
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
VirtualFolderNode::VirtualFolderNode(const Utils::FileName &folderPath, int priority)
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
ProjectNode::ProjectNode(const Utils::FileName &projectFilePath)
        : FolderNode(projectFilePath, ProjectNodeType)
{
    // project node "manages" itself
    setProjectNode(this);
    setDisplayName(projectFilePath.fileName());
}

QString ProjectNode::vcsTopic() const
{
    const QString dir = filePath().toFileInfo().absolutePath();

    if (Core::IVersionControl *const vc =
            Core::VcsManager::findVersionControlForDirectory(dir))
        return vc->vcsTopic(dir);

    return QString();
}

QList<ProjectNode*> ProjectNode::subProjectNodes() const
{
    return m_subProjectNodes;
}

bool ProjectNode::canAddSubProject(const QString &proFilePath) const
{
    Q_UNUSED(proFilePath)
    return false;
}

bool ProjectNode::addSubProjects(const QStringList &proFilePaths)
{
    Q_UNUSED(proFilePaths)
    return false;
}

bool ProjectNode::removeSubProjects(const QStringList &proFilePaths)
{
    Q_UNUSED(proFilePaths)
    return false;
}

bool ProjectNode::addFiles(const QStringList &filePaths, QStringList *notAdded)
{
    Q_UNUSED(filePaths)
    Q_UNUSED(notAdded)
    return false;
}

bool ProjectNode::removeFiles(const QStringList &filePaths, QStringList *notRemoved)
{
    Q_UNUSED(filePaths)
    Q_UNUSED(notRemoved)
    return false;
}

bool ProjectNode::deleteFiles(const QStringList &filePaths)
{
    Q_UNUSED(filePaths)
    return false;
}

bool ProjectNode::canRenameFile(const QString &filePath, const QString &newFilePath)
{
    Q_UNUSED(filePath);
    Q_UNUSED(newFilePath);
    return true;
}

bool ProjectNode::renameFile(const QString &filePath, const QString &newFilePath)
{
    Q_UNUSED(filePath)
    Q_UNUSED(newFilePath)
    return false;
}

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

        ProjectTree::instance()->emitFoldersAboutToBeAdded(this, folderNodes);

        foreach (ProjectNode *project, subProjects) {
            QTC_ASSERT(!project->parentFolderNode() || project->parentFolderNode() == this,
                       qDebug("Project node has already a parent"));
            project->setParentFolderNode(this);
            m_subFolderNodes.append(project);
            m_subProjectNodes.append(project);
        }
        Utils::sort(m_subFolderNodes);
        Utils::sort(m_subProjectNodes);

        ProjectTree::instance()->emitFoldersAdded(this);
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

        ProjectTree::instance()->emitFoldersAboutToBeRemoved(this, toRemove);

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

        ProjectTree::instance()->emitFoldersRemoved(this);
    }
}

ProjectNode *ProjectNode::asProjectNode()
{
    return this;
}


/*!
  \class ProjectExplorer::SessionNode
*/

SessionNode::SessionNode()
    : FolderNode(Utils::FileName::fromString(QLatin1String("session")), SessionNodeType)
{
}

QList<ProjectAction> SessionNode::supportedActions(Node *node) const
{
    Q_UNUSED(node)
    return QList<ProjectAction>();
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
    ProjectTree::instance()->emitNodeSortKeyAboutToChange(node);
    ProjectTree::instance()->emitNodeSortKeyChanged(node);
}

SessionNode *SessionNode::asSessionNode()
{
    return this;
}

QList<ProjectNode*> SessionNode::projectNodes() const
{
    return m_projectNodes;
}

QString SessionNode::addFileFilter() const
{
    return QLatin1String("*.c; *.cc; *.cpp; *.cp; *.cxx; *.c++; *.h; *.hh; *.hpp; *.hxx;");
}

void SessionNode::addProjectNodes(const QList<ProjectNode*> &projectNodes)
{
    if (!projectNodes.isEmpty()) {
        QList<FolderNode*> folderNodes;
        foreach (ProjectNode *projectNode, projectNodes)
            folderNodes << projectNode;

        ProjectTree::instance()->emitFoldersAboutToBeAdded(this, folderNodes);

        foreach (ProjectNode *project, projectNodes) {
            QTC_ASSERT(!project->parentFolderNode(),
                qDebug("Project node has already a parent folder"));
            project->setParentFolderNode(this);
            m_subFolderNodes.append(project);
            m_projectNodes.append(project);
        }

        Utils::sort(m_subFolderNodes);
        Utils::sort(m_projectNodes);

        ProjectTree::instance()->emitFoldersAdded(this);
   }
}

void SessionNode::removeProjectNodes(const QList<ProjectNode*> &projectNodes)
{
    if (!projectNodes.isEmpty()) {
        QList<FolderNode*> toRemove;
        foreach (ProjectNode *projectNode, projectNodes)
            toRemove << projectNode;

        Utils::sort(toRemove);

        ProjectTree::instance()->emitFoldersAboutToBeRemoved(this, toRemove);

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

        ProjectTree::instance()->emitFoldersRemoved(this);
    }
}
