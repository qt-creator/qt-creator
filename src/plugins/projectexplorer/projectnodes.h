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

#ifndef PROJECTNODES_H
#define PROJECTNODES_H

#include "projectexplorer_export.h"

#include <QIcon>

#include <QObject>
#include <QStringList>
#include <QDebug>

QT_BEGIN_NAMESPACE
class QFileInfo;
QT_END_NAMESPACE

namespace Core { class MimeDatabase; }

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
    // hides actions that use the path(): Open containing folder, open terminal here and Find in Directory
    HidePathActions,
    HideFileActions,
    HideFolderActions,
    HasSubProjectRunConfigurations
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
    int line() const;
    virtual QString displayName() const;
    virtual QString tooltip() const;
    virtual bool isEnabled() const;

    virtual QList<ProjectAction> supportedActions(Node *node) const;

    void setPath(const QString &path);
    void setLine(int line);
    void setPathAndLine(const QString &path, int line);
    void emitNodeUpdated();

protected:
    Node(NodeType nodeType, const QString &path, int line = -1);

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
    int m_line;
};

class PROJECTEXPLORER_EXPORT FileNode : public Node {
    Q_OBJECT
public:
    FileNode(const QString &filePath, const FileType fileType, bool generated, int line = -1);

    FileType fileType() const;
    bool isGenerated() const;

private:
    // managed by ProjectNode
    friend class FolderNode;
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

    virtual bool addFiles(const QStringList &filePaths, QStringList *notAdded = 0);
    virtual bool removeFiles(const QStringList &filePaths, QStringList *notRemoved = 0);
    virtual bool deleteFiles(const QStringList &filePaths);
    virtual bool renameFile(const QString &filePath, const QString &newFilePath);

    class AddNewInformation
    {
    public:
        AddNewInformation(const QString &name, int p)
            :displayName(name), priority(p)
        {}
        QString displayName;
        int priority;
    };

    virtual AddNewInformation addNewInformation(const QStringList &files, Node *context) const;

    void addFileNodes(const QList<FileNode*> &files);
    void removeFileNodes(const QList<FileNode*> &files);

    void addFolderNodes(const QList<FolderNode*> &subFolders);
    void removeFolderNodes(const QList<FolderNode*> &subFolders);

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
    QString vcsTopic() const;

    // all subFolders that are projects
    QList<ProjectNode*> subProjectNodes() const;

    // determines if the project will be shown in the flat view
    // TODO find a better name
    void aboutToChangeHasBuildTargets();
    void hasBuildTargetsChanged();
    virtual bool hasBuildTargets() const = 0;

    virtual bool canAddSubProject(const QString &proFilePath) const = 0;

    virtual bool addSubProjects(const QStringList &proFilePaths) = 0;

    virtual bool removeSubProjects(const QStringList &proFilePaths) = 0;

    // by default returns false
    virtual bool deploysFolder(const QString &folder) const;

    // TODO node parameter not really needed
    virtual QList<ProjectExplorer::RunConfiguration *> runConfigurationsFor(Node *node) = 0;


    QList<NodesWatcher*> watchers() const;
    void registerWatcher(NodesWatcher *watcher);
    void unregisterWatcher(NodesWatcher *watcher);

    void accept(NodesVisitor *visitor);

    bool isEnabled() const { return true; }

    // to be called in implementation of
    // the corresponding public functions
    void addProjectNodes(const QList<ProjectNode*> &subProjects);
    void removeProjectNodes(const QList<ProjectNode*> &subProjects);

protected:
    // this is just the in-memory representation, a subclass
    // will add the persistent stuff
    explicit ProjectNode(const QString &projectFilePath);

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

    QList<ProjectAction> supportedActions(Node *node) const;

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

    // Emitted whenever the model needs to send a update signal.
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
    friend class FolderNode;
    friend class SessionNode;
    friend class Node;
};

template<class T1, class T3>
bool isSorted(const T1 &list, T3 sorter)
{
    typename T1::const_iterator it, iit, end;
    end = list.constEnd();
    it = list.constBegin();
    if (it == end)
        return true;

    iit = list.constBegin();
    ++iit;

    while (iit != end) {
        if (!sorter(*it, *iit))
            return false;
        it = iit++;
    }
    return true;
}

template <class T1, class T2, class T3>
void compareSortedLists(T1 oldList, T2 newList, T1 &removedList, T2 &addedList, T3 sorter)
{
    Q_ASSERT(isSorted(oldList, sorter));
    Q_ASSERT(isSorted(newList, sorter));

    typename T1::const_iterator oldIt, oldEnd;
    typename T2::const_iterator newIt, newEnd;

    oldIt = oldList.constBegin();
    oldEnd = oldList.constEnd();

    newIt = newList.constBegin();
    newEnd = newList.constEnd();

    while (oldIt != oldEnd && newIt != newEnd) {
        if (sorter(*oldIt, *newIt)) {
            removedList.append(*oldIt);
            ++oldIt;
        } else if (sorter(*newIt, *oldIt)) {
            addedList.append(*newIt);
            ++newIt;
        } else {
            ++oldIt;
            ++newIt;
        }
    }

    while (oldIt != oldEnd) {
        removedList.append(*oldIt);
        ++oldIt;
    }

    while (newIt != newEnd) {
        addedList.append(*newIt);
        ++newIt;
    }
}

template <class T1, class T3>
T1 subtractSortedList(T1 list1, T1 list2, T3 sorter)
{
    Q_ASSERT(ProjectExplorer::isSorted(list1, sorter));
    Q_ASSERT(ProjectExplorer::isSorted(list2, sorter));

    typename T1::const_iterator list1It, list1End;
    typename T1::const_iterator list2It, list2End;

    list1It = list1.constBegin();
    list1End = list1.constEnd();

    list2It = list2.constBegin();
    list2End = list2.constEnd();

    T1 result;

    while (list1It != list1End && list2It != list2End) {
        if (sorter(*list1It, *list2It)) {
            result.append(*list1It);
            ++list1It;
        } else if (sorter(*list2It, *list1It)) {
            qWarning() << "subtractSortedList: subtracting value that isn't in set";
        } else {
            ++list1It;
            ++list2It;
        }
    }

    while (list1It != list1End) {
        result.append(*list1It);
        ++list1It;
    }

    return result;
}

} // namespace ProjectExplorer

Q_DECLARE_METATYPE(ProjectExplorer::Node *)

#endif // PROJECTNODES_H
