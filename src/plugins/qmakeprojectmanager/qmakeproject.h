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
#include "qmakeprojectmanager.h"
#include "qmakenodes.h"
#include "qmakeparsernodes.h"

#include <projectexplorer/project.h>
#include <projectexplorer/runconfiguration.h>

#include <QStringList>
#include <QFutureInterface>
#include <QTimer>
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

namespace Internal {
class CentralizedFolderWatcher;
class QmakeProjectFiles;
}

class  QMAKEPROJECTMANAGER_EXPORT QmakeProject : public ProjectExplorer::Project
{
    Q_OBJECT

public:
    explicit QmakeProject(const Utils::FileName &proFile);
    ~QmakeProject() final;

    QmakeProFile *rootProFile() const;

    bool supportsKit(ProjectExplorer::Kit *k, QString *errorMesage) const final;

    QmakeProFileNode *rootProjectNode() const final;
    bool validParse(const Utils::FileName &proFilePath) const;
    bool parseInProgress(const Utils::FileName &proFilePath) const;

    virtual QStringList filesGeneratedFrom(const QString &file) const final;

    enum Parsing {ExactParse, ExactAndCumulativeParse };
    QList<QmakeProFile *> allProFiles(const QList<ProjectType> &projectTypes = QList<ProjectType>(),
                                      Parsing parse = ExactParse) const;
    QList<QmakeProFile *> applicationProFiles(Parsing parse = ExactParse) const;
    bool hasApplicationProFile(const Utils::FileName &path) const;

    QList<Core::Id> creationIds(Core::Id base,
                                ProjectExplorer::IRunConfigurationFactory::CreationMode mode,
                                const QList<ProjectType> &projectTypes = {});

    static void notifyChanged(const Utils::FileName &name);

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

    /// \internal
    void scheduleAsyncUpdate(QmakeProFile *file,
                             QmakeProFile::AsyncUpdateDelay delay = QmakeProFile::ParseLater);
    /// \internal
    void incrementPendingEvaluateFutures();
    /// \internal
    void decrementPendingEvaluateFutures();
    /// \internal
    bool wasEvaluateCanceled();

    void updateCodeModels();

    void watchFolders(const QStringList &l, QmakePriFile *file);
    void unwatchFolders(const QStringList &l, QmakePriFile *file);

    bool needsConfiguration() const final;

    void configureAsExampleProject(const QSet<Core::Id> &platforms) final;

    bool requiresTargetPanel() const final;

    /// \internal
    QString disabledReasonForRunConfiguration(const Utils::FileName &proFilePath);

    /// used by the default implementation of shadowBuildDirectory
    static QString buildNameFor(const ProjectExplorer::Kit *k);

    void emitBuildDirectoryInitialized();
    static void proFileParseError(const QString &errorMessage);

    ProjectExplorer::ProjectImporter *projectImporter() const final;

    enum AsyncUpdateState { Base, AsyncFullUpdatePending, AsyncPartialUpdatePending, AsyncUpdateInProgress, ShuttingDown };
    AsyncUpdateState asyncUpdateState() const;

    QString mapProFilePathToTarget(const Utils::FileName &proFilePath);

signals:
    void proFileUpdated(QmakeProjectManager::QmakeProFile *pro, bool, bool);
    void buildDirectoryInitialized();
    void proFilesEvaluated();

public:
    void scheduleAsyncUpdate(QmakeProFile::AsyncUpdateDelay delay = QmakeProFile::ParseLater);
    void scheduleAsyncUpdateLater() { scheduleAsyncUpdate(); }

protected:
    RestoreResult fromMap(const QVariantMap &map, QString *errorMessage) final;

private:
    void asyncUpdate();
    void buildFinished(bool success);
    void activeTargetWasChanged();

    void setAllBuildConfigurationsEnabled(bool enabled);

    QString executableFor(const QmakeProFile *file);
    void updateRunConfigurations();

    void updateCppCodeModel();
    void updateQmlJSCodeModel();

    static QList<QmakeProFile *> collectAllProFiles(QmakeProFile *file, Parsing parse,
                                                    const QList<ProjectType> &projectTypes);

    static bool equalFileList(const QStringList &a, const QStringList &b);

    void updateBuildSystemData();
    void collectData(const QmakeProFile *file, ProjectExplorer::DeploymentData &deploymentData);
    void collectApplicationData(const QmakeProFile *file,
                                ProjectExplorer::DeploymentData &deploymentData);
    void collectLibraryData(const QmakeProFile *file,
            ProjectExplorer::DeploymentData &deploymentData);
    void startAsyncTimer(QmakeProFile::AsyncUpdateDelay delay);
    bool matchesKit(const ProjectExplorer::Kit *kit);

    void warnOnToolChainMismatch(const QmakeProFile *pro) const;
    void testToolChain(ProjectExplorer::ToolChain *tc, const Utils::FileName &path) const;

    mutable QSet<const QPair<Utils::FileName, Utils::FileName>> m_toolChainWarnings;

    // Current configuration
    QString m_oldQtIncludePath;
    QString m_oldQtLibsPath;

    std::unique_ptr<QmakeProFile> m_rootProFile;

    QMakeVfs *m_qmakeVfs = nullptr;

    // cached data during project rescan
    QMakeGlobals *m_qmakeGlobals = nullptr;
    int m_qmakeGlobalsRefCnt = 0;

    QString m_qmakeSysroot;

    QTimer m_asyncUpdateTimer;
    QFutureInterface<void> *m_asyncUpdateFutureInterface = nullptr;
    int m_pendingEvaluateFuturesCount = 0;
    AsyncUpdateState m_asyncUpdateState = Base;
    bool m_cancelEvaluate = false;
    QList<QmakeProFile *> m_partialEvaluate;

    CppTools::CppProjectUpdater *m_cppCodeModelUpdater = nullptr;

    Internal::CentralizedFolderWatcher *m_centralizedFolderWatcher = nullptr;

    ProjectExplorer::Target *m_activeTarget = nullptr;
    mutable ProjectExplorer::ProjectImporter *m_projectImporter = nullptr;
};

} // namespace QmakeProjectManager
