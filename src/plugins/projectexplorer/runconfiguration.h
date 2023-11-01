// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "buildtargetinfo.h"
#include "projectconfiguration.h"
#include "task.h"

#include <utils/aspects.h>
#include <utils/environment.h>
#include <utils/macroexpander.h>

#include <functional>
#include <memory>

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
    void setGlobalSettings(Utils::AspectContainer *settings);

    bool isUsingGlobalSettings() const { return m_useGlobalSettings; }
    void setUsingGlobalSettings(bool value);
    void resetProjectToGlobalSettings();

    Utils::AspectContainer *projectSettings() const { return m_projectSettings; }
    Utils::AspectContainer *currentSettings() const;

    struct Data : Utils::BaseAspect::Data
    {
        Utils::AspectContainer *currentSettings = nullptr;
    };

protected:
    friend class RunConfiguration;
    void fromMap(const Utils::Store &map) override;
    void toMap(Utils::Store &data) const override;
    void toActiveMap(Utils::Store &data) const override;

private:
    bool m_useGlobalSettings = false;
    Utils::AspectContainer *m_projectSettings = nullptr; // Owned if present.
    Utils::AspectContainer *m_globalSettings = nullptr;  // Not owned.
};

// Documentation inside.
class PROJECTEXPLORER_EXPORT RunConfiguration : public ProjectConfiguration
{
    Q_OBJECT

public:
    ~RunConfiguration() override;

    virtual QString disabledReason() const;
    virtual bool isEnabled() const;

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

    ProjectExplorer::ProjectNode *productNode() const;

    template <class T = Utils::AspectContainer> T *currentSettings(Utils::Id id) const
    {
        if (auto a = qobject_cast<GlobalOrProjectAspect *>(aspect(id)))
            return qobject_cast<T *>(a->currentSettings());
        return nullptr;
    }

    using ProjectConfiguration::registerAspect;
    using AspectFactory = std::function<Utils::BaseAspect *(Target *)>;
    template <class T> static void registerAspect()
    {
        addAspectFactory([](Target *target) { return new T(target); });
    }

    QMap<Utils::Id, Utils::Store> settingsData() const; // FIXME: Merge into aspectData?
    Utils::AspectContainerData aspectData() const;

    void update();

    const Utils::MacroExpander *macroExpander() const { return &m_expander; }

signals:
    void enabledChanged();

protected:
    RunConfiguration(Target *target, Utils::Id id);

    /// convenience function to get current build system. Try to avoid.
    BuildSystem *activeBuildSystem() const;

    using Updater = std::function<void()>;
    void setUpdater(const Updater &updater);

    Task createConfigurationIssue(const QString &description) const;

private:
    // Any additional data should be handled by aspects.
    void fromMap(const Utils::Store &map) final;
    void toMap(Utils::Store &map) const final;
    void toMapSimple(Utils::Store &map) const;

    static void addAspectFactory(const AspectFactory &aspectFactory);

    friend class RunConfigurationCreationInfo;
    friend class RunConfigurationFactory;
    friend class Target;

    QString m_buildKey;
    CommandLineGetter m_commandLineGetter;
    RunnableModifier m_runnableModifier;
    Updater m_updater;
    Utils::MacroExpander m_expander;
    Utils::Store m_pristineState;
    bool m_customized = false;
};

class RunConfigurationCreationInfo
{
public:
    enum CreationMode {AlwaysCreate, ManualCreationOnly};
    RunConfiguration *create(Target *target) const;

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

    static RunConfiguration *restore(Target *parent, const Utils::Store &map);
    static RunConfiguration *clone(Target *parent, RunConfiguration *source);
    static const QList<RunConfigurationCreationInfo> creatorsForTarget(Target *parent);

    Utils::Id runConfigurationId() const { return m_runConfigurationId; }

    static QString decoratedTargetName(const QString &targetName, Target *kit);

protected:
    virtual QList<RunConfigurationCreationInfo> availableCreators(Target *target) const;
    virtual bool supportsBuildKey(Target *target, const QString &key) const;

    using RunConfigurationCreator = std::function<RunConfiguration *(Target *)>;

    template <class RunConfig>
    void registerRunConfiguration(Utils::Id runConfigurationId)
    {
        m_creator = [runConfigurationId](Target *t) -> RunConfiguration * {
            return new RunConfig(t, runConfigurationId);
        };
        m_runConfigurationId = runConfigurationId;
    }

    void addSupportedProjectType(Utils::Id projectTypeId);
    void addSupportedTargetDeviceType(Utils::Id deviceTypeId);
    void setDecorateDisplayNames(bool on);

private:
    bool canHandle(Target *target) const;
    RunConfiguration *create(Target *target) const;

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
    QList<RunConfigurationCreationInfo> availableCreators(Target *parent) const override;
    bool supportsBuildKey(Target *target, const QString &key) const override;

    const QString m_fixedBuildTarget;
    const bool m_decorateTargetName;
};

} // namespace ProjectExplorer
