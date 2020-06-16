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

#include "qbsprofilemanager.h"

#include "qbsnodes.h"

#include <projectexplorer/extracompiler.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectnodes.h>
#include <projectexplorer/task.h>

#include <utils/environment.h>

#include <QFutureWatcher>
#include <QHash>
#include <QJsonObject>

#include <functional>

namespace CppTools { class CppProjectUpdater; }

namespace QbsProjectManager {
namespace Internal {

class ErrorInfo;
class QbsBuildConfiguration;
class QbsProjectParser;
class QbsSession;

class QbsProject : public ProjectExplorer::Project
{
    Q_OBJECT

public:
    explicit QbsProject(const Utils::FilePath &filename);
    ~QbsProject();

    ProjectExplorer::ProjectImporter *projectImporter() const override;

    ProjectExplorer::DeploymentKnowledge deploymentKnowledge() const override;

    void configureAsExampleProject(ProjectExplorer::Kit *kit) final;

private:
    mutable ProjectExplorer::ProjectImporter *m_importer = nullptr;
};

class QbsBuildSystem final : public ProjectExplorer::BuildSystem
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
    bool addFilesToProduct(const QStringList &filePaths,
            const QJsonObject &product,
            const QJsonObject &group,
            QStringList *notAdded);
    ProjectExplorer::RemovedFilesFromProject removeFilesFromProduct(const QStringList &filePaths,
            const QJsonObject &product,
            const QJsonObject &group,
            QStringList *notRemoved);
    bool renameFileInProduct(const QString &oldPath,
            const QString &newPath, const QJsonObject &product,
            const QJsonObject &group);

    static ProjectExplorer::FileType fileTypeFor(const QSet<QString> &tags);

    QString profile() const;
    void parseCurrentBuildConfiguration();
    void scheduleParsing() { m_parsingScheduled = true; }
    bool parsingScheduled() const { return m_parsingScheduled; }
    void cancelParsing();
    void updateAfterBuild();

    QbsSession *session() const { return m_session; }
    QJsonObject projectData() const { return m_projectData; }

    void generateErrors(const ErrorInfo &e);

    void delayParsing();

private:
    friend class QbsProject;

    void handleQbsParsingDone(bool success);
    void changeActiveTarget(ProjectExplorer::Target *t);
    void prepareForParsing();
    void updateDocuments();
    void updateCppCodeModel();
    void updateQmlJsCodeModel();
    void updateExtraCompilers();
    void updateApplicationTargets();
    void updateDeploymentInfo();
    void updateBuildTargetData();
    bool checkCancelStatus();
    void updateAfterParse();
    void delayedUpdateAfterParse();
    void updateProjectNodes(const std::function<void()> &continuation);
    Utils::FilePath installRoot();

    static bool ensureWriteableQbsFile(const QString &file);

    QbsSession * const m_session;
    QSet<Core::IDocument *> m_qbsDocuments;
    QJsonObject m_projectData; // TODO: Perhaps store this in the root project node instead?

    QbsProjectParser *m_qbsProjectParser = nullptr;
    QFutureInterface<bool> *m_qbsUpdateFutureInterface = nullptr;
    using TreeCreationWatcher = QFutureWatcher<QbsProjectNode *>;
    TreeCreationWatcher *m_treeCreationWatcher = nullptr;
    Utils::Environment m_lastParseEnv;
    bool m_parsingScheduled = false;

    enum CancelStatus {
        CancelStatusNone,
        CancelStatusCancelingForReparse,
        CancelStatusCancelingAltoghether
    } m_cancelStatus = CancelStatusNone;

    CppTools::CppProjectUpdater *m_cppCodeModelUpdater = nullptr;

    QHash<ProjectExplorer::ExtraCompilerFactory *, QStringList> m_sourcesForGeneratedFiles;
    QList<ProjectExplorer::ExtraCompiler *> m_extraCompilers;

    QHash<QString, Utils::Environment> m_envCache;

    ProjectExplorer::BuildSystem::ParseGuard m_guard;
    QbsBuildConfiguration *m_buildConfiguration = nullptr;
};

} // namespace Internal
} // namespace QbsProjectManager
