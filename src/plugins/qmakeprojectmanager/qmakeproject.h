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

#include "qmakenodes.h"
#include "qmakeparsernodes.h"

#include <projectexplorer/deploymentdata.h>
#include <projectexplorer/project.h>
#include <projectexplorer/toolchain.h>

#include <QStringList>
#include <QFutureInterface>
#include <QFuture>

QT_BEGIN_NAMESPACE
class QMakeGlobals;
class QMakeVfs;
QT_END_NAMESPACE

namespace CppTools { class CppProjectUpdater; }
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
    RestoreResult fromMap(const QVariantMap &map, QString *errorMessage) final;

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
                  const QStringList &filePaths,
                  QStringList *notAdded = nullptr) override;
    ProjectExplorer::RemovedFilesFromProject removeFiles(ProjectExplorer::Node *context,
                                                         const QStringList &filePaths,
                                                         QStringList *notRemoved = nullptr) override;
    bool deleteFiles(ProjectExplorer::Node *context,
                     const QStringList &filePaths) override;
    bool canRenameFile(ProjectExplorer::Node *context,
                       const QString &filePath, const QString &newFilePath) override;
    bool renameFile(ProjectExplorer::Node *context,
                    const QString &filePath, const QString &newFilePath) override;
    bool addDependencies(ProjectExplorer::Node *context,
                         const QStringList &dependencies) override;
    void triggerParsing() final;

    QStringList filesGeneratedFrom(const QString &file) const final;
    QVariant additionalData(Utils::Id id) const final;

    void asyncUpdate();
    void buildFinished(bool success);
    void activeTargetWasChanged(ProjectExplorer::Target *);

    QString executableFor(const QmakeProFile *file);

    void updateCppCodeModel();
    void updateQmlJSCodeModel();

    static bool equalFileList(const QStringList &a, const QStringList &b);

    void updateBuildSystemData();
    void collectData(const QmakeProFile *file, ProjectExplorer::DeploymentData &deploymentData);
    void collectApplicationData(const QmakeProFile *file,
                                ProjectExplorer::DeploymentData &deploymentData);
    void collectLibraryData(const QmakeProFile *file,
            ProjectExplorer::DeploymentData &deploymentData);
    void startAsyncTimer(QmakeProFile::AsyncUpdateDelay delay);

    void warnOnToolChainMismatch(const QmakeProFile *pro) const;
    void testToolChain(ProjectExplorer::ToolChain *tc, const Utils::FilePath &path) const;

    /// \internal
    QtSupport::ProFileReader *createProFileReader(const QmakeProFile *qmakeProFile);
    /// \internal
    QMakeGlobals *qmakeGlobals();
    /// \internal
    QMakeVfs *qmakeVfs();
    /// \internal
    QString qmakeSysroot();
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

    static void proFileParseError(const QString &errorMessage);

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

    QString m_qmakeSysroot;

    QFutureInterface<void> m_asyncUpdateFutureInterface;
    int m_pendingEvaluateFuturesCount = 0;
    AsyncUpdateState m_asyncUpdateState = Base;
    bool m_cancelEvaluate = false;
    QList<QmakeProFile *> m_partialEvaluate;

    CppTools::CppProjectUpdater *m_cppCodeModelUpdater = nullptr;

    Internal::CentralizedFolderWatcher *m_centralizedFolderWatcher = nullptr;

    ProjectExplorer::BuildSystem::ParseGuard m_guard;
    bool m_firstParseNeeded = true;
};

} // namespace QmakeProjectManager
