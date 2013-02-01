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

#ifndef PROJECTNODES_H
#define PROJECTNODES_H

#include "projectexplorer_export.h"

#include <QIcon>

#include <QObject>
#include <QStringList>

QT_BEGIN_NAMESPACE
class QFileInfo;
QT_END_NAMESPACE

namespace Core {
    class MimeDatabase;
}

namespace ProjectExplorer {
class RunConfiguration;

enum NodeType {
    FileNodeType = 1,
    FolderNodeType,
    VirtualFolderNodeType,
    ProjectNodeType,
    SessionNodeType
};

// File types common for qt projects
enum FileType {
    UnknownFileType = 0,
    HeaderType,
    SourceType,
    FormType,
    ResourceType,
    QMLType,
    ProjectFileType,
    FileTypeSize
};

class Node;
class FileNode;
class FileContainerNode;
class FolderNode;
class ProjectNode;
class NodesWatcher;
class NodesVisitor;
class SessionManager;

// Documentation inside.
class PROJECTEXPLORER_EXPORT Node : public QObject {
    Q_OBJECT
public:
    NodeType nodeType() const;
    ProjectNode *projectNode() const;     // managing project
    FolderNode *parentFolderNode() const; // parent folder or project
    QString path() const;                 // file system path
    virtual int line() const;
    virtual QString displayName() const;
    virtual QString vcsTopic() const;
    virtual QString tooltip() const;
    virtual bool isEnabled() const;

    void setPath(const QString &path);
    void emitNodeUpdated();

protected:
    Node(NodeType nodeType, const QString &path);

    void setNodeType(NodeType type);
    void setProjectNode(ProjectNode *project);
    void setParentFolderNode(FolderNode *parentFolder);

    void emitNodeSortKeyAboutToChange();
    void emitNodeSortKeyChanged();

private:
    NodeType m_nodeType;
    ProjectNode *m_projectNode;
    FolderNode *m_folderNode;
    QString m_path;
};

class PROJECTEXPLORER_EXPORT FileNode : public Node {
    Q_OBJECT
public:
    FileNode(const QString &filePath, const FileType fileType, bool generated);

    FileType fileType() const;
    bool isGenerated() const;

private:
    // managed by ProjectNode
    friend class ProjectNode;

    FileType m_fileType;
    bool m_generated;
};

// Documentation inside.
class PROJECTEXPLORER_EXPORT FolderNode : public Node {
    Q_OBJECT
public:
    explicit FolderNode(const QString &folderPath, NodeType nodeType = FolderNodeType);
    virtual ~FolderNode();

    QString displayName() const;
    QIcon icon() const;

    QList<FileNode*> fileNodes() const;
    QList<FolderNode*> subFolderNodes() const;

    virtual void accept(NodesVisitor *visitor);

    void setDisplayName(const QString &name);
    void setIcon(const QIcon &icon);

    FileNode *findFile(const QString &path);
    FolderNode *findSubFolder(const QString &path);

protected:
    QList<FolderNode*> m_subFolderNodes;
    QList<FileNode*> m_fileNodes;

private:
    // managed by ProjectNode
    friend class ProjectNode;
    QString m_displayName;
    mutable QIcon m_icon;
};

class PROJECTEXPLORER_EXPORT VirtualFolderNode : public FolderNode
{
    Q_OBJECT
public:
    explicit VirtualFolderNode(const QString &folderPath, int priority);
    virtual ~VirtualFolderNode();

    int priority() const;
private:
    int m_priority;
};

// Documentation inside.
class PROJECTEXPLORER_EXPORT ProjectNode : public FolderNode
{
    Q_OBJECT

public:
    enum ProjectAction {
        AddSubProject,
        RemoveSubProject,
        // Let's the user select to which project file
        // the file is added
        AddNewFile,
        AddExistingFile,
        // Removes a file from the project, optionally also
        // delete it on disc
        RemoveFile,
        // Deletes a file from the file system, informs the project
        // that a file was deleted
        // DeleteFile is a define on windows...
        EraseFile,
        Rename,
        HasSubProjectRunConfigurations
    };

    QString vcsTopic() const;

    // all subFolders that are projects
    QList<ProjectNode*> subProjectNodes() const;

    // determines if the project will be shown in the flat view
    // TODO find a better name
    void aboutToChangeHasBuildTargets();
    void hasBuildTargetsChanged();
    virtual bool hasBuildTargets() const = 0;

    virtual QList<ProjectAction> supportedActions(Node *node) const = 0;

    virtual bool canAddSubProject(const QString &proFilePath) const = 0;

    virtual bool addSubProjects(const QStringList &proFilePaths) = 0;

    virtual bool removeSubProjects(const QStringList &proFilePaths) = 0;

    virtual bool addFiles(const FileType fileType,
                          const QStringList &filePaths,
                          QStringList *notAdded = 0) = 0;
    // TODO: Maybe remove fileType, can be detected by project
    virtual bool removeFiles(const FileType fileType,
                             const QStringList &filePaths,
                             QStringList *notRemoved = 0) = 0;
    virtual bool deleteFiles(const FileType fileType,
                             const QStringList &filePaths) = 0;
    virtual bool renameFile(const FileType fileType,
                             const QString &filePath,
                             const QString &newFilePath) = 0;
    // by default returns false
    virtual bool deploysFolder(const QString &folder) const;

    // TODO node parameter not really needed
    virtual QList<ProjectExplorer::RunConfiguration *> runConfigurationsFor(Node *node) = 0;


    QList<NodesWatcher*> watchers() const;
    void registerWatcher(NodesWatcher *watcher);
    void unregisterWatcher(NodesWatcher *watcher);

    void accept(NodesVisitor *visitor);

    bool isEnabled() const { return true; }

protected:
    // this is just the in-memory representation, a subclass
    // will add the persistent stuff
    explicit ProjectNode(const QString &projectFilePath);

    // to be called in implementation of
    // the corresponding public methods
    void addProjectNodes(const QList<ProjectNode*> &subProjects);
    void removeProjectNodes(const QList<ProjectNode*> &subProjects);

    void addFolderNodes(const QList<FolderNode*> &subFolders, FolderNode *parentFolder);
    void removeFolderNodes(const QList<FolderNode*> &subFolders, FolderNode *parentFolder);

    void addFileNodes(const QList<FileNode*> &files, FolderNode *parentFolder);
    void removeFileNodes(const QList<FileNode*> &files, FolderNode *parentFolder);

private slots:
    void watcherDestroyed(QObject *watcher);

private:
    QList<ProjectNode*> m_subProjectNodes;
    QList<NodesWatcher*> m_watchers;

    // let SessionNode call setParentFolderNode
    friend class SessionNode;
};

// Documentation inside.
class PROJECTEXPLORER_EXPORT SessionNode : public FolderNode {
    Q_OBJECT
    friend class SessionManager;
public:
    SessionNode(QObject *parentObject);

    QList<ProjectNode*> projectNodes() const;

    QList<NodesWatcher*> watchers() const;
    void registerWatcher(NodesWatcher *watcher);
    void unregisterWatcher(NodesWatcher *watcher);

    void accept(NodesVisitor *visitor);

    bool isEnabled() const { return true; }

protected:
    void addProjectNodes(const QList<ProjectNode*> &projectNodes);
    void removeProjectNodes(const QList<ProjectNode*> &projectNodes);

private slots:
    void watcherDestroyed(QObject *watcher);

private:
    QList<ProjectNode*> m_projectNodes;
    QList<NodesWatcher*> m_watchers;
};

// Documentation inside.
class PROJECTEXPLORER_EXPORT NodesWatcher : public QObject {
    Q_OBJECT
public:
    explicit NodesWatcher(QObject *parent = 0);

signals:
    // everything

    // Emited whenever the model needs to send a update signal.
    void nodeUpdated(ProjectExplorer::Node *node);

    // projects
    void aboutToChangeHasBuildTargets(ProjectExplorer::ProjectNode*);
    void hasBuildTargetsChanged(ProjectExplorer::ProjectNode *node);

    // folders & projects
    void foldersAboutToBeAdded(FolderNode *parentFolder,
                               const QList<FolderNode*> &newFolders);
    void foldersAdded();

    void foldersAboutToBeRemoved(FolderNode *parentFolder,
                               const QList<FolderNode*> &staleFolders);
    void foldersRemoved();

    // files
    void filesAboutToBeAdded(FolderNode *folder,
                               const QList<FileNode*> &newFiles);
    void filesAdded();

    void filesAboutToBeRemoved(FolderNode *folder,
                               const QList<FileNode*> &staleFiles);
    void filesRemoved();
    void nodeSortKeyAboutToChange(Node *node);
    void nodeSortKeyChanged();

private:

    // let project & session emit signals
    friend class ProjectNode;
    friend class SessionNode;
    friend class Node;
};

} // namespace ProjectExplorer

// HACK: THERE SHOULD BE ONE PLACE TO MAKE THE FILE ENDING->FILE TYPE ASSOCIATION
ProjectExplorer::FileType typeForFileName(const Core::MimeDatabase *db, const QFileInfo &file);

#endif // PROJECTNODES_H
