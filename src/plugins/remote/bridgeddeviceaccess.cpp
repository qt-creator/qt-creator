// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "bridgeddeviceaccess.h"

#include "remotelinuxtr.h"

#include <coreplugin/icore.h>

#include <client/bridgedfileaccess.h>

#include <utils/async.h>
#include <utils/commandline.h>
#include <utils/futuresynchronizer.h>
#include <utils/processinfo.h>
#include <utils/processinterface.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>
#include <utils/terminalhooks.h>

#include <QLoggingCategory>
#include <QPointer>
#include <QThread>

#include <QtTaskTree/QConditional>

#include <chrono>

using namespace Core;
using namespace ProjectExplorer;
using namespace QtTaskTree;
using namespace Utils;

namespace Remote::Internal {

static constexpr int MaxBridgeAttempts = 3;
static constexpr std::chrono::seconds BridgeRetryDelay{5};

/*!
    \class Remote::Internal::BridgedDeviceAccess
    \internal

    Gives a device whose commands go through a launcher on the host the file
    access of a command bridge, with a fallback that runs every operation as
    a command of its own while the bridge is not up.

    Bringing the bridge up starts commands on the device, so off the main
    thread it happens in place, and on the main thread in the background,
    with the fallback answering meanwhile. A failed attempt is retried a few
    times. The hooks that create the file access run with the file access
    locked, so they must not call back into this object.
*/

BridgedDeviceAccess::BridgedDeviceAccess(LogCategory log, const Hooks &hooks)
    : m_log(log)
    , m_hooks(hooks)
{
    QTC_CHECK(m_hooks.rootPath && m_hooks.bridgeEnvironment && m_hooks.createBridge
              && m_hooks.createFallback && m_hooks.dropped);
}

BridgedDeviceAccess::~BridgedDeviceAccess()
{
    // The task uses this object and the hooks.
    for (QFuture<void> &bringUp : m_bringUps) {
        bringUp.cancel();
        bringUp.waitForFinished();
    }
    m_fileAccess.writeLocked()->reset();
}

DeviceFileAccessPtr BridgedDeviceAccess::cachedFileAccess() const
{
    auto fileAccess = m_fileAccess.readLocked();
    if (*fileAccess && !shouldRetryBridge())
        return *fileAccess;
    return {};
}

std::function<void()> BridgedDeviceAccess::bridgeExitHandler()
{
    return [self = QPointer<BridgedDeviceAccess>(this)] {
        if (!self)
            return;
        // Queued: this runs on the thread the bridge answers on, and dropping
        // the access destroys the very client that is reporting.
        QMetaObject::invokeMethod(
            self.get(),
            [self] {
                if (self)
                    self->drop();
            },
            Qt::QueuedConnection);
    };
}

Result<DeviceFileAccessPtr> BridgedDeviceAccess::createBridgeFileAccess(
    FileAccessLock &fileAccess,
    const std::optional<Result<FilePath>> &bridgeInPlace,
    const Environment &bridgeEnvironment)
{
    if (bridgeInPlace) {
        const Result<FilePath> &inPlace = *bridgeInPlace;
        if (inPlace) {
            std::unique_ptr<CmdBridge::FileAccess> fAccess
                = m_hooks.createBridge(bridgeExitHandler());
            fAccess->setPidMarker(Utils::pidMarkerTemplate());
            const Result<> initResult = fAccess->init(*inPlace, bridgeEnvironment, false);
            if (initResult)
                return DeviceFileAccessPtr(std::move(fAccess));

            qCWarning(m_log).noquote() << "Could not run the command bridge from"
                                       << inPlace->toUserOutput() << ":" << initResult.error();
        } else {
            qCWarning(m_log).noquote() << inPlace.error();
        }
    }

    std::unique_ptr<CmdBridge::FileAccess> fAccess = m_hooks.createBridge(bridgeExitHandler());
    fAccess->setPidMarker(Utils::pidMarkerTemplate());

    // Deploying runs commands on the device, which asks for the very file
    // access being set up here. Leave a fallback in place for the duration, or
    // those calls start another attempt of their own.
    *fileAccess = m_hooks.createFallback();
    fileAccess.unlock();
    const CmdBridge::FileAccess::DeployResult deployResult
        = fAccess->deployAndInit(ICore::libexecPath(), m_hooks.rootPath(), bridgeEnvironment);
    fileAccess.lock();

    if (!deployResult)
        return ResultError(deployResult.error().message);

    return DeviceFileAccessPtr(std::move(fAccess));
}

DeviceFileAccessPtr BridgedDeviceAccess::fallBack(FileAccessLock &fileAccess, const QString &reason)
{
    m_retryBridge = ++m_bridgeAttempts < MaxBridgeAttempts;
    m_bridgeRetryTimer = QDeadlineTimer(BridgeRetryDelay);

    qCWarning(m_log).noquote() << "Failed to start the command bridge:" << reason
                               << ", falling back to slow direct access";

    *fileAccess = m_hooks.createFallback();
    return *fileAccess;
}

DeviceFileAccessPtr BridgedDeviceAccess::bringUp(int generation)
{
    // Asked before the lock is taken: this runs from the file access factory,
    // and anything that reaches back for the device's file access while it
    // waits would deadlock on it.
    std::optional<Result<FilePath>> bridgeInPlace;
    if (m_hooks.bridgeInPlace)
        bridgeInPlace = m_hooks.bridgeInPlace();
    const Result<Environment> bridgeEnvironment = m_hooks.bridgeEnvironment();

    FileAccessLock fileAccess = m_fileAccess.writeLocked();
    m_creatingFileAccess = false;

    // The bridge answers deviceEnvironment() with the one it was started
    // with, so an empty one would leave the device with no PATH.
    if (!bridgeEnvironment)
        return fallBack(fileAccess, bridgeEnvironment.error());

    const Result<DeviceFileAccessPtr> fAccess
        = createBridgeFileAccess(fileAccess, bridgeInPlace, *bridgeEnvironment);
    if (!fAccess)
        return fallBack(fileAccess, fAccess.error());

    // The bridge was set up for settings the device has moved on from since.
    // Not a failed attempt, so it is not counted.
    if (generation != m_generation) {
        m_retryBridge = true;
        m_bridgeRetryTimer = QDeadlineTimer(std::chrono::seconds::zero());
        if (!*fileAccess)
            *fileAccess = m_hooks.createFallback();
        return *fileAccess;
    }

    *fileAccess = *fAccess;
    m_bridgeAttempts = 0;
    return *fileAccess;
}

DeviceFileAccessPtr BridgedDeviceAccess::fileAccess()
{
    if (DeviceFileAccessPtr fileAccess = cachedFileAccess())
        return fileAccess;

    FileAccessLock fileAccess = m_fileAccess.writeLocked();
    if (*fileAccess && !shouldRetryBridge())
        return *fileAccess;

    if (!*fileAccess)
        *fileAccess = m_hooks.createFallback();

    if (m_creatingFileAccess)
        return *fileAccess;

    m_creatingFileAccess = true;
    m_retryBridge = false;
    const int generation = m_generation;

    if (QThread::isMainThread()) {
        m_bringUps.removeIf([](const QFuture<void> &future) { return future.isFinished(); });
        m_bringUps.append(Utils::asyncRun([this, generation] { bringUp(generation); }));
        Utils::futureSynchronizer()->addFuture(m_bringUps.last());
        return *fileAccess;
    }

    fileAccess.unlock();
    return bringUp(generation);
}

/*!
    Drops the file access and everything that was detected about the device,
    so that the next file access starts from scratch.
*/
void BridgedDeviceAccess::drop()
{
    if (m_hooks.resetCaches)
        m_hooks.resetCaches();

    {
        auto fileAccess = m_fileAccess.writeLocked();
        fileAccess->reset();
        ++m_generation;
        m_retryBridge = false;
        m_bridgeAttempts = 0;
    }

    m_hooks.dropped();
}

/*!
    Drops the file access without counting it as a failure, for a bridge
    that was set up with settings the device has moved on from.
*/
void BridgedDeviceAccess::dropStale() const
{
    {
        auto fileAccess = m_fileAccess.writeLocked();
        fileAccess->reset();
        ++m_generation;
    }

    m_hooks.dropped();
}

Result<RunResult> BridgeFallbackFileAccess::runInShellImpl(
    const CommandLine &cmdLine, const QByteArray &stdInData) const
{
    // Split the way UnixDeviceFileAccess quoted these arguments: for a shell
    // on the device, not for the one on this host.
    Process proc;
    proc.setWriteData(stdInData);
    proc.setCommand(
        {m_rootPath.withNewPath(cmdLine.executable().path()),
         ProcessArgs::splitArgs(cmdLine.arguments(), OsTypeLinux)});
    proc.runBlocking();

    return RunResult{
        proc.resultData().m_exitCode,
        proc.readAllRawStandardOutput(),
        proc.readAllRawStandardError(),
    };
}

WrappedProcessInterface *createBridgedProcessInterface(
    const IDevice::ConstPtr &device, const DeviceCommandWrapper &wrapCommand)
{
    std::weak_ptr<const IDevice> weakDevice = device;

    const auto wrapCommandLine =
        [wrapCommand](
            const ProcessSetupData &setupData,
            const QString &markerTemplate,
            const QString & /*exitCodeTemplate*/) -> Result<CommandLine> {
        return wrapCommand(
            markerTemplate,
            setupData.m_commandLine,
            setupData.m_environment,
            setupData.rawWorkingDirectory(),
            !setupData.m_ptyData
                && !setupData.m_extraData.value(Utils::TARGET_REPORTS_PID).toBool());
    };

    // The process id is the device's, which a signal sent from this host
    // would not reach.
    const auto controlSignalFunction = [weakDevice](ControlSignal controlSignal, qint64 remotePid) {
        const IDevice::ConstPtr device = weakDevice.lock();
        if (!device)
            return;

        const auto access = std::dynamic_pointer_cast<CmdBridge::FileAccess>(device->fileAccess());
        if (access) {
            access->signalProcess(remotePid, controlSignal);
            return;
        }

        const int signal = ProcessInterface::controlSignalToInt(controlSignal);
        Process process;
        process.setCommand(
            {device->rootPath().withNewPath("kill"),
             {QString("-%1").arg(signal), QString::number(remotePid)}});
        process.runBlocking();
    };

    auto processInterface = new WrappedProcessInterface(wrapCommandLine, controlSignalFunction);

    QObject::connect(device.get(), &QObject::destroyed, processInterface, [processInterface] {
        processInterface->emitDone(
            ProcessResultData{
                -1,
                ProcessExitStatus::CrashExit,
                ProcessError::UnknownError,
                Tr::tr("Device is shut down."),
            });
    });

    return processInterface;
}

OpenTerminal openTerminalWithDefaultShell(const std::function<FilePath()> &rootPath)
{
    return [rootPath](const Environment &env,
                      const FilePath &workingDir,
                      const Continuation<> &cont) {
        const Result<FilePath> shell = Terminal::defaultShellForDevice(rootPath());
        if (!shell) {
            cont(ResultError(shell.error()));
            return;
        }

        Process process;
        process.setTerminalMode(TerminalMode::Detached);
        process.setEnvironment(env);
        process.setWorkingDirectory(workingDir);
        process.setCommand(CommandLine{*shell});
        process.start();

        cont(ResultOk);
    };
}

ExecutableItem bridgedSignalOperationRecipe(
    const FilePath &rootPath,
    const SignalOperationData &data,
    const Storage<Result<>> &resultStorage)
{
    Storage<qint64> pid;

    const auto onFindProcessSetup = [rootPath, data](Async<Result<qint64>> &task) {
        task.setConcurrentCallData(
            [](QPromise<Result<qint64>> &promise,
               const FilePath &filePath,
               const FilePath &rootPath) {
                const Result<QList<ProcessInfo>> list = ProcessInfo::processInfoList(rootPath);
                if (!list) {
                    promise.addResult(ResultError(list.error()));
                    return;
                }

                for (const ProcessInfo &processInfo : *list) {
                    if (processInfo.commandLine == filePath.path()) {
                        promise.addResult(processInfo.processId);
                        return;
                    }
                }
                promise.addResult(ResultError(Tr::tr("Process not found.")));
            },
            data.filePath,
            rootPath);
    };

    const auto onFindProcessDone = [pid, resultStorage](const Async<Result<qint64>> &task) {
        const Result<qint64> result = task.result();
        if (!result) {
            *resultStorage = ResultError(result.error());
            return DoneResult::Error;
        }
        *pid = *result;
        return DoneResult::Success;
    };

    const auto onProcessSetup = [rootPath, pid, data](Process &process) {
        const int signal = data.mode == SignalOperationMode::InterruptByPid ? 2 : 9;
        process.setCommand(
            {rootPath.withNewPath("kill"), {QString("-%1").arg(signal), QString::number(*pid)}});
    };

    const auto onProcessDone = [resultStorage](const Process &process, DoneWith result) {
        if (result == DoneWith::Error)
            *resultStorage = ResultError(process.exitMessage());
        else if (result == DoneWith::Cancel)
            *resultStorage = ResultError(Tr::tr("Signal operation canceled."));
    };

    // clang-format off
    return Group {
        pid,
        If ([data] { return data.mode == SignalOperationMode::KillByPath; }) >> Then {
            AsyncTask<Result<qint64>>(onFindProcessSetup, onFindProcessDone),
        } >> Else {
            QSyncTask([data, pid] { *pid = data.pid; })
        },
        ProcessTask(onProcessSetup, onProcessDone),
    };
    // clang-format on
}

} // namespace Remote::Internal
