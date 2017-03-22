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

#include "projectexplorer_export.h"

#include <QFutureInterface>
#include <QIcon>
#include <QObject>
#include <QStringList>

#include <utils/fileutils.h>

#include <functional>

namespace Utils { class MimeType; }

namespace ProjectExplorer {
class RunConfiguration;

enum class NodeType : quint16 {
    File = 1,
    Folder,
    VirtualFolder,
    Project
};

// File types common for qt projects
enum class FileType : quint16 {
    Unknown = 0,
    Header,
    Source,
    Form,
    StateChart,
    Resource,
    QML,
    Project,
    FileTypeSize
};

enum ProjectAction {
    // Special value to indicate that the actions are handled by the parent
    InheritedFromParent,
    AddSubProject,
    RemoveSubProject,
    // Let's the user select to which project file
    // the file is added
    AddNewFile,
    AddExistingFile,
    // Add files, which match user defined filters,
    // from an existing directory and its subdirectories
    AddExistingDirectory,
    // Removes a file from the project, optionally also
    // delete it on disc
    RemoveFile,
    // Deletes a file from the file system, informs the project
    // that a file was deleted
    // DeleteFile is a define on windows...
    EraseFile,
    Rename,
    DuplicateFile,
    // hides actions that use the path(): Open containing folder, open terminal here and Find in Directory
    HidePathActions,
    HideFileActions,
    HideFolderActions,
    HasSubProjectRunConfigurations
};

class FileNode;
class FolderNode;
class ProjectNode;

// Documentation inside.
class PROJECTEXPLORER_EXPORT Node : public QObject
{
    Q_OBJECT
public:
    enum PriorityLevel {
        DefaultPriority = 0,
        DefaultFilePriority = 100000,
        DefaultFolderPriority = 200000,
        DefaultVirtualFolderPriority = 300000,
        DefaultProjectPriority = 400000,
        DefaultProjectFilePriority = 500000
    };

    virtual ~Node();
    NodeType nodeType() const;
    int priority() const;

    ProjectNode *parentProjectNode() const; // parent project, will be nullptr for the top-level project
    FolderNode *parentFolderNode() const; // parent folder or project

    ProjectNode *managingProject();  // project managing this node.
                                     // result is nullptr if node is the SessionNode
                                     // or node if node is a ProjectNode directly below SessionNode
                                     // or node->parentProjectNode() for all other cases.
    const ProjectNode *managingProject() const; // see above.

    const Utils::FileName &filePath() const;  // file system path
    int line() const;
    virtual QString displayName() const;
    virtual QString tooltip() const;
    bool isEnabled() const;

    virtual QList<ProjectAction> supportedActions(Node *node) const;

    void setEnabled(bool enabled);
    void setAbsoluteFilePathAndLine(const Utils::FileName &filePath, int line);

    virtual FileNode *asFileNode() { return nullptr; }
    virtual const FileNode *asFileNode() const { return nullptr; }
    virtual FolderNode *asFolderNode() { return nullptr; }
    virtual const FolderNode *asFolderNode() const { return nullptr; }
    virtual ProjectNode *asProjectNode() { return nullptr; }
    virtual const ProjectNode *asProjectNode() const { return nullptr; }

    static bool sortByPath(const Node *a, const Node *b);
    void setParentFolderNode(FolderNode *parentFolder);

    static FileType fileTypeForMimeType(const Utils::MimeType &mt);
    static FileType fileTypeForFileName(const Utils::FileName &file);

protected:
    Node(NodeType nodeType, const Utils::FileName &filePath, int line = -1);

    void setPriority(int priority);

private:
    FolderNode *m_parentFolderNode = nullptr;
    Utils::FileName m_filePath;
    int m_line = -1;
    int m_priority = DefaultPriority;
    const NodeType m_nodeType;
    bool m_isEnabled = true;
};

class PROJECTEXPLORER_EXPORT FileNode : public Node
{
public:
    FileNode(const Utils::FileName &filePath, const FileType fileType, bool generated, int line = -1);
    FileNode(const FileNode &other) : FileNode(other.filePath(), other.fileType(), true) {}

    FileType fileType() const;
    bool isGenerated() const;

    FileNode *asFileNode() final { return this; }
    const FileNode *asFileNode() const final { return this; }

    static QList<FileNode *> scanForFiles(const Utils::FileName &directory,
                                          const std::function<FileNode *(const Utils::FileName &fileName)> factory,
                                          QFutureInterface<QList<FileNode *>> *future = nullptr);

private:
    FileType m_fileType;
    bool m_generated;
};

// Documentation inside.
class PROJECTEXPLORER_EXPORT FolderNode : public Node
{
public:
    explicit FolderNode(const Utils::FileName &folderPath, NodeType nodeType = NodeType::Folder,
                        const QString &displayName = QString());
    ~FolderNode() override;

    QString displayName() const override;
    QIcon icon() const;

    Node *findNode(const std::function<bool(Node *)> &filter);
    QList<Node *> findNodes(const std::function<bool(Node *)> &filter);

    void forEachNode(const std::function<void(FileNode *)> &fileTask,
                     const std::function<void(FolderNode *)> &folderTask = {},
                     const std::function<bool(const FolderNode *)> &folderFilterTask = {}) const;
    void forEachGenericNode(const std::function<void(Node *)> &genericTask) const;
    const QList<Node *> nodes() const { return m_nodes; }
    QList<FileNode *> fileNodes() const;
    FileNode *fileNode(const Utils::FileName &file) const;
    QList<FolderNode *> folderNodes() const;
    using FolderNodeFactory = std::function<FolderNode *(const Utils::FileName &)>;
    void addNestedNodes(const QList<FileNode *> &files, const Utils::FileName &overrideBaseDir = Utils::FileName(),
                        const FolderNodeFactory &factory = [](const Utils::FileName &fn) { return new FolderNode(fn); });
    void addNestedNode(FileNode *fileNode, const Utils::FileName &overrideBaseDir = Utils::FileName(),
                       const FolderNodeFactory &factory = [](const Utils::FileName &fn) { return new FolderNode(fn); });
    void compress();

    bool isAncesterOf(Node *n);

    // takes ownership of newNode.
    // Will delete newNode if oldNode is not a child of this node.
    bool replaceSubtree(Node *oldNode, Node *newNode);

    void setDisplayName(const QString &name);
    void setIcon(const QIcon &icon);

    virtual QString addFileFilter() const;

    virtual bool addFiles(const QStringList &filePaths, QStringList *notAdded = 0);
    virtual bool removeFiles(const QStringList &filePaths, QStringList *notRemoved = 0);
    virtual bool deleteFiles(const QStringList &filePaths);
    virtual bool canRenameFile(const QString &filePath, const QString &newFilePath);
    virtual bool renameFile(const QString &filePath, const QString &newFilePath);

    class AddNewInformation
    {
    public:
        AddNewInformation(const QString &name, int p)
            :displayName(name), priority(p)
        { }
        QString displayName;
        int priority;
    };

    virtual AddNewInformation addNewInformation(const QStringList &files, Node *context) const;


    // determines if node will be shown in the flat view, by default folder and projects aren't shown
    virtual bool showInSimpleTree() const;

    void addNode(Node *node);
    void removeNode(Node *node);

    bool isEmpty() const;

    FolderNode *asFolderNode() override { return this; }
    const FolderNode *asFolderNode() const override { return this; }

protected:
    QList<Node *> m_nodes;

private:
    QString m_displayName;
    mutable QIcon m_icon;
};

class PROJECTEXPLORER_EXPORT VirtualFolderNode : public FolderNode
{
public:
    explicit VirtualFolderNode(const Utils::FileName &folderPath, int priority);

    void setAddFileFilter(const QString &filter) { m_addFileFilter = filter; }
    QString addFileFilter() const override;

private:
    QString m_addFileFilter;
};

// Documentation inside.
class PROJECTEXPLORER_EXPORT ProjectNode : public FolderNode
{
public:
    virtual bool canAddSubProject(const QString &proFilePath) const;
    virtual bool addSubProject(const QString &proFile);
    virtual bool removeSubProject(const QString &proFilePath);

    bool addFiles(const QStringList &filePaths, QStringList *notAdded = 0) override;
    bool removeFiles(const QStringList &filePaths, QStringList *notRemoved = 0) override;
    bool deleteFiles(const QStringList &filePaths) override;
    bool canRenameFile(const QString &filePath, const QString &newFilePath) override;
    bool renameFile(const QString &filePath, const QString &newFilePath) override;

    // by default returns false
    virtual bool deploysFolder(const QString &folder) const;

    virtual QList<RunConfiguration *> runConfigurations() const;

    ProjectNode *projectNode(const Utils::FileName &file) const;

    ProjectNode *asProjectNode() final { return this; }
    const ProjectNode *asProjectNode() const final { return this; }

protected:
    explicit ProjectNode(const Utils::FileName &projectFilePath);
};

} // namespace ProjectExplorer

Q_DECLARE_METATYPE(ProjectExplorer::Node *)
Q_DECLARE_METATYPE(ProjectExplorer::FolderNode *)
