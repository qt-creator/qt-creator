// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "projectexplorer_export.h"
#include "projectconfiguration.h"
#include "target.h"
#include "task.h"

#include <utils/environment.h>
#include <utils/filepath.h>

namespace Utils { class MacroExpander; }

namespace ProjectExplorer {

namespace Internal { class BuildConfigurationPrivate; }

class BuildDirectoryAspect;
class BuildInfo;
class BuildSystem;
class BuildStepList;
class DeployConfiguration;
class Kit;
class Node;
class Project;
class RunConfiguration;
class Target;

class PROJECTEXPLORER_EXPORT BuildConfiguration : public ProjectConfiguration
{
    Q_OBJECT

protected:
    friend class BuildConfigurationFactory;
    friend class Target;
    explicit BuildConfiguration(Target *target, Utils::Id id);

public:
    ~BuildConfiguration() override;

    Utils::FilePath buildDirectory() const;
    Utils::FilePath rawBuildDirectory() const;
    void setBuildDirectory(const Utils::FilePath &dir);

    BuildSystem *buildSystem() const;

    virtual QWidget *createConfigWidget();

    using WidgetAdder = std::function<void(QWidget *, const QString &)>;
    void addConfigWidgets(const WidgetAdder &adder);
    virtual void addSubConfigWidgets(const WidgetAdder &adder);

    // Maybe the BuildConfiguration is not the best place for the environment
    Utils::Environment baseEnvironment() const;
    QString baseEnvironmentText() const;
    Utils::Environment environment() const;
    void setUserEnvironmentChanges(const Utils::EnvironmentItems &diff);
    Utils::EnvironmentItems userEnvironmentChanges() const;
    bool useSystemEnvironment() const;
    void setUseSystemEnvironment(bool b);

    virtual void addToEnvironment(Utils::Environment &env) const;

    bool parseStdOut() const;
    void setParseStdOut(bool b);
    const QList<Utils::Id> customParsers() const;
    void setCustomParsers(const QList<Utils::Id> &parsers);

    BuildStepList *buildSteps() const;
    BuildStepList *cleanSteps() const;

    void appendInitialBuildStep(Utils::Id id);
    void appendInitialCleanStep(Utils::Id id);

    void addDeployConfiguration(DeployConfiguration *dc);
    bool removeDeployConfiguration(DeployConfiguration *dc);
    const QList<DeployConfiguration *> deployConfigurations() const;
    void setActiveDeployConfiguration(DeployConfiguration *dc);
    DeployConfiguration *activeDeployConfiguration() const;
    void setActiveDeployConfiguration(DeployConfiguration *dc, SetActive cascade);
    void updateDefaultDeployConfigurations();
    ProjectConfigurationModel *deployConfigurationModel() const;

    void updateDefaultRunConfigurations();
    const QList<RunConfiguration *> runConfigurations() const;
    void addRunConfiguration(RunConfiguration *rc);
    void removeRunConfiguration(RunConfiguration *rc);
    void removeAllRunConfigurations();
    RunConfiguration *activeRunConfiguration() const;
    void setActiveRunConfiguration(RunConfiguration *rc);
    ProjectConfigurationModel *runConfigurationModel() const;

    QVariant extraData(const Utils::Key &name) const;
    void setExtraData(const Utils::Key &name, const QVariant &value);

    virtual BuildConfiguration *clone(Target *target) const;
    void fromMap(const Utils::Store &map) override;
    void toMap(Utils::Store &map) const override;

    bool isEnabled() const;
    QString disabledReason() const;

    virtual bool regenerateBuildFiles(Node *node);

    virtual void restrictNextBuild(const RunConfiguration *rc);

    enum BuildType {
        Unknown,
        Debug,
        Profile,
        Release
    };
    virtual BuildType buildType() const;

    static QString buildTypeName(BuildType type);

    static void setupBuildDirMacroExpander(
        Utils::MacroExpander &exp,
        const Utils::FilePath &mainFilePath,
        const QString &projectName,
        const Kit *kit,
        const QString &bcName,
        BuildType buildType,
        const QString &buildSystem,
        bool documentationOnly);
    static Utils::FilePath buildDirectoryFromTemplate(const Utils::FilePath &projectDir,
                                                      const Utils::FilePath &mainFilePath,
                                                      const QString &projectName,
                                                      const Kit *kit,
                                                      const QString &bcName,
                                                      BuildType buildType,
                                                      const QString &buildSystem);

    bool isActive() const;
    QString activeBuildKey() const; // Build key of active run configuration

    void updateCacheAndEmitEnvironmentChanged();

    ProjectExplorer::BuildDirectoryAspect *buildDirectoryAspect() const;
    void setConfigWidgetDisplayName(const QString &display);
    void setBuildDirectoryHistoryCompleter(const Utils::Key &history);
    void setConfigWidgetHasFrame(bool configWidgetHasFrame);
    void setBuildDirectorySettingsKey(const Utils::Key &key);

    void doInitialize(const BuildInfo &info);

    bool createBuildDirectory();

    // For tools that need to manipulate the main build command's argument list
    virtual void setInitialArgs(const QStringList &);
    virtual QStringList initialArgs() const;
    virtual QStringList additionalArgs() const;

    virtual void reconfigure() {}
    virtual void stopReconfigure() {}

signals:
    void kitChanged();

    void environmentChanged();
    void buildDirectoryInitialized();
    void buildDirectoryChanged();
    void buildTypeChanged();

    void removedRunConfiguration(ProjectExplorer::RunConfiguration *rc);
    void addedRunConfiguration(ProjectExplorer::RunConfiguration *rc);
    void activeRunConfigurationChanged(ProjectExplorer::RunConfiguration *rc);
    void runConfigurationsUpdated();

    void removedDeployConfiguration(ProjectExplorer::DeployConfiguration *dc);
    void addedDeployConfiguration(ProjectExplorer::DeployConfiguration *dc);
    void activeDeployConfigurationChanged(ProjectExplorer::DeployConfiguration *dc);

protected:
    void setInitializer(const std::function<void(const BuildInfo &info)> &initializer);

private:
    bool addConfigurationsFromMap(const Utils::Store &map, bool setActiveConfigurations);
    void setExtraDataFromMap(const Utils::Store &map);
    void storeConfigurationsToMap(Utils::Store &map) const;

    void emitBuildDirectoryChanged();
    Internal::BuildConfigurationPrivate *d = nullptr;
};

class PROJECTEXPLORER_EXPORT BuildConfigurationFactory
{
protected:
    BuildConfigurationFactory();
    BuildConfigurationFactory(const BuildConfigurationFactory &) = delete;
    BuildConfigurationFactory &operator=(const BuildConfigurationFactory &) = delete;

    virtual ~BuildConfigurationFactory(); // Needed for dynamic_casts in importers.

public:
    // List of build information that can be used to create a new build configuration via
    // "Add Build Configuration" button.
    const QList<BuildInfo> allAvailableBuilds(const Target *parent) const;

    // List of build information that can be used to initially set up a new build configuration.
    const QList<BuildInfo>
        allAvailableSetups(const Kit *k, const Utils::FilePath &projectPath) const;

    BuildConfiguration *create(Target *parent, const BuildInfo &info) const;

    static BuildConfiguration *restore(Target *parent, const Utils::Store &map);

    static BuildConfigurationFactory *find(const Kit *k, const Utils::FilePath &projectPath);
    static BuildConfigurationFactory *find(Target *parent);

    using IssueReporter = std::function<
        Tasks(Kit *, const Utils::FilePath &projectDir, const Utils::FilePath &buildDirectory)>;
    void setIssueReporter(const IssueReporter &issueReporter);
    const Tasks reportIssues(ProjectExplorer::Kit *kit,
                             const Utils::FilePath &projectPath,
                             const Utils::FilePath &buildDir) const;

protected:
    using BuildGenerator
        = std::function<QList<BuildInfo>(const Kit *, const Utils::FilePath &, bool)>;
    void setBuildGenerator(const BuildGenerator &buildGenerator);

    bool supportsTargetDeviceType(Utils::Id id) const;
    void setSupportedProjectType(Utils::Id id);
    void setSupportedProjectMimeTypeName(const QString &mimeTypeName);
    void setSupportedProjectMimeTypeNames(const QStringList &mimeTypeNames);
    void addSupportedTargetDeviceType(Utils::Id id);
    void setDefaultDisplayName(const QString &defaultDisplayName);

    using BuildConfigurationCreator = std::function<BuildConfiguration *(Target *)>;

    template <class BuildConfig>
    void registerBuildConfiguration(Utils::Id buildConfigId)
    {
        m_creator = [buildConfigId](Target *t) { return new BuildConfig(t, buildConfigId); };
        m_buildConfigId = buildConfigId;
    }

private:
    bool canHandle(const ProjectExplorer::Target *t) const;

    BuildConfigurationCreator m_creator;
    Utils::Id m_buildConfigId;
    Utils::Id m_supportedProjectType;
    QList<Utils::Id> m_supportedTargetDeviceTypes;
    QStringList m_supportedProjectMimeTypeNames;
    IssueReporter m_issueReporter;
    BuildGenerator m_buildGenerator;
};

PROJECTEXPLORER_EXPORT BuildConfiguration *activeBuildConfig(const Project *project);
PROJECTEXPLORER_EXPORT BuildConfiguration *activeBuildConfigForActiveProject();
PROJECTEXPLORER_EXPORT BuildConfiguration *activeBuildConfigForCurrentProject();

} // namespace ProjectExplorer
