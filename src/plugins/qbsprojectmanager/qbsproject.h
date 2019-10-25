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

#include "qbsprojectmanager.h"

#include "qbsnodes.h"

#include <projectexplorer/extracompiler.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectnodes.h>
#include <projectexplorer/task.h>

#include <utils/environment.h>

#include <qbs.h>

#include <QHash>
#include <QTimer>

namespace CppTools { class CppProjectUpdater; }

namespace QbsProjectManager {
namespace Internal {

class QbsBuildConfiguration;
class QbsProjectParser;

class QbsProject : public ProjectExplorer::Project
{
    Q_OBJECT

public:
    explicit QbsProject(const Utils::FilePath &filename);
    ~QbsProject();

    ProjectExplorer::ProjectImporter *projectImporter() const override;

    ProjectExplorer::DeploymentKnowledge deploymentKnowledge() const override;

    void configureAsExampleProject() final;

    static QString uniqueProductName(const qbs::ProductData &product);

private:
    mutable ProjectExplorer::ProjectImporter *m_importer = nullptr;
};

class QbsBuildSystem : public ProjectExplorer::BuildSystem
{
    Q_OBJECT

public:
    explicit QbsBuildSystem(QbsBuildConfiguration *bc);
    ~QbsBuildSystem() final;

    void triggerParsing() final;
    bool supportsAction(ProjectExplorer::Node *context,
                        ProjectExplorer::ProjectAction action,
                        const ProjectExplorer::Node *node) const final;
    bool addFiles(ProjectExplorer::Node *context,
                  const QStringList &filePaths,
                  QStringList *notAdded = nullptr) final;
    ProjectExplorer::RemovedFilesFromProject removeFiles(ProjectExplorer::Node *context,
                                                         const QStringList &filePaths,
                                                         QStringList *notRemoved = nullptr) final;
    bool renameFile(ProjectExplorer::Node *context,
                    const QString &filePath, const QString &newFilePath) final;

    QStringList filesGeneratedFrom(const QString &sourceFile) const final;
    QVariant additionalData(Core::Id id) const final;

    bool isProjectEditable() const;
    // qbs::ProductData and qbs::GroupData are held by the nodes in the project tree.
    // These methods change those trees and invalidate the lot, so pass in copies of
    // the data we are interested in!
    // The overhead is not as big as it seems at first glance: These all are handles
    // for shared data.
    bool addFilesToProduct(const QStringList &filePaths, const qbs::ProductData productData,
                           const qbs::GroupData groupData, QStringList *notAdded);
    ProjectExplorer::RemovedFilesFromProject removeFilesFromProduct(const QStringList &filePaths,
            const qbs::ProductData &productData, const qbs::GroupData &groupData,
            QStringList *notRemoved);
    bool renameFileInProduct(const QString &oldPath,
            const QString &newPath, const qbs::ProductData productData,
            const qbs::GroupData groupData);

    qbs::BuildJob *build(const qbs::BuildOptions &opts, QStringList products, QString &error);
    qbs::CleanJob *clean(const qbs::CleanOptions &opts, const QStringList &productNames,
                         QString &error);
    qbs::InstallJob *install(const qbs::InstallOptions &opts);

    static ProjectExplorer::FileType fileTypeFor(const QSet<QString> &tags);

    QString profile() const;
    void parseCurrentBuildConfiguration();
    void scheduleParsing() { m_parsingScheduled = true; }
    bool parsingScheduled() const { return m_parsingScheduled; }
    void cancelParsing();
    void updateAfterBuild();

    qbs::Project qbsProject() const;
    qbs::ProjectData qbsProjectData() const;

    void generateErrors(const qbs::ErrorInfo &e);

    void delayParsing();

private:
    friend class QbsProject;
    void handleQbsParsingDone(bool success);

    void rebuildProjectTree();

    void changeActiveTarget(ProjectExplorer::Target *t);

    void prepareForParsing();
    void updateDocuments(const std::set<QString> &files);
    void updateCppCodeModel();
    void updateQmlJsCodeModel();
    void updateApplicationTargets();
    void updateDeploymentInfo();
    void updateBuildTargetData();
    void handleRuleExecutionDone();
    bool checkCancelStatus();
    void updateAfterParse();
    void delayedUpdateAfterParse();
    void updateProjectNodes();
    Utils::FilePath installRoot();

    static bool ensureWriteableQbsFile(const QString &file);

    template<typename Options> qbs::AbstractJob *buildOrClean(const Options &opts,
            const QStringList &productNames, QString &error);

    qbs::Project m_qbsProject;
    qbs::ProjectData m_projectData; // Cached m_qbsProject.projectData()
    Utils::Environment m_lastParseEnv;

    QbsProjectParser *m_qbsProjectParser = nullptr;

    QFutureInterface<bool> *m_qbsUpdateFutureInterface = nullptr;
    bool m_parsingScheduled = false;

    enum CancelStatus {
        CancelStatusNone,
        CancelStatusCancelingForReparse,
        CancelStatusCancelingAltoghether
    } m_cancelStatus = CancelStatusNone;

    CppTools::CppProjectUpdater *m_cppCodeModelUpdater = nullptr;

    QTimer m_parsingDelay;
    QList<ProjectExplorer::ExtraCompiler *> m_extraCompilers;
    bool m_extraCompilersPending = false;

    QHash<QString, Utils::Environment> m_envCache;

    ProjectExplorer::BuildSystem::ParseGuard m_guard;
    QbsBuildConfiguration *m_buildConfiguration = nullptr;
};

} // namespace Internal
} // namespace QbsProjectManager
