// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qbsprofilemanager.h"

#include "qbsnodes.h"

#include <projectexplorer/extracompiler.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectnodes.h>

#include <utils/environment.h>
#include <utils/id.h>

#include <QFutureWatcher>
#include <QHash>
#include <QJsonObject>

#include <functional>

namespace ProjectExplorer { class ProjectUpdater; }

namespace QbsProjectManager {
namespace Internal {

class ErrorInfo;
class QbsBuildConfiguration;
class QbsProjectParser;
class QbsRequest;
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
    explicit QbsBuildSystem(ProjectExplorer::BuildConfiguration *bc);
    ~QbsBuildSystem() final;

    void triggerParsing() final;
    bool supportsAction(ProjectExplorer::Node *context,
                        ProjectExplorer::ProjectAction action,
                        const ProjectExplorer::Node *node) const final;
    bool addFiles(ProjectExplorer::Node *context,
                  const Utils::FilePaths &filePaths,
                  Utils::FilePaths *notAdded = nullptr) final;
    ProjectExplorer::RemovedFilesFromProject removeFiles(ProjectExplorer::Node *context,
                                                         const Utils::FilePaths &filePaths,
                                                         Utils::FilePaths *notRemoved = nullptr) final;
    bool renameFiles(
        ProjectExplorer::Node *context,
        const Utils::FilePairs &filesToRename,
        Utils::FilePaths *notRenamed) final;
    bool addDependencies(ProjectExplorer::Node *context, const QStringList &dependencies) final;
    Utils::FilePaths filesGeneratedFrom(const Utils::FilePath &sourceFile) const final;
    QVariant additionalData(Utils::Id id) const final;

    bool isProjectEditable() const;
    bool addFilesToProduct(const Utils::FilePaths &filePaths,
            const QJsonObject &product,
            const QJsonObject &group,
            Utils::FilePaths *notAdded);
    ProjectExplorer::RemovedFilesFromProject removeFilesFromProduct(const Utils::FilePaths &filePaths,
            const QJsonObject &product,
            const QJsonObject &group,
            Utils::FilePaths *notRemoved);
    bool renameFileInProduct(const QString &oldPath,
            const QString &newPath, const QJsonObject &product,
            const QJsonObject &group);
    bool renameFilesInProduct(
        const Utils::FilePairs &files,
        const QJsonObject &product,
        const QJsonObject &group,
        Utils::FilePaths *notRenamed);
    bool addDependenciesToProduct(
        const QStringList &deps, const QJsonObject &product, const QJsonObject &group);

    static ProjectExplorer::FileType fileTypeFor(const QSet<QString> &tags);

    QString profile() const;
    void scheduleParsing(const QVariantMap &extraConfig);
    void updateAfterBuild();

    QbsSession *session() const { return m_session; }
    QJsonObject projectData() const { return m_projectData; }

    void generateErrors(const ErrorInfo &e);

    void delayParsing();

private:
    friend class QbsProject;
    friend class QbsRequestObject;

    void startParsing(const QVariantMap &extraConfig);
    void cancelParsing();

    ProjectExplorer::ExtraCompiler *findExtraCompiler(
            const ExtraCompilerFilter &filter) const override;

    void handleQbsParsingDone(bool success);
    void prepareForParsing();
    void updateDocuments();
    void updateCppCodeModel();
    void updateExtraCompilers();
    void updateApplicationTargets();
    void updateDeploymentInfo();
    void updateBuildTargetData();
    void updateAfterParse();
    void updateProjectNodes(const std::function<void()> &continuation);
    Utils::FilePath installRoot();
    Utils::FilePath locationFilePath(const QJsonObject &loc) const;
    Utils::FilePath groupFilePath(const QJsonObject &group) const;
    QbsBuildConfiguration *qbsBuildConfig() const;

    void updateQmlCodeModelInfo(ProjectExplorer::QmlCodeModelInfo &projectInfo) final;

    static bool ensureWriteableQbsFile(const Utils::FilePath &file);

    QbsSession * const m_session;
    QSet<Core::IDocument *> m_qbsDocuments;
    QJsonObject m_projectData; // TODO: Perhaps store this in the root project node instead?

    QbsProjectParser *m_qbsProjectParser = nullptr;
    using TreeCreationWatcher = QFutureWatcher<QbsProjectNode *>;
    TreeCreationWatcher *m_treeCreationWatcher = nullptr;
    Utils::Environment m_lastParseEnv;
    std::unique_ptr<QbsRequest> m_parseRequest;

    ProjectExplorer::ProjectUpdater *m_cppCodeModelUpdater = nullptr;

    QHash<ProjectExplorer::ExtraCompilerFactory *, QStringList> m_sourcesForGeneratedFiles;
    QList<ProjectExplorer::ExtraCompiler *> m_extraCompilers;

    QHash<QString, Utils::Environment> m_envCache;

    ProjectExplorer::BuildSystem::ParseGuard m_guard;
};

} // namespace Internal
} // namespace QbsProjectManager
