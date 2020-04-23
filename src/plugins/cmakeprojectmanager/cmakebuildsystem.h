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

#include "builddirmanager.h"

#include <projectexplorer/buildsystem.h>

namespace ProjectExplorer { class ExtraCompiler; }

namespace CppTools {
class CppProjectUpdater;
} // namespace CppTools

namespace CMakeProjectManager {
namespace Internal {

class CMakeBuildConfiguration;

// --------------------------------------------------------------------
// CMakeBuildSystem:
// --------------------------------------------------------------------

class CMakeBuildSystem final : public ProjectExplorer::BuildSystem
{
    Q_OBJECT

public:
    explicit CMakeBuildSystem(CMakeBuildConfiguration *bc);
    ~CMakeBuildSystem() final;

    void triggerParsing() final;

    bool supportsAction(ProjectExplorer::Node *context,
                        ProjectExplorer::ProjectAction action,
                        const ProjectExplorer::Node *node) const override;

    bool addFiles(ProjectExplorer::Node *context,
                  const QStringList &filePaths, QStringList *) final;

    QStringList filesGeneratedFrom(const QString &sourceFile) const final;

    // Actions:
    void runCMake();
    void runCMakeAndScanProjectTree();

    bool persistCMakeState();
    void clearCMakeCache();

    // Context menu actions:
    void buildCMakeTarget(const QString &buildTarget);

    // Queries:
    const QList<ProjectExplorer::BuildTargetInfo> appTargets() const;
    QStringList buildTargetTitles() const;
    const QList<CMakeBuildTarget> &buildTargets() const;
    ProjectExplorer::DeploymentData deploymentData() const;

    CMakeBuildConfiguration *cmakeBuildConfiguration() const;

private:
    // Parser states:
    void handleParsingSuccess();
    void handleParsingError();

    // Treescanner states:
    void handleTreeScanningFinished();

    // Combining Treescanner and Parser states:
    void combineScanAndParse();

    std::unique_ptr<CMakeProjectNode> generateProjectTree(
        const QList<const ProjectExplorer::FileNode *> &allFiles);

    void checkAndReportError(QString &errorMessage);

    void updateProjectData();
    QList<ProjectExplorer::ExtraCompiler *> findExtraCompilers();
    void updateQmlJSCodeModel();

    void handleParsingSucceeded();
    void handleParsingFailed(const QString &msg);

    void wireUpConnections(const ProjectExplorer::Project *p);

    BuildDirManager m_buildDirManager;

    ProjectExplorer::TreeScanner m_treeScanner;
    QHash<QString, bool> m_mimeBinaryCache;
    QList<const ProjectExplorer::FileNode *> m_allFiles;

    bool m_waitingForScan = false;
    bool m_waitingForParse = false;
    bool m_combinedScanAndParseResult = false;

    ParseGuard m_currentGuard;

    CppTools::CppProjectUpdater *m_cppCodeModelUpdater = nullptr;
    QList<ProjectExplorer::ExtraCompiler *> m_extraCompilers;
    QList<CMakeBuildTarget> m_buildTargets;
};

} // namespace Internal
} // namespace CMakeProjectManager
