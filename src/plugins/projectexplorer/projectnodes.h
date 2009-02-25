/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#ifndef PROJECTNODES_H
#define PROJECTNODES_H

#include <QtCore/QObject>
#include <QtCore/QStringList>
#include <QtGui/QIcon>

#include "projectexplorer_export.h"

QT_BEGIN_NAMESPACE
class QFileInfo;
QT_END_NAMESPACE


namespace Core {
    class MimeDatabase;
}

namespace ProjectExplorer {

//
// = Node hierarchy =
//
//  The nodes are arranged in a tree where leaves are FileNodes and non-leaves are FolderNodes
//  A Project is a special Folder that manages the files and normal folders underneath it.
//
//  The Watcher emits signals for structural changes in the hierarchy.
//  A Visitor can be used to traverse all Projects and other Folders.
//

enum NodeType {
    FileNodeType = 1,
    FolderNodeType,
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

class PROJECTEXPLORER_EXPORT Node : public QObject {
    Q_OBJECT
public:
    NodeType nodeType() const;
    ProjectNode *projectNode() const;     // managing project
    FolderNode *parentFolderNode() const; // parent folder or project
    QString path() const;                 // file system path

protected:
    Node(NodeType nodeType, const QString &path);

    void setNodeType(NodeType type);
    void setProjectNode(ProjectNode *project);
    void setParentFolderNode(FolderNode *parentFolder);
    void setPath(const QString &path);

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

class PROJECTEXPLORER_EXPORT FolderNode : public Node {
    Q_OBJECT
public:
    explicit FolderNode(const QString &folderPath);
    virtual ~FolderNode();

    QString name() const;
    QIcon icon() const;

    QList<FileNode*> fileNodes() const;
    QList<FolderNode*> subFolderNodes() const;

    virtual void accept(NodesVisitor *visitor);

protected:
    void setFolderName(const QString &name);
    void setIcon(const QIcon &icon);

    QList<FolderNode*> m_subFolderNodes;
    QList<FileNode*> m_fileNodes;

private:
    // managed by ProjectNode
    friend class ProjectNode;
    QString m_folderName;
    QIcon m_icon;
};

class PROJECTEXPLORER_EXPORT ProjectNode : public FolderNode
{
    Q_OBJECT

public:
    enum ProjectAction {
        AddSubProject,
        RemoveSubProject,
        AddFile,
        RemoveFile
    };

    // all subFolders that are projects
    QList<ProjectNode*> subProjectNodes() const;

    // determines if the project will be shown in the flat view
    // TODO find a better name
    virtual bool hasTargets() const = 0;

    virtual QList<ProjectAction> supportedActions() const = 0;

    virtual bool addSubProjects(const QStringList &proFilePaths) = 0;

    virtual bool removeSubProjects(const QStringList &proFilePaths) = 0;

    virtual bool addFiles(const FileType fileType,
                          const QStringList &filePaths,
                          QStringList *notAdded = 0) = 0;
    // TODO: Maybe remove fileType, can be detected by project
    virtual bool removeFiles(const FileType fileType,
                             const QStringList &filePaths,
                             QStringList *notRemoved = 0) = 0;
    virtual bool renameFile(const FileType fileType,
                             const QString &filePath,
                             const QString &newFilePath) = 0;

    QList<NodesWatcher*> watchers() const;
    void registerWatcher(NodesWatcher *watcher);
    void unregisterWatcher(NodesWatcher *watcher);

    void accept(NodesVisitor *visitor);

    static bool sortNodesByPath(Node *n1, Node *n2);

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

class PROJECTEXPLORER_EXPORT SessionNode : public FolderNode {
    Q_OBJECT
public:
    SessionNode(const QString &sessionFilePath, QObject *parentObject);

    QList<ProjectNode*> projectNodes() const;

    QList<NodesWatcher*> watchers() const;
    void registerWatcher(NodesWatcher *watcher);
    void unregisterWatcher(NodesWatcher *watcher);

    void accept(NodesVisitor *visitor);

protected:
    void addProjectNodes(const QList<ProjectNode*> &projectNodes);
    void removeProjectNodes(const QList<ProjectNode*> &projectNodes);

private slots:
    void watcherDestroyed(QObject *watcher);

private:
    QList<ProjectNode*> m_projectNodes;
    QList<NodesWatcher*> m_watchers;
};

class PROJECTEXPLORER_EXPORT NodesWatcher : public QObject {
    Q_OBJECT
public:
    explicit NodesWatcher(QObject *parent = 0);

signals:
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

private:

    // let project & session emit signals
    friend class ProjectNode;
    friend class SessionNode;
};


} // namespace ProjectExplorer

// HACK: THERE SHOULD BE ONE PLACE TO MAKE THE FILE ENDING->FILE TYPE ASSOCIATION
ProjectExplorer::FileType typeForFileName(const Core::MimeDatabase *db, const QFileInfo &file);

#endif // PROJECTNODES_H
