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

#ifndef QT4NODES_H
#define QT4NODES_H

#include <projectexplorer/projectnodes.h>
#include <projectexplorer/project.h>

#include <QtCore/QHash>
#include <QtCore/QStringList>
#include <QtCore/QTimer>
#include <QtCore/QDateTime>
#include <QtCore/QMap>

// defined in proitems.h
QT_BEGIN_NAMESPACE
class ProFile;
QT_END_NAMESPACE

namespace Core {
class ICore;
}

namespace Qt4ProjectManager {

// Import base classes into namespace
using ProjectExplorer::Node;
using ProjectExplorer::FileNode;
using ProjectExplorer::FolderNode;
using ProjectExplorer::ProjectNode;
using ProjectExplorer::NodesWatcher;

// Import enums into namespace
using ProjectExplorer::NodeType;
using ProjectExplorer::FileNodeType;
using ProjectExplorer::FolderNodeType;
using ProjectExplorer::ProjectNodeType;

using ProjectExplorer::UnknownFileType;
using ProjectExplorer::ProjectFileType;

class Qt4Project;

namespace Internal {

using ProjectExplorer::FileType;

class ProFileReader;
class DirectoryWatcher;
class FileWatcher;

//  Type of projects
enum Qt4ProjectType {
    InvalidProject = 0,
    ApplicationTemplate,
    LibraryTemplate,
    ScriptTemplate,
    SubDirsTemplate
};

// Other variables of interest
enum Qt4Variable {
    DefinesVar = 1,
    IncludePathVar,
    CxxCompilerVar,
    UiDirVar,
    MocDirVar
};

class Qt4PriFileNode;
class Qt4ProFileNode;

// Implements ProjectNode for qt4 pro files
class Qt4PriFileNode : public ProjectExplorer::ProjectNode {
    Q_OBJECT
    Q_DISABLE_COPY(Qt4PriFileNode)
public:
    Qt4PriFileNode(Qt4Project *project, Qt4ProFileNode* qt4ProFileNode, const QString &filePath);

    void update(ProFile *includeFile, ProFileReader *reader);

// ProjectNode interface
    QList<ProjectAction> supportedActions() const;

    bool hasTargets() const { return false; }

    bool addSubProjects(const QStringList &proFilePaths);
    bool removeSubProjects(const QStringList &proFilePaths);

    bool addFiles(const FileType fileType, const QStringList &filePaths,
                  QStringList *notAdded = 0);
    bool removeFiles(const FileType fileType, const QStringList &filePaths,
                     QStringList *notRemoved = 0);
    bool renameFile(const FileType fileType,
                    const QString &filePath, const QString &newFilePath);

protected:
    void clear();
    static QStringList varNames(FileType type);

    enum ChangeType {
        AddToProFile,
        RemoveFromProFile
    };

    bool changeIncludes(ProFile *includeFile,
                       const QStringList &proFilePaths,
                       ChangeType change);

    void changeFiles(const FileType fileType,
                     const QStringList &filePaths,
                     QStringList *notChanged,
                     ChangeType change);

    QString buildDir() const;
    ProFileReader *createProFileReader() const;

private slots:
    void scheduleUpdate();

private:
    void save(ProFile *includeFile);
    bool priFileWritable(const QString &path);
    bool saveModifiedEditors(const QString &path);

    Qt4Project *m_project;
    Qt4ProFileNode *m_qt4ProFileNode;
    QString m_projectFilePath;
    QString m_projectDir;

    // TODO we might be better off using an IFile* and the FileManager for
    // watching changes to the .pro and .pri files on disk
    FileWatcher *m_fileWatcher;

    // managed by Qt4ProFileNode
    friend class Qt4ProFileNode;
};

// Implements ProjectNode for qt4 pro files
class Qt4ProFileNode : public Qt4PriFileNode {
    Q_OBJECT
    Q_DISABLE_COPY(Qt4ProFileNode)
public:
    Qt4ProFileNode(Qt4Project *project,
                   const QString &filePath,
                   QObject *parent = 0);
    ~Qt4ProFileNode();

    bool hasTargets() const;

    Qt4ProjectType projectType() const;

    QStringList variableValue(const Qt4Variable var) const;

public slots:
    void scheduleUpdate();
    void update();
private slots:
    void updateUiFiles(const QString& buildConfiguration);
    void buildStateChanged(ProjectExplorer::Project*);

private:
    Qt4ProFileNode *createSubProFileNode(const QString &path);

    QStringList uiDirPaths(ProFileReader *reader) const;
    QStringList mocDirPaths(ProFileReader *reader) const;
    QStringList includePaths(ProFileReader *reader) const;
    QStringList subDirsPaths(ProFileReader *reader) const;
    QStringList qBuildSubDirsPaths(const QString &scanDir)  const;



    void invalidate();

    Qt4ProjectType m_projectType;
    QHash<Qt4Variable, QStringList> m_varValues;
    bool m_isQBuildProject;
    QTimer m_updateTimer;

    QMap<QString, QDateTime> m_uitimestamps;
    friend class Qt4NodeHierarchy;
};

class Qt4NodesWatcher : public ProjectExplorer::NodesWatcher {
    Q_OBJECT
    Q_DISABLE_COPY(Qt4NodesWatcher)
public:
    Qt4NodesWatcher(QObject *parent = 0);

signals:
    void projectTypeChanged(Qt4ProjectManager::Internal::Qt4ProFileNode *projectNode,
                            const Qt4ProjectManager::Internal::Qt4ProjectType oldType,
                            const Qt4ProjectManager::Internal::Qt4ProjectType newType);

    void variablesChanged(Qt4ProFileNode *projectNode,
                          const QHash<Qt4Variable, QStringList> &oldValues,
                          const QHash<Qt4Variable, QStringList> &newValues);

    void proFileUpdated(Qt4ProjectManager::Internal::Qt4ProFileNode *projectNode);

private:
    // let them emit signals
    friend class Qt4ProFileNode;
    friend class Qt4PriFileNode;
};

} // namespace Internal
} // namespace Qt4ProjectManager

#endif // QT4NODES_H
