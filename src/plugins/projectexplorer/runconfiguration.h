// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "buildtargetinfo.h"
#include "projectconfiguration.h"
#include "task.h"

#include <utils/aspects.h>
#include <utils/macroexpander.h>

#include <functional>

namespace Utils {
class OutputFormatter;
class ProcessRunData;
}

namespace ProjectExplorer {
class BuildConfiguration;
class BuildSystem;
class GlobalOrProjectAspect;
class ProjectNode;
class RunConfigurationFactory;
class RunConfiguration;
class RunConfigurationCreationInfo;
class Target;
class BuildTargetInfo;

/**
 * Contains start program entries that are retrieved
 * from the cmake file api
 */
class LauncherInfo
{
public:
    QString type;
    Utils::FilePath command;
    QStringList arguments;
};

/**
 * Contains a start program entry that is displayed in the run configuration interface.
 *
 * This follows the design for the use of "Test Launcher", the
 * Wrappers for running executables on the host system and "Emulator",
 * wrappers for cross-compiled applications, which are supported for
 * example by the cmake build system.
 */
class PROJECTEXPLORER_EXPORT Launcher
{
public:
    Launcher() = default;

    /// Create a single launcher from the \p launcherInfo parameter, which can be of type "Test launcher" or "Emulator"
    Launcher(const LauncherInfo &launcherInfo,  const Utils::FilePath &sourceDirectory);

    /// Create a combined launcher from the passed info parameters, with \p testLauncherInfo
    /// as first and \p emulatorLauncherInfo appended
    Launcher(const LauncherInfo &testLauncherInfo, const LauncherInfo &emulatorlauncherInfo, const Utils::FilePath &sourceDirectory);

    bool operator==(const Launcher &other) const
    {
        return id == other.id && displayName == other.displayName && command == other.command
               && arguments == other.arguments;
    }

    QString id;
    QString displayName;
    Utils::FilePath command;
    QStringList arguments;
};

/**
 * An interface to facilitate switching between hunks of
 * global and per-project configuration data.
 *
 */

class PROJECTEXPLORER_EXPORT GlobalOrProjectAspect : public Utils::BaseAspect
{
    Q_OBJECT

public:
    GlobalOrProjectAspect();
    ~GlobalOrProjectAspect() override;

    void setProjectSettings(Utils::AspectContainer *settings);
    void setGlobalSettings(Utils::AspectContainer *settings, Utils::Id settingsPage);

    bool isUsingGlobalSettings() const { return m_useGlobalSettings; }
    void setUsingGlobalSettings(bool value);
    void resetProjectToGlobalSettings();

    Utils::AspectContainer *projectSettings() const { return m_projectSettings; }
    Utils::AspectContainer *currentSettings() const;
    Utils::Id settingsPage() const { return m_settingsPage; }

    struct Data : Utils::BaseAspect::Data
    {
        Utils::AspectContainer *currentSettings = nullptr;
    };

signals:
    void currentSettingsChanged();
    void wasResetToGlobalValues();

protected:
    friend class RunConfiguration;
    void fromMap(const Utils::Store &map) override;
    void toMap(Utils::Store &data) const override;
    void toActiveMap(Utils::Store &data) const override;

private:
    bool m_useGlobalSettings = false;
    Utils::AspectContainer *m_projectSettings = nullptr; // Owned if present.
    Utils::AspectContainer *m_globalSettings = nullptr;  // Not owned.
    Utils::Id m_settingsPage;
};

PROJECTEXPLORER_EXPORT QWidget *createGlobalOrProjectAspectWidget(GlobalOrProjectAspect *aspect);
PROJECTEXPLORER_EXPORT QWidget *createRunConfigAspectWidget(GlobalOrProjectAspect *aspect);

// Documentation inside.
class PROJECTEXPLORER_EXPORT RunConfiguration : public ProjectConfiguration
{
    Q_OBJECT

public:
    ~RunConfiguration() override;

    virtual QString disabledReason(Utils::Id runMode) const;
    virtual bool isEnabled(Utils::Id runMode) const;

    QWidget *createConfigurationWidget();

    bool isConfigured() const;
    bool isCustomized() const;
    bool hasCreator() const;
    virtual Tasks checkForIssues() const { return {}; }
    void setPristineState();

    using CommandLineGetter = std::function<Utils::CommandLine()>;
    void setCommandLineGetter(const CommandLineGetter &cmdGetter);
    Utils::CommandLine commandLine() const;
    bool isPrintEnvironmentEnabled() const;

    using RunnableModifier = std::function<void(Utils::ProcessRunData &)>;
    void setRunnableModifier(const RunnableModifier &extraModifier);

    virtual Utils::ProcessRunData runnable() const;
    virtual QVariantHash extraData() const;

    // Return a handle to the build system target that created this run configuration.
    // May return an empty string if no target built the executable!
    QString buildKey() const { return m_buildKey; }
    // The BuildTargetInfo corresponding to the buildKey.
    BuildTargetInfo buildTargetInfo() const;

    // An identifier for the purpose of syncing this run config with its counterparts
    // in other build configurations.
    // For auto-created run configurations, this is the build key.
    QString uniqueId() const;
    void setUniqueId(const QString &id);

    ProjectExplorer::ProjectNode *productNode() const;

    template <class T = Utils::AspectContainer> T *currentSettings(Utils::Id id) const
    {
        if (auto a = qobject_cast<GlobalOrProjectAspect *>(aspect(id)))
            return qobject_cast<T *>(a->currentSettings());
        return nullptr;
    }

    using ProjectConfiguration::registerAspect;
    using AspectFactory = std::function<Utils::BaseAspect *(BuildConfiguration *)>;
    template <class T> static void registerAspect()
    {
        addAspectFactory([](BuildConfiguration *bc) { return new T(bc); });
    }

    QMap<Utils::Id, Utils::Store> settingsData() const; // FIXME: Merge into aspectData?
    Utils::AspectContainerData aspectData() const;

    void update();

    virtual RunConfiguration *clone(BuildConfiguration *bc);
    void cloneFromOther(const RunConfiguration *rc);

    BuildConfiguration *buildConfiguration() const { return m_buildConfiguration; }
    const QList<BuildConfiguration *> syncableBuildConfigurations() const;
    void forEachLinkedRunConfig(const std::function<void(RunConfiguration *)> &handler);
    void makeActive();

    BuildSystem *buildSystem() const;

    bool equals(const RunConfiguration *other) const;

    static void setupMacroExpander(
        Utils::MacroExpander &exp, const RunConfiguration *rc, bool documentationOnly);

protected:
    RunConfiguration(BuildConfiguration *bc, Utils::Id id);

    using Updater = std::function<void()>;
    void setUpdater(const Updater &updater);

    Task createConfigurationIssue(const QString &description) const;

    void setUsesEmptyBuildKeys() { m_usesEmptyBuildKeys = true; }

private:
    // Any additional data should be handled by aspects.
    void fromMap(const Utils::Store &map) final;
    void toMap(Utils::Store &map) const final;
    void toMapSimple(Utils::Store &map) const;

    static void addAspectFactory(const AspectFactory &aspectFactory);

    friend class BuildConfiguration;
    friend class RunConfigurationCreationInfo;
    friend class RunConfigurationFactory;
    friend class Target;

    BuildConfiguration * const m_buildConfiguration;
    QString m_buildKey;
    CommandLineGetter m_commandLineGetter;
    RunnableModifier m_runnableModifier;
    Updater m_updater;
    Utils::Store m_pristineState;
    QString m_uniqueId;
    bool m_customized = false;
    bool m_usesEmptyBuildKeys = false;
};

class RunConfigurationCreationInfo
{
public:
    enum CreationMode {AlwaysCreate, ManualCreationOnly};
    RunConfiguration *create(BuildConfiguration *bc) const;

    const RunConfigurationFactory *factory = nullptr;
    QString buildKey;
    QString displayName;
    QString displayNameUniquifier;
    Utils::FilePath projectFilePath;
    CreationMode creationMode = AlwaysCreate;
    bool useTerminal = false;
};

class PROJECTEXPLORER_EXPORT RunConfigurationFactory
{
public:
    RunConfigurationFactory();
    RunConfigurationFactory(const RunConfigurationFactory &) = delete;
    RunConfigurationFactory operator=(const RunConfigurationFactory &) = delete;
    virtual ~RunConfigurationFactory();

    static RunConfiguration *restore(BuildConfiguration *bc, const Utils::Store &map);
    static const QList<RunConfigurationCreationInfo> creatorsForBuildConfig(BuildConfiguration *bc);

    Utils::Id runConfigurationId() const { return m_runConfigurationId; }
    static QString decoratedTargetName(const QString &targetName, Kit *kit);

protected:
    virtual QList<RunConfigurationCreationInfo> availableCreators(BuildConfiguration *bc) const;
    virtual bool supportsBuildKey(BuildConfiguration *bc, const QString &key) const;

    using RunConfigurationCreator = std::function<RunConfiguration *(BuildConfiguration *)>;

    template <class RunConfig>
    void registerRunConfiguration(Utils::Id runConfigurationId)
    {
        m_creator = [runConfigurationId](BuildConfiguration *bc) -> RunConfiguration * {
            return new RunConfig(bc, runConfigurationId);
        };
        m_runConfigurationId = runConfigurationId;
    }

    void addSupportedProjectType(Utils::Id projectTypeId);
    void addSupportedTargetDeviceType(Utils::Id deviceTypeId);
    void setDecorateDisplayNames(bool on);

private:
    bool canHandle(Target *target) const;
    RunConfiguration *create(BuildConfiguration *bc) const;

    friend class RunConfigurationCreationInfo;
    friend class RunConfiguration;
    RunConfigurationCreator m_creator;
    Utils::Id m_runConfigurationId;
    QList<Utils::Id> m_supportedProjectTypes;
    QList<Utils::Id> m_supportedTargetDeviceTypes;
    bool m_decorateDisplayNames = false;
};

class PROJECTEXPLORER_EXPORT FixedRunConfigurationFactory : public RunConfigurationFactory
{
public:
    explicit FixedRunConfigurationFactory(const QString &displayName,
                                          bool addDeviceName = false);

private:
    QList<RunConfigurationCreationInfo> availableCreators(BuildConfiguration *bc) const override;
    bool supportsBuildKey(BuildConfiguration *bc, const QString &key) const override;

    const QString m_fixedBuildTarget;
    const bool m_decorateTargetName;
};

PROJECTEXPLORER_EXPORT RunConfiguration *activeRunConfig(const Project *project);
PROJECTEXPLORER_EXPORT RunConfiguration *activeRunConfigForActiveProject();
PROJECTEXPLORER_EXPORT RunConfiguration *activeRunConfigForCurrentProject();

} // namespace ProjectExplorer
