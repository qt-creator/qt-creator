// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include "cmake_global.h"
#include "cmakeconfigitem.h"

#include <projectexplorer/buildaspects.h>
#include <projectexplorer/buildconfiguration.h>

namespace CMakeProjectManager {
class CMakeProject;

namespace Internal {

class CMakeBuildSystem;
class CMakeBuildConfigurationPrivate;
class CMakeBuildSettingsWidget;
class CMakeProjectImporter;

} // namespace Internal

class CMAKE_EXPORT CMakeBuildConfiguration : public ProjectExplorer::BuildConfiguration
{
    Q_OBJECT

public:
    CMakeBuildConfiguration(ProjectExplorer::Target *target, Utils::Id id);
    ~CMakeBuildConfiguration() override;

    static Utils::FilePath
    shadowBuildDirectory(const Utils::FilePath &projectFilePath, const ProjectExplorer::Kit *k,
                         const QString &bcName, BuildConfiguration::BuildType buildType);
    static bool isIos(const ProjectExplorer::Kit *k);
    static bool hasQmlDebugging(const CMakeConfig &config);

    // Context menu action:
    void buildTarget(const QString &buildTarget);
    ProjectExplorer::BuildSystem *buildSystem() const final;

    void setSourceDirectory(const Utils::FilePath& path);
    Utils::FilePath sourceDirectory() const;

    void addToEnvironment(Utils::Environment &env) const override;

    Utils::Environment configureEnvironment() const;
    void setUserConfigureEnvironmentChanges(const Utils::EnvironmentItems &diff);
    Utils::EnvironmentItems userConfigureEnvironmentChanges() const;
    bool useClearConfigureEnvironment() const;
    void setUseClearConfigureEnvironment(bool b);
    void updateAndEmitConfigureEnvironmentChanged();

    Utils::Environment baseConfigureEnvironment() const;
    QString baseConfigureEnvironmentText() const;

signals:
    void signingFlagsChanged();
    void configureEnvironmentChanged();

protected:
    bool fromMap(const QVariantMap &map) override;

private:
    QVariantMap toMap() const override;
    BuildType buildType() const override;

    ProjectExplorer::NamedWidget *createConfigWidget() override;

    virtual CMakeConfig signingFlags() const;

    Internal::CMakeBuildSystem *m_buildSystem = nullptr;

    friend class Internal::CMakeBuildSettingsWidget;
    friend class Internal::CMakeBuildSystem;

    Internal::CMakeBuildConfigurationPrivate *d = nullptr;
};

class CMAKE_EXPORT CMakeBuildConfigurationFactory
    : public ProjectExplorer::BuildConfigurationFactory
{
public:
    CMakeBuildConfigurationFactory();

    enum BuildType {
        BuildTypeNone = 0,
        BuildTypeDebug = 1,
        BuildTypeRelease = 2,
        BuildTypeRelWithDebInfo = 3,
        BuildTypeProfile = 4,
        BuildTypeMinSizeRel = 5,
        BuildTypeLast = 6
    };
    static BuildType buildTypeFromByteArray(const QByteArray &in);
    static ProjectExplorer::BuildConfiguration::BuildType cmakeBuildTypeToBuildType(const BuildType &in);

private:
    static ProjectExplorer::BuildInfo createBuildInfo(BuildType buildType);

    friend class Internal::CMakeProjectImporter;
};

namespace Internal {

class InitialCMakeArgumentsAspect final : public Utils::StringAspect
{
    Q_OBJECT

    CMakeConfig m_cmakeConfiguration;
public:
    InitialCMakeArgumentsAspect();

    const CMakeConfig &cmakeConfiguration() const;
    const QStringList allValues() const;
    void setAllValues(const QString &values, QStringList &additionalArguments);
    void setCMakeConfiguration(const CMakeConfig &config);

    void fromMap(const QVariantMap &map) final;
    void toMap(QVariantMap &map) const final;
};

class AdditionalCMakeOptionsAspect final : public Utils::StringAspect
{
    Q_OBJECT

public:
    AdditionalCMakeOptionsAspect();
};

class SourceDirectoryAspect final : public Utils::StringAspect
{
    Q_OBJECT

public:
    SourceDirectoryAspect();
};

class BuildTypeAspect final : public Utils::StringAspect
{
    Q_OBJECT

public:
    BuildTypeAspect();
    using Utils::StringAspect::update;
};

} // namespace Internal
} // namespace CMakeProjectManager
