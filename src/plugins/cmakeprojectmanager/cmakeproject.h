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
#include "cmakeprojectnodes.h"

#include <projectexplorer/extracompiler.h>
#include <projectexplorer/project.h>
#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/namedwidget.h>
#include <coreplugin/idocument.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>

#include <utils/fileutils.h>

#include <QFuture>
#include <QXmlStreamReader>
#include <QPushButton>
#include <QLineEdit>

QT_BEGIN_NAMESPACE
class QFileSystemWatcher;
QT_END_NAMESPACE

namespace CMakeProjectManager {

namespace Internal {
class BuildDirManager;
class CMakeFile;
class CMakeBuildSettingsWidget;
class CMakeBuildConfiguration;
class CMakeProjectNode;
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
    QString executable; // TODO: rename to output?
    TargetType targetType = UtilityType;
    QString workingDirectory;
    QString sourceDirectory;
    QString makeCommand;
    QString makeCleanCommand;

    // code model
    QStringList includeFiles;
    QStringList compilerOptions;
    QByteArray defines;
    QStringList files;

    void clear();
};

class CMAKE_EXPORT CMakeProject : public ProjectExplorer::Project
{
    Q_OBJECT
    // for changeBuildDirectory
    friend class Internal::CMakeBuildSettingsWidget;
public:
    CMakeProject(Internal::CMakeManager *manager, const Utils::FileName &filename);
    ~CMakeProject() override;

    QString displayName() const override;

    QStringList files(FilesMode fileMode) const override;
    QStringList buildTargetTitles(bool runnable = false) const;
    QList<CMakeBuildTarget> buildTargets() const;
    bool hasBuildTarget(const QString &title) const;

    CMakeBuildTarget buildTargetForTitle(const QString &title);

    bool needsConfiguration() const override;
    bool requiresTargetPanel() const override;
    bool knowsAllBuildExecutables() const override;

    bool supportsKit(ProjectExplorer::Kit *k, QString *errorMessage = 0) const override;

    void runCMake();

signals:
    /// emitted when cmake is running:
    void parsingStarted();

protected:
    RestoreResult fromMap(const QVariantMap &map, QString *errorMessage) override;
    bool setupTarget(ProjectExplorer::Target *t) override;

private:
    void handleCmakeFileChanged();

    void handleActiveTargetChanged();
    void handleActiveBuildConfigurationChanged();
    void handleParsingStarted();
    void parseCMakeOutput();
    void updateQmlJSCodeModel();

    void buildTree(Internal::CMakeProjectNode *rootNode, QList<ProjectExplorer::FileNode *> list);
    void gatherFileNodes(ProjectExplorer::FolderNode *parent, QList<ProjectExplorer::FileNode *> &list) const;
    ProjectExplorer::FolderNode *findOrCreateFolder(Internal::CMakeProjectNode *rootNode, QString directory);
    void createGeneratedCodeModelSupport();
    QStringList filesGeneratedFrom(const QString &sourceFile) const override;
    void updateTargetRunConfigurations(ProjectExplorer::Target *t);
    void updateApplicationAndDeploymentTargets();
    QStringList getCXXFlagsFor(const CMakeBuildTarget &buildTarget, QHash<QString, QStringList> &cache);
    bool extractCXXFlagsFromMake(const CMakeBuildTarget &buildTarget, QHash<QString, QStringList> &cache);
    bool extractCXXFlagsFromNinja(const CMakeBuildTarget &buildTarget, QHash<QString, QStringList> &cache);

    ProjectExplorer::Target *m_connectedTarget = nullptr;

    // TODO probably need a CMake specific node structure
    QList<CMakeBuildTarget> m_buildTargets;
    QFuture<void> m_codeModelFuture;
    QList<ProjectExplorer::ExtraCompiler *> m_extraCompilers;

    QSet<Internal::CMakeFile *> m_watchedFiles;

    friend class Internal::CMakeBuildConfiguration;
    friend class Internal::CMakeFile;
};

} // namespace CMakeProjectManager
