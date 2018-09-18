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

#include "projectconfiguration.h"
#include "projectexplorerconstants.h"
#include "applicationlauncher.h"
#include "buildtargetinfo.h"
#include "devicesupport/idevice.h"

#include <utils/environment.h>
#include <utils/port.h>
#include <utils/processhandle.h>
#include <utils/qtcassert.h>
#include <utils/icon.h>

#include <QPointer>
#include <QWidget>

#include <functional>
#include <memory>

namespace Utils { class OutputFormatter; }

namespace ProjectExplorer {
class BuildConfiguration;
class GlobalOrProjectAspect;
class Node;
class RunConfigurationFactory;
class RunConfiguration;
class RunConfigurationCreationInfo;
class RunControl;
class RunWorkerFactory;
class Target;

namespace Internal {
class RunControlPrivate;
class RunWorkerPrivate;
class SimpleRunControlPrivate;
} // Internal

/**
 * An interface for a hunk of global or per-project
 * configuration data.
 *
 */

class PROJECTEXPLORER_EXPORT ISettingsAspect : public QObject
{
    Q_OBJECT

public:
    /// Create a configuration widget for this settings aspect.
    using ConfigWidgetCreator = std::function<QWidget *()>;

    explicit ISettingsAspect(const ConfigWidgetCreator &configWidgetCreator);
    QWidget *createConfigWidget() const;

protected:
    ///
    friend class GlobalOrProjectAspect;
    /// Converts current object into map for storage.
    virtual void toMap(QVariantMap &map) const = 0;
    /// Read object state from @p map.
    virtual void fromMap(const QVariantMap &map) = 0;

    ConfigWidgetCreator m_configWidgetCreator;
};


/**
 * An interface to facilitate switching between hunks of
 * global and per-project configuration data.
 *
 */

class PROJECTEXPLORER_EXPORT GlobalOrProjectAspect : public ProjectConfigurationAspect
{
    Q_OBJECT

public:
    GlobalOrProjectAspect();
    ~GlobalOrProjectAspect() override;

    void setProjectSettings(ISettingsAspect *settings);
    void setGlobalSettings(ISettingsAspect *settings);

    bool isUsingGlobalSettings() const { return m_useGlobalSettings; }
    void setUsingGlobalSettings(bool value);
    void resetProjectToGlobalSettings();

    ISettingsAspect *projectSettings() const { return m_projectSettings; }
    ISettingsAspect *globalSettings() const { return m_globalSettings; }
    ISettingsAspect *currentSettings() const;

protected:
    friend class RunConfiguration;
    void fromMap(const QVariantMap &map) override;
    void toMap(QVariantMap &data) const override;

private:
    bool m_useGlobalSettings = false;
    ISettingsAspect *m_projectSettings = nullptr; // Owned if present.
    ISettingsAspect *m_globalSettings = nullptr;  // Not owned.
};

class PROJECTEXPLORER_EXPORT Runnable
{
public:
    Runnable() = default;

    QString executable;
    QString commandLineArguments;
    QString workingDirectory;
    Utils::Environment environment;
    IDevice::ConstPtr device; // Override the kit's device. Keep unset by default.

    // FIXME: Not necessarily a display name
    QString displayName() const { return executable; }
};

// Documentation inside.
class PROJECTEXPLORER_EXPORT RunConfiguration : public StatefulProjectConfiguration
{
    Q_OBJECT

public:
    ~RunConfiguration() override;

    bool isActive() const override;

    QString disabledReason() const override;

    virtual QWidget *createConfigurationWidget();

    virtual bool isConfigured() const;
    // Pop up configuration dialog in case for example the executable is missing.
    enum ConfigurationState { Configured, UnConfigured, Waiting };
    // TODO rename function
    virtual ConfigurationState ensureConfigured(QString *errorMessage = nullptr);

    Target *target() const;
    Project *project() const override;

    Utils::OutputFormatter *createOutputFormatter() const;

    bool fromMap(const QVariantMap &map) override;
    QVariantMap toMap() const override;

    virtual Runnable runnable() const;

    // Return a handle to the build system target that created this run configuration.
    // May return an empty string if no target built the executable!
    QString buildKey() const { return m_buildKey; }
    // The BuildTargetInfo corresponding to the buildKey.
    BuildTargetInfo buildTargetInfo() const;

    static RunConfiguration *startupRunConfiguration();
    virtual bool canRunForNode(const ProjectExplorer::Node *) const { return false; }

    template <class T = ISettingsAspect> T *currentSettings(Core::Id id) const
    {
        if (auto a = qobject_cast<GlobalOrProjectAspect *>(aspect(id)))
            return qobject_cast<T *>(a->currentSettings());
        return nullptr;
    }

    using AspectFactory = std::function<ProjectConfigurationAspect *(Target *)>;
    template <class T> static void registerAspect()
    {
        addAspectFactory([](Target *target) { return new T(target); });
    }

signals:
    void requestRunActionsUpdate();
    void configurationFinished();

protected:
    RunConfiguration(Target *target, Core::Id id);

    /// convenience function to get current build configuration.
    BuildConfiguration *activeBuildConfiguration() const;

    template<class T> void setOutputFormatter()
    {
        m_outputFormatterCreator = [](Project *project) { return new T(project); };
    }

    virtual void updateEnabledState();
    virtual void doAdditionalSetup(const RunConfigurationCreationInfo &) {}

private:
    static void addAspectFactory(const AspectFactory &aspectFactory);

    friend class RunConfigurationCreationInfo;

    QString m_buildKey;
    std::function<Utils::OutputFormatter *(Project *)> m_outputFormatterCreator;
};

class RunConfigurationCreationInfo
{
public:
    enum CreationMode {AlwaysCreate, ManualCreationOnly};
    RunConfiguration *create(Target *target) const;

    const RunConfigurationFactory *factory = nullptr;
    Core::Id id;
    QString buildKey;
    QString displayName;
    CreationMode creationMode = AlwaysCreate;
    bool useTerminal = false;
};

class PROJECTEXPLORER_EXPORT RunConfigurationFactory
{
public:
    RunConfigurationFactory();
    virtual ~RunConfigurationFactory();

    static RunConfiguration *restore(Target *parent, const QVariantMap &map);
    static RunConfiguration *clone(Target *parent, RunConfiguration *source);
    static const QList<RunConfigurationCreationInfo> creatorsForTarget(Target *parent);

    Core::Id runConfigurationBaseId() const { return m_runConfigBaseId; }

    static QString decoratedTargetName(const QString targetName, Target *kit);

protected:
    virtual QList<RunConfigurationCreationInfo> availableCreators(Target *parent) const;

    using RunConfigurationCreator = std::function<RunConfiguration *(Target *)>;

    template <class RunConfig>
    void registerRunConfiguration(Core::Id runConfigBaseId)
    {
        m_creator = [runConfigBaseId](Target *t) -> RunConfiguration * {
            return new RunConfig(t, runConfigBaseId);
        };
        m_runConfigBaseId = runConfigBaseId;
        m_ownTypeChecker = [](RunConfiguration *runConfig) {
            return qobject_cast<RunConfig *>(runConfig) != nullptr;
        };
    }

    void addSupportedProjectType(Core::Id id);
    void addSupportedTargetDeviceType(Core::Id id);
    void setDecorateDisplayNames(bool on);

    template<class Worker>
    RunWorkerFactory *addRunWorkerFactory(Core::Id runMode)
    {
        return addRunWorkerFactoryHelper(runMode, [](RunControl *rc) { return new Worker(rc); });
    }

private:
    RunWorkerFactory *addRunWorkerFactoryHelper
        (Core::Id runMode, const std::function<RunWorker *(RunControl *)> &creator);

    RunConfigurationFactory(const RunConfigurationFactory &) = delete;
    RunConfigurationFactory operator=(const RunConfigurationFactory &) = delete;

    bool canHandle(Target *target) const;

    friend class RunConfigurationCreationInfo;
    RunConfigurationCreator m_creator;
    Core::Id m_runConfigBaseId;
    QList<Core::Id> m_supportedProjectTypes;
    QList<Core::Id> m_supportedTargetDeviceTypes;
    bool m_decorateDisplayNames = false;
    QList<RunWorkerFactory *> m_ownedRunWorkerFactories;
    std::function<bool(RunConfiguration *)> m_ownTypeChecker;
};

class PROJECTEXPLORER_EXPORT FixedRunConfigurationFactory : public RunConfigurationFactory
{
public:
    explicit FixedRunConfigurationFactory(const QString &displayName,
                                          bool addDeviceName = false);

    QList<RunConfigurationCreationInfo> availableCreators(Target *parent) const override;

private:
    const QString m_fixedBuildTarget;
    const bool m_decorateTargetName;
};

class PROJECTEXPLORER_EXPORT RunWorker : public QObject
{
    Q_OBJECT

public:
    explicit RunWorker(RunControl *runControl);
    ~RunWorker() override;

    RunControl *runControl() const;

    void addStartDependency(RunWorker *dependency);
    void addStopDependency(RunWorker *dependency);

    void setId(const QString &id);

    void setStartTimeout(int ms, const std::function<void()> &callback = {});
    void setStopTimeout(int ms, const std::function<void()> &callback = {});

    void recordData(const QString &channel, const QVariant &data);
    QVariant recordedData(const QString &channel) const;

    // Part of read-only interface of RunControl for convenience.
    void appendMessage(const QString &msg, Utils::OutputFormat format, bool appendNewLine = true);
    IDevice::ConstPtr device() const;
    const Runnable &runnable() const;
    Core::Id runMode() const;

    // States
    void initiateStart();
    void reportStarted();

    void initiateStop();
    void reportStopped();

    void reportDone();

    void reportFailure(const QString &msg = QString());
    void setSupportsReRunning(bool reRunningSupported);
    bool supportsReRunning() const;

    static QString userMessageForProcessError(QProcess::ProcessError, const QString &programName);

    bool isEssential() const;
    void setEssential(bool essential);

signals:
    void started();
    void stopped();

protected:
    void virtual start();
    void virtual stop();
    void virtual onFinished() {}

private:
    friend class Internal::RunControlPrivate;
    friend class Internal::RunWorkerPrivate;
    const std::unique_ptr<Internal::RunWorkerPrivate> d;
};

class PROJECTEXPLORER_EXPORT RunWorkerFactory
{
public:
    using WorkerCreator = std::function<RunWorker *(RunControl *)>;
    using Constraint = std::function<bool(RunConfiguration *)>;

    RunWorkerFactory();
    virtual ~RunWorkerFactory();

    bool canRun(RunConfiguration *runConfiguration, Core::Id runMode) const;

    void setProducer(const WorkerCreator &producer);
    void addConstraint(const Constraint &constraint);
    void addSupportedRunMode(Core::Id runMode);

    WorkerCreator producer() const { return m_producer; }

private:
    // FIXME: That's temporary until ownership has been transferred to
    // the individual plugins.
    friend class ProjectExplorerPlugin;
    static void destroyRemainingRunWorkerFactories();

    QList<Core::Id> m_supportedRunModes;
    QList<Constraint> m_constraints;
    WorkerCreator m_producer;
};

/**
 * A RunControl controls the running of an application or tool
 * on a target device. It controls start and stop, and handles
 * application output.
 *
 * RunControls are created by RunControlFactories.
 */

class PROJECTEXPLORER_EXPORT RunControl : public QObject
{
    Q_OBJECT

public:
    RunControl(RunConfiguration *runConfiguration, Core::Id mode);
    RunControl(const IDevice::ConstPtr &device, Core::Id mode);
    ~RunControl() override;

    void initiateStart();
    void initiateReStart();
    void initiateStop();
    void forceStop();
    void initiateFinish();

    bool promptToStop(bool *optionalPrompt = nullptr) const;
    void setPromptToStop(const std::function<bool(bool *)> &promptToStop);

    bool supportsReRunning() const;

    virtual QString displayName() const;
    void setDisplayName(const QString &displayName);

    bool isRunning() const;
    bool isStarting() const;
    bool isStopping() const;
    bool isStopped() const;

    void setIcon(const Utils::Icon &icon);
    Utils::Icon icon() const;

    Utils::ProcessHandle applicationProcessHandle() const;
    void setApplicationProcessHandle(const Utils::ProcessHandle &handle);
    IDevice::ConstPtr device() const;

    RunConfiguration *runConfiguration() const;
    Project *project() const;

    Utils::OutputFormatter *outputFormatter() const;
    Core::Id runMode() const;

    const Runnable &runnable() const;
    void setRunnable(const Runnable &runnable);

    virtual void appendMessage(const QString &msg, Utils::OutputFormat format);

    static bool showPromptToStopDialog(const QString &title, const QString &text,
                                       const QString &stopButtonText = QString(),
                                       const QString &cancelButtonText = QString(),
                                       bool *prompt = nullptr);

    RunWorker *createWorker(Core::Id id);

    using WorkerCreator = RunWorkerFactory::WorkerCreator;
    using Constraint = RunWorkerFactory::Constraint;

    static void registerWorkerCreator(Core::Id id, const WorkerCreator &workerCreator);

    static void registerWorker(Core::Id runMode, const WorkerCreator &producer,
                               const Constraint &constraint = {})
    {
        auto factory = new RunWorkerFactory;
        factory->setProducer(producer);
        factory->addSupportedRunMode(runMode);
        factory->addConstraint(constraint);
    }
    template <class Worker>
    static void registerWorker(Core::Id runMode, const Constraint &constraint)
    {
        auto factory = new RunWorkerFactory;
        factory->setProducer([](RunControl *rc) { return new Worker(rc); });
        factory->addSupportedRunMode(runMode);
        factory->addConstraint(constraint);
    }

    static WorkerCreator producer(RunConfiguration *runConfiguration, Core::Id runMode);

signals:
    void appendMessageRequested(ProjectExplorer::RunControl *runControl,
                                const QString &msg, Utils::OutputFormat format);
    void aboutToStart();
    void started();
    void stopped();
    void finished();
    void applicationProcessHandleChanged(QPrivateSignal); // Use setApplicationProcessHandle

private:
    friend class RunWorker;
    friend class Internal::RunWorkerPrivate;

    const std::unique_ptr<Internal::RunControlPrivate> d;
};


/**
 * A simple TargetRunner for cases where a plain ApplicationLauncher is
 * sufficient for running purposes.
 */

class PROJECTEXPLORER_EXPORT SimpleTargetRunner : public RunWorker
{
    Q_OBJECT

public:
    explicit SimpleTargetRunner(RunControl *runControl);

    void setRunnable(const Runnable &runnable);

    void setDevice(const IDevice::ConstPtr &device);
    IDevice::ConstPtr device() const;

protected:
    void start() override;
    void stop() override;

private:
    void onProcessStarted();
    void onProcessFinished(int exitCode, QProcess::ExitStatus status);
    void onProcessError(QProcess::ProcessError error);

    ApplicationLauncher m_launcher;
    Runnable m_runnable;
    IDevice::ConstPtr m_device;
    bool m_stopReported = false;
    bool m_useTerminal = false;
};

} // namespace ProjectExplorer
