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
#include "cmakeprojectimporter.h"
#include "treescanner.h"

#include <projectexplorer/extracompiler.h>
#include <projectexplorer/project.h>

#include <utils/fileutils.h>

#include <QFuture>
#include <QHash>

#include <memory>

QT_BEGIN_NAMESPACE
class QFileSystemWatcher;
QT_END_NAMESPACE

namespace CMakeProjectManager {

namespace Internal {
class CMakeBuildConfiguration;
class CMakeBuildSettingsWidget;
class CMakeManager;
} // namespace Internal

enum TargetType {
    ExecutableType = 0,
    StaticLibraryType = 2,
    DynamicLibraryType = 3,
    UtilityType = 64
};

class CMAKE_EXPORT CMakeBuildTarget
{
public:
    QString title;
    Utils::FileName executable; // TODO: rename to output?
    TargetType targetType = UtilityType;
    Utils::FileName workingDirectory;
    Utils::FileName sourceDirectory;
    Utils::FileName makeCommand;

    // code model
    QList<Utils::FileName> includeFiles;
    QStringList compilerOptions;
    QByteArray defines;
    QList<Utils::FileName> files;

    void clear();
};

class CMAKE_EXPORT CMakeProject : public ProjectExplorer::Project
{
    Q_OBJECT

public:
    CMakeProject(Internal::CMakeManager *manager, const Utils::FileName &filename);
    ~CMakeProject() final;

    QString displayName() const final;

    QStringList files(FilesMode fileMode) const final;
    QStringList buildTargetTitles(bool runnable = false) const;
    bool hasBuildTarget(const QString &title) const;

    CMakeBuildTarget buildTargetForTitle(const QString &title);

    bool needsConfiguration() const final;
    bool requiresTargetPanel() const final;
    bool knowsAllBuildExecutables() const final;

    bool supportsKit(ProjectExplorer::Kit *k, QString *errorMessage = 0) const final;

    void runCMake();
    void scanProjectTree();

    // Context menu actions:
    void buildCMakeTarget(const QString &buildTarget);

    ProjectExplorer::ProjectImporter *projectImporter() const final;

signals:
    /// emitted when cmake is running:
    void parsingStarted();

protected:
    RestoreResult fromMap(const QVariantMap &map, QString *errorMessage) final;
    bool setupTarget(ProjectExplorer::Target *t) final;

private:
    QList<CMakeBuildTarget> buildTargets() const;

    void handleActiveTargetChanged();
    void handleActiveBuildConfigurationChanged();
    void handleParsingStarted();
    void handleTreeScanningFinished();
    void updateProjectData(Internal::CMakeBuildConfiguration *cmakeBc);
    void updateQmlJSCodeModel();

    void createGeneratedCodeModelSupport();
    QStringList filesGeneratedFrom(const QString &sourceFile) const final;
    void updateTargetRunConfigurations(ProjectExplorer::Target *t);
    void updateApplicationAndDeploymentTargets();

    ProjectExplorer::Target *m_connectedTarget = nullptr;

    // TODO probably need a CMake specific node structure
    QList<CMakeBuildTarget> m_buildTargets;
    QFuture<void> m_codeModelFuture;
    QList<ProjectExplorer::ExtraCompiler *> m_extraCompilers;

    Internal::TreeScanner m_treeScanner;
    QHash<QString, bool> m_mimeBinaryCache;
    QList<const ProjectExplorer::FileNode *> m_allFiles;
    mutable std::unique_ptr<Internal::CMakeProjectImporter> m_projectImporter;

    friend class Internal::CMakeBuildConfiguration;
    friend class Internal::CMakeBuildSettingsWidget;
};

} // namespace CMakeProjectManager
