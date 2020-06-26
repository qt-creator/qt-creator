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
#include <QStringList>

#include <utils/fileutils.h>
#include <utils/id.h>
#include <utils/optional.h>

#include <functional>

namespace Utils { class MimeType; }

namespace ProjectExplorer {

class BuildSystem;
class Project;

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

enum class ProductType { App, Lib, Other, None };

enum ProjectAction {
    // Special value to indicate that the actions are handled by the parent
    InheritedFromParent,
    AddSubProject,
    AddExistingProject,
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
    // hides actions that use the path(): Open containing folder, open terminal here and Find in Directory
    HidePathActions,
    HideFileActions,
    HideFolderActions,
};

enum class RemovedFilesFromProject { Ok, Wildcard, Error };

class FileNode;
class FolderNode;
class ProjectNode;
class ContainerNode;

// Documentation inside.
class PROJECTEXPLORER_EXPORT Node
{
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

    virtual bool isFolderNodeType() const { return false; }
    virtual bool isProjectNodeType() const { return false; }
    virtual bool isVirtualFolderType() const { return false; }

    int priority() const;

    ProjectNode *parentProjectNode() const; // parent project, will be nullptr for the top-level project
    FolderNode *parentFolderNode() const; // parent folder or project

    ProjectNode *managingProject();  // project managing this node.
                                     // result is the container's rootProject node if this is a project container node
                                     // (i.e. possibly null)
                                     // or node if node is a top-level ProjectNode directly below a container
                                     // or node->parentProjectNode() for all other cases.
    const ProjectNode *managingProject() const; // see above.

    Project *getProject() const;

    const Utils::FilePath &filePath() const;  // file system path
    int line() const;
    virtual QString displayName() const;
    virtual QString tooltip() const;
    bool isEnabled() const;
    bool listInProject() const;
    bool isGenerated() const;

    virtual bool supportsAction(ProjectAction action, const Node *node) const;

    void setEnabled(bool enabled);
    void setAbsoluteFilePathAndLine(const Utils::FilePath &filePath, int line);

    virtual FileNode *asFileNode() { return nullptr; }
    virtual const FileNode *asFileNode() const { return nullptr; }
    virtual FolderNode *asFolderNode() { return nullptr; }
    virtual const FolderNode *asFolderNode() const { return nullptr; }
    virtual ProjectNode *asProjectNode() { return nullptr; }
    virtual const ProjectNode *asProjectNode() const { return nullptr; }
    virtual ContainerNode *asContainerNode() { return nullptr; }
    virtual const ContainerNode *asContainerNode() const { return nullptr; }

    virtual QString buildKey() const { return QString(); }

    static bool sortByPath(const Node *a, const Node *b);
    void setParentFolderNode(FolderNode *parentFolder);

    void setListInProject(bool l);
    void setIsGenerated(bool g);
    void setPriority(int priority);
    void setLine(int line);

    static FileType fileTypeForMimeType(const Utils::MimeType &mt);
    static FileType fileTypeForFileName(const Utils::FilePath &file);

    QString path() const { return pathOrDirectory(false); }
    QString directory() const { return pathOrDirectory(true); }

protected:
    Node();
    Node(const Node &other) = delete;
    bool operator=(const Node &other) = delete;

    void setFilePath(const Utils::FilePath &filePath);

private:
    QString pathOrDirectory(bool dir) const;

    FolderNode *m_parentFolderNode = nullptr;
    Utils::FilePath m_filePath;
    int m_line = -1;
    int m_priority = DefaultPriority;

    enum NodeFlag : quint16 {
        FlagNone = 0,
        FlagIsEnabled = 1 << 0,
        FlagIsGenerated = 1 << 1,
        FlagListInProject = 1 << 2,
    };
    NodeFlag m_flags = FlagIsEnabled;
};

class PROJECTEXPLORER_EXPORT FileNode : public Node
{
public:
    FileNode(const Utils::FilePath &filePath, const FileType fileType);

    FileNode *clone() const;

    FileType fileType() const;

    FileNode *asFileNode() final { return this; }
    const FileNode *asFileNode() const final { return this; }

    static QList<FileNode *>
    scanForFiles(const Utils::FilePath &directory,
                 const std::function<FileNode *(const Utils::FilePath &fileName)> factory,
                 QFutureInterface<QList<FileNode *>> *future = nullptr);
    bool supportsAction(ProjectAction action, const Node *node) const override;
    QString displayName() const override;

private:
    FileType m_fileType;
};

// Documentation inside.
class PROJECTEXPLORER_EXPORT FolderNode : public Node
{
public:
    explicit FolderNode(const Utils::FilePath &folderPath);

    QString displayName() const override;
    QIcon icon() const;

    bool isFolderNodeType() const override { return true; }

    Node *findNode(const std::function<bool(Node *)> &filter);
    QList<Node *> findNodes(const std::function<bool(Node *)> &filter);

    void forEachNode(const std::function<void(FileNode *)> &fileTask,
                     const std::function<void(FolderNode *)> &folderTask = {},
                     const std::function<bool(const FolderNode *)> &folderFilterTask = {}) const;
    void forEachGenericNode(const std::function<void(Node *)> &genericTask) const;
    void forEachProjectNode(const std::function<void(const ProjectNode *)> &genericTask) const;
    ProjectNode *findProjectNode(const std::function<bool(const ProjectNode *)> &predicate);
    const QList<Node *> nodes() const;
    QList<FileNode *> fileNodes() const;
    FileNode *fileNode(const Utils::FilePath &file) const;
    QList<FolderNode *> folderNodes() const;
    FolderNode *folderNode(const Utils::FilePath &directory) const;

    using FolderNodeFactory = std::function<std::unique_ptr<FolderNode>(const Utils::FilePath &)>;
    void addNestedNodes(std::vector<std::unique_ptr<FileNode>> &&files,
                        const Utils::FilePath &overrideBaseDir = Utils::FilePath(),
                        const FolderNodeFactory &factory
                        = [](const Utils::FilePath &fn) { return std::make_unique<FolderNode>(fn); });
    void addNestedNode(std::unique_ptr<FileNode> &&fileNode,
                       const Utils::FilePath &overrideBaseDir = Utils::FilePath(),
                       const FolderNodeFactory &factory
                       = [](const Utils::FilePath &fn) { return std::make_unique<FolderNode>(fn); });
    void compress();

    // takes ownership of newNode.
    // Will delete newNode if oldNode is not a child of this node.
    bool replaceSubtree(Node *oldNode, std::unique_ptr<Node> &&newNode);

    void setDisplayName(const QString &name);
    void setIcon(const QIcon &icon);

    class LocationInfo
    {
    public:
        LocationInfo() = default;
        LocationInfo(const QString &dn,
                     const Utils::FilePath &p,
                     const int l = 0,
                     const unsigned int prio = 0)
            : path(p)
            , line(l)
            , priority(prio)
            , displayName(dn)
        {}

        Utils::FilePath path;
        int line = -1;
        unsigned int priority = 0;
        QString displayName;
    };
    void setLocationInfo(const QVector<LocationInfo> &info);
    const QVector<LocationInfo> locationInfo() const;

    QString addFileFilter() const;
    void setAddFileFilter(const QString &filter) { m_addFileFilter = filter; }

    bool supportsAction(ProjectAction action, const Node *node) const override;

    virtual bool addFiles(const QStringList &filePaths, QStringList *notAdded = nullptr);
    virtual RemovedFilesFromProject removeFiles(const QStringList &filePaths,
                                                QStringList *notRemoved = nullptr);
    virtual bool deleteFiles(const QStringList &filePaths);
    virtual bool canRenameFile(const QString &filePath, const QString &newFilePath);
    virtual bool renameFile(const QString &filePath, const QString &newFilePath);
    virtual bool addDependencies(const QStringList &dependencies);

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

    // determines if node will always be shown when hiding empty directories
    bool showWhenEmpty() const;
    void setShowWhenEmpty(bool showWhenEmpty);

    void addNode(std::unique_ptr<Node> &&node);

    bool isEmpty() const;

    FolderNode *asFolderNode() override { return this; }
    const FolderNode *asFolderNode() const override { return this; }

protected:
    virtual void handleSubTreeChanged(FolderNode *node);

    std::vector<std::unique_ptr<Node>> m_nodes;
    QVector<LocationInfo> m_locations;

private:
    std::unique_ptr<Node> takeNode(Node *node);

    QString m_displayName;
    QString m_addFileFilter;
    mutable QIcon m_icon;
    bool m_showWhenEmpty = false;
};

class PROJECTEXPLORER_EXPORT VirtualFolderNode : public FolderNode
{
public:
    explicit VirtualFolderNode(const Utils::FilePath &folderPath);

    bool isFolderNodeType() const override { return false; }
    bool isVirtualFolderType() const override { return true; }
};

// Documentation inside.
class PROJECTEXPLORER_EXPORT ProjectNode : public FolderNode
{
public:
    explicit ProjectNode(const Utils::FilePath &projectFilePath);

    virtual bool canAddSubProject(const QString &proFilePath) const;
    virtual bool addSubProject(const QString &proFile);
    virtual QStringList subProjectFileNamePatterns() const;
    virtual bool removeSubProject(const QString &proFilePath);
    virtual Utils::optional<Utils::FilePath> visibleAfterAddFileAction() const {
        return Utils::nullopt;
    }

    bool isFolderNodeType() const override { return false; }
    bool isProjectNodeType() const override { return true; }
    bool showInSimpleTree() const override { return true; }

    bool addFiles(const QStringList &filePaths, QStringList *notAdded = nullptr) final;
    RemovedFilesFromProject removeFiles(const QStringList &filePaths,
                                        QStringList *notRemoved = nullptr) final;
    bool deleteFiles(const QStringList &filePaths) final;
    bool canRenameFile(const QString &filePath, const QString &newFilePath) final;
    bool renameFile(const QString &filePath, const QString &newFilePath) final;
    bool addDependencies(const QStringList &dependencies) final;
    bool supportsAction(ProjectAction action, const Node *node) const final;

    // by default returns false
    virtual bool deploysFolder(const QString &folder) const;

    ProjectNode *projectNode(const Utils::FilePath &file) const;

    ProjectNode *asProjectNode() final { return this; }
    const ProjectNode *asProjectNode() const final { return this; }

    virtual QStringList targetApplications() const { return {}; }
    virtual bool parseInProgress() const { return false; }

    virtual bool validParse() const { return false; }
    virtual QVariant data(Utils::Id role) const;
    virtual bool setData(Utils::Id role, const QVariant &value) const;

    bool isProduct() const { return m_productType != ProductType::None; }
    ProductType productType() const { return m_productType; }

    // TODO: Currently used only for "Build for current run config" functionality, but we should
    //       probably use it to centralize the node-specific "Build" functionality that
    //       currently each project manager plugin adds to the context menu by itself.
    //       The function should then move up to the Node class, so it can also serve the
    //       "build single file" case.
    virtual void build() {}

    void setFallbackData(Utils::Id key, const QVariant &value);

protected:
    void setProductType(ProductType type) { m_productType = type; }

    QString m_target;

private:
    BuildSystem *buildSystem() const;

    QHash<Utils::Id, QVariant> m_fallbackData; // Used in data(), unless overridden.
    ProductType m_productType = ProductType::None;
};

class PROJECTEXPLORER_EXPORT ContainerNode : public FolderNode
{
public:
    ContainerNode(Project *project);

    QString displayName() const final;
    bool supportsAction(ProjectAction action, const Node *node) const final;

    bool isFolderNodeType() const override { return false; }
    bool isProjectNodeType() const override { return true; }

    ContainerNode *asContainerNode() final { return this; }
    const ContainerNode *asContainerNode() const final { return this; }

    ProjectNode *rootProjectNode() const;
    Project *project() const { return m_project; }

    void removeAllChildren();

private:
    void handleSubTreeChanged(FolderNode *node) final;

    Project *m_project;
};

} // namespace ProjectExplorer

Q_DECLARE_METATYPE(ProjectExplorer::Node *)
Q_DECLARE_METATYPE(ProjectExplorer::FolderNode *)
