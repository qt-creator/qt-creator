/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef QT4NODES_H
#define QT4NODES_H

#include "qt4buildconfiguration.h"

#include <coreplugin/ifile.h>
#include <projectexplorer/projectnodes.h>
#include <projectexplorer/project.h>
#include <projectexplorer/runconfiguration.h>

#include <QtCore/QHash>
#include <QtCore/QStringList>
#include <QtCore/QDateTime>
#include <QtCore/QMap>
#include <QtCore/QFutureWatcher>
#include <QtCore/QFileSystemWatcher>

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
class Qt4UiCodeModelSupport;

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
    UiDirVar,
    MocDirVar,
    PkgConfigVar,
    PrecompiledHeaderVar,
    LibDirectoriesVar,
    ConfigVar,
    QmlImportPathVar,
    Makefile,
    SymbianCapabilities,
    Deployment
};

class Qt4PriFileNode;
class Qt4ProFileNode;

class Qt4PriFile : public Core::IFile
{
    Q_OBJECT
public:
    Qt4PriFile(Qt4PriFileNode *qt4PriFile);
    virtual bool save(const QString &fileName = QString());
    virtual QString fileName() const;
    virtual void rename(const QString &newName);

    virtual QString defaultPath() const;
    virtual QString suggestedFileName() const;
    virtual QString mimeType() const;

    virtual bool isModified() const;
    virtual bool isReadOnly() const;
    virtual bool isSaveAsAllowed() const;

    ReloadBehavior reloadBehavior(ChangeTrigger state, ChangeType type) const;
    void reload(ReloadFlag flag, ChangeType type);

private:
    Qt4PriFileNode *m_priFile;
};

// Implements ProjectNode for qt4 pro files
class Qt4PriFileNode : public ProjectExplorer::ProjectNode
{
    Q_OBJECT
    Q_DISABLE_COPY(Qt4PriFileNode)
public:
    Qt4PriFileNode(Qt4Project *project, Qt4ProFileNode* qt4ProFileNode, const QString &filePath);

    void update(ProFile *includeFileExact, ProFileReader *readerExact, ProFile *includeFileCumlative, ProFileReader *readerCumalative);


// ProjectNode interface
    QList<ProjectAction> supportedActions(Node *node) const;

    bool hasBuildTargets() const { return false; }

    bool canAddSubProject(const QString &proFilePath) const;

    bool addSubProjects(const QStringList &proFilePaths);
    bool removeSubProjects(const QStringList &proFilePaths);

    bool addFiles(const FileType fileType, const QStringList &filePaths,
                  QStringList *notAdded = 0);
    bool removeFiles(const FileType fileType, const QStringList &filePaths,
                     QStringList *notRemoved = 0);
    bool deleteFiles(const FileType fileType,
                     const QStringList &filePaths);
    bool renameFile(const FileType fileType,
                    const QString &filePath, const QString &newFilePath);

    void folderChanged(const QString &changedFolder);

    bool deploysFolder(const QString &folder) const;
    QList<ProjectExplorer::RunConfiguration *> runConfigurationsFor(Node *node);

protected:
    void clear();
    static QStringList varNames(FileType type);
    static QStringList dynamicVarNames(ProFileReader *readerExact, ProFileReader *readerCumulative);
    static QSet<QString> filterFiles(ProjectExplorer::FileType fileType, const QSet<QString> &files);

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

private slots:
    void scheduleUpdate();

private:
    void save(const QStringList &lines);
    bool priFileWritable(const QString &path);
    bool saveModifiedEditors();
    QStringList formResources(const QString &formFile) const;
    QStringList baseVPaths(ProFileReader *reader, const QString &projectDir);
    QStringList fullVPaths(const QStringList &baseVPaths, ProFileReader *reader, FileType type, const QString &qmakeVariable, const QString &projectDir);
    void watchFolders(const QSet<QString> &folders);

    Qt4Project *m_project;
    Qt4ProFileNode *m_qt4ProFileNode;
    QString m_projectFilePath;
    QString m_projectDir;

    QMap<QString, Qt4UiCodeModelSupport *> m_uiCodeModelSupport;
    Qt4PriFile *m_qt4PriFile;

    // Memory is cheap...
    // TODO (really that cheap?)
    QMap<ProjectExplorer::FileType, QSet<QString> > m_files;
    QSet<QString> m_recursiveEnumerateFiles;
    QSet<QString> m_watchedFolders;

    // managed by Qt4ProFileNode
    friend class Qt4ProFileNode;
    friend class Qt4PriFile; // for scheduling updates on modified
    // internal temporary subtree representation
    friend struct InternalNode;
};

struct TargetInformation
{
    bool valid;
    QString workingDir;
    QString target;
    QString executable;
    QString buildDir;
    bool operator==(const TargetInformation &other) const
    {
        return workingDir == other.workingDir
                && target == other.target
                && executable == other.executable
                && valid == valid
                && buildDir == buildDir;
    }
    bool operator!=(const TargetInformation &other) const
    {
        return !(*this == other);
    }

    TargetInformation()
        : valid(false)
    {}

    TargetInformation(const TargetInformation &other)
        : valid(other.valid),
          workingDir(other.workingDir),
          target(other.target),
          executable(other.executable),
          buildDir(other.buildDir)
    {
    }

};

struct InstallsItem {
    InstallsItem(QString p, QStringList f) : path(p), files(f) {}
    QString path;
    QStringList files;
};

struct InstallsList {
    void clear() { targetPath.clear(); items.clear(); }
    QString targetPath;
    QList<InstallsItem> items;
};

// Implements ProjectNode for qt4 pro files
class Qt4ProFileNode : public Qt4PriFileNode
{
    Q_OBJECT
    Q_DISABLE_COPY(Qt4ProFileNode)
public:
    Qt4ProFileNode(Qt4Project *project,
                   const QString &filePath,
                   QObject *parent = 0);
    ~Qt4ProFileNode();

    bool isParent(Qt4ProFileNode *node);

    bool hasBuildTargets() const;

    Qt4ProjectType projectType() const;

    QStringList variableValue(const Qt4Variable var) const;

    void updateCodeModelSupportFromBuild(const QStringList &files);
    void updateCodeModelSupportFromEditor(const QString &uiFileName, const QString &contents);

    QString buildDir(Qt4BuildConfiguration *bc = 0) const;

    QString uiDirectory() const;
    static QString uiHeaderFile(const QString &uiDir, const QString &formFile);

    const Qt4ProFileNode *findProFileFor(const QString &string) const;
    TargetInformation targetInformation(const QString &fileName) const;
    TargetInformation targetInformation() const;

    InstallsList installsList() const;

    QString makefile() const;
    QStringList symbianCapabilities() const;

    void update();
    void scheduleUpdate();

    void emitProFileInvalidated();
    void emitProFileUpdated();

    bool validParse() const;

    bool hasBuildTargets(Qt4ProjectType projectType) const;

public slots:
    void asyncUpdate();

private slots:
    void buildStateChanged(ProjectExplorer::Project*);
    void applyAsyncEvaluate();

private:
    void setupReader();
    enum EvalResult { EvalFail, EvalPartial, EvalOk };
    EvalResult evaluate();
    void applyEvaluate(EvalResult parseResult, bool async);

    void asyncEvaluate(QFutureInterface<EvalResult> &fi);

    typedef QHash<Qt4Variable, QStringList> Qt4VariablesHash;

    void createUiCodeModelSupport();
    QStringList updateUiFiles();

    QString uiDirPath(ProFileReader *reader) const;
    QString mocDirPath(ProFileReader *reader) const;
    QStringList includePaths(ProFileReader *reader) const;
    QStringList libDirectories(ProFileReader *reader) const;
    QStringList subDirsPaths(ProFileReader *reader) const;
    TargetInformation targetInformation(ProFileReader *reader) const;
    void setupInstallsList(const ProFileReader *reader);

    void invalidate();

    Qt4ProjectType m_projectType;
    Qt4VariablesHash m_varValues;

    QMap<QString, QDateTime> m_uitimestamps;
    TargetInformation m_qt4targetInformation;
    InstallsList m_installsList;
    friend class Qt4NodeHierarchy;

    bool m_validParse;

    // Async stuff
    QFutureWatcher<EvalResult> m_parseFutureWatcher;
    ProFileReader *m_readerExact;
    ProFileReader *m_readerCumulative;
};

class Qt4NodesWatcher : public ProjectExplorer::NodesWatcher
{
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

    void proFileUpdated(Qt4ProjectManager::Internal::Qt4ProFileNode *projectNode, bool success);
    void proFileInvalidated(Qt4ProjectManager::Internal::Qt4ProFileNode *projectNode);

private:
    // let them emit signals
    friend class Qt4ProFileNode;
    friend class Qt4PriFileNode;
};

} // namespace Internal
} // namespace Qt4ProjectManager

#endif // QT4NODES_H
