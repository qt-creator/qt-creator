// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "devicesupport/idevicefwd.h"
#include "runconfiguration.h"

#include <utils/commandline.h>
#include <utils/environment.h>
#include <utils/outputformatter.h>
#include <utils/processhandle.h>
#include <utils/qtcassert.h>

#include <QHash>
#include <QProcess> // FIXME: Remove
#include <QVariant>

#include <functional>
#include <memory>

namespace Tasking { class Group; }

namespace Utils {
class Icon;
class MacroExpander;
class OutputLineParser;
class ProcessRunData;
} // Utils

namespace ProjectExplorer {
class RunConfiguration;
class RunControl;
class Target;

namespace Internal {
class RunControlPrivate;
class RunWorkerPrivate;
class SimpleTargetRunnerPrivate;
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

    void recordData(const Utils::Key &channel, const QVariant &data);
    QVariant recordedData(const Utils::Key &channel) const;

    // Part of read-only interface of RunControl for convenience.
    void appendMessage(const QString &msg, Utils::OutputFormat format, bool appendNewLine = true);
    IDeviceConstPtr device() const;

    // States
    void initiateStart();
    void reportStarted();

    void initiateStop();
    void reportStopped();

    void reportDone();

    void reportFailure(const QString &msg = QString());
    void setSupportsReRunning(bool reRunningSupported);

    static QString userMessageForProcessError(QProcess::ProcessError,
                                              const Utils::FilePath &programName);

    bool isEssential() const;
    void setEssential(bool essential);

signals:
    void started();
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

    RunWorkerFactory();
    ~RunWorkerFactory();

    static void dumpAll(); // For debugging only.

protected:
    template <typename Worker>
    void setProduct() { setProducer([](RunControl *rc) { return new Worker(rc); }); }
    void setProducer(const WorkerCreator &producer);
    void setSupportedRunConfigs(const QList<Utils::Id> &runConfigs);
    void addSupportedRunMode(Utils::Id runMode);
    void addSupportedRunConfig(Utils::Id runConfig);
    void addSupportedDeviceType(Utils::Id deviceType);

private:
    friend class RunControl;
    bool canCreate(Utils::Id runMode, Utils::Id deviceType, const QString &runConfigId) const;
    RunWorker *create(RunControl *runControl) const;

    WorkerCreator m_producer;
    QList<Utils::Id> m_supportedRunModes;
    QList<Utils::Id> m_supportedRunConfigurations;
    QList<Utils::Id> m_supportedDeviceTypes;
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
    explicit RunControl(Utils::Id mode);
    ~RunControl() override;

    void setTarget(Target *target);
    void setKit(Kit *kit);

    void copyDataFromRunConfiguration(RunConfiguration *runConfig);
    void copyDataFromRunControl(RunControl *runControl);

    void setAutoDeleteOnStop(bool autoDelete);

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
    Target *target() const;
    Project *project() const;
    Kit *kit() const;
    const Utils::MacroExpander *macroExpander() const;

    const Utils::BaseAspect::Data *aspect(Utils::Id instanceId) const;
    const Utils::BaseAspect::Data *aspect(Utils::BaseAspect::Data::ClassId classId) const;
    template <typename T> const typename T::Data *aspect() const {
        return dynamic_cast<const typename T::Data *>(aspect(&T::staticMetaObject));
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

    RunWorker *createWorker(Utils::Id workerId);

    bool createMainWorker();
    static bool canRun(Utils::Id runMode, Utils::Id deviceType, Utils::Id runConfigId);
    void postMessage(const QString &msg, Utils::OutputFormat format, bool appendNewLine = true);

signals:
    void appendMessage(const QString &msg, Utils::OutputFormat format);
    void aboutToStart();
    void started();
    void stopped();
    void applicationProcessHandleChanged(QPrivateSignal);

private:
    void setDevice(const IDeviceConstPtr &device);

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
    ~SimpleTargetRunner() override;

protected:
    void setStartModifier(const std::function<void()> &startModifier);

    Utils::CommandLine commandLine() const;
    void setCommandLine(const Utils::CommandLine &commandLine);

    void setEnvironment(const Utils::Environment &environment);
    void setWorkingDirectory(const Utils::FilePath &workingDirectory);

    void forceRunOnHost();

private:
    void start() final;
    void stop() final;

    const Utils::ProcessRunData &runnable() const = delete;
    void setRunnable(const Utils::ProcessRunData &) = delete;

    const std::unique_ptr<Internal::SimpleTargetRunnerPrivate> d;
};

class PROJECTEXPLORER_EXPORT SimpleTargetRunnerFactory : public RunWorkerFactory
{
public:
    explicit SimpleTargetRunnerFactory(const QList<Utils::Id> &runConfig);
};

class PROJECTEXPLORER_EXPORT OutputFormatterFactory
{
protected:
    OutputFormatterFactory();

public:
    virtual ~OutputFormatterFactory();

    static QList<Utils::OutputLineParser *> createFormatters(Target *target);

protected:
    using FormatterCreator = std::function<QList<Utils::OutputLineParser *>(Target *)>;
    void setFormatterCreator(const FormatterCreator &creator);

private:
    FormatterCreator m_creator;
};

} // namespace ProjectExplorer
