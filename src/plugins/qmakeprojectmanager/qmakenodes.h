/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef QMAKENODES_H
#define QMAKENODES_H

#include "qmakeprojectmanager_global.h"
#include "proparser/prowriter.h"

#include <coreplugin/idocument.h>
#include <projectexplorer/projectnodes.h>
#include <qtsupport/uicodemodelsupport.h>

#include <QHash>
#include <QStringList>
#include <QDateTime>
#include <QMap>
#include <QFutureWatcher>

// defined in proitems.h
QT_BEGIN_NAMESPACE
class ProFile;
QT_END_NAMESPACE

namespace Utils { class FileName; }

namespace Core { class ICore; }

namespace QtSupport {
class BaseQtVersion;
class ProFileReader;
}

namespace ProjectExplorer {
class RunConfiguration;
class Project;
}

namespace QmakeProjectManager {
class QmakeBuildConfiguration;
class QmakeProFileNode;
class QmakeProject;

//  Type of projects
enum QmakeProjectType {
    InvalidProject = 0,
    ApplicationTemplate,
    StaticLibraryTemplate,
    SharedLibraryTemplate,
    ScriptTemplate,
    AuxTemplate,
    SubDirsTemplate
};

// Other variables of interest
enum QmakeVariable {
    DefinesVar = 1,
    IncludePathVar,
    CppFlagsVar,
    CppHeaderVar,
    CppSourceVar,
    ObjCSourceVar,
    ObjCHeaderVar,
    ResourceVar,
    ExactResourceVar,
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
    TargetExtVar,
    TargetVersionExtVar,
    StaticLibExtensionVar,
    ShLibExtensionVar,
    AndroidArchVar,
    AndroidDeploySettingsFile,
    AndroidPackageSourceDir,
    AndroidExtraLibs,
    IsoIconsVar
};

namespace Internal {
class QmakePriFile;
struct InternalNode;

class EvalInput;
class EvalResult;
class PriFileEvalResult;
// TOOD can probably move into the .cpp file
class VariableAndVPathInformation
{
public:
    QString variable;
    QStringList vPathsExact;
    QStringList vPathsCumulative;
};

}

// Implements ProjectNode for qmake .pri files
class QMAKEPROJECTMANAGER_EXPORT QmakePriFileNode : public ProjectExplorer::ProjectNode
{
public:
    QmakePriFileNode(QmakeProject *project, QmakeProFileNode *qmakeProFileNode, const Utils::FileName &filePath);
    ~QmakePriFileNode();

    void update(const Internal::PriFileEvalResult &result);


// ProjectNode interface
    QList<ProjectExplorer::ProjectAction> supportedActions(Node *node) const;

    bool showInSimpleTree() const { return false; }

    bool canAddSubProject(const QString &proFilePath) const;

    bool addSubProjects(const QStringList &proFilePaths);
    bool removeSubProjects(const QStringList &proFilePaths);

    bool addFiles(const QStringList &filePaths, QStringList *notAdded = 0);
    bool removeFiles(const QStringList &filePaths, QStringList *notRemoved = 0);
    bool deleteFiles(const QStringList &filePaths);
    bool renameFile(const QString &filePath, const QString &newFilePath);
    AddNewInformation addNewInformation(const QStringList &files, Node *context) const;

    bool setProVariable(const QString &var, const QStringList &values,
                        const QString &scope = QString(),
                        int flags = QmakeProjectManager::Internal::ProWriter::ReplaceValues);

    bool folderChanged(const QString &changedFolder, const QSet<Utils::FileName> &newFiles);

    bool deploysFolder(const QString &folder) const;
    QList<ProjectExplorer::RunConfiguration *> runConfigurations() const;

    QmakeProFileNode *proFileNode() const;
    QList<QmakePriFileNode*> subProjectNodesExact() const;

    // Set by parent
    bool includedInExactParse() const;

    static QSet<Utils::FileName> recursiveEnumerate(const QString &folder);
protected:
    void setIncludedInExactParse(bool b);
    static QStringList varNames(ProjectExplorer::FileType type, QtSupport::ProFileReader *readerExact);
    static QStringList varNamesForRemoving();
    static QString varNameForAdding(const QString &mimeType);
    static QStringList dynamicVarNames(QtSupport::ProFileReader *readerExact, QtSupport::ProFileReader *readerCumulative, bool isQt5);
    static QSet<Utils::FileName> filterFilesProVariables(ProjectExplorer::FileType fileType, const QSet<Utils::FileName> &files);
    static QSet<Utils::FileName> filterFilesRecursiveEnumerata(ProjectExplorer::FileType fileType, const QSet<Utils::FileName> &files);

    enum ChangeType {
        AddToProFile,
        RemoveFromProFile
    };

    void changeFiles(const QString &mimeType,
                     const QStringList &filePaths,
                     QStringList *notChanged,
                     ChangeType change);

private:
    void scheduleUpdate();

    static bool ensureWriteableProFile(const QString &file);
    static QPair<ProFile *, QStringList> readProFile(const QString &file);
    static QPair<ProFile *, QStringList> readProFileFromContents(const QString &contents);
    void save(const QStringList &lines);
    bool priFileWritable(const QString &path);
    bool saveModifiedEditors();
    QStringList formResources(const QString &formFile) const;
    static QStringList baseVPaths(QtSupport::ProFileReader *reader, const QString &projectDir, const QString &buildDir);
    static QStringList fullVPaths(const QStringList &baseVPaths, QtSupport::ProFileReader *reader, const QString &qmakeVariable, const QString &projectDir);
    static Internal::PriFileEvalResult extractValues(const Internal::EvalInput &input, ProFile *includeFileExact, ProFile *includeFileCumlative,
                                                     const QList<QList<Internal::VariableAndVPathInformation>> &variableAndVPathInformation);
    void watchFolders(const QSet<QString> &folders);

    QmakeProject *m_project;
    QmakeProFileNode *m_qmakeProFileNode;
    Utils::FileName m_projectFilePath;
    QString m_projectDir;

    QMap<QString, QtSupport::UiCodeModelSupport *> m_uiCodeModelSupport;
    Internal::QmakePriFile *m_qmakePriFile;

    // Memory is cheap...
    QMap<ProjectExplorer::FileType, QSet<Utils::FileName> > m_files;
    QSet<Utils::FileName> m_recursiveEnumerateFiles;
    QSet<QString> m_watchedFolders;
    bool m_includedInExactParse;

    // managed by QmakeProFileNode
    friend class QmakeProjectManager::QmakeProFileNode;
    friend class Internal::QmakePriFile; // for scheduling updates on modified
    // internal temporary subtree representation
    friend struct Internal::InternalNode;
};

namespace Internal {
class QmakePriFile : public Core::IDocument
{
    Q_OBJECT
public:
    QmakePriFile(QmakePriFileNode *qmakePriFile);
    virtual bool save(QString *errorString, const QString &fileName, bool autoSave);

    virtual QString defaultPath() const;
    virtual QString suggestedFileName() const;

    virtual bool isModified() const;
    virtual bool isSaveAsAllowed() const;

    ReloadBehavior reloadBehavior(ChangeTrigger state, ChangeType type) const;
    bool reload(QString *errorString, ReloadFlag flag, ChangeType type);

private:
    QmakePriFileNode *m_priFile;
};

class ProVirtualFolderNode : public ProjectExplorer::VirtualFolderNode
{
public:
    explicit ProVirtualFolderNode(const Utils::FileName &folderPath, int priority, const QString &typeName)
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

class QMAKEPROJECTMANAGER_EXPORT TargetInformation
{
public:
    bool valid;
    QString target;
    QString destDir;
    QString buildDir;
    QString buildTarget;
    bool operator==(const TargetInformation &other) const
    {
        return target == other.target
                && valid == other.valid
                && destDir == other.destDir
                && buildDir == other.buildDir
                && buildTarget == other.buildTarget;
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
          target(other.target),
          destDir(other.destDir),
          buildDir(other.buildDir),
          buildTarget(other.buildTarget)
    {
    }

};

struct QMAKEPROJECTMANAGER_EXPORT InstallsItem {
    InstallsItem(QString p, QStringList f) : path(p), files(f) {}
    QString path;
    QStringList files;
};

struct QMAKEPROJECTMANAGER_EXPORT InstallsList {
    void clear() { targetPath.clear(); items.clear(); }
    QString targetPath;
    QList<InstallsItem> items;
};

struct QMAKEPROJECTMANAGER_EXPORT ProjectVersion {
    int major;
    int minor;
    int patch;
};

// Implements ProjectNode for qmake .pro files
class QMAKEPROJECTMANAGER_EXPORT QmakeProFileNode : public QmakePriFileNode
{
public:
    QmakeProFileNode(QmakeProject *project, const Utils::FileName &filePath);
    ~QmakeProFileNode();

    bool isParent(QmakeProFileNode *node);

    bool showInSimpleTree() const;

    AddNewInformation addNewInformation(const QStringList &files, Node *context) const;

    QmakeProjectType projectType() const;

    QStringList variableValue(const QmakeVariable var) const;
    QString singleVariableValue(const QmakeVariable var) const;

    bool isSubProjectDeployable(const QString &filePath) const {
        return !m_subProjectsNotToDeploy.contains(filePath);
    }

    QString sourceDir() const;
    QString buildDir(QmakeBuildConfiguration *bc = 0) const;

    QString uiDirectory(const QString &buildDir) const;
    static QString uiHeaderFile(const QString &uiDir, const Utils::FileName &formFile);
    QHash<QString, QString> uiFiles() const;

    QmakeProFileNode *findProFileFor(const Utils::FileName &string) const;
    TargetInformation targetInformation() const;

    InstallsList installsList() const;

    QString makefile() const;
    QString objectExtension() const;
    QString objectsDirectory() const;
    QByteArray cxxDefines() const;
    bool isDeployable() const;

    enum AsyncUpdateDelay { ParseNow, ParseLater };
    void scheduleUpdate(AsyncUpdateDelay delay);

    bool validParse() const;
    bool parseInProgress() const;

    bool showInSimpleTree(QmakeProjectType projectType) const;
    bool isDebugAndRelease() const;
    bool isQtcRunnable() const;

    void setParseInProgress(bool b);
    void setParseInProgressRecursive(bool b);
    void setValidParse(bool b);
    void setValidParseRecursive(bool b);
    void emitProFileUpdatedRecursive();

    void asyncUpdate();

private:
    void applyAsyncEvaluate();

    void setupReader();
    Internal::EvalInput evalInput() const;

    static Internal::EvalResult *evaluate(const Internal::EvalInput &input);
    void applyEvaluate(Internal::EvalResult *parseResult);

    void asyncEvaluate(QFutureInterface<Internal::EvalResult *> &fi, Internal::EvalInput input);
    void cleanupProFileReaders();

    typedef QHash<QmakeVariable, QStringList> QmakeVariablesHash;

    void updateUiFiles(const QString &buildDir);

    static QStringList fileListForVar(QtSupport::ProFileReader *readerExact, QtSupport::ProFileReader *readerCumulative,
                                      const QString &varName, const QString &projectDir, const QString &buildDir);
    static QString uiDirPath(QtSupport::ProFileReader *reader, const QString &buildDir);
    static QString mocDirPath(QtSupport::ProFileReader *reader, const QString &buildDir);
    static QStringList includePaths(QtSupport::ProFileReader *reader, const QString &buildDir, const QString &projectDir);
    static QStringList libDirectories(QtSupport::ProFileReader *reader);
    static Utils::FileNameList subDirsPaths(QtSupport::ProFileReader *reader, const QString &projectDir, QStringList *subProjectsNotToDeploy, QStringList *errors);

    static TargetInformation targetInformation(QtSupport::ProFileReader *reader, QtSupport::ProFileReader *readerBuildPass, const QString &buildDir, const QString &projectFilePath);
    static InstallsList installsList(const QtSupport::ProFileReader *reader, const QString &projectFilePath, const QString &projectDir);

    bool m_isDeployable;

    bool m_validParse;
    bool m_parseInProgress;

    QmakeProjectType m_projectType;
    QmakeVariablesHash m_varValues;

    QMap<QString, QDateTime> m_uitimestamps;
    TargetInformation m_qmakeTargetInformation;
    QStringList m_subProjectsNotToDeploy;
    InstallsList m_installsList;

    QHash<QString, QString> m_uiFiles; // ui-file path, ui header path

    // Async stuff
    QFutureWatcher<Internal::EvalResult *> m_parseFutureWatcher;
    QtSupport::ProFileReader *m_readerExact;
    QtSupport::ProFileReader *m_readerCumulative;
};

} // namespace QmakeProjectManager

#endif // QMAKENODES_H
