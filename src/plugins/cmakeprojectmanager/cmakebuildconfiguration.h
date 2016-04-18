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

#include "cmakeconfigitem.h"
#include "configmodel.h"

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/abi.h>

namespace ProjectExplorer { class ToolChain; }

namespace CMakeProjectManager {
class CMakeBuildInfo;
class CMakeProject;

namespace Internal {

class BuildDirManager;
class CMakeBuildConfigurationFactory;
class CMakeBuildSettingsWidget;

class CMakeBuildConfiguration : public ProjectExplorer::BuildConfiguration
{
    Q_OBJECT
    friend class CMakeBuildConfigurationFactory;

public:
    CMakeBuildConfiguration(ProjectExplorer::Target *parent);
    ~CMakeBuildConfiguration();

    bool isEnabled() const override;
    QString disabledReason() const override;

    ProjectExplorer::NamedWidget *createConfigWidget() override;

    QVariantMap toMap() const override;

    BuildType buildType() const override;

    void emitBuildTypeChanged();

    void setCMakeConfiguration(const CMakeConfig &config);
    bool hasCMakeConfiguration() const;
    CMakeConfig cmakeConfiguration() const;

    QString error() const;

    BuildDirManager *buildDirManager() const;

    bool isParsing() const;

    void maybeForceReparse();
    void resetData();
    bool persistCMakeState();

    static Utils::FileName
    shadowBuildDirectory(const Utils::FileName &projectFilePath, const ProjectExplorer::Kit *k,
                         const QString &bcName, BuildConfiguration::BuildType buildType);

signals:
    void errorOccured(const QString &message);

    void parsingStarted();
    void dataAvailable();

protected:
    CMakeBuildConfiguration(ProjectExplorer::Target *parent, CMakeBuildConfiguration *source);
    bool fromMap(const QVariantMap &map) override;

private:
    void ctor();
    QList<ConfigModel::DataItem> completeCMakeConfiguration() const;
    void setCurrentCMakeConfiguration(const QList<ConfigModel::DataItem> &items);

    void setError(const QString &message);

    CMakeConfig m_configuration;
    QString m_error;

    mutable QList<CMakeConfigItem> m_completeConfigurationCache;

    BuildDirManager *m_buildDirManager = nullptr;

    friend class CMakeBuildSettingsWidget;
    friend class CMakeProjectManager::CMakeProject;
};

class CMakeBuildConfigurationFactory : public ProjectExplorer::IBuildConfigurationFactory
{
    Q_OBJECT

public:
    CMakeBuildConfigurationFactory(QObject *parent = 0);

    int priority(const ProjectExplorer::Target *parent) const override;
    QList<ProjectExplorer::BuildInfo *> availableBuilds(const ProjectExplorer::Target *parent) const override;
    int priority(const ProjectExplorer::Kit *k, const QString &projectPath) const override;
    QList<ProjectExplorer::BuildInfo *> availableSetups(const ProjectExplorer::Kit *k,
                                                        const QString &projectPath) const override;
    ProjectExplorer::BuildConfiguration *create(ProjectExplorer::Target *parent,
                                                const ProjectExplorer::BuildInfo *info) const override;

    bool canClone(const ProjectExplorer::Target *parent, ProjectExplorer::BuildConfiguration *source) const override;
    CMakeBuildConfiguration *clone(ProjectExplorer::Target *parent, ProjectExplorer::BuildConfiguration *source) override;
    bool canRestore(const ProjectExplorer::Target *parent, const QVariantMap &map) const override;
    CMakeBuildConfiguration *restore(ProjectExplorer::Target *parent, const QVariantMap &map) override;

private:
    bool canHandle(const ProjectExplorer::Target *t) const;

    enum BuildType { BuildTypeNone = 0,
                     BuildTypeDebug = 1,
                     BuildTypeRelease = 2,
                     BuildTypeRelWithDebInfo = 3,
                     BuildTypeMinSizeRel = 4,
                     BuildTypeLast = 5 };

    CMakeBuildInfo *createBuildInfo(const ProjectExplorer::Kit *k,
                                    const QString &sourceDir,
                                    BuildType buildType) const;
};

} // namespace Internal
} // namespace CMakeProjectManager
