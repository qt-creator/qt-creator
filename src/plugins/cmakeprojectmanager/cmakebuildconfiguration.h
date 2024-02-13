// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "cmake_global.h"
#include "cmakeconfigitem.h"

#include <projectexplorer/buildaspects.h>
#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/environmentaspect.h>

#include <qtsupport/qtbuildaspects.h>

namespace CMakeProjectManager {
class CMakeProject;

namespace Internal {

class CMakeBuildSystem;
class CMakeBuildSettingsWidget;
class CMakeProjectImporter;

class InitialCMakeArgumentsAspect final : public Utils::StringAspect
{
public:
    InitialCMakeArgumentsAspect(Utils::AspectContainer *container);

    const CMakeConfig &cmakeConfiguration() const;
    const QStringList allValues() const;
    void setAllValues(const QString &values, QStringList &additionalArguments);
    void setCMakeConfiguration(const CMakeConfig &config);

    void fromMap(const Utils::Store &map) final;
    void toMap(Utils::Store &map) const final;

private:
    CMakeConfig m_cmakeConfiguration;
};

class ConfigureEnvironmentAspect final: public ProjectExplorer::EnvironmentAspect
{
public:
    ConfigureEnvironmentAspect(Utils::AspectContainer *container,
                               ProjectExplorer::BuildConfiguration *buildConfig);

    void fromMap(const Utils::Store &map) override;
    void toMap(Utils::Store &map) const override;
};

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

    void addToEnvironment(Utils::Environment &env) const override;

    Utils::Environment configureEnvironment() const;
    Internal::CMakeBuildSystem *cmakeBuildSystem() const;

    QStringList additionalCMakeArguments() const;
    void setAdditionalCMakeArguments(const QStringList &args);

    void setInitialCMakeArguments(const QStringList &args);
    void setCMakeBuildType(const QString &cmakeBuildType, bool quiet = false);

    Internal::InitialCMakeArgumentsAspect initialCMakeArguments{this};
    Utils::StringAspect additionalCMakeOptions{this};
    Utils::FilePathAspect sourceDirectory{this};
    Utils::StringAspect buildTypeAspect{this};
    QtSupport::QmlDebuggingAspect qmlDebugging{this};
    Internal::ConfigureEnvironmentAspect configureEnv{this, this};

signals:
    void signingFlagsChanged();
    void configureEnvironmentChanged();

private:
    BuildType buildType() const override;

    ProjectExplorer::NamedWidget *createConfigWidget() override;

    virtual CMakeConfig signingFlags() const;

    void setInitialBuildAndCleanSteps(const ProjectExplorer::Target *target);
    void setBuildPresetToBuildSteps(const ProjectExplorer::Target *target);
    void filterConfigArgumentsFromAdditionalCMakeArguments();

    Internal::CMakeBuildSystem *m_buildSystem = nullptr;

    friend class Internal::CMakeBuildSettingsWidget;
    friend class Internal::CMakeBuildSystem;
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

namespace Internal { void setupCMakeBuildConfiguration(); }

} // namespace CMakeProjectManager
