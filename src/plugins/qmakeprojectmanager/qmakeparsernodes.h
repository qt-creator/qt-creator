/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include "qmakeprojectmanager_global.h"
#include "proparser/prowriter.h"
#include "proparser/profileevaluator.h"

#include <coreplugin/idocument.h>
#include <cpptools/generatedcodemodelsupport.h>

#include <QHash>
#include <QStringList>
#include <QMap>
#include <QFutureWatcher>

#include <memory>

namespace Utils { class FileName; }
namespace QtSupport { class ProFileReader; }
namespace ProjectExplorer { class RunConfiguration; }

namespace QmakeProjectManager {
class QmakeBuildConfiguration;
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
    Source,
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
    AndroidArch,
    AndroidDeploySettingsFile,
    AndroidPackageSourceDir,
    AndroidExtraLibs,
    IsoIcons,
    QmakeProjectName,
    QmakeCc,
    QmakeCxx
};
uint qHash(Variable key, uint seed = 0);

namespace Internal {
class QmakeEvalInput;
class QmakeEvalResult;
class QmakePriFileEvalResult;
} // namespace Internal;

class InstallsList;

// Implements ProjectNode for qmake .pri files
class QMAKEPROJECTMANAGER_EXPORT QmakePriFile
{
public:
    QmakePriFile(QmakeProject *project, QmakeProFile *qmakeProFile, const Utils::FileName &filePath);
    virtual ~QmakePriFile();

    Utils::FileName filePath() const;
    Utils::FileName directoryPath() const;
    virtual QString displayName() const;

    QmakePriFile *parent() const;
    QmakeProject *project() const;
    QVector<QmakePriFile *> children() const;

    QmakePriFile *findPriFile(const Utils::FileName &fileName);

    bool knowsFile(const Utils::FileName &filePath) const;

    void makeEmpty();

    QSet<Utils::FileName> files(const ProjectExplorer::FileType &type) const;

    void update(const Internal::QmakePriFileEvalResult &result);

    // ProjectNode interface
    virtual bool canAddSubProject(const QString &proFilePath) const;

    virtual bool addSubProject(const QString &proFile);
    virtual bool removeSubProjects(const QString &proFilePath);

    virtual bool addFiles(const QStringList &filePaths, QStringList *notAdded = nullptr);
    virtual bool removeFiles(const QStringList &filePaths, QStringList *notRemoved = nullptr);
    virtual bool deleteFiles(const QStringList &filePaths);
    virtual bool canRenameFile(const QString &filePath, const QString &newFilePath);
    virtual bool renameFile(const QString &filePath, const QString &newFilePath);

    bool setProVariable(const QString &var, const QStringList &values,
                        const QString &scope = QString(),
                        int flags = QmakeProjectManager::Internal::ProWriter::ReplaceValues);

    bool folderChanged(const QString &changedFolder, const QSet<Utils::FileName> &newFiles);

    virtual bool deploysFolder(const QString &folder) const;

    QmakeProFile *proFile() const;
    QVector<QmakePriFile *> subPriFilesExact() const;

    // Set by parent
    bool includedInExactParse() const;

    static QSet<Utils::FileName> recursiveEnumerate(const QString &folder);

    void scheduleUpdate();

protected:
    void setIncludedInExactParse(bool b);
    static QStringList varNames(ProjectExplorer::FileType type, QtSupport::ProFileReader *readerExact);
    static QStringList varNamesForRemoving();
    static QString varNameForAdding(const QString &mimeType);
    static QSet<Utils::FileName> filterFilesProVariables(ProjectExplorer::FileType fileType, const QSet<Utils::FileName> &files);
    static QSet<Utils::FileName> filterFilesRecursiveEnumerata(ProjectExplorer::FileType fileType, const QSet<Utils::FileName> &files);

    enum ChangeType {
        AddToProFile,
        RemoveFromProFile
    };

    enum class Change { Save, TestOnly };
    bool renameFile(const QString &oldName,
                    const QString &newName,
                    const QString &mimeType,
                    Change mode = Change::Save);
    void changeFiles(const QString &mimeType,
                     const QStringList &filePaths,
                     QStringList *notChanged,
                     ChangeType change,
                     Change mode = Change::Save);

    void addChild(QmakePriFile *pf);

private:
    void setParent(QmakePriFile *p);

    bool prepareForChange();
    static bool ensureWriteableProFile(const QString &file);
    static QPair<ProFile *, QStringList> readProFile(const QString &file);
    static QPair<ProFile *, QStringList> readProFileFromContents(const QString &contents);
    void save(const QStringList &lines);
    bool priFileWritable(const QString &absoluteFilePath);
    bool saveModifiedEditors();
    QStringList formResources(const QString &formFile) const;
    static QStringList baseVPaths(QtSupport::ProFileReader *reader, const QString &projectDir, const QString &buildDir);
    static QStringList fullVPaths(const QStringList &baseVPaths, QtSupport::ProFileReader *reader, const QString &qmakeVariable, const QString &projectDir);
    static void extractSources(
            QHash<const ProFile *, Internal::QmakePriFileEvalResult *> proToResult,
            Internal::QmakePriFileEvalResult *fallback,
            QVector<ProFileEvaluator::SourceFile> sourceFiles, ProjectExplorer::FileType type);
    static void extractInstalls(
            QHash<const ProFile *, Internal::QmakePriFileEvalResult *> proToResult,
            Internal::QmakePriFileEvalResult *fallback,
            const InstallsList &installList);
    static void processValues(Internal::QmakePriFileEvalResult &result);
    void watchFolders(const QSet<Utils::FileName> &folders);

    QmakeProject *m_project = nullptr;
    QmakeProFile *m_qmakeProFile = nullptr;
    QmakePriFile *m_parent = nullptr;
    QVector<QmakePriFile *> m_children;

    std::unique_ptr<Core::IDocument> m_priFileDocument;

    // Memory is cheap...
    QMap<ProjectExplorer::FileType, QSet<Utils::FileName>> m_files;
    QSet<Utils::FileName> m_recursiveEnumerateFiles; // FIXME: Remove this?!
    QSet<QString> m_watchedFolders;
    bool m_includedInExactParse = true;

    friend class QmakeProFile;
};

class QMAKEPROJECTMANAGER_EXPORT TargetInformation
{
public:
    bool valid = false;
    QString target;
    Utils::FileName destDir;
    Utils::FileName buildDir;
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
    TargetInformation(const TargetInformation &other) = default;
};

class QMAKEPROJECTMANAGER_EXPORT InstallsItem {
public:
    InstallsItem() = default;
    InstallsItem(QString p, QVector<ProFileEvaluator::SourceFile> f, bool a)
        : path(p), files(f), active(a) {}
    QString path;
    QVector<ProFileEvaluator::SourceFile> files;
    bool active = false;
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
    QmakeProFile(QmakeProject *project, const Utils::FileName &filePath);
    ~QmakeProFile() override;

    bool isParent(QmakeProFile *node);
    QString displayName() const final;

    QList<QmakeProFile *> allProFiles();
    QmakeProFile *findProFile(const Utils::FileName &fileName);

    ProjectType projectType() const;

    QStringList variableValue(const Variable var) const;
    QString singleVariableValue(const Variable var) const;

    bool isSubProjectDeployable(const Utils::FileName &filePath) const {
        return !m_subProjectsNotToDeploy.contains(filePath);
    }

    Utils::FileName sourceDir() const;
    Utils::FileName buildDir(QmakeBuildConfiguration *bc = nullptr) const;

    Utils::FileNameList generatedFiles(const Utils::FileName &buildDirectory,
                                       const Utils::FileName &sourceFile,
                                       const ProjectExplorer::FileType &sourceFileType) const;
    QList<ProjectExplorer::ExtraCompiler *> extraCompilers() const;

    TargetInformation targetInformation() const;
    InstallsList installsList() const;

    QString makefile() const;
    QString objectExtension() const;
    QString objectsDirectory() const;
    QByteArray cxxDefines() const;

    enum AsyncUpdateDelay { ParseNow, ParseLater };
    using QmakePriFile::scheduleUpdate;
    void scheduleUpdate(AsyncUpdateDelay delay);

    bool validParse() const;
    bool parseInProgress() const;

    bool isDebugAndRelease() const;
    bool isQtcRunnable() const;

    void setParseInProgressRecursive(bool b);

    void asyncUpdate();

private:
    void setParseInProgress(bool b);
    void setValidParseRecursive(bool b);

    void applyAsyncEvaluate();

    void setupReader();
    Internal::QmakeEvalInput evalInput() const;

    static Internal::QmakeEvalResult *evaluate(const Internal::QmakeEvalInput &input);
    void applyEvaluate(Internal::QmakeEvalResult *parseResult);

    void asyncEvaluate(QFutureInterface<Internal::QmakeEvalResult *> &fi, Internal::QmakeEvalInput input);
    void cleanupProFileReaders();

    void updateGeneratedFiles(const Utils::FileName &buildDir);

    static QString uiDirPath(QtSupport::ProFileReader *reader, const Utils::FileName &buildDir);
    static QString mocDirPath(QtSupport::ProFileReader *reader, const Utils::FileName &buildDir);
    static QString sysrootify(const QString &path, const QString &sysroot, const QString &baseDir, const QString &outputDir);
    static QStringList includePaths(QtSupport::ProFileReader *reader, const Utils::FileName &sysroot, const Utils::FileName &buildDir, const QString &projectDir);
    static QStringList libDirectories(QtSupport::ProFileReader *reader);
    static Utils::FileNameList subDirsPaths(QtSupport::ProFileReader *reader, const QString &projectDir, QStringList *subProjectsNotToDeploy, QStringList *errors);

    static TargetInformation targetInformation(QtSupport::ProFileReader *reader, QtSupport::ProFileReader *readerBuildPass, const Utils::FileName &buildDir, const Utils::FileName &projectFilePath);
    static InstallsList installsList(const QtSupport::ProFileReader *reader, const QString &projectFilePath, const QString &projectDir, const QString &buildDir);

    void setupExtraCompiler(const Utils::FileName &buildDir,
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
    Utils::FileNameList m_subProjectsNotToDeploy;
    InstallsList m_installsList;

    // Async stuff
    QFutureWatcher<Internal::QmakeEvalResult *> m_parseFutureWatcher;
    QtSupport::ProFileReader *m_readerExact = nullptr;
    QtSupport::ProFileReader *m_readerCumulative = nullptr;
};

} // namespace QmakeProjectManager
