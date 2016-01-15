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

#ifndef QMAKEPROJECT_H
#define QMAKEPROJECT_H

#include "qmakeprojectmanager_global.h"
#include "qmakeprojectmanager.h"
#include "qmakenodes.h"

#include <projectexplorer/project.h>
#include <projectexplorer/projectnodes.h>

#include <QStringList>
#include <QFutureInterface>
#include <QTimer>
#include <QFuture>

QT_BEGIN_NAMESPACE
class ProFileGlobals;
class QMakeVfs;
QT_END_NAMESPACE

namespace ProjectExplorer { class DeploymentData; }
namespace QtSupport { class ProFileReader; }

namespace QmakeProjectManager {
class QmakeBuildConfiguration;
class QmakePriFileNode;
class QmakeProFileNode;

namespace Internal {
class CentralizedFolderWatcher;
class QmakeProjectFile;
class QmakeProjectFiles;
class QmakeProjectConfigWidget;
}

class  QMAKEPROJECTMANAGER_EXPORT QmakeProject : public ProjectExplorer::Project
{
    Q_OBJECT

public:
    QmakeProject(QmakeManager *manager, const QString &proFile);
    ~QmakeProject() override;

    QString displayName() const override;
    QmakeManager *projectManager() const override;

    bool supportsKit(ProjectExplorer::Kit *k, QString *errorMesage) const override;

    QmakeProFileNode *rootProjectNode() const override;
    bool validParse(const Utils::FileName &proFilePath) const;
    bool parseInProgress(const Utils::FileName &proFilePath) const;

    virtual QStringList files(FilesMode fileMode) const override;
    virtual QStringList filesGeneratedFrom(const QString &file) const override;

    enum Parsing {ExactParse, ExactAndCumulativeParse };
    QList<QmakeProFileNode *> allProFiles(const QList<QmakeProjectType> &projectTypes = QList<QmakeProjectType>(),
                                          Parsing parse = ExactParse) const;
    QList<QmakeProFileNode *> applicationProFiles(Parsing parse = ExactParse) const;
    bool hasApplicationProFile(const Utils::FileName &path) const;

    static QList<QmakeProFileNode *> nodesWithQtcRunnable(QList<QmakeProFileNode *> nodes);
    static QList<Core::Id> idsForNodes(Core::Id base, const QList<QmakeProFileNode *> &nodes);

    void notifyChanged(const Utils::FileName &name);

    /// \internal
    QtSupport::ProFileReader *createProFileReader(const QmakeProFileNode *qmakeProFileNode, QmakeBuildConfiguration *bc = 0);
    /// \internal
    ProFileGlobals *qmakeGlobals();
    /// \internal
    QMakeVfs *qmakeVfs();
    /// \internal
    void destroyProFileReader(QtSupport::ProFileReader *reader);

    /// \internal
    void scheduleAsyncUpdate(QmakeProjectManager::QmakeProFileNode *node,
                             QmakeProFileNode::AsyncUpdateDelay delay = QmakeProFileNode::ParseLater);
    /// \internal
    void incrementPendingEvaluateFutures();
    /// \internal
    void decrementPendingEvaluateFutures();
    /// \internal
    bool wasEvaluateCanceled();

    // For QmakeProFileNode after a on disk change
    void updateFileList();
    void updateCodeModels();

    void watchFolders(const QStringList &l, QmakePriFileNode *node);
    void unwatchFolders(const QStringList &l, QmakePriFileNode *node);

    bool needsConfiguration() const override;

    void configureAsExampleProject(const QSet<Core::Id> &platforms) override;

    bool requiresTargetPanel() const override;

    /// \internal
    QString disabledReasonForRunConfiguration(const Utils::FileName &proFilePath);

    /// used by the default implementation of shadowBuildDirectory
    static QString buildNameFor(const ProjectExplorer::Kit *k);

    void emitBuildDirectoryInitialized();
    static void proFileParseError(const QString &errorMessage);

    ProjectExplorer::ProjectImporter *createProjectImporter() const override;

    enum AsyncUpdateState { Base, AsyncFullUpdatePending, AsyncPartialUpdatePending, AsyncUpdateInProgress, ShuttingDown };
    AsyncUpdateState asyncUpdateState() const;

signals:
    void proFileUpdated(QmakeProjectManager::QmakeProFileNode *node, bool, bool);
    void buildDirectoryInitialized();
    void proFilesEvaluated();

public slots:
    void scheduleAsyncUpdate(QmakeProFileNode::AsyncUpdateDelay delay = QmakeProFileNode::ParseLater);
    void scheduleAsyncUpdateLater() { scheduleAsyncUpdate(); }

protected:
    RestoreResult fromMap(const QVariantMap &map, QString *errorMessage) override;

private:
    void asyncUpdate();
    void buildFinished(bool success);
    void activeTargetWasChanged();

    QString executableFor(const QmakeProFileNode *node);
    void updateRunConfigurations();

    void updateCppCodeModel();
    void updateQmlJSCodeModel();

    static void collectAllProFiles(QList<QmakeProFileNode *> &list, QmakeProFileNode *node, Parsing parse,
                                   const QList<QmakeProjectManager::QmakeProjectType> &projectTypes);
    static void findProFile(const Utils::FileName &fileName, QmakeProFileNode *root, QList<QmakeProFileNode *> &list);
    static bool hasSubNode(QmakePriFileNode *root, const Utils::FileName &path);

    static bool equalFileList(const QStringList &a, const QStringList &b);

    void updateBuildSystemData();
    void collectData(const QmakeProFileNode *node, ProjectExplorer::DeploymentData &deploymentData);
    void collectApplicationData(const QmakeProFileNode *node,
                                ProjectExplorer::DeploymentData &deploymentData);
    void collectLibraryData(const QmakeProFileNode *node,
            ProjectExplorer::DeploymentData &deploymentData);
    void startAsyncTimer(QmakeProFileNode::AsyncUpdateDelay delay);
    bool matchesKit(const ProjectExplorer::Kit *kit);

    // Current configuration
    QString m_oldQtIncludePath;
    QString m_oldQtLibsPath;

    // cached lists of all of files
    Internal::QmakeProjectFiles *m_projectFiles = nullptr;

    QMakeVfs *m_qmakeVfs = nullptr;

    // cached data during project rescan
    ProFileGlobals *m_qmakeGlobals = nullptr;
    int m_qmakeGlobalsRefCnt = 0;

    QTimer m_asyncUpdateTimer;
    QFutureInterface<void> *m_asyncUpdateFutureInterface = nullptr;
    int m_pendingEvaluateFuturesCount = 0;
    AsyncUpdateState m_asyncUpdateState = Base;
    bool m_cancelEvaluate = false;
    QList<QmakeProFileNode *> m_partialEvaluate;

    QFuture<void> m_codeModelFuture;

    Internal::CentralizedFolderWatcher *m_centralizedFolderWatcher = nullptr;

    ProjectExplorer::Target *m_activeTarget = nullptr;

    friend class Internal::QmakeProjectFile;
    friend class Internal::QmakeProjectConfigWidget;
    friend class QmakeManager; // to schedule a async update if the unconfigured settings change
};

} // namespace QmakeProjectManager


#endif // QMAKEPROJECT_H
