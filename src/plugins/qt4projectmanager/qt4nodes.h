/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef QT4NODES_H
#define QT4NODES_H

#include <coreplugin/ifile.h>
#include <projectexplorer/projectnodes.h>
#include <projectexplorer/project.h>

#include <QtCore/QHash>
#include <QtCore/QStringList>
#include <QtCore/QDateTime>
#include <QtCore/QMap>
#include <QtCore/QFutureWatcher>

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
    PrecompiledHeaderVar
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

    virtual QString defaultPath() const;
    virtual QString suggestedFileName() const;
    virtual QString mimeType() const;

    virtual bool isModified() const;
    virtual bool isReadOnly() const;
    virtual bool isSaveAsAllowed() const;

    virtual void modified(Core::IFile::ReloadBehavior *behavior);

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
    QList<ProjectAction> supportedActions() const;

    bool hasBuildTargets() const { return false; }

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

private slots:
    void scheduleUpdate();

private:
    void save(const QStringList &lines);
    bool priFileWritable(const QString &path);
    bool saveModifiedEditors();
    QStringList baseVPaths(ProFileReader *reader, const QString &projectDir);
    QStringList fullVPaths(const QStringList &baseVPaths, ProFileReader *reader, FileType type, const QString &qmakeVariable, const QString &projectDir);

    Qt4Project *m_project;
    Qt4ProFileNode *m_qt4ProFileNode;
    QString m_projectFilePath;
    QString m_projectDir;

    QMap<QString, Qt4UiCodeModelSupport *> m_uiCodeModelSupport;
    Qt4PriFile *m_qt4PriFile;

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
    bool operator==(const TargetInformation &other) const
    {
        return workingDir == other.workingDir
                && target == other.target
                && executable == other.executable
                && valid == valid;
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
          executable(other.executable)
    {
    }

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

    QString buildDir() const;

    QString uiDirectory() const;
    static QString uiHeaderFile(const QString &uiDir, const QString &formFile);

    Qt4ProFileNode *findProFileFor(const QString &string);
    TargetInformation targetInformation(const QString &fileName);
    TargetInformation targetInformation();

    void update();
    void scheduleUpdate();

public slots:
    void asyncUpdate();

private slots:
    void buildStateChanged(ProjectExplorer::Project*);
    void applyAsyncEvaluate();

private:
    void setupReader();
    bool evaluate();
    void applyEvaluate(bool parseResult, bool async);

    void asyncEvaluate(QFutureInterface<bool> &fi);

    typedef QHash<Qt4Variable, QStringList> Qt4VariablesHash;

    void createUiCodeModelSupport();
    QStringList updateUiFiles();

    QStringList uiDirPaths(ProFileReader *reader) const;
    QStringList mocDirPaths(ProFileReader *reader) const;
    QStringList includePaths(ProFileReader *reader) const;
    QStringList subDirsPaths(ProFileReader *reader) const;
    TargetInformation targetInformation(ProFileReader *reader) const;

    void invalidate();

    Qt4ProjectType m_projectType;
    Qt4VariablesHash m_varValues;

    QMap<QString, QDateTime> m_uitimestamps;
    TargetInformation m_qt4targetInformation;
    friend class Qt4NodeHierarchy;

    // Async stuff
    QFutureWatcher<bool> m_parseFutureWatcher;
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

    void proFileUpdated(Qt4ProjectManager::Internal::Qt4ProFileNode *projectNode);

private:
    // let them emit signals
    friend class Qt4ProFileNode;
    friend class Qt4PriFileNode;
};

} // namespace Internal
} // namespace Qt4ProjectManager

#endif // QT4NODES_H
