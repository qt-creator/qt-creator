/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.2, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/

#ifndef QT4NODES_H
#define QT4NODES_H

#include <projectexplorer/projectnodes.h>

#include <QtCore/QHash>
#include <QtCore/QStringList>
#include <QtCore/QTimer>

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

class ProFileCache;
class ProFileReader;
class DirectoryWatcher;

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
    Qt4PriFileNode(Qt4Project *project,
                   const QString &filePath);

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

private slots:
    void save();

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
    
private:
    bool priFileWritable(const QString &path);
    bool saveModifiedEditors(const QString &path);

    Core::ICore *m_core;
    Qt4Project *m_project;
    QString m_projectFilePath;
    QString m_projectDir;
    ProFile *m_includeFile;
    QTimer *m_saveTimer;

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

    bool hasTargets() const;

    Qt4ProjectType projectType() const;

    QStringList variableValue(const Qt4Variable var) const;

public slots:
    void update();

private slots:
    void fileChanged(const QString &filePath);

private:
    void updateGeneratedFiles();

    ProFileReader *createProFileReader() const;
    Qt4ProFileNode *createSubProFileNode(const QString &path);

    QStringList uiDirPaths(ProFileReader *reader) const;
    QStringList mocDirPaths(ProFileReader *reader) const;
    QStringList includePaths(ProFileReader *reader) const;
    QStringList subDirsPaths(ProFileReader *reader) const;
    QStringList qBuildSubDirsPaths(const QString &scanDir)  const;

    QString buildDir() const;

    void invalidate();

    Qt4ProjectType m_projectType;
    QHash<Qt4Variable, QStringList> m_varValues;
    bool m_isQBuildProject;

    ProFileCache *m_cache;
    DirectoryWatcher *m_dirWatcher;

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
