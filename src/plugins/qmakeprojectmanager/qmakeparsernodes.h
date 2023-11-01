// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmakeprojectmanager_global.h"
#include "proparser/prowriter.h"
#include "proparser/profileevaluator.h"

#include <coreplugin/idocument.h>
#include <cppeditor/generatedcodemodelsupport.h>
#include <projectexplorer/projectnodes.h>
#include <utils/textfileformat.h>

#include <QFutureWatcher>
#include <QHash>
#include <QLoggingCategory>
#include <QMap>
#include <QPair>
#include <QPointer>
#include <QStringList>

#include <memory>

namespace ProjectExplorer { class ExtraCompilerFactory; }

namespace Utils {
class FilePath;
class FileSystemWatcher;
} // Utils;

namespace QtSupport { class ProFileReader; }

namespace QmakeProjectManager {
class QmakeBuildSystem;
class QmakeProFile;
class QmakeProject;

//  Type of projects
enum class ProjectType {
    Invalid = 0,
    ApplicationTemplate,
    StaticLibraryTemplate,
    SharedLibraryTemplate,
    ScriptTemplate,
    AuxTemplate,
    SubDirsTemplate
};

// Other variables of interest
enum class Variable {
    Defines = 1,
    IncludePath,
    CppFlags,
    CFlags,
    ExactSource,
    CumulativeSource,
    ExactResource,
    CumulativeResource,
    UiDir,
    HeaderExtension,
    CppExtension,
    MocDir,
    PkgConfig,
    PrecompiledHeader,
    LibDirectories,
    Config,
    Qt,
    QmlImportPath,
    QmlDesignerImportPath,
    Makefile,
    ObjectExt,
    ObjectsDir,
    Version,
    TargetExt,
    TargetVersionExt,
    StaticLibExtension,
    ShLibExtension,
    AndroidAbi,
    AndroidAbis,
    AndroidDeploySettingsFile,
    AndroidPackageSourceDir,
    AndroidExtraLibs,
    AndroidApplicationArgs,
    IosDeploymentTarget,
    AppmanPackageDir,
    AppmanManifest,
    IsoIcons,
    QmakeProjectName,
    QmakeCc,
    QmakeCxx
};
size_t qHash(Variable key, uint seed = 0);

namespace Internal {
Q_DECLARE_LOGGING_CATEGORY(qmakeNodesLog)
class QmakeEvalInput;
class QmakeEvalResult;
using QmakeEvalResultPtr = std::shared_ptr<QmakeEvalResult>; // FIXME: Use unique_ptr once we require Qt 6
class QmakePriFileEvalResult;
} // namespace Internal;

class InstallsList;

enum class FileOrigin { ExactParse, CumulativeParse };
size_t qHash(FileOrigin fo);
using SourceFile = QPair<Utils::FilePath, FileOrigin>;
using SourceFiles = QSet<SourceFile>;

// Implements ProjectNode for qmake .pri files
class QMAKEPROJECTMANAGER_EXPORT QmakePriFile
{
public:
    QmakePriFile(QmakeBuildSystem *buildSystem, QmakeProFile *qmakeProFile, const Utils::FilePath &filePath);
    explicit QmakePriFile(const Utils::FilePath &filePath);
    virtual ~QmakePriFile();

    void finishInitialization(QmakeBuildSystem *buildSystem, QmakeProFile *qmakeProFile);

    const Utils::FilePath &filePath() const { return m_filePath; }
    Utils::FilePath directoryPath() const;
    QString deviceRoot() const;
    virtual QString displayName() const;

    QmakePriFile *parent() const;
    QmakeProject *project() const;
    const QVector<QmakePriFile *> children() const;

    QmakePriFile *findPriFile(const Utils::FilePath &fileName);
    const QmakePriFile *findPriFile(const Utils::FilePath &fileName) const;

    bool knowsFile(const Utils::FilePath &filePath) const;

    void makeEmpty();

    // Files of the specified type declared in this file.
    SourceFiles files(const ProjectExplorer::FileType &type) const;

    // Files of the specified type declared in this file and in included .pri files.
    const QSet<Utils::FilePath> collectFiles(const ProjectExplorer::FileType &type) const;

    void update(const Internal::QmakePriFileEvalResult &result);

    bool canAddSubProject(const Utils::FilePath &proFilePath) const;

    bool addSubProject(const Utils::FilePath &proFile);
    bool removeSubProjects(const Utils::FilePath &proFilePath);

    bool addFiles(const Utils::FilePaths &filePaths, Utils::FilePaths *notAdded = nullptr);
    bool removeFiles(const Utils::FilePaths &filePaths, Utils::FilePaths *notRemoved = nullptr);
    bool deleteFiles(const Utils::FilePaths &filePaths);
    bool canRenameFile(const Utils::FilePath &oldFilePath, const Utils::FilePath &newFilePath);
    bool renameFile(const Utils::FilePath &oldFilePath, const Utils::FilePath &newFilePath);
    bool addDependencies(const QStringList &dependencies);

    bool setProVariable(const QString &var, const QStringList &values,
                        const QString &scope = QString(),
                        int flags = QmakeProjectManager::Internal::ProWriter::ReplaceValues);

    bool folderChanged(const QString &changedFolder, const QSet<Utils::FilePath> &newFiles);

    bool deploysFolder(const QString &folder) const;

    QmakeProFile *proFile() const;
    QVector<QmakePriFile *> subPriFilesExact() const;

    // Set by parent
    bool includedInExactParse() const;

    static QSet<Utils::FilePath> recursiveEnumerate(const QString &folder);

    void scheduleUpdate();

    QmakeBuildSystem *buildSystem() const;

protected:
    void setIncludedInExactParse(bool b);
    static QStringList varNames(ProjectExplorer::FileType type, QtSupport::ProFileReader *readerExact);
    static QStringList varNamesForRemoving();
    static QString varNameForAdding(const QString &mimeType);
    static QSet<Utils::FilePath> filterFilesProVariables(ProjectExplorer::FileType fileType, const QSet<Utils::FilePath> &files);
    static QSet<Utils::FilePath> filterFilesRecursiveEnumerata(ProjectExplorer::FileType fileType, const QSet<Utils::FilePath> &files);

    enum ChangeType {
        AddToProFile,
        RemoveFromProFile
    };

    enum class Change { Save, TestOnly };
    bool renameFile(const Utils::FilePath &oldFilePath, const Utils::FilePath &newFilePath, Change mode);
    void changeFiles(const QString &mimeType,
                     const Utils::FilePaths &filePaths,
                     Utils::FilePaths *notChanged,
                     ChangeType change,
                     Change mode = Change::Save);

    void addChild(QmakePriFile *pf);

private:
    void setParent(QmakePriFile *p);

    bool prepareForChange();
    static bool ensureWriteableProFile(const QString &file);
    QPair<ProFile *, QStringList> readProFile();
    static QPair<ProFile *, QStringList> readProFileFromContents(const QString &contents);
    void save(const QStringList &lines);
    bool saveModifiedEditors();
    Utils::FilePaths formResources(const Utils::FilePath &formFile) const;
    static QStringList baseVPaths(QtSupport::ProFileReader *reader, const QString &projectDir, const QString &buildDir);
    static QStringList fullVPaths(const QStringList &baseVPaths, QtSupport::ProFileReader *reader, const QString &qmakeVariable, const QString &projectDir);
    static void processValues(Internal::QmakePriFileEvalResult &result);
    void watchFolders(const QSet<Utils::FilePath> &folders);

    QString continuationIndent() const;

    QPointer<QmakeBuildSystem> m_buildSystem;
    QmakeProFile *m_qmakeProFile = nullptr;
    QmakePriFile *m_parent = nullptr;
    QVector<QmakePriFile *> m_children;

    Utils::TextFileFormat m_textFormat;

    // Memory is cheap...
    QMap<ProjectExplorer::FileType, SourceFiles> m_files;
    QSet<Utils::FilePath> m_recursiveEnumerateFiles; // FIXME: Remove this?!
    QSet<QString> m_watchedFolders;
    const Utils::FilePath m_filePath;
    bool m_includedInExactParse = true;

    friend class QmakeProFile;
};

class QMAKEPROJECTMANAGER_EXPORT TargetInformation
{
public:
    bool valid = false;
    QString target;
    Utils::FilePath destDir;
    Utils::FilePath buildDir;
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

    TargetInformation() = default;
};

class QMAKEPROJECTMANAGER_EXPORT InstallsItem {
public:
    InstallsItem() = default;
    InstallsItem(QString p, QVector<ProFileEvaluator::SourceFile> f, bool a, bool e)
        : path(p), files(f), active(a), executable(e) {}
    QString path;
    QVector<ProFileEvaluator::SourceFile> files;
    bool active = false;
    bool executable = false;
};

class QMAKEPROJECTMANAGER_EXPORT InstallsList {
public:
    void clear() { targetPath.clear(); items.clear(); }
    QString targetPath;
    QVector<InstallsItem> items;
};

// Implements ProjectNode for qmake .pro files
class QMAKEPROJECTMANAGER_EXPORT QmakeProFile : public QmakePriFile
{
public:
    QmakeProFile(QmakeBuildSystem *buildSystem, const Utils::FilePath &filePath);
    explicit QmakeProFile(const Utils::FilePath &filePath);
    ~QmakeProFile() override;

    bool isParent(QmakeProFile *node);
    QString displayName() const final;

    QList<QmakeProFile *> allProFiles();
    QmakeProFile *findProFile(const Utils::FilePath &fileName);
    const QmakeProFile *findProFile(const Utils::FilePath &fileName) const;

    ProjectType projectType() const;

    QStringList variableValue(const Variable var) const;
    QString singleVariableValue(const Variable var) const;

    bool isSubProjectDeployable(const Utils::FilePath &filePath) const {
        return !m_subProjectsNotToDeploy.contains(filePath);
    }

    Utils::FilePath sourceDir() const;

    Utils::FilePaths generatedFiles(const Utils::FilePath &buildDirectory,
                                       const Utils::FilePath &sourceFile,
                                       const ProjectExplorer::FileType &sourceFileType) const;
    QList<ProjectExplorer::ExtraCompiler *> extraCompilers() const;
    ProjectExplorer::ExtraCompiler *findExtraCompiler(
            const std::function<bool(ProjectExplorer::ExtraCompiler *)> &filter);

    TargetInformation targetInformation() const;
    InstallsList installsList() const;
    const QStringList featureRoots() const { return m_featureRoots; }

    QByteArray cxxDefines() const;

    enum AsyncUpdateDelay { ParseNow, ParseLater };
    using QmakePriFile::scheduleUpdate;
    void scheduleUpdate(AsyncUpdateDelay delay);

    bool validParse() const;
    bool parseInProgress() const;

    void setParseInProgressRecursive(bool b);

    void asyncUpdate();

    bool isFileFromWildcard(const QString &filePath) const;

private:
    void cleanupFutureWatcher();
    void setupFutureWatcher();

    void setParseInProgress(bool b);
    void setValidParseRecursive(bool b);

    void setupReader();
    Internal::QmakeEvalInput evalInput() const;

    static Internal::QmakeEvalResultPtr evaluate(const Internal::QmakeEvalInput &input);
    void applyEvaluate(const Internal::QmakeEvalResultPtr &parseResult);

    void asyncEvaluate(QPromise<Internal::QmakeEvalResultPtr> &promise,
                       Internal::QmakeEvalInput input);
    void cleanupProFileReaders();

    void updateGeneratedFiles(const Utils::FilePath &buildDir);

    static QString uiDirPath(QtSupport::ProFileReader *reader, const Utils::FilePath &buildDir);
    static QString mocDirPath(QtSupport::ProFileReader *reader, const Utils::FilePath &buildDir);
    static QString sysrootify(const QString &path, const QString &sysroot, const QString &baseDir, const QString &outputDir);
    static QStringList includePaths(QtSupport::ProFileReader *reader, const Utils::FilePath &sysroot, const Utils::FilePath &buildDir, const QString &projectDir);
    static QStringList libDirectories(QtSupport::ProFileReader *reader);
    static Utils::FilePaths subDirsPaths(QtSupport::ProFileReader *reader, const QString &projectDir, QStringList *subProjectsNotToDeploy, QStringList *errors);

    static TargetInformation targetInformation(QtSupport::ProFileReader *reader, QtSupport::ProFileReader *readerBuildPass, const Utils::FilePath &buildDir, const Utils::FilePath &projectFilePath);
    static InstallsList installsList(const QtSupport::ProFileReader *reader, const QString &projectFilePath, const QString &projectDir, const QString &buildDir);

    void setupExtraCompiler(const Utils::FilePath &buildDir,
                             const ProjectExplorer::FileType &fileType,
                             ProjectExplorer::ExtraCompilerFactory *factory);

    bool m_validParse = false;
    bool m_parseInProgress = false;

    QString m_displayName;
    ProjectType m_projectType = ProjectType::Invalid;

    using VariablesHash = QHash<Variable, QStringList>;
    VariablesHash m_varValues;

    QList<ProjectExplorer::ExtraCompiler *> m_extraCompilers;

    TargetInformation m_qmakeTargetInformation;
    Utils::FilePaths m_subProjectsNotToDeploy;
    InstallsList m_installsList;
    QStringList m_featureRoots;

    std::unique_ptr<Utils::FileSystemWatcher> m_wildcardWatcher;
    QMap<QString, QStringList> m_wildcardDirectoryContents;

    // Async stuff
    QFutureWatcher<Internal::QmakeEvalResultPtr> *m_parseFutureWatcher = nullptr;
    QtSupport::ProFileReader *m_readerExact = nullptr;
    QtSupport::ProFileReader *m_readerCumulative = nullptr;
};

} // namespace QmakeProjectManager
