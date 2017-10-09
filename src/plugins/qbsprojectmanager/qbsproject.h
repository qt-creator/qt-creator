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

#include <cpptools/projectinfo.h>

#include <projectexplorer/extracompiler.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectnodes.h>
#include <projectexplorer/task.h>

#include <utils/environment.h>

#include <qbs.h>

#include <QHash>
#include <QTimer>

namespace Core { class IDocument; }
namespace CppTools { class CppProjectUpdater; }
namespace ProjectExplorer { class BuildConfiguration; }

namespace QbsProjectManager {
namespace Internal {

class QbsProjectParser;
class QbsBuildConfiguration;

class QbsProject : public ProjectExplorer::Project
{
    Q_OBJECT

public:
    explicit QbsProject(const Utils::FileName &filename);
    ~QbsProject() override;

    QbsRootProjectNode *rootProjectNode() const override;

    QStringList filesGeneratedFrom(const QString &sourceFile) const override;

    bool isProjectEditable() const;
    // qbs::ProductData and qbs::GroupData are held by the nodes in the project tree.
    // These methods change those trees and invalidate the lot, so pass in copies of
    // the data we are interested in!
    // The overhead is not as big as it seems at first glance: These all are handles
    // for shared data.
    bool addFilesToProduct(const QStringList &filePaths, const qbs::ProductData productData,
                           const qbs::GroupData groupData, QStringList *notAdded);
    bool removeFilesFromProduct(const QStringList &filePaths,
            const qbs::ProductData productData, const qbs::GroupData groupData,
            QStringList *notRemoved);
    bool renameFileInProduct(const QString &oldPath,
            const QString &newPath, const qbs::ProductData productData,
            const qbs::GroupData groupData);

    qbs::BuildJob *build(const qbs::BuildOptions &opts, QStringList products, QString &error);
    qbs::CleanJob *clean(const qbs::CleanOptions &opts, const QStringList &productNames,
                         QString &error);
    qbs::InstallJob *install(const qbs::InstallOptions &opts);

    static ProjectExplorer::FileType fileTypeFor(const QSet<QString> &tags);

    QString profileForTarget(const ProjectExplorer::Target *t) const;
    bool isParsing() const;
    bool hasParseResult() const;
    void parseCurrentBuildConfiguration();
    void scheduleParsing() { m_parsingScheduled = true; }
    bool parsingScheduled() const { return m_parsingScheduled; }
    void cancelParsing();
    void updateAfterBuild();

    void registerQbsProjectParser(QbsProjectParser *p);

    qbs::Project qbsProject() const;
    qbs::ProjectData qbsProjectData() const;

    bool needsSpecialDeployment() const override;
    void generateErrors(const qbs::ErrorInfo &e);

    static QString productDisplayName(const qbs::Project &project,
                                      const qbs::ProductData &product);
    static QString uniqueProductName(const qbs::ProductData &product);

    void configureAsExampleProject(const QSet<Core::Id> &platforms) final;

    void invalidate();
    void delayParsing();

private:
    void handleQbsParsingDone(bool success);

    void rebuildProjectTree();

    void changeActiveTarget(ProjectExplorer::Target *t);
    void buildConfigurationChanged(ProjectExplorer::BuildConfiguration *bc);
    void startParsing();

    void parse(const QVariantMap &config, const Utils::Environment &env, const QString &dir,
               const QString &configName);

    void prepareForParsing();
    void updateDocuments(const QSet<QString> &files);
    void updateCppCodeModel();
    void updateCppCompilerCallData();
    void updateQmlJsCodeModel();
    void updateApplicationTargets();
    void updateDeploymentInfo();
    void updateBuildTargetData();
    void handleRuleExecutionDone();
    bool checkCancelStatus();
    void updateAfterParse();
    void updateProjectNodes();

    void projectLoaded() override;
    ProjectExplorer::ProjectImporter *projectImporter() const override;
    bool needsConfiguration() const override { return targets().isEmpty(); }
    bool requiresTargetPanel() const override { return !targets().isEmpty(); }

    static bool ensureWriteableQbsFile(const QString &file);

    template<typename Options> qbs::AbstractJob *buildOrClean(const Options &opts,
            const QStringList &productNames, QString &error);

    QHash<ProjectExplorer::Target *, qbs::Project> m_qbsProjects;
    qbs::Project m_qbsProject;
    qbs::ProjectData m_projectData;
    QSet<Core::IDocument *> m_qbsDocuments;

    QbsProjectParser *m_qbsProjectParser;

    QFutureInterface<bool> *m_qbsUpdateFutureInterface;
    bool m_parsingScheduled;

    enum CancelStatus {
        CancelStatusNone,
        CancelStatusCancelingForReparse,
        CancelStatusCancelingAltoghether
    } m_cancelStatus;

    CppTools::CppProjectUpdater *m_cppCodeModelUpdater = nullptr;
    CppTools::ProjectInfo m_cppCodeModelProjectInfo;

    QbsBuildConfiguration *m_currentBc;
    mutable ProjectExplorer::ProjectImporter *m_importer = nullptr;

    QTimer m_parsingDelay;
    QList<ProjectExplorer::ExtraCompiler *> m_extraCompilers;
    bool m_extraCompilersPending;
};

} // namespace Internal
} // namespace QbsProjectManager
