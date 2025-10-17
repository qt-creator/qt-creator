// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "projectexplorer_export.h"

#include <QIcon>
#include <QStringList>

#include <coreplugin/iversioncontrol.h>
#include <utils/filepath.h>
#include <utils/id.h>

#include <functional>
#include <optional>
#include <utility>
#include <variant>

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
    App,
    Lib,
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

class PROJECTEXPLORER_EXPORT DirectoryIcon
{
public:
    explicit DirectoryIcon(const QString &overlay);

    QIcon icon() const; // only safe in UI thread

private:
    QString m_overlay;
    static QHash<QString, QIcon> m_cache;
};

using IconCreator = std::function<QIcon()>;

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
    virtual QString rawDisplayName() const { return displayName(); }
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

    virtual QString buildKey() const { return {}; }

    static bool sortByPath(const Node *a, const Node *b);
    void setParentFolderNode(FolderNode *parentFolder);

    void setListInProject(bool l);
    void setIsGenerated(bool g);
    void setPriority(int priority);
    void setLine(int line);

    static FileType fileTypeForMimeType(const Utils::MimeType &mt);
    static FileType fileTypeForFileName(const Utils::FilePath &file);

    Utils::FilePath path() const { return pathOrDirectory(false); }
    Utils::FilePath directory() const { return pathOrDirectory(true); }

protected:
    Node();
    Node(const Node &other) = delete;
    bool operator=(const Node &other) = delete;

    void setFilePath(const Utils::FilePath &filePath);

private:
    Utils::FilePath pathOrDirectory(bool dir) const;

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

    bool supportsAction(ProjectAction action, const Node *node) const override;
    QString displayName() const override;

    bool hasError() const;
    void setHasError(const bool error);
    void setHasError(const bool error) const;

    Core::IVersionControl::FileState modificationState() const;
    void resetModificationState();

    QIcon icon() const;
    void setIcon(const QIcon icon);

    bool useUnavailableMarker() const;
    void setUseUnavailableMarker(bool useUnavailableMarker);

private:
    FileType m_fileType;
    mutable std::optional<Core::IVersionControl::FileState> m_modificationState;
    mutable QIcon m_icon;
    mutable bool m_hasError = false;
    bool m_useUnavailableMarker = false;
};

// Documentation inside.
class PROJECTEXPLORER_EXPORT FolderNode : public Node
{
public:
    explicit FolderNode(const Utils::FilePath &folderPath);

    QString displayName() const override;
    // only safe from UI thread
    QIcon icon() const;

    bool isFolderNodeType() const override { return true; }

    Node *findNode(const std::function<bool(Node *)> &filter);
    QList<Node *> findNodes(const std::function<bool(Node *)> &filter);

    void forEachNode(const std::function<void(FileNode *)> &fileTask,
                     const std::function<void(FolderNode *)> &folderTask = {},
                     const std::function<bool(const FolderNode *)> &folderFilterTask = {}) const;
    void forEachGenericNode(const std::function<void(Node *)> &genericTask) const;
    void forEachProjectNode(const std::function<void(const ProjectNode *)> &genericTask) const;
    void forEachFileNode(const std::function<void(FileNode *)> &fileTask) const;
    void forEachFolderNode(const std::function<void(FolderNode *)> &folderTask) const;
    ProjectNode *findProjectNode(const std::function<bool(const ProjectNode *)> &predicate); // recursive
    FolderNode *findChildFolderNode(const std::function<bool (FolderNode *)> &predicate) const; // non-recursive
    FileNode *findChildFileNode(const std::function<bool (FileNode *)> &predicate) const; // non-recursive
    const QList<Node *> nodes() const;
    FileNode *fileNode(const Utils::FilePath &file) const;
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
    virtual void compress();

    // takes ownership of newNode.
    // Will delete newNode if oldNode is not a child of this node.
    bool replaceSubtree(Node *oldNode, std::unique_ptr<Node> &&newNode);

    void setDisplayName(const QString &name);
    // you have to make sure the QIcon is created in the UI thread if you are calling setIcon(QIcon)
    void setIcon(const QIcon &icon);
    void setIcon(const DirectoryIcon &directoryIcon);
    void setIcon(const QString &path);
    void setIcon(const IconCreator &iconCreator);

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
    void setLocationInfo(const QList<LocationInfo> &info);
    const QList<LocationInfo> locationInfo() const;

    QString addFileFilter() const;
    void setAddFileFilter(const QString &filter) { m_addFileFilter = filter; }

    bool supportsAction(ProjectAction action, const Node *node) const override;

    virtual bool addFiles(const Utils::FilePaths &filePaths, Utils::FilePaths *notAdded = nullptr);
    virtual RemovedFilesFromProject removeFiles(const Utils::FilePaths &filePaths,
                                                Utils::FilePaths *notRemoved = nullptr);
    virtual bool deleteFiles(const Utils::FilePaths &filePaths);
    virtual bool canRenameFile(const Utils::FilePath &oldFilePath,
                               const Utils::FilePath &newFilePath);
    virtual bool renameFiles(const Utils::FilePairs &filesToRename, Utils::FilePaths *notRenamed);
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

    virtual AddNewInformation addNewInformation(const Utils::FilePaths &files, Node *context) const;


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
    QList<LocationInfo> m_locations;

private:
    std::unique_ptr<Node> takeNode(Node *node);

    QString m_displayName;
    QString m_addFileFilter;
    mutable std::variant<QIcon, DirectoryIcon, QString, IconCreator> m_icon;
    bool m_showWhenEmpty = false;
};

class PROJECTEXPLORER_EXPORT VirtualFolderNode : public FolderNode
{
public:
    explicit VirtualFolderNode(const Utils::FilePath &folderPath);

    bool isFolderNodeType() const override { return false; }
    bool isVirtualFolderType() const override { return true; }

    bool isSourcesOrHeaders() const { return m_isSourcesOrHeaders; }
    void setIsSourcesOrHeaders(bool on) { m_isSourcesOrHeaders = on; }

private:
    bool m_isSourcesOrHeaders = false; // "Sources" or "Headers"
};

// Documentation inside.
class PROJECTEXPLORER_EXPORT ProjectNode : public FolderNode
{
public:
    explicit ProjectNode(const Utils::FilePath &projectFilePath);

    virtual bool canAddSubProject(const Utils::FilePath &proFilePath) const;
    virtual bool addSubProject(const Utils::FilePath &proFile);
    virtual QStringList subProjectFileNamePatterns() const;
    virtual bool removeSubProject(const Utils::FilePath &proFilePath);
    virtual std::optional<Utils::FilePath> visibleAfterAddFileAction() const {
        return std::nullopt;
    }

    bool isFolderNodeType() const override { return false; }
    bool isProjectNodeType() const override { return true; }
    bool showInSimpleTree() const override { return true; }

    bool addFiles(const Utils::FilePaths &filePaths, Utils::FilePaths *notAdded = nullptr) final;
    RemovedFilesFromProject removeFiles(const Utils::FilePaths &filePaths,
                                        Utils::FilePaths *notRemoved = nullptr) final;
    bool deleteFiles(const Utils::FilePaths &filePaths) final;
    bool canRenameFile(
        const Utils::FilePath &oldFilePath, const Utils::FilePath &newFilePath) override;
    bool renameFiles(const Utils::FilePairs &filesToRename, Utils::FilePaths *notRenamed) final;
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
    QString rawDisplayName() const final;
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

    Project *m_project = nullptr;
};

class PROJECTEXPLORER_EXPORT ResourceFileNode : public ProjectExplorer::FileNode
{
public:
    ResourceFileNode(const Utils::FilePath &filePath, const QString &qrcPath, const QString &displayName);

    QString displayName() const override;
    QString qrcPath() const;
    bool supportsAction(ProjectExplorer::ProjectAction action, const Node *node) const override;

private:
    QString m_qrcPath;
    QString m_displayName;
};

} // namespace ProjectExplorer

Q_DECLARE_METATYPE(ProjectExplorer::Node *)
Q_DECLARE_METATYPE(ProjectExplorer::FolderNode *)
