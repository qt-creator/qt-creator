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
#include "cmakebuildsystem.h"
#include "cmakebuildtarget.h"
#include "cmakeprojectimporter.h"

#include <projectexplorer/buildsystem.h>
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
    bool mustUpdateCMakeStateBeforeBuild() const;

protected:
    bool setupTarget(ProjectExplorer::Target *t) final;

private:
    QStringList filesGeneratedFrom(const QString &sourceFile) const final;

    ProjectExplorer::DeploymentKnowledge deploymentKnowledge() const override;
    ProjectExplorer::MakeInstallCommand makeInstallCommand(const ProjectExplorer::Target *target,
                                                           const QString &installRoot) override;

    mutable std::unique_ptr<Internal::CMakeProjectImporter> m_projectImporter;

    friend class CMakeBuildSystem;
};

} // namespace CMakeProjectManager
