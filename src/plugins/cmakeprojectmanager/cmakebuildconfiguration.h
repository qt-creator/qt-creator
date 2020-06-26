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

#include <projectexplorer/buildaspects.h>
#include <projectexplorer/buildconfiguration.h>

namespace CMakeProjectManager {
class CMakeProject;

namespace Internal {

class CMakeBuildSystem;
class CMakeBuildSettingsWidget;

class CMakeBuildConfiguration final : public ProjectExplorer::BuildConfiguration
{
    Q_OBJECT

    friend class ProjectExplorer::BuildConfigurationFactory;
    CMakeBuildConfiguration(ProjectExplorer::Target *target, Utils::Id id);
    ~CMakeBuildConfiguration() final;

public:
    CMakeConfig configurationFromCMake() const;

    QStringList extraCMakeArguments() const;

    QStringList initialCMakeArguments() const;

    QString error() const;
    QString warning() const;

    static Utils::FilePath
    shadowBuildDirectory(const Utils::FilePath &projectFilePath, const ProjectExplorer::Kit *k,
                         const QString &bcName, BuildConfiguration::BuildType buildType);

    // Context menu action:
    void buildTarget(const QString &buildTarget);
    ProjectExplorer::BuildSystem *buildSystem() const final;

    void runCMakeWithExtraArguments();

signals:
    void errorOccurred(const QString &message);
    void warningOccurred(const QString &message);

private:
    QVariantMap toMap() const override;
    BuildType buildType() const override;

    ProjectExplorer::NamedWidget *createConfigWidget() override;

    bool fromMap(const QVariantMap &map) override;

    enum ForceEnabledChanged { False, True };
    void clearError(ForceEnabledChanged fec = ForceEnabledChanged::False);

    void setConfigurationFromCMake(const CMakeConfig &config);

    void setExtraCMakeArguments(const QStringList &args);
    void setInitialCMakeArguments(const QStringList &args);

    void setError(const QString &message);
    void setWarning(const QString &message);

    CMakeConfig m_initialConfiguration;
    QString m_error;
    QString m_warning;

    CMakeConfig m_configurationFromCMake;
    CMakeBuildSystem *m_buildSystem = nullptr;

    QStringList m_extraCMakeArguments;

    friend class CMakeBuildSettingsWidget;
    friend class CMakeBuildSystem;
    friend class CMakeProject;
};

class CMakeProjectImporter;

class CMakeBuildConfigurationFactory final : public ProjectExplorer::BuildConfigurationFactory
{
public:
    CMakeBuildConfigurationFactory();

    enum BuildType { BuildTypeNone = 0,
                     BuildTypeDebug = 1,
                     BuildTypeRelease = 2,
                     BuildTypeRelWithDebInfo = 3,
                     BuildTypeMinSizeRel = 4,
                     BuildTypeLast = 5 };
    static BuildType buildTypeFromByteArray(const QByteArray &in);
    static ProjectExplorer::BuildConfiguration::BuildType cmakeBuildTypeToBuildType(const BuildType &in);

private:
    static ProjectExplorer::BuildInfo createBuildInfo(BuildType buildType);

    friend class CMakeProjectImporter;
};

class InitialCMakeArgumentsAspect final : public ProjectExplorer::BaseStringAspect
{
    Q_OBJECT

public:
    InitialCMakeArgumentsAspect();
};

} // namespace Internal
} // namespace CMakeProjectManager
