// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmakeprojectmanager_global.h"

#include "qmakenodes.h"
#include "qmakeparsernodes.h"

#include <projectexplorer/deploymentdata.h>
#include <projectexplorer/project.h>
#include <projectexplorer/toolchain.h>

#include <QStringList>
#include <QFutureInterface>

QT_BEGIN_NAMESPACE
class QMakeGlobals;
class QMakeVfs;
QT_END_NAMESPACE

namespace CppEditor { class CppProjectUpdater; }
namespace ProjectExplorer { class DeploymentData; }
namespace QtSupport { class ProFileReader; }

namespace QmakeProjectManager {

class QmakeBuildConfiguration;

namespace Internal { class CentralizedFolderWatcher; }

class  QMAKEPROJECTMANAGER_EXPORT QmakeProject final : public ProjectExplorer::Project
{
    Q_OBJECT

public:
    explicit QmakeProject(const Utils::FilePath &proFile);
    ~QmakeProject() final;

    ProjectExplorer::Tasks projectIssues(const ProjectExplorer::Kit *k) const final;

    void configureAsExampleProject(ProjectExplorer::Kit *kit) final;

    ProjectExplorer::ProjectImporter *projectImporter() const final;

protected:
    RestoreResult fromMap(const Utils::Store &map, QString *errorMessage) final;

private:
    ProjectExplorer::DeploymentKnowledge deploymentKnowledge() const override;

    mutable ProjectExplorer::ProjectImporter *m_projectImporter = nullptr;
};

// FIXME: This export here is only there to appease the current version
// of the appman plugin. This _will_ go away, one way or the other.
class QMAKEPROJECTMANAGER_EXPORT QmakeBuildSystem : public ProjectExplorer::BuildSystem
{
    Q_OBJECT

public:
    explicit QmakeBuildSystem(QmakeBuildConfiguration *bc);
    ~QmakeBuildSystem();

    bool supportsAction(ProjectExplorer::Node *context,
                        ProjectExplorer::ProjectAction action,
                        const ProjectExplorer::Node *node) const override;

    bool addFiles(ProjectExplorer::Node *context,
                  const Utils::FilePaths &filePaths,
                  Utils::FilePaths *notAdded = nullptr) override;
    ProjectExplorer::RemovedFilesFromProject removeFiles(ProjectExplorer::Node *context,
                                                         const Utils::FilePaths &filePaths,
                                                         Utils::FilePaths *notRemoved = nullptr) override;
    bool deleteFiles(ProjectExplorer::Node *context,
                     const Utils::FilePaths &filePaths) override;
    bool canRenameFile(ProjectExplorer::Node *context,
                       const Utils::FilePath &oldFilePath,
                       const Utils::FilePath &newFilePath) override;
    bool renameFile(ProjectExplorer::Node *context,
                    const Utils::FilePath &oldFilePath,
                    const Utils::FilePath &newFilePath) override;
    bool addDependencies(ProjectExplorer::Node *context,
                         const QStringList &dependencies) override;
    QString name() const final { return QLatin1String("qmake"); }
    void triggerParsing() final;

    Utils::FilePaths filesGeneratedFrom(const Utils::FilePath &file) const final;
    QVariant additionalData(Utils::Id id) const final;
    QList<QPair<Utils::Id, QString>> generators() const override;
    void runGenerator(Utils::Id id) override;

    void asyncUpdate();
    void buildFinished(bool success);
    void activeTargetWasChanged(ProjectExplorer::Target *);

    Utils::FilePath executableFor(const QmakeProFile *file);

    void updateCppCodeModel();
    void updateQmlJSCodeModel();

    static bool equalFileList(const QStringList &a, const QStringList &b);

    void updateBuildSystemData();
    void collectData(const QmakeProFile *file, ProjectExplorer::DeploymentData &deploymentData);
    void collectApplicationData(const QmakeProFile *file,
                                ProjectExplorer::DeploymentData &deploymentData);
    Utils::FilePaths allLibraryTargetFiles(const QmakeProFile *file) const;
    void collectLibraryData(const QmakeProFile *file,
            ProjectExplorer::DeploymentData &deploymentData);
    void startAsyncTimer(QmakeProFile::AsyncUpdateDelay delay);

    void warnOnToolChainMismatch(const QmakeProFile *pro) const;
    void testToolChain(ProjectExplorer::ToolChain *tc, const Utils::FilePath &path) const;

    QString deviceRoot() const;

    /// \internal
    QtSupport::ProFileReader *createProFileReader(const QmakeProFile *qmakeProFile);
    /// \internal
    QMakeGlobals *qmakeGlobals() const;
    /// \internal
    QMakeVfs *qmakeVfs() const;
    /// \internal
    const Utils::FilePath &qmakeSysroot() const;
    /// \internal
    void destroyProFileReader(QtSupport::ProFileReader *reader);
    void deregisterFromCacheManager();

    /// \internal
    void scheduleAsyncUpdateFile(QmakeProFile *file,
                                 QmakeProFile::AsyncUpdateDelay delay = QmakeProFile::ParseLater);
    /// \internal
    void incrementPendingEvaluateFutures();
    /// \internal
    void decrementPendingEvaluateFutures();
    /// \internal
    bool wasEvaluateCanceled();

    void updateCodeModels();
    void updateDocuments();

    void watchFolders(const QStringList &l, QmakePriFile *file);
    void unwatchFolders(const QStringList &l, QmakePriFile *file);

    static void proFileParseError(const QString &errorMessage, const Utils::FilePath &filePath);

    enum AsyncUpdateState { Base, AsyncFullUpdatePending, AsyncPartialUpdatePending, AsyncUpdateInProgress, ShuttingDown };
    AsyncUpdateState asyncUpdateState() const;

    QmakeProFile *rootProFile() const;

    void notifyChanged(const Utils::FilePath &name);

    enum Action { BUILD, REBUILD, CLEAN };
    void buildHelper(Action action, bool isFileBuild,
                     QmakeProFileNode *profile,
                     ProjectExplorer::FileNode *buildableFile);

    Utils::FilePath buildDir(const Utils::FilePath &proFilePath) const;
    QmakeBuildConfiguration *qmakeBuildConfiguration() const;

    void scheduleUpdateAllNowOrLater();

private:
    ProjectExplorer::ExtraCompiler *findExtraCompiler(
            const ExtraCompilerFilter &filter) const override;

    void scheduleUpdateAll(QmakeProFile::AsyncUpdateDelay delay);
    void scheduleUpdateAllLater() { scheduleUpdateAll(QmakeProFile::ParseLater); }

    mutable QSet<const QPair<Utils::FilePath, Utils::FilePath>> m_toolChainWarnings;

    // Current configuration
    QString m_oldQtIncludePath;
    QString m_oldQtLibsPath;

    std::unique_ptr<QmakeProFile> m_rootProFile;

    QMakeVfs *m_qmakeVfs = nullptr;

    // cached data during project rescan
    std::unique_ptr<QMakeGlobals> m_qmakeGlobals;
    int m_qmakeGlobalsRefCnt = 0;
    bool m_invalidateQmakeVfsContents = false;

    Utils::FilePath m_qmakeSysroot;

    std::unique_ptr<QFutureInterface<void>> m_asyncUpdateFutureInterface;
    int m_pendingEvaluateFuturesCount = 0;
    AsyncUpdateState m_asyncUpdateState = Base;
    bool m_cancelEvaluate = false;
    QList<QmakeProFile *> m_partialEvaluate;

    CppEditor::CppProjectUpdater *m_cppCodeModelUpdater = nullptr;

    Internal::CentralizedFolderWatcher *m_centralizedFolderWatcher = nullptr;

    ProjectExplorer::BuildSystem::ParseGuard m_guard;
    bool m_firstParseNeeded = true;
};

} // namespace QmakeProjectManager
