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
#include "qmakeparsernodes.h"
#include "proparser/prowriter.h"
#include "proparser/profileevaluator.h"

#include <coreplugin/idocument.h>
#include <projectexplorer/projectnodes.h>
#include <cpptools/generatedcodemodelsupport.h>

#include <QHash>
#include <QStringList>
#include <QMap>
#include <QFutureWatcher>

namespace Utils { class FileName; }

namespace QtSupport { class ProFileReader; }

namespace ProjectExplorer { class RunConfiguration; }

namespace QmakeProjectManager {
class QmakeBuildConfiguration;
class QmakeProFileNode;
class QmakeProject;

namespace Internal {
class QmakePriFileNodeDocument;
struct InternalNode;

class EvalInput;
class EvalResult;
class PriFileEvalResult;
}

struct InstallsList;

// Implements ProjectNode for qmake .pri files
class QMAKEPROJECTMANAGER_EXPORT QmakePriFileNode : public ProjectExplorer::ProjectNode
{
public:
    QmakePriFileNode(QmakeProject *project, QmakeProFileNode *qmakeProFileNode, const Utils::FileName &filePath);
    ~QmakePriFileNode() override;

    void update(const Internal::PriFileEvalResult &result);

    // ProjectNode interface
    QList<ProjectExplorer::ProjectAction> supportedActions(Node *node) const override;

    bool showInSimpleTree() const override { return false; }

    bool canAddSubProject(const QString &proFilePath) const override;

    bool addSubProjects(const QStringList &proFilePaths) override;
    bool removeSubProjects(const QStringList &proFilePaths) override;

    bool addFiles(const QStringList &filePaths, QStringList *notAdded = 0) override;
    bool removeFiles(const QStringList &filePaths, QStringList *notRemoved = 0) override;
    bool deleteFiles(const QStringList &filePaths) override;
    bool canRenameFile(const QString &filePath, const QString &newFilePath) override;
    bool renameFile(const QString &filePath, const QString &newFilePath) override;
    AddNewInformation addNewInformation(const QStringList &files, Node *context) const override;

    bool setProVariable(const QString &var, const QStringList &values,
                        const QString &scope = QString(),
                        int flags = QmakeProjectManager::Internal::ProWriter::ReplaceValues);

    bool folderChanged(const QString &changedFolder, const QSet<Utils::FileName> &newFiles);

    bool deploysFolder(const QString &folder) const override;
    QList<ProjectExplorer::RunConfiguration *> runConfigurations() const override;

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

private:
    void scheduleUpdate();

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
            QHash<const ProFile *, Internal::PriFileEvalResult *> proToResult,
            Internal::PriFileEvalResult *fallback,
            QVector<ProFileEvaluator::SourceFile> sourceFiles, ProjectExplorer::FileType type);
    static void extractInstalls(
            QHash<const ProFile *, Internal::PriFileEvalResult *> proToResult,
            Internal::PriFileEvalResult *fallback,
            const InstallsList &installList);
    static void processValues(Internal::PriFileEvalResult &result);
    void watchFolders(const QSet<QString> &folders);

    QmakeProject *m_project;
    QmakeProFileNode *m_qmakeProFileNode;
    Utils::FileName m_projectFilePath;
    QString m_projectDir;

    Internal::QmakePriFileNodeDocument *m_qmakePriFile;

    // Memory is cheap...
    QMap<ProjectExplorer::FileType, QSet<Utils::FileName> > m_files;
    QSet<Utils::FileName> m_recursiveEnumerateFiles;
    QSet<QString> m_watchedFolders;
    bool m_includedInExactParse = true;

    // managed by QmakeProFileNode
    friend class QmakeProjectManager::QmakeProFileNode;
    friend class Internal::QmakePriFileNodeDocument; // for scheduling updates on modified
    // internal temporary subtree representation
    friend struct Internal::InternalNode;
};

class QMAKEPROJECTMANAGER_EXPORT TargetInformation
{
public:
    bool valid = false;
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

    TargetInformation() = default;

    TargetInformation(const TargetInformation &other) = default;
};

struct QMAKEPROJECTMANAGER_EXPORT InstallsItem {
    InstallsItem() = default;
    InstallsItem(QString p, QVector<ProFileEvaluator::SourceFile> f, bool a)
        : path(p), files(f), active(a) {}
    QString path;
    QVector<ProFileEvaluator::SourceFile> files;
    bool active = false;
};

struct QMAKEPROJECTMANAGER_EXPORT InstallsList {
    void clear() { targetPath.clear(); items.clear(); }
    QString targetPath;
    QVector<InstallsItem> items;
};

// Implements ProjectNode for qmake .pro files
class QMAKEPROJECTMANAGER_EXPORT QmakeProFileNode : public QmakePriFileNode
{
public:
    QmakeProFileNode(QmakeProject *project, const Utils::FileName &filePath);
    ~QmakeProFileNode() override;

    bool isParent(QmakeProFileNode *node);

    bool showInSimpleTree() const override;

    AddNewInformation addNewInformation(const QStringList &files, Node *context) const override;

    QmakeProjectManager::ProjectType projectType() const;

    QStringList variableValue(const Variable var) const;
    QString singleVariableValue(const Variable var) const;

    bool isSubProjectDeployable(const QString &filePath) const {
        return !m_subProjectsNotToDeploy.contains(filePath);
    }

    QString sourceDir() const;
    QString buildDir(QmakeBuildConfiguration *bc = 0) const;

    QStringList generatedFiles(const QString &buildDirectory,
                               const ProjectExplorer::FileNode *sourceFile) const;
    QList<ProjectExplorer::ExtraCompiler *> extraCompilers() const;

    QmakeProFileNode *findProFileFor(const Utils::FileName &string) const;
    TargetInformation targetInformation() const;

    InstallsList installsList() const;

    QString makefile() const;
    QString objectExtension() const;
    QString objectsDirectory() const;
    QByteArray cxxDefines() const;

    void scheduleUpdate(QmakeProFile::AsyncUpdateDelay delay);

    bool validParse() const;
    bool parseInProgress() const;

    bool showInSimpleTree(ProjectType projectType) const;
    bool isDebugAndRelease() const;
    bool isQtcRunnable() const;

    void setParseInProgressRecursive(bool b);

    void asyncUpdate();

private:
    void setParseInProgress(bool b);
    void setValidParseRecursive(bool b);

    void applyAsyncEvaluate();

    void setupReader();
    Internal::EvalInput evalInput() const;

    static Internal::EvalResult *evaluate(const Internal::EvalInput &input);
    void applyEvaluate(Internal::EvalResult *parseResult);

    void asyncEvaluate(QFutureInterface<Internal::EvalResult *> &fi, Internal::EvalInput input);
    void cleanupProFileReaders();

    using VariablesHash = QHash<Variable, QStringList>;

    void updateGeneratedFiles(const QString &buildDir);

    static QString uiDirPath(QtSupport::ProFileReader *reader, const QString &buildDir);
    static QString mocDirPath(QtSupport::ProFileReader *reader, const QString &buildDir);
    static QString sysrootify(const QString &path, const QString &sysroot, const QString &baseDir, const QString &outputDir);
    static QStringList includePaths(QtSupport::ProFileReader *reader, const QString &sysroot, const QString &buildDir, const QString &projectDir);
    static QStringList libDirectories(QtSupport::ProFileReader *reader);
    static Utils::FileNameList subDirsPaths(QtSupport::ProFileReader *reader, const QString &projectDir, QStringList *subProjectsNotToDeploy, QStringList *errors);

    static TargetInformation targetInformation(QtSupport::ProFileReader *reader, QtSupport::ProFileReader *readerBuildPass, const QString &buildDir, const QString &projectFilePath);
    static InstallsList installsList(const QtSupport::ProFileReader *reader, const QString &projectFilePath, const QString &projectDir, const QString &buildDir);

    bool m_validParse = false;
    bool m_parseInProgress = false;

    ProjectType m_projectType = ProjectType::Invalid;
    VariablesHash m_varValues;
    QList<ProjectExplorer::ExtraCompiler *> m_extraCompilers;

    TargetInformation m_qmakeTargetInformation;
    QStringList m_subProjectsNotToDeploy;
    InstallsList m_installsList;

    // Async stuff
    QFutureWatcher<Internal::EvalResult *> m_parseFutureWatcher;
    QtSupport::ProFileReader *m_readerExact = nullptr;
    QtSupport::ProFileReader *m_readerCumulative = nullptr;
};

} // namespace QmakeProjectManager
