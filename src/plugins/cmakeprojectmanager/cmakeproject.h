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

#include "cmake_global.h"

#include "builddirmanager.h"
#include "cmakebuildtarget.h"
#include "cmakeprojectimporter.h"
#include "treescanner.h"

#include <projectexplorer/extracompiler.h>
#include <projectexplorer/projectmacro.h>
#include <projectexplorer/project.h>

#include <utils/fileutils.h>

#include <QFuture>
#include <QHash>
#include <QTimer>

#include <memory>

namespace CppTools { class CppProjectUpdater; }
namespace ProjectExplorer { class FileNode; }

namespace CMakeProjectManager {

namespace Internal {
class CMakeBuildConfiguration;
class CMakeBuildSettingsWidget;
class CMakeProjectNode;
} // namespace Internal

class CMAKE_EXPORT CMakeProject : public ProjectExplorer::Project
{
    Q_OBJECT

public:
    explicit CMakeProject(const Utils::FileName &filename);
    ~CMakeProject() final;

    QStringList buildTargetTitles(bool runnable = false) const;
    bool hasBuildTarget(const QString &title) const;

    CMakeBuildTarget buildTargetForTitle(const QString &title);

    bool needsConfiguration() const final;
    bool requiresTargetPanel() const final;
    bool knowsAllBuildExecutables() const final;

    bool supportsKit(ProjectExplorer::Kit *k, QString *errorMessage = 0) const final;

    void runCMake();
    void runCMakeAndScanProjectTree();

    // Context menu actions:
    void buildCMakeTarget(const QString &buildTarget);

    ProjectExplorer::ProjectImporter *projectImporter() const final;

    bool persistCMakeState();
    void clearCMakeCache();
    bool mustUpdateCMakeStateBeforeBuild();

protected:
    RestoreResult fromMap(const QVariantMap &map, QString *errorMessage) final;
    bool setupTarget(ProjectExplorer::Target *t) final;

private:
    QList<CMakeBuildTarget> buildTargets() const;

    void handleReparseRequest(int reparseParameters);

    void startParsing(int reparseParameters);

    void handleTreeScanningFinished();
    void handleParsingSuccess(Internal::CMakeBuildConfiguration *bc);
    void handleParsingError(Internal::CMakeBuildConfiguration *bc);
    void combineScanAndParse(Internal::CMakeBuildConfiguration *bc);
    void updateProjectData(Internal::CMakeBuildConfiguration *bc);
    void updateQmlJSCodeModel();

    Internal::CMakeProjectNode *
    generateProjectTree(const QList<const ProjectExplorer::FileNode*> &allFiles) const;

    void createGeneratedCodeModelSupport();
    QStringList filesGeneratedFrom(const QString &sourceFile) const final;
    void updateTargetRunConfigurations(ProjectExplorer::Target *t);
    void updateApplicationAndDeploymentTargets();

    // TODO probably need a CMake specific node structure
    QList<CMakeBuildTarget> m_buildTargets;
    CppTools::CppProjectUpdater *m_cppCodeModelUpdater = nullptr;
    QList<ProjectExplorer::ExtraCompiler *> m_extraCompilers;

    Internal::TreeScanner m_treeScanner;
    Internal::BuildDirManager m_buildDirManager;

    bool m_waitingForScan = false;
    bool m_waitingForParse = false;
    bool m_combinedScanAndParseResult = false;

    QHash<QString, bool> m_mimeBinaryCache;
    QList<const ProjectExplorer::FileNode *> m_allFiles;
    mutable std::unique_ptr<Internal::CMakeProjectImporter> m_projectImporter;

    QTimer m_delayedParsingTimer;
    int m_delayedParsingParameters = 0;

    friend class Internal::CMakeBuildConfiguration;
    friend class Internal::CMakeBuildSettingsWidget;
};

} // namespace CMakeProjectManager
