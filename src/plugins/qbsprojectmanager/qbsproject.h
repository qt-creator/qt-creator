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

namespace CppEditor { class CppProjectUpdater; }

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
    explicit QbsBuildSystem(QbsBuildConfiguration *bc);
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
    bool renameFile(ProjectExplorer::Node *context,
                    const Utils::FilePath &oldFilePath, const Utils::FilePath &newFilePath) final;
    Utils::FilePaths filesGeneratedFrom(const Utils::FilePath &sourceFile) const final;
    QVariant additionalData(Utils::Id id) const final;
    QString name() const final { return QLatin1String("qbs"); }

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

    static ProjectExplorer::FileType fileTypeFor(const QSet<QString> &tags);

    QString profile() const;
    void scheduleParsing();
    void updateAfterBuild();

    QbsSession *session() const { return m_session; }
    QJsonObject projectData() const { return m_projectData; }

    void generateErrors(const ErrorInfo &e);

    void delayParsing();

private:
    friend class QbsProject;
    friend class QbsRequestObject;

    void startParsing();
    void cancelParsing();

    ProjectExplorer::ExtraCompiler *findExtraCompiler(
            const ExtraCompilerFilter &filter) const override;

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
    void updateAfterParse();
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
    std::unique_ptr<QbsRequest> m_parseRequest;

    CppEditor::CppProjectUpdater *m_cppCodeModelUpdater = nullptr;

    QHash<ProjectExplorer::ExtraCompilerFactory *, QStringList> m_sourcesForGeneratedFiles;
    QList<ProjectExplorer::ExtraCompiler *> m_extraCompilers;

    QHash<QString, Utils::Environment> m_envCache;

    ProjectExplorer::BuildSystem::ParseGuard m_guard;
    QbsBuildConfiguration *m_buildConfiguration = nullptr;
};

} // namespace Internal
} // namespace QbsProjectManager
