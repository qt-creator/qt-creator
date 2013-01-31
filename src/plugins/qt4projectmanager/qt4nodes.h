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

#ifndef QT4NODES_H
#define QT4NODES_H

#include "qt4projectmanager_global.h"

#include <utils/fileutils.h>
#include <coreplugin/idocument.h>
#include <projectexplorer/projectnodes.h>

#include <QHash>
#include <QStringList>
#include <QDateTime>
#include <QMap>
#include <QFutureWatcher>

// defined in proitems.h
QT_BEGIN_NAMESPACE
class ProFile;
QT_END_NAMESPACE

namespace Core {
class ICore;
}

namespace QtSupport {
class BaseQtVersion;
class ProFileReader;
}

namespace ProjectExplorer {
class RunConfiguration;
class Project;
}

namespace Qt4ProjectManager {
class Qt4BuildConfiguration;
class Qt4ProFileNode;
class Qt4Project;

//  Type of projects
enum Qt4ProjectType {
    InvalidProject = 0,
    ApplicationTemplate,
    LibraryTemplate,
    ScriptTemplate,
    AuxTemplate,
    SubDirsTemplate
};

// Other variables of interest
enum Qt4Variable {
    DefinesVar = 1,
    IncludePathVar,
    CppFlagsVar,
    CppHeaderVar,
    CppSourceVar,
    ObjCSourceVar,
    UiDirVar,
    MocDirVar,
    PkgConfigVar,
    PrecompiledHeaderVar,
    LibDirectoriesVar,
    ConfigVar,
    QtVar,
    QmlImportPathVar,
    Makefile,
    ObjectExt,
    ObjectsDir,
    VersionVar,
    TargetVersionExtVar,
    StaticLibExtensionVar,
    ShLibExtensionVar
};

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

using ProjectExplorer::FileType;

namespace Internal {
class Qt4UiCodeModelSupport;
class Qt4PriFile;
struct InternalNode;
}

// Implements ProjectNode for qt4 pro files
class QT4PROJECTMANAGER_EXPORT Qt4PriFileNode : public ProjectExplorer::ProjectNode
{
    Q_OBJECT

public:
    Qt4PriFileNode(Qt4Project *project, Qt4ProFileNode* qt4ProFileNode, const QString &filePath);
    ~Qt4PriFileNode();

    void update(ProFile *includeFileExact, QtSupport::ProFileReader *readerExact, ProFile *includeFileCumlative, QtSupport::ProFileReader *readerCumalative);


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

    QList<Qt4PriFileNode*> subProjectNodesExact() const;

    // Set by parent
    bool includedInExactParse() const;

protected:
    void setIncludedInExactParse(bool b);
    static QStringList varNames(FileType type);
    static QStringList dynamicVarNames(QtSupport::ProFileReader *readerExact, QtSupport::ProFileReader *readerCumulative, QtSupport::BaseQtVersion *qtVersion);
    static QSet<Utils::FileName> filterFilesProVariables(ProjectExplorer::FileType fileType, const QSet<Utils::FileName> &files);
    static QSet<Utils::FileName> filterFilesRecursiveEnumerata(ProjectExplorer::FileType fileType, const QSet<Utils::FileName> &files);

    enum ChangeType {
        AddToProFile,
        RemoveFromProFile
    };

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
    QStringList baseVPaths(QtSupport::ProFileReader *reader, const QString &projectDir, const QString &buildDir) const;
    QStringList fullVPaths(const QStringList &baseVPaths, QtSupport::ProFileReader *reader, const QString &qmakeVariable, const QString &projectDir) const;
    void watchFolders(const QSet<QString> &folders);

    Qt4Project *m_project;
    Qt4ProFileNode *m_qt4ProFileNode;
    QString m_projectFilePath;
    QString m_projectDir;

    QMap<QString, Internal::Qt4UiCodeModelSupport *> m_uiCodeModelSupport;
    Internal::Qt4PriFile *m_qt4PriFile;

    // Memory is cheap...
    QMap<ProjectExplorer::FileType, QSet<Utils::FileName> > m_files;
    QSet<Utils::FileName> m_recursiveEnumerateFiles;
    QSet<QString> m_watchedFolders;
    bool m_includedInExactParse;

    // managed by Qt4ProFileNode
    friend class Qt4ProjectManager::Qt4ProFileNode;
    friend class Internal::Qt4PriFile; // for scheduling updates on modified
    // internal temporary subtree representation
    friend struct Internal::InternalNode;
};

namespace Internal {
class Qt4PriFile : public Core::IDocument
{
    Q_OBJECT
public:
    Qt4PriFile(Qt4PriFileNode *qt4PriFile);
    virtual bool save(QString *errorString, const QString &fileName, bool autoSave);
    virtual QString fileName() const;
    virtual void rename(const QString &newName);

    virtual QString defaultPath() const;
    virtual QString suggestedFileName() const;
    virtual QString mimeType() const;

    virtual bool isModified() const;
    virtual bool isSaveAsAllowed() const;

    ReloadBehavior reloadBehavior(ChangeTrigger state, ChangeType type) const;
    bool reload(QString *errorString, ReloadFlag flag, ChangeType type);

private:
    Qt4PriFileNode *m_priFile;
};

class Qt4NodesWatcher : public ProjectExplorer::NodesWatcher
{
    Q_OBJECT

public:
    Qt4NodesWatcher(QObject *parent = 0);

signals:
    void projectTypeChanged(Qt4ProjectManager::Qt4ProFileNode *projectNode,
                            const Qt4ProjectManager::Qt4ProjectType oldType,
                            const Qt4ProjectManager::Qt4ProjectType newType);

    void variablesChanged(Qt4ProFileNode *projectNode,
                          const QHash<Qt4Variable, QStringList> &oldValues,
                          const QHash<Qt4Variable, QStringList> &newValues);

    void proFileUpdated(Qt4ProjectManager::Qt4ProFileNode *projectNode, bool success, bool parseInProgress);

private:
    // let them emit signals
    friend class Qt4ProjectManager::Qt4ProFileNode;
    friend class Qt4PriFileNode;
};

class ProVirtualFolderNode : public ProjectExplorer::VirtualFolderNode
{
public:
    explicit ProVirtualFolderNode(const QString &folderPath, int priority, const QString &typeName)
        : VirtualFolderNode(folderPath, priority), m_typeName(typeName)
    {

    }

    QString displayName() const
    {
        return m_typeName;
    }

    QString tooltip() const
    {
        return QString();
    }

private:
    QString m_typeName;
};

} // namespace Internal

struct QT4PROJECTMANAGER_EXPORT TargetInformation
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
                && valid == other.valid
                && buildDir == other.buildDir;
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

struct QT4PROJECTMANAGER_EXPORT InstallsItem {
    InstallsItem(QString p, QStringList f) : path(p), files(f) {}
    QString path;
    QStringList files;
};

struct QT4PROJECTMANAGER_EXPORT InstallsList {
    void clear() { targetPath.clear(); items.clear(); }
    QString targetPath;
    QList<InstallsItem> items;
};

struct QT4PROJECTMANAGER_EXPORT ProjectVersion {
    int major;
    int minor;
    int patch;
};

// Implements ProjectNode for qt4 pro files
class QT4PROJECTMANAGER_EXPORT Qt4ProFileNode : public Qt4PriFileNode
{
    Q_OBJECT

public:
    Qt4ProFileNode(Qt4Project *project,
                   const QString &filePath,
                   QObject *parent = 0);
    ~Qt4ProFileNode();

    bool isParent(Qt4ProFileNode *node);

    bool hasBuildTargets() const;

    Qt4ProjectType projectType() const;

    QStringList variableValue(const Qt4Variable var) const;
    QString singleVariableValue(const Qt4Variable var) const;

    bool isSubProjectDeployable(const QString &filePath) const {
        return !m_subProjectsNotToDeploy.contains(filePath);
    }

    void updateCodeModelSupportFromBuild();
    void updateCodeModelSupportFromEditor(const QString &uiFileName, const QString &contents);

    QString sourceDir() const;
    QString buildDir(Qt4BuildConfiguration *bc = 0) const;

    QString uiDirectory() const;
    static QString uiHeaderFile(const QString &uiDir, const QString &formFile);
    QStringList uiFiles() const;

    const Qt4ProFileNode *findProFileFor(const QString &string) const;
    TargetInformation targetInformation(const QString &fileName) const;
    TargetInformation targetInformation() const;

    InstallsList installsList() const;

    QString makefile() const;
    QString objectExtension() const;
    QString objectsDirectory() const;
    QByteArray cxxDefines() const;
    bool isDeployable() const;
    QString resolvedMkspecPath() const;

    void update();
    void scheduleUpdate();

    bool validParse() const;
    bool parseInProgress() const;

    bool hasBuildTargets(Qt4ProjectType projectType) const;
    bool isDebugAndRelease() const;

    void setParseInProgress(bool b);
    void setParseInProgressRecursive(bool b);
    void setValidParse(bool b);
    void setValidParseRecursive(bool b);
    void emitProFileUpdatedRecursive();
public slots:
    void asyncUpdate();

private slots:
    void buildStateChanged(ProjectExplorer::Project*);
    void applyAsyncEvaluate();

private:
    void setupReader();
    enum EvalResult { EvalAbort, EvalFail, EvalPartial, EvalOk };
    EvalResult evaluate();
    void applyEvaluate(EvalResult parseResult, bool async);

    void asyncEvaluate(QFutureInterface<EvalResult> &fi);

    typedef QHash<Qt4Variable, QStringList> Qt4VariablesHash;

    void createUiCodeModelSupport();

    QStringList fileListForVar(QtSupport::ProFileReader *readerExact, QtSupport::ProFileReader *readerCumulative,
                               const QString &varName, const QString &projectDir, const QString &buildDir) const;
    QString uiDirPath(QtSupport::ProFileReader *reader) const;
    QString mocDirPath(QtSupport::ProFileReader *reader) const;
    QStringList includePaths(QtSupport::ProFileReader *reader) const;
    QStringList libDirectories(QtSupport::ProFileReader *reader) const;
    QStringList subDirsPaths(QtSupport::ProFileReader *reader, QStringList *subProjectsNotToDeploy = 0) const;

    TargetInformation targetInformation(QtSupport::ProFileReader *reader) const;
    void setupInstallsList(const QtSupport::ProFileReader *reader);

    Qt4ProjectType m_projectType;
    Qt4VariablesHash m_varValues;
    bool m_isDeployable;

    QMap<QString, QDateTime> m_uitimestamps;
    TargetInformation m_qt4targetInformation;
    QString m_resolvedMkspecPath;
    QStringList m_subProjectsNotToDeploy;
    InstallsList m_installsList;
    friend class Qt4NodeHierarchy;

    bool m_validParse;
    bool m_parseInProgress;

    QStringList m_uiHeaderFiles;

    // Async stuff
    QFutureWatcher<EvalResult> m_parseFutureWatcher;
    QtSupport::ProFileReader *m_readerExact;
    QtSupport::ProFileReader *m_readerCumulative;
};

} // namespace Qt4ProjectManager

#endif // QT4NODES_H
