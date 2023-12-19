// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "projectnodes.h"

#include "buildsystem.h"
#include "project.h"
#include "projectexplorerconstants.h"
#include "target.h"

#include <coreplugin/icore.h>
#include <coreplugin/iversioncontrol.h>
#include <coreplugin/vcsmanager.h>

#include <utils/fileutils.h>
#include <utils/fsengine/fileiconprovider.h>
#include <utils/hostosinfo.h>
#include <utils/mimeutils.h>
#include <utils/pointeralgorithm.h>
#include <utils/qtcassert.h>
#include <utils/threadutils.h>
#include <utils/utilsicons.h>

#include <QDir>
#include <QFileInfo>
#include <QIcon>

#include <memory>

using namespace Utils;

namespace ProjectExplorer {

QHash<QString, QIcon> DirectoryIcon::m_cache;

static FolderNode *recursiveFindOrCreateFolderNode(FolderNode *folder,
                                                   const FilePath &directory,
                                                   const FilePath &overrideBaseDir,
                                                   const FolderNode::FolderNodeFactory &factory)
{
    Utils::FilePath path = overrideBaseDir.isEmpty() ? folder->filePath() : overrideBaseDir;

    Utils::FilePath directoryWithoutPrefix;
    bool isRelative = false;

    if (path.isEmpty() || path.isRootPath()) {
        directoryWithoutPrefix = directory;
        isRelative = false;
    } else {
        if (directory.isChildOf(path) || directory == path) {
            isRelative = true;
            directoryWithoutPrefix = directory.relativeChildPath(path);
        } else {
            const FilePath relativePath = directory.relativePathFrom(path);
            if (relativePath.path().count("../") < 5) {
                isRelative = true;
                directoryWithoutPrefix = relativePath;
            } else {
                isRelative = false;
                path.clear();
                directoryWithoutPrefix = directory;
            }
        }
    }
    QStringList parts = directoryWithoutPrefix.path().split('/', Qt::SkipEmptyParts);
    if (directory.osType() != OsTypeWindows && !isRelative && !parts.isEmpty())
        parts[0].prepend('/');

    ProjectExplorer::FolderNode *parent = folder;
    for (const QString &part : std::as_const(parts)) {
        path = path.pathAppended(part).cleanPath();
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

QIcon FileNode::icon() const
{
    if (hasError())
        return Utils::Icons::WARNING.icon();
    if (m_icon.isNull())
        m_icon = Utils::FileIconProvider::icon(filePath());
    return m_icon;
}

void FileNode::setIcon(const QIcon icon)
{
    m_icon = icon;
}

bool FileNode::hasError() const
{
    return m_hasError;
}

void FileNode::setHasError(bool error)
{
    m_hasError = error;
}

void FileNode::setHasError(bool error) const
{
    m_hasError = error;
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
    return fileTypeForMimeType(Utils::mimeTypeForFile(file, Utils::MimeMatchMode::MatchExtension));
}

FilePath Node::pathOrDirectory(bool dir) const
{
    const FolderNode *folder = asFolderNode();
    if (isVirtualFolderType() && folder) {
        FilePath location;
        // Virtual Folder case
        // If there are files directly below or no subfolders, take the folder path
        auto Any = [](auto) { return true; };
        if (folder->findChildFileNode(Any) || !folder->findChildFolderNode(Any)) {
            location = m_filePath;
        } else {
            // Otherwise we figure out a commonPath from the subfolders
            FilePaths list;
            folder->forEachFolderNode([&](FolderNode *f) { list << f->filePath(); });
            location = FileUtils::commonPath(list);
        }

        QTC_CHECK(!location.needsDevice());
        QFileInfo fi = location.toFileInfo();
        while ((!fi.exists() || !fi.isDir()) && !fi.isRoot() && (fi.fileName() != fi.absolutePath()))
            fi.setFile(fi.absolutePath());
        return FilePath::fromString(fi.absoluteFilePath());
    }

    if (m_filePath.isEmpty())
        return {};

    if (m_filePath.needsDevice()) {
        if (dir)
            return m_filePath.isDir() ? m_filePath.absoluteFilePath() : m_filePath.absolutePath();
        return m_filePath;
    }

    FilePath location;
    QFileInfo fi = m_filePath.toFileInfo();
    // remove any /suffixes, which e.g. ResourceNode uses
    // Note this could be removed again by making path() a true path again
    // That requires changes in both the VirtualFolderNode and ResourceNode
    while (!fi.exists() && !fi.isRoot())
        fi.setFile(fi.absolutePath());

    if (dir)
        location = FilePath::fromString(fi.isDir() ? fi.absoluteFilePath() : fi.absolutePath());
    else
        location = FilePath::fromString(fi.absoluteFilePath());

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
    (QStyle::S_PDirIcon). Calling this method is only safe in the UI thread.

    \sa setIcon()
 */
QIcon FolderNode::icon() const
{
    QTC_CHECK(isMainThread());

    // Instantiating the Icon provider is expensive.
    if (auto strPtr = std::get_if<QString>(&m_icon)) {
        m_icon = QIcon(*strPtr);
    } else if (auto directoryIconPtr = std::get_if<DirectoryIcon>(&m_icon)) {
        m_icon = directoryIconPtr->icon();
    } else if (auto creatorPtr = std::get_if<IconCreator>(&m_icon)) {
        m_icon = (*creatorPtr)();
    } else {
        auto iconPtr = std::get_if<QIcon>(&m_icon);
        if (!iconPtr || iconPtr->isNull())
            m_icon = Utils::FileIconProvider::icon(QFileIconProvider::Folder);
    }
    return std::get<QIcon>(m_icon);
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

void FolderNode::forEachFileNode(const std::function<void (FileNode *)> &fileTask) const
{
    for (const std::unique_ptr<Node> &n : m_nodes) {
        if (FileNode *fn = n->asFileNode())
            fileTask(fn);
    }
}

void FolderNode::forEachFolderNode(const std::function<void (FolderNode *)> &folderTask) const
{
    for (const std::unique_ptr<Node> &n : m_nodes) {
        if (FolderNode *fn = n->asFolderNode())
            folderTask(fn);
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

FolderNode *FolderNode::findChildFolderNode(const std::function<bool(FolderNode *)> &predicate) const
{
    for (const std::unique_ptr<Node> &n : m_nodes) {
        if (FolderNode *fn = n->asFolderNode())
            if (predicate(fn))
                return fn;
    }
    return nullptr;
}

FileNode *FolderNode::findChildFileNode(const std::function<bool(FileNode *)> &predicate) const
{
    for (const std::unique_ptr<Node> &n : m_nodes) {
        if (FileNode *fn = n->asFileNode())
            if (predicate(fn))
                return fn;
    }
    return nullptr;
}

const QList<Node *> FolderNode::nodes() const
{
    return Utils::toRawPointer<QList>(m_nodes);
}

FileNode *FolderNode::fileNode(const Utils::FilePath &file) const
{
    return static_cast<FileNode *>(Utils::findOrDefault(m_nodes,
                                                        [&file](const std::unique_ptr<Node> &n) {
        const FileNode *fn = n->asFileNode();
        return fn && fn->filePath() == file;
    }));
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
        forEachFolderNode([&](FolderNode *fn) { fn->compress(); });
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

/*!
    Sets the \a icon for this node. Note that creating QIcon instances is only safe in the UI thread.
*/
void FolderNode::setIcon(const QIcon &icon)
{
    m_icon = icon;
}

/*!
    Sets the \a directoryIcon that is used to create the icon for this node on demand.
*/
void FolderNode::setIcon(const DirectoryIcon &directoryIcon)
{
    m_icon = directoryIcon;
}

/*!
    Sets the \a path that is used to create the icon for this node on demand.
*/
void FolderNode::setIcon(const QString &path)
{
    m_icon = path;
}

/*!
    Sets the \a iconCreator function that is used to create the icon for this node on demand.
*/
void FolderNode::setIcon(const IconCreator &iconCreator)
{
    m_icon = iconCreator;
}

void FolderNode::setLocationInfo(const QVector<FolderNode::LocationInfo> &info)
{
    m_locations = Utils::sorted(info, &LocationInfo::priority);
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

bool FolderNode::addFiles(const FilePaths &filePaths, FilePaths *notAdded)
{
    ProjectNode *pn = managingProject();
    if (pn)
        return pn->addFiles(filePaths, notAdded);
    return false;
}

RemovedFilesFromProject FolderNode::removeFiles(const FilePaths &filePaths, FilePaths *notRemoved)
{
    if (ProjectNode * const pn = managingProject())
        return pn->removeFiles(filePaths, notRemoved);
    return RemovedFilesFromProject::Error;
}

bool FolderNode::deleteFiles(const FilePaths &filePaths)
{
    ProjectNode *pn = managingProject();
    if (pn)
        return pn->deleteFiles(filePaths);
    return false;
}

bool FolderNode::canRenameFile(const FilePath &oldFilePath, const FilePath &newFilePath)
{
    ProjectNode *pn = managingProject();
    if (pn)
        return pn->canRenameFile(oldFilePath, newFilePath);
    return false;
}

bool FolderNode::renameFile(const FilePath &oldFilePath, const FilePath &newFilePath)
{
    ProjectNode *pn = managingProject();
    if (pn)
        return pn->renameFile(oldFilePath, newFilePath);
    return false;
}

bool FolderNode::addDependencies(const QStringList &dependencies)
{
    if (ProjectNode * const pn = managingProject())
        return pn->addDependencies(dependencies);
    return false;
}

FolderNode::AddNewInformation FolderNode::addNewInformation(const FilePaths &files, Node *context) const
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
VirtualFolderNode::VirtualFolderNode(const FilePath &folderPath) :
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
ProjectNode::ProjectNode(const FilePath &projectFilePath) :
    FolderNode(projectFilePath)
{
    setPriority(DefaultProjectPriority);
    setListInProject(true);
    setDisplayName(projectFilePath.fileName());
}

bool ProjectNode::canAddSubProject(const FilePath &proFilePath) const
{
    Q_UNUSED(proFilePath)
    return false;
}

bool ProjectNode::addSubProject(const FilePath &proFilePath)
{
    Q_UNUSED(proFilePath)
    return false;
}

QStringList ProjectNode::subProjectFileNamePatterns() const
{
    return {};
}

bool ProjectNode::removeSubProject(const FilePath &proFilePath)
{
    Q_UNUSED(proFilePath)
    return false;
}

bool ProjectNode::addFiles(const FilePaths &filePaths, FilePaths *notAdded)
{
    if (BuildSystem *bs = buildSystem())
        return bs->addFiles(this, filePaths, notAdded);
    return false;
}

RemovedFilesFromProject ProjectNode::removeFiles(const FilePaths &filePaths, FilePaths *notRemoved)
{
    if (BuildSystem *bs = buildSystem())
        return bs->removeFiles(this, filePaths, notRemoved);
    return RemovedFilesFromProject::Error;
}

bool ProjectNode::deleteFiles(const FilePaths &filePaths)
{
    if (BuildSystem *bs = buildSystem())
        return bs->deleteFiles(this, filePaths);
    return false;
}

bool ProjectNode::canRenameFile(const Utils::FilePath &oldFilePath, const Utils::FilePath &newFilePath)
{
    if (BuildSystem *bs = buildSystem())
        return bs->canRenameFile(this, oldFilePath, newFilePath);
    return true;
}

bool ProjectNode::renameFile(const Utils::FilePath &oldFilePath, const Utils::FilePath &newFilePath)
{
    if (BuildSystem *bs = buildSystem())
        return bs->renameFile(this, oldFilePath, newFilePath);
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

    const FilePath fp = m_project->projectFilePath();
    const FilePath dir = fp.isDir() ? fp.absoluteFilePath() : fp.absolutePath();
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

/*!
    \class ProjectExplorer::DirectoryIcon

    The DirectoryIcon class represents a directory icon with an overlay.

    The QIcon is created on demand and globally cached, so other DirectoryIcon
    instances with the same overlay share the same QIcon instance.
*/

/*!
    Creates a DirectoryIcon for the specified \a overlay.
*/
DirectoryIcon::DirectoryIcon(const QString &overlay)
    : m_overlay(overlay)
{}

/*!
    Returns the icon for this DirectoryIcon. Calling this method is only safe in the UI thread.
*/
QIcon DirectoryIcon::icon() const
{
    QTC_CHECK(isMainThread());
    const auto it = m_cache.find(m_overlay);
    if (it != m_cache.end())
        return it.value();
    const QIcon icon = Utils::FileIconProvider::directoryIcon(m_overlay);
    m_cache.insert(m_overlay, icon);
    return icon;
}

ResourceFileNode::ResourceFileNode(const FilePath &filePath, const QString &qrcPath, const QString &displayName)
    : FileNode(filePath, FileNode::fileTypeForFileName(filePath))
    , m_qrcPath(qrcPath)
    , m_displayName(displayName)
{
}

QString ResourceFileNode::displayName() const
{
    return m_displayName;
}

QString ResourceFileNode::qrcPath() const
{
    return m_qrcPath;
}

bool ResourceFileNode::supportsAction(ProjectAction action, const Node *node) const
{
    if (action == HidePathActions)
        return false;
    return parentFolderNode()->supportsAction(action, node);
}

} // namespace ProjectExplorer
