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

#include <projectexplorer/extracompiler.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectmacro.h>
#include <projectexplorer/treescanner.h>

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
    explicit CMakeProject(const Utils::FilePath &filename);
    ~CMakeProject() final;

    ProjectExplorer::Tasks projectIssues(const ProjectExplorer::Kit *k) const final;

    void runCMake();
    void runCMakeAndScanProjectTree();

    // Context menu actions:
    void buildCMakeTarget(const QString &buildTarget);

    ProjectExplorer::ProjectImporter *projectImporter() const final;

    bool persistCMakeState();
    void clearCMakeCache();
    bool mustUpdateCMakeStateBeforeBuild();

    void checkAndReportError(QString &errorMessage) const;
    void reportError(const QString &errorMessage) const;

    void requestReparse(int reparseParameters);

protected:
    bool setupTarget(ProjectExplorer::Target *t) final;

private:
    void startParsing(int reparseParameters);

    void handleTreeScanningFinished();
    void handleParsingSuccess(Internal::CMakeBuildConfiguration *bc);
    void handleParsingError(Internal::CMakeBuildConfiguration *bc);
    void combineScanAndParse(Internal::CMakeBuildConfiguration *bc);
    void updateProjectData(Internal::CMakeBuildConfiguration *bc);
    void updateQmlJSCodeModel(Internal::CMakeBuildConfiguration *bc);

    QList<ProjectExplorer::ExtraCompiler *> findExtraCompilers() const;
    QStringList filesGeneratedFrom(const QString &sourceFile) const final;

    ProjectExplorer::DeploymentKnowledge deploymentKnowledge() const override;
    bool hasMakeInstallEquivalent() const override { return true; }
    ProjectExplorer::MakeInstallCommand makeInstallCommand(const ProjectExplorer::Target *target,
                                                           const QString &installRoot) override;

    CppTools::CppProjectUpdater *m_cppCodeModelUpdater = nullptr;
    QList<ProjectExplorer::ExtraCompiler *> m_extraCompilers;

    ProjectExplorer::TreeScanner m_treeScanner;

    bool m_waitingForScan = false;
    bool m_waitingForParse = false;
    bool m_combinedScanAndParseResult = false;

    QHash<QString, bool> m_mimeBinaryCache;
    QList<const ProjectExplorer::FileNode *> m_allFiles;
    mutable std::unique_ptr<Internal::CMakeProjectImporter> m_projectImporter;

    QTimer m_delayedParsingTimer;
    int m_delayedParsingParameters = 0;

    ParseGuard m_parseGuard;

    friend class Internal::CMakeBuildConfiguration;
    friend class Internal::CMakeBuildSettingsWidget;
};

} // namespace CMakeProjectManager
