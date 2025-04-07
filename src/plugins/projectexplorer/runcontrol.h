// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "devicesupport/idevicefwd.h"
#include "runconfiguration.h"

#include <solutions/tasking/tasktreerunner.h>

#include <utils/commandline.h>
#include <utils/environment.h>
#include <utils/outputformatter.h>
#include <utils/processhandle.h>
#include <utils/processenums.h>
#include <utils/qtcassert.h>

#include <QHash>
#include <QProcess> // FIXME: Remove
#include <QVariant>

#include <functional>
#include <memory>

namespace Utils {
class Icon;
class MacroExpander;
class OutputLineParser;
class ProcessRunData;
class Process;
} // Utils

namespace ProjectExplorer {
class RunConfiguration;
class RunControl;
class Target;

namespace Internal {
class RunControlPrivate;
class RunWorkerPrivate;
class ProcessRunnerPrivate;
} // Internal

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

    // Part of read-only interface of RunControl for convenience.
    void appendMessage(const QString &msg, Utils::OutputFormat format, bool appendNewLine = true);

    // States
    void initiateStart();
    void reportStarted();

    void initiateStop();
    void reportStopped();

    void reportDone();

    void reportFailure(const QString &msg = QString());

signals:
    void started();
    void stopping();
    void stopped();

protected:
    void virtual start();
    void virtual stop();

private:
    friend class Internal::RunControlPrivate;
    friend class Internal::RunWorkerPrivate;
    const std::unique_ptr<Internal::RunWorkerPrivate> d;
};

class PROJECTEXPLORER_EXPORT RunWorkerFactory
{
public:
    using WorkerCreator = std::function<RunWorker *(RunControl *)>;
    using RecipeCreator = std::function<Tasking::Group(RunControl *)>;

    RunWorkerFactory();
    ~RunWorkerFactory();

    static void dumpAll(); // For debugging only.

protected:
    template <typename Worker>
    void setProduct() { setProducer([](RunControl *rc) { return new Worker(rc); }); }
    void setId(Utils::Id id) { m_id = id; }
    void setProducer(const WorkerCreator &producer);
    void setRecipeProducer(const RecipeCreator &producer);
    void setSupportedRunConfigs(const QList<Utils::Id> &runConfigs);
    void addSupportedRunMode(Utils::Id runMode);
    void addSupportedRunConfig(Utils::Id runConfig);
    void addSupportedDeviceType(Utils::Id deviceType);
    void addSupportForLocalRunConfigs();
    void cloneProduct(Utils::Id exitstingStepId, Utils::Id overrideId = Utils::Id());

private:
    friend class RunControl;
    bool canCreate(Utils::Id runMode, Utils::Id deviceType, const QString &runConfigId) const;
    RunWorker *create(RunControl *runControl) const;

    WorkerCreator m_producer;
    QList<Utils::Id> m_supportedRunModes;
    QList<Utils::Id> m_supportedRunConfigurations;
    QList<Utils::Id> m_supportedDeviceTypes;
    Utils::Id m_id;
};

/**
 * A RunControl controls the running of an application or tool
 * on a target device. It controls start and stop, and handles
 * application output.
 *
 * RunControls are created by RunControlFactories.
 */

class PROJECTEXPLORER_EXPORT RunControl final : public QObject
{
    Q_OBJECT

public:
    explicit RunControl(Utils::Id mode);
    ~RunControl() final;

    void start();

    void setBuildConfiguration(BuildConfiguration *bc);
    void setKit(Kit *kit);

    void copyDataFromRunConfiguration(RunConfiguration *runConfig);
    void copyDataFromRunControl(RunControl *runControl);
    void resetDataForAttachToCore();

    void setRunRecipe(const Tasking::Group &group);

    void initiateStart();
    void initiateReStart();
    void initiateStop();
    void forceStop();

    bool promptToStop(bool *optionalPrompt = nullptr) const;
    void setPromptToStop(const std::function<bool(bool *)> &promptToStop);

    // Note: Works only in the task tree mode
    void setSupportsReRunning(bool reRunningSupported);
    bool supportsReRunning() const;

    QString displayName() const;
    void setDisplayName(const QString &displayName);

    bool isRunning() const;
    bool isStarting() const;
    bool isStopped() const;

    void setIcon(const Utils::Icon &icon);
    Utils::Icon icon() const;

    Utils::ProcessHandle applicationProcessHandle() const;
    void setApplicationProcessHandle(const Utils::ProcessHandle &handle);
    IDeviceConstPtr device() const;

    // FIXME: Try to cut down to amount of functions.
    BuildConfiguration *buildConfiguration() const;
    Target *target() const; // FIXME: Eliminate callers and remove again.
    Project *project() const;
    Kit *kit() const;
    const Utils::MacroExpander *macroExpander() const;

    const Utils::BaseAspect::Data *aspectData(Utils::Id instanceId) const;
    const Utils::BaseAspect::Data *aspectData(Utils::BaseAspect::Data::ClassId classId) const;
    template <typename T> const typename T::Data *aspectData() const {
        return dynamic_cast<const typename T::Data *>(aspectData(&T::staticMetaObject));
    }

    QString buildKey() const;
    Utils::FilePath buildDirectory() const;
    Utils::Environment buildEnvironment() const;

    Utils::Store settingsData(Utils::Id id) const;

    Utils::FilePath targetFilePath() const;
    Utils::FilePath projectFilePath() const;

    void setupFormatter(Utils::OutputFormatter *formatter) const;
    Utils::Id runMode() const;
    bool isPrintEnvironmentEnabled() const;

    const Utils::ProcessRunData &runnable() const;

    const Utils::CommandLine &commandLine() const;
    void setCommandLine(const Utils::CommandLine &command);

    const Utils::FilePath &workingDirectory() const;
    void setWorkingDirectory(const Utils::FilePath &workingDirectory);

    const Utils::Environment &environment() const;
    void setEnvironment(const Utils::Environment &environment);

    const QVariantHash &extraData() const;
    void setExtraData(const QVariantHash &extraData);

    static bool showPromptToStopDialog(const QString &title, const QString &text,
                                       const QString &stopButtonText = QString(),
                                       const QString &cancelButtonText = QString(),
                                       bool *prompt = nullptr);

    static void provideAskPassEntry(Utils::Environment &env);

    RunWorker *createWorker(Utils::Id runMode);

    bool createMainWorker();
    static bool canRun(Utils::Id runMode, Utils::Id deviceType, Utils::Id runConfigId);
    void postMessage(const QString &msg, Utils::OutputFormat format, bool appendNewLine = true);

    void requestDebugChannel();
    bool usesDebugChannel() const;
    QUrl debugChannel() const;

    void requestQmlChannel();
    bool usesQmlChannel() const;
    QUrl qmlChannel() const;
    // FIXME: Don't use. Convert existing users to portsgatherer.
    void setQmlChannel(const QUrl &channel);

    void requestPerfChannel();
    bool usesPerfChannel() const;
    QUrl perfChannel() const;

    void requestWorkerChannel();
    QUrl workerChannel() const;

    void setAttachPid(Utils::ProcessHandle pid);
    Utils::ProcessHandle attachPid() const;

    void showOutputPane();

signals:
    void appendMessage(const QString &msg, Utils::OutputFormat format);
    void aboutToStart();
    void started();
    void stopped();
    void applicationProcessHandleChanged(QPrivateSignal);
    void stdOutData(const QByteArray &data);

private:
    void setDevice(const IDeviceConstPtr &device);

    friend class RunWorker;
    friend class Internal::RunWorkerPrivate;

    const std::unique_ptr<Internal::RunControlPrivate> d;
};

PROJECTEXPLORER_EXPORT Tasking::Group processRecipe(RunControl *runControl,
    const std::function<void(Utils::Process &)> &startModifier = {},
    bool suppressDefaultStdOutHandling = false);

class PROJECTEXPLORER_EXPORT ProcessRunner final : public RunWorker
{
    Q_OBJECT

public:
    explicit ProcessRunner(RunControl *runControl);
    ~ProcessRunner() override;

    void setStartModifier(const std::function<void(Utils::Process &)> &startModifier);
    void suppressDefaultStdOutHandling();

private:
    void start() final;
    void stop() final;

    const std::unique_ptr<Internal::ProcessRunnerPrivate> d;
};

class PROJECTEXPLORER_EXPORT ProcessRunnerFactory : public RunWorkerFactory
{
public:
    explicit ProcessRunnerFactory(const QList<Utils::Id> &runConfig);
};

PROJECTEXPLORER_EXPORT
void addOutputParserFactory(const std::function<Utils::OutputLineParser *(Target *)> &);

PROJECTEXPLORER_EXPORT QList<Utils::OutputLineParser *> createOutputParsers(Target *target);

class PROJECTEXPLORER_EXPORT RunInterface : public QObject
{
    Q_OBJECT

signals:
    void started();  // Recipe -> RunWorker
    void canceled(); // RunWorker -> Recipe
};

PROJECTEXPLORER_EXPORT Tasking::Storage<RunInterface> runStorage();
using Canceler = std::function<std::pair<RunInterface *, void (RunInterface::*)()>()>;
PROJECTEXPLORER_EXPORT Canceler canceler();

class PROJECTEXPLORER_EXPORT RecipeRunner final : public RunWorker
{
    Q_OBJECT

public:
    RecipeRunner(RunControl *runControl, const Tasking::Group &recipe)
        : RunWorker(runControl), m_recipe(recipe)
    {}

signals:
    void canceled();

private:
    void start() final;
    void stop() final;

    Tasking::TaskTreeRunner m_taskTreeRunner;
    const Tasking::Group m_recipe;
};

} // namespace ProjectExplorer
