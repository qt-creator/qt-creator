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

#include "buildconfiguration.h"
#include "buildsystem.h"
#include "project.h"
#include "projectexplorerconstants.h"
#include "projecttree.h"
#include "target.h"

#include <coreplugin/fileiconprovider.h>
#include <coreplugin/icore.h>
#include <coreplugin/iversioncontrol.h>
#include <coreplugin/vcsmanager.h>

#include <utils/algorithm.h>
#include <utils/fileutils.h>
#include <utils/hostosinfo.h>
#include <utils/mimetypes/mimedatabase.h>
#include <utils/mimetypes/mimetype.h>
#include <utils/pointeralgorithm.h>
#include <utils/qtcassert.h>
#include <utils/stringutils.h>

#include <QFileInfo>
#include <QDir>
#include <QIcon>
#include <QStyle>
#include <QTimer>

#include <memory>

namespace ProjectExplorer {

static FolderNode *recursiveFindOrCreateFolderNode(FolderNode *folder,
                                                   const Utils::FilePath &directory,
                                                   const Utils::FilePath &overrideBaseDir,
                                                   const FolderNode::FolderNodeFactory &factory)
{
    Utils::FilePath path = overrideBaseDir.isEmpty() ? folder->filePath() : overrideBaseDir;

    Utils::FilePath directoryWithoutPrefix;
    bool isRelative = false;

    if (path.isEmpty() || path.toFileInfo().isRoot()) {
        directoryWithoutPrefix = directory;
        isRelative = false;
    } else {
        if (directory.isChildOf(path) || directory == path) {
            isRelative = true;
            directoryWithoutPrefix = directory.relativeChildPath(path);
        } else {
            isRelative = false;
            path.clear();
            directoryWithoutPrefix = directory;
        }
    }
    QStringList parts = directoryWithoutPrefix.toString().split('/', Qt::SkipEmptyParts);
    if (!Utils::HostOsInfo::isWindowsHost() && !isRelative && !parts.isEmpty())
        parts[0].prepend('/');

    ProjectExplorer::FolderNode *parent = folder;
    foreach (const QString &part, parts) {
        path = path.pathAppended(part);
        // Find folder in subFolders
        FolderNode *next = parent->folderNode(path);
        if (!next) {
            // No FolderNode yet, so create it
            auto tmp = factory(path);
            tmp->setDisplayName(part);
            next = tmp.get();
            parent->addNode(std::move(tmp));
        }
        parent = next;
    }
    return parent;
}

/*!
  \class ProjectExplorer::Node

  \brief The Node class is the base class of all nodes in the node hierarchy.

  The nodes are arranged in a tree where leaves are FileNodes and non-leaves are FolderNodes
  A Project is a special Folder that manages the files and normal folders underneath it.

  The Watcher emits signals for structural changes in the hierarchy.
  A Visitor can be used to traverse all Projects and other Folders.

  \sa ProjectExplorer::FileNode, ProjectExplorer::FolderNode, ProjectExplorer::ProjectNode
  \sa ProjectExplorer::NodesWatcher
*/

Node::Node() = default;

void Node::setPriority(int p)
{
    m_priority = p;
}

void Node::setFilePath(const Utils::FilePath &filePath)
{
    m_filePath = filePath;
}

void Node::setLine(int line)
{
    m_line = line;
}

void Node::setListInProject(bool l)
{
    if (l)
        m_flags = static_cast<NodeFlag>(m_flags | FlagListInProject);
    else
        m_flags = static_cast<NodeFlag>(m_flags & ~FlagListInProject);
}

void Node::setIsGenerated(bool g)
{
    if (g)
        m_flags = static_cast<NodeFlag>(m_flags | FlagIsGenerated);
    else
        m_flags = static_cast<NodeFlag>(m_flags & ~FlagIsGenerated);
}

void Node::setAbsoluteFilePathAndLine(const Utils::FilePath &path, int line)
{
    if (m_filePath == path && m_line == line)
        return;

    m_filePath = path;
    m_line = line;
}

Node::~Node() = default;

int Node::priority() const
{
    return m_priority;
}

/*!
  Returns \c true if the Node should be listed as part of the projects file list.
  */
bool Node::listInProject() const
{
    return (m_flags & FlagListInProject) == FlagListInProject;
}

/*!
  The project that owns and manages the node. It is the first project in the list
  of ancestors.
  */
ProjectNode *Node::parentProjectNode() const
{
    if (!m_parentFolderNode)
        return nullptr;
    auto pn = m_parentFolderNode->asProjectNode();
    if (pn)
        return pn;
    return m_parentFolderNode->parentProjectNode();
}

/*!
  The parent in the node hierarchy.
  */
FolderNode *Node::parentFolderNode() const
{
    return m_parentFolderNode;
}

ProjectNode *Node::managingProject()
{
    if (asContainerNode())
        return asContainerNode()->rootProjectNode();
    QTC_ASSERT(m_parentFolderNode, return nullptr);
    ProjectNode *pn = parentProjectNode();
    return pn ? pn : asProjectNode(); // projects manage themselves...
}

const ProjectNode *Node::managingProject() const
{
    return const_cast<Node *>(this)->managingProject();
}

Project *Node::getProject() const
{
    if (const ContainerNode * const cn = asContainerNode())
        return cn->project();
    if (!m_parentFolderNode)
        return nullptr;
    return m_parentFolderNode->getProject();
}

/*!
  The path of the file or folder in the filesystem the node represents.
  */
const Utils::FilePath &Node::filePath() const
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
    if ((m_flags & FlagIsEnabled) == 0)
        return false;
    FolderNode *parent = parentFolderNode();
    return parent ? parent->isEnabled() : true;
}

/*!
  Returns \c true if the file is automatically generated by a compile step.
  */
bool Node::isGenerated() const
{
    return (m_flags & FlagIsGenerated) == FlagIsGenerated;
}

bool Node::supportsAction(ProjectAction, const Node *) const
{
    return false;
}

void Node::setEnabled(bool enabled)
{
    if (enabled)
        m_flags = static_cast<NodeFlag>(m_flags | FlagIsEnabled);
    else
        m_flags = static_cast<NodeFlag>(m_flags & ~FlagIsEnabled);
}

bool Node::sortByPath(const Node *a, const Node *b)
{
    return a->filePath() < b->filePath();
}

void Node::setParentFolderNode(FolderNode *parentFolder)
{
    m_parentFolderNode = parentFolder;
}

FileType Node::fileTypeForMimeType(const Utils::MimeType &mt)
{
    FileType type = FileType::Source;
    if (mt.isValid()) {
        const QString mtName = mt.name();
        if (mtName == Constants::C_HEADER_MIMETYPE
                || mtName == Constants::CPP_HEADER_MIMETYPE)
            type = FileType::Header;
        else if (mtName == Constants::FORM_MIMETYPE)
            type = FileType::Form;
        else if (mtName == Constants::RESOURCE_MIMETYPE)
            type = FileType::Resource;
        else if (mtName == Constants::SCXML_MIMETYPE)
            type = FileType::StateChart;
        else if (mtName == Constants::QML_MIMETYPE
                 || mtName == Constants::QMLUI_MIMETYPE)
            type = FileType::QML;
    } else {
        type = FileType::Unknown;
    }
    return type;
}

FileType Node::fileTypeForFileName(const Utils::FilePath &file)
{
    return fileTypeForMimeType(Utils::mimeTypeForFile(file.toString(),
                                                      Utils::MimeMatchMode::MatchExtension));
}

QString Node::pathOrDirectory(bool dir) const
{
    QString location;
    const FolderNode *folder = asFolderNode();
    if (isVirtualFolderType() && folder) {
        // Virtual Folder case
        // If there are files directly below or no subfolders, take the folder path
        if (!folder->fileNodes().isEmpty() || folder->folderNodes().isEmpty()) {
            location = m_filePath.toString();
        } else {
            // Otherwise we figure out a commonPath from the subfolders
            QStringList list;
            foreach (FolderNode *f, folder->folderNodes())
                list << f->filePath().toString() + QLatin1Char('/');
            location = Utils::commonPath(list);
        }

        QFileInfo fi(location);
        while ((!fi.exists() || !fi.isDir()) && !fi.isRoot())
            fi.setFile(fi.absolutePath());
        location = fi.absoluteFilePath();
    } else if (!m_filePath.isEmpty()) {
        QFileInfo fi = m_filePath.toFileInfo();
        // remove any /suffixes, which e.g. ResourceNode uses
        // Note this could be removed again by making path() a true path again
        // That requires changes in both the VirtualFolderNode and ResourceNode
        while (!fi.exists() && !fi.isRoot())
            fi.setFile(fi.absolutePath());

        if (dir)
            location = fi.isDir() ? fi.absoluteFilePath() : fi.absolutePath();
        else
            location = fi.absoluteFilePath();
    }
    return location;
}

/*!
  \class ProjectExplorer::FileNode

  \brief The FileNode class is an in-memory presentation of a file.

  All file nodes are leaf nodes.

  \sa ProjectExplorer::FolderNode, ProjectExplorer::ProjectNode
*/

FileNode::FileNode(const Utils::FilePath &filePath, const FileType fileType) :
    m_fileType(fileType)
{
    setFilePath(filePath);
    setListInProject(true);
    if (fileType == FileType::Project)
        setPriority(DefaultProjectFilePriority);
    else
        setPriority(DefaultFilePriority);
}

FileNode *FileNode::clone() const
{
    auto fn = new FileNode(filePath(), fileType());
    fn->setLine(line());
    fn->setIsGenerated(isGenerated());
    fn->setEnabled(isEnabled());
    fn->setPriority(priority());
    fn->setListInProject(listInProject());
    return fn;
}

FileType FileNode::fileType() const
{
    return m_fileType;
}

static QList<FileNode *> scanForFilesRecursively(const Utils::FilePath &directory,
                                                 const std::function<FileNode *(const Utils::FilePath &)> factory,
                                                 QSet<QString> &visited, QFutureInterface<QList<FileNode*>> *future,
                                                 double progressStart, double progressRange,
                                                 const QList<Core::IVersionControl*> &versionControls)
{
    QList<FileNode *> result;

    const QDir baseDir = QDir(directory.toString());

    // Do not follow directory loops:
    const int visitedCount = visited.count();
    visited.insert(baseDir.canonicalPath());
    if (visitedCount == visited.count())
        return result;

    const QFileInfoList entries = baseDir.entryInfoList(QStringList(), QDir::AllEntries|QDir::NoDotAndDotDot);
    double progress = 0;
    const double progressIncrement = progressRange / static_cast<double>(entries.count());
    int lastIntProgress = 0;
    for (const QFileInfo &entry : entries) {
        if (future && future->isCanceled())
            return result;

        const Utils::FilePath entryName = Utils::FilePath::fromString(entry.absoluteFilePath());
        if (!Utils::contains(versionControls, [&entryName](const Core::IVersionControl *vc) {
                             return vc->isVcsFileOrDirectory(entryName);
            })) {
            if (entry.isDir())
                result.append(scanForFilesRecursively(entryName, factory, visited, future, progress, progressIncrement, versionControls));
            else if (FileNode *node = factory(entryName))
                result.append(node);
        }
        if (future) {
            progress += progressIncrement;
            const int intProgress = std::min(static_cast<int>(progressStart + progress), future->progressMaximum());
            if (lastIntProgress < intProgress) {
                future->setProgressValue(intProgress);
                lastIntProgress = intProgress;
            }
        }
    }
    if (future)
        future->setProgressValue(std::min(static_cast<int>(progressStart + progressRange), future->progressMaximum()));
    return result;
}

QList<FileNode *>
FileNode::scanForFiles(const Utils::FilePath &directory,
                       const std::function<FileNode *(const Utils::FilePath &)> factory,
                       QFutureInterface<QList<FileNode *>> *future)
{
    QSet<QString> visited;
    if (future)
        future->setProgressRange(0, 1000000);
    return scanForFilesRecursively(directory, factory, visited, future, 0.0, 1000000.0,
                                   Core::VcsManager::versionControls());
}

bool FileNode::supportsAction(ProjectAction action, const Node *node) const
{
    if (action == InheritedFromParent)
        return true;
    FolderNode *parentFolder = parentFolderNode();
    return parentFolder && parentFolder->supportsAction(action, node);
}

QString FileNode::displayName() const
{
    int l = line();
    if (l < 0)
        return Node::displayName();
    return Node::displayName() + ':' + QString::number(l);
}

/*!
  \class ProjectExplorer::FolderNode

  In-memory presentation of a folder. Note that the node itself + all children (files and folders) are "managed" by the owning project.

  \sa ProjectExplorer::FileNode, ProjectExplorer::ProjectNode
*/
FolderNode::FolderNode(const Utils::FilePath &folderPath)
{
    setFilePath(folderPath);
    setPriority(DefaultFolderPriority);
    setListInProject(false);
    setIsGenerated(false);
    m_displayName = folderPath.toUserOutput();
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

Node *FolderNode::findNode(const std::function<bool(Node *)> &filter)
{
    if (filter(this))
        return this;

    for (const std::unique_ptr<Node> &n : m_nodes) {
        if (n->asFileNode() && filter(n.get())) {
            return n.get();
        } else if (FolderNode *folder = n->asFolderNode()) {
            Node *result = folder->findNode(filter);
            if (result)
                return result;
        }
    }
    return nullptr;
}

QList<Node *> FolderNode::findNodes(const std::function<bool(Node *)> &filter)
{
    QList<Node *> result;
    if (filter(this))
        result.append(this);
    for (const std::unique_ptr<Node> &n  : m_nodes) {
        if (n->asFileNode() && filter(n.get()))
            result.append(n.get());
        else if (FolderNode *folder = n->asFolderNode())
            result.append(folder->findNodes(filter));
    }
    return result;
}

void FolderNode::forEachNode(const std::function<void(FileNode *)> &fileTask,
                             const std::function<void(FolderNode *)> &folderTask,
                             const std::function<bool(const FolderNode *)> &folderFilterTask) const
{
    if (folderFilterTask) {
        if (!folderFilterTask(this))
            return;
    }
    if (fileTask) {
        for (const std::unique_ptr<Node> &n : m_nodes) {
            if (FileNode *fn = n->asFileNode())
                fileTask(fn);
        }
    }
    for (const std::unique_ptr<Node> &n : m_nodes) {
        if (FolderNode *fn = n->asFolderNode()) {
            if (folderTask)
                folderTask(fn);
            fn->forEachNode(fileTask, folderTask, folderFilterTask);
        }
    }
}

void FolderNode::forEachGenericNode(const std::function<void(Node *)> &genericTask) const
{
    for (const std::unique_ptr<Node> &n : m_nodes) {
        genericTask(n.get());
        if (FolderNode *fn = n->asFolderNode())
            fn->forEachGenericNode(genericTask);
    }
}

void FolderNode::forEachProjectNode(const std::function<void(const ProjectNode *)> &task) const
{
    if (const ProjectNode *projectNode = asProjectNode())
        task(projectNode);

    for (const std::unique_ptr<Node> &n : m_nodes) {
        if (FolderNode *fn = n->asFolderNode())
            fn->forEachProjectNode(task);
    }
}

ProjectNode *FolderNode::findProjectNode(const std::function<bool(const ProjectNode *)> &predicate)
{
    if (ProjectNode *projectNode = asProjectNode()) {
        if (predicate(projectNode))
            return projectNode;
    }

    for (const std::unique_ptr<Node> &n : m_nodes) {
        if (FolderNode *fn = n->asFolderNode()) {
            if (ProjectNode *pn = fn->findProjectNode(predicate))
                return pn;
        }
    }
    return nullptr;
}

const QList<Node *> FolderNode::nodes() const
{
    return Utils::toRawPointer<QList>(m_nodes);
}

QList<FileNode*> FolderNode::fileNodes() const
{
    QList<FileNode *> result;
    for (const std::unique_ptr<Node> &n : m_nodes) {
        if (FileNode *fn = n->asFileNode())
            result.append(fn);
    }
    return result;
}

FileNode *FolderNode::fileNode(const Utils::FilePath &file) const
{
    return static_cast<FileNode *>(Utils::findOrDefault(m_nodes,
                                                        [&file](const std::unique_ptr<Node> &n) {
        const FileNode *fn = n->asFileNode();
        return fn && fn->filePath() == file;
    }));
}

QList<FolderNode*> FolderNode::folderNodes() const
{
    QList<FolderNode *> result;
    for (const std::unique_ptr<Node> &n : m_nodes) {
        if (FolderNode *fn = n->asFolderNode())
            result.append(fn);
    }
    return result;
}

FolderNode *FolderNode::folderNode(const Utils::FilePath &directory) const
{
    Node *node = Utils::findOrDefault(m_nodes, [directory](const std::unique_ptr<Node> &n) {
        FolderNode *fn = n->asFolderNode();
        return fn && fn->filePath() == directory;
    });
    return static_cast<FolderNode *>(node);
}

void FolderNode::addNestedNode(std::unique_ptr<FileNode> &&fileNode,
                               const Utils::FilePath &overrideBaseDir,
                               const FolderNodeFactory &factory)
{
    FolderNode *folder = recursiveFindOrCreateFolderNode(this, fileNode->filePath().parentDir(),
                                                         overrideBaseDir, factory);
    folder->addNode(std::move(fileNode));
}

void FolderNode::addNestedNodes(std::vector<std::unique_ptr<FileNode> > &&files,
                                const Utils::FilePath &overrideBaseDir,
                                const FolderNode::FolderNodeFactory &factory)
{
    using DirWithNodes = std::pair<Utils::FilePath, std::vector<std::unique_ptr<FileNode>>>;
    std::vector<DirWithNodes> fileNodesPerDir;
    for (auto &f : files) {
        const Utils::FilePath parentDir = f->filePath().parentDir();
        const auto it = std::lower_bound(fileNodesPerDir.begin(), fileNodesPerDir.end(), parentDir,
            [](const DirWithNodes &nad, const Utils::FilePath &dir) { return nad.first < dir; });
        if (it != fileNodesPerDir.end() && it->first == parentDir) {
            it->second.emplace_back(std::move(f));
        } else {
            DirWithNodes dirWithNodes;
            dirWithNodes.first = parentDir;
            dirWithNodes.second.emplace_back(std::move(f));
            fileNodesPerDir.insert(it, std::move(dirWithNodes));
        }
    }

    for (DirWithNodes &dirWithNodes : fileNodesPerDir) {
        FolderNode * const folderNode = recursiveFindOrCreateFolderNode(this, dirWithNodes.first,
                                                                        overrideBaseDir, factory);
        for (auto &f : dirWithNodes.second)
            folderNode->addNode(std::move(f));
    }
}

// "Compress" a tree of foldernodes such that foldernodes with exactly one foldernode as a child
// are merged into one. This e.g. turns a sequence of FolderNodes "foo" "bar" "baz" into one
// FolderNode named "foo/bar/baz", saving a lot of clicks in the Project View to get to the actual
// files.
void FolderNode::compress()
{
    if (auto subFolder = m_nodes.size() == 1 ? m_nodes.at(0)->asFolderNode() : nullptr) {
        const bool sameType = (isFolderNodeType() && subFolder->isFolderNodeType())
                || (isProjectNodeType() && subFolder->isProjectNodeType())
                || (isVirtualFolderType() && subFolder->isVirtualFolderType());
        if (!sameType)
            return;

        // Only one subfolder: Compress!
        setDisplayName(QDir::toNativeSeparators(displayName() + "/" + subFolder->displayName()));
        for (Node *n : subFolder->nodes()) {
            std::unique_ptr<Node> toMove = subFolder->takeNode(n);
            toMove->setParentFolderNode(nullptr);
            addNode(std::move(toMove));
        }
        setAbsoluteFilePathAndLine(subFolder->filePath(), -1);

        takeNode(subFolder);

        compress();
    } else {
        for (FolderNode *fn : folderNodes())
            fn->compress();
    }
}

bool FolderNode::replaceSubtree(Node *oldNode, std::unique_ptr<Node> &&newNode)
{
    std::unique_ptr<Node> keepAlive;
    if (!oldNode) {
        addNode(std::move(newNode)); // Happens e.g. when a project is registered
    } else {
        auto it = std::find_if(m_nodes.begin(), m_nodes.end(),
                               [oldNode](const std::unique_ptr<Node> &n) {
            return oldNode == n.get();
        });
        QTC_ASSERT(it != m_nodes.end(), return false);
        if (newNode) {
            newNode->setParentFolderNode(this);
            keepAlive = std::move(*it);
            *it = std::move(newNode);
        } else {
            keepAlive = takeNode(oldNode); // Happens e.g. when project is shutting down
        }
    }
    handleSubTreeChanged(this);
    return true;
}

void FolderNode::setDisplayName(const QString &name)
{
    if (m_displayName == name)
        return;
    m_displayName = name;
}

void FolderNode::setIcon(const QIcon &icon)
{
    m_icon = icon;
}

void FolderNode::setLocationInfo(const QVector<FolderNode::LocationInfo> &info)
{
    m_locations = info;
    Utils::sort(m_locations, &LocationInfo::priority);
}

const QVector<FolderNode::LocationInfo> FolderNode::locationInfo() const
{
    return m_locations;
}

QString FolderNode::addFileFilter() const
{
    if (!m_addFileFilter.isNull())
        return m_addFileFilter;

    FolderNode *fn = parentFolderNode();
    return fn ? fn->addFileFilter() : QString();
}

bool FolderNode::supportsAction(ProjectAction action, const Node *node) const
{
    if (action == InheritedFromParent)
        return true;
    FolderNode *parentFolder = parentFolderNode();
    return parentFolder && parentFolder->supportsAction(action, node);
}

bool FolderNode::addFiles(const QStringList &filePaths, QStringList *notAdded)
{
    ProjectNode *pn = managingProject();
    if (pn)
        return pn->addFiles(filePaths, notAdded);
    return false;
}

RemovedFilesFromProject FolderNode::removeFiles(const QStringList &filePaths,
                                                QStringList *notRemoved)
{
    if (ProjectNode * const pn = managingProject())
        return pn->removeFiles(filePaths, notRemoved);
    return RemovedFilesFromProject::Error;
}

bool FolderNode::deleteFiles(const QStringList &filePaths)
{
    ProjectNode *pn = managingProject();
    if (pn)
        return pn->deleteFiles(filePaths);
    return false;
}

bool FolderNode::canRenameFile(const QString &filePath, const QString &newFilePath)
{
    ProjectNode *pn = managingProject();
    if (pn)
        return pn->canRenameFile(filePath, newFilePath);
    return false;
}

bool FolderNode::renameFile(const QString &filePath, const QString &newFilePath)
{
    ProjectNode *pn = managingProject();
    if (pn)
        return pn->renameFile(filePath, newFilePath);
    return false;
}

bool FolderNode::addDependencies(const QStringList &dependencies)
{
    if (ProjectNode * const pn = managingProject())
        return pn->addDependencies(dependencies);
    return false;
}

FolderNode::AddNewInformation FolderNode::addNewInformation(const QStringList &files, Node *context) const
{
    Q_UNUSED(files)
    return AddNewInformation(displayName(), context == this ? 120 : 100);
}

/*!
  Adds a node specified by \a node to the internal list of nodes.
*/

void FolderNode::addNode(std::unique_ptr<Node> &&node)
{
    QTC_ASSERT(node, return);
    QTC_ASSERT(!node->parentFolderNode(), qDebug("Node has already a parent folder"));
    node->setParentFolderNode(this);
    m_nodes.emplace_back(std::move(node));
}

/*!
  Return a node specified by \a node from the internal list.
*/

std::unique_ptr<Node> FolderNode::takeNode(Node *node)
{
    return Utils::takeOrDefault(m_nodes, node);
}

bool FolderNode::showInSimpleTree() const
{
    return false;
}

bool FolderNode::showWhenEmpty() const
{
    return m_showWhenEmpty;
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
VirtualFolderNode::VirtualFolderNode(const Utils::FilePath &folderPath) :
    FolderNode(folderPath)
{
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
ProjectNode::ProjectNode(const Utils::FilePath &projectFilePath) :
    FolderNode(projectFilePath)
{
    setPriority(DefaultProjectPriority);
    setListInProject(true);
    setDisplayName(projectFilePath.fileName());
}

bool ProjectNode::canAddSubProject(const QString &proFilePath) const
{
    Q_UNUSED(proFilePath)
    return false;
}

bool ProjectNode::addSubProject(const QString &proFilePath)
{
    Q_UNUSED(proFilePath)
    return false;
}

QStringList ProjectNode::subProjectFileNamePatterns() const
{
    return QStringList();
}

bool ProjectNode::removeSubProject(const QString &proFilePath)
{
    Q_UNUSED(proFilePath)
    return false;
}

bool ProjectNode::addFiles(const QStringList &filePaths, QStringList *notAdded)
{
    if (BuildSystem *bs = buildSystem())
        return bs->addFiles(this, filePaths, notAdded);
    return false;
}

RemovedFilesFromProject ProjectNode::removeFiles(const QStringList &filePaths,
                                                 QStringList *notRemoved)
{
    if (BuildSystem *bs = buildSystem())
        return bs->removeFiles(this, filePaths, notRemoved);
    return RemovedFilesFromProject::Error;
}

bool ProjectNode::deleteFiles(const QStringList &filePaths)
{
    if (BuildSystem *bs = buildSystem())
        return bs->deleteFiles(this, filePaths);
    return false;
}

bool ProjectNode::canRenameFile(const QString &filePath, const QString &newFilePath)
{
    if (BuildSystem *bs = buildSystem())
        return bs->canRenameFile(this, filePath, newFilePath);
    return true;
}

bool ProjectNode::renameFile(const QString &filePath, const QString &newFilePath)
{
    if (BuildSystem *bs = buildSystem())
        return bs->renameFile(this, filePath, newFilePath);
    return false;
}

bool ProjectNode::addDependencies(const QStringList &dependencies)
{
    if (BuildSystem *bs = buildSystem())
        return bs->addDependencies(this, dependencies);
    return false;
}

bool ProjectNode::supportsAction(ProjectAction action, const Node *node) const
{
    if (BuildSystem *bs = buildSystem())
        return bs->supportsAction(const_cast<ProjectNode *>(this), action, node);
    return false;
}

bool ProjectNode::deploysFolder(const QString &folder) const
{
    Q_UNUSED(folder)
    return false;
}

ProjectNode *ProjectNode::projectNode(const Utils::FilePath &file) const
{
    for (const std::unique_ptr<Node> &n: m_nodes) {
        if (ProjectNode *pnode = n->asProjectNode())
            if (pnode->filePath() == file)
                return pnode;
    }
    return nullptr;
}

QVariant ProjectNode::data(Utils::Id role) const
{
    return m_fallbackData.value(role);
}

bool ProjectNode::setData(Utils::Id role, const QVariant &value) const
{
    Q_UNUSED(role)
    Q_UNUSED(value)
    return false;
}

void ProjectNode::setFallbackData(Utils::Id key, const QVariant &value)
{
    m_fallbackData.insert(key, value);
}

BuildSystem *ProjectNode::buildSystem() const
{
    Project *p = getProject();
    Target *t = p ? p->activeTarget() : nullptr;
    return t ? t->buildSystem() : nullptr;
}

bool FolderNode::isEmpty() const
{
    return m_nodes.size() == 0;
}

void FolderNode::handleSubTreeChanged(FolderNode *node)
{
    if (FolderNode *parent = parentFolderNode())
        parent->handleSubTreeChanged(node);
}

void FolderNode::setShowWhenEmpty(bool showWhenEmpty)
{
    m_showWhenEmpty = showWhenEmpty;
}

ContainerNode::ContainerNode(Project *project)
    : FolderNode(project->projectDirectory()), m_project(project)
{
}

QString ContainerNode::displayName() const
{
    QString name = m_project->displayName();

    const QFileInfo fi = m_project->projectFilePath().toFileInfo();
    const QString dir = fi.isDir() ? fi.absoluteFilePath() : fi.absolutePath();
    if (Core::IVersionControl *vc = Core::VcsManager::findVersionControlForDirectory(dir)) {
        QString vcsTopic = vc->vcsTopic(dir);
        if (!vcsTopic.isEmpty())
            name += " [" + vcsTopic + ']';
    }

    return name;
}

bool ContainerNode::supportsAction(ProjectAction action, const Node *node) const
{
    const Node *rootNode = m_project->rootProjectNode();
    return rootNode && rootNode->supportsAction(action, node);
}

ProjectNode *ContainerNode::rootProjectNode() const
{
    return m_project->rootProjectNode();
}

void ContainerNode::removeAllChildren()
{
    m_nodes.clear();
}

void ContainerNode::handleSubTreeChanged(FolderNode *node)
{
    m_project->handleSubTreeChanged(node);
}

} // namespace ProjectExplorer
