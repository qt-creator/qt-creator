// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "wsldevice.h"

#include "wslapi.h"
#include "wslconstants.h"
#include "wsldevicewidget.h"
#include "wsltr.h"

#include <coreplugin/icore.h>

#include <projectexplorer/devicesupport/devicemanager.h>
#include <projectexplorer/devicesupport/idevicefactory.h>
#include <projectexplorer/projectexplorerconstants.h>

#include <client/bridgedfileaccess.h>
#include <client/cmdbridgeclient.h>

#include <utils/algorithm.h>
#include <utils/async.h>
#include <utils/commandline.h>
#include <utils/devicefileaccess.h>
#include <utils/environment.h>
#include <utils/fsengine/fsengine.h>
#include <utils/futuresynchronizer.h>
#include <utils/globaltasktree.h>
#include <utils/infolabel.h>
#include <utils/layoutbuilder.h>
#include <utils/osspecificaspects.h>
#include <utils/processinfo.h>
#include <utils/processinterface.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>
#include <utils/shutdownguard.h>
#include <utils/synchronizedvalue.h>
#include <utils/terminalhooks.h>
#include <utils/url.h>

#include <QComboBox>
#include <QDeadlineTimer>
#include <QDialog>
#include <QDialogButtonBox>
#include <QLoggingCategory>
#include <QPointer>
#include <QPushButton>
#include <QThread>

#include <QtTaskTree/QConditional>

#include <chrono>
#include <optional>

#ifdef Q_OS_WIN
#include <qt_windows.h>
#endif

using namespace Core;
using namespace ProjectExplorer;
using namespace QtTaskTree;
using namespace Utils;

namespace Wsl {
namespace Internal {

static Q_LOGGING_CATEGORY(wslDeviceLog, "qtc.wsl.device", QtWarningMsg);

const char WslDeviceDistributionKey[] = "WslDeviceDistribution";
const char WslDeviceUserNameKey[] = "WslDeviceUserName";
const char WslDeviceMountRootKey[] = "WslDeviceMountRoot";
const char WslDeviceAppendWindowsPathKey[] = "WslDeviceAppendWindowsPath";

const char WslKnownDistributionsKey[] = "Wsl/KnownDistributions";

class WslDeviceFileAccess final : public CmdBridge::FileAccess
{
public:
    WslDeviceFileAccess(WslDevicePrivate *dev, const std::function<void()> &errorExitHandler)
        : CmdBridge::FileAccess(errorExitHandler)
        , m_dev(dev)
    {}

    QString mapToDevicePath(const QString &hostPath) const final;

    WslDevicePrivate *m_dev = nullptr;
};

class WslFallbackFileAccess final : public UnixDeviceFileAccess
{
public:
    WslFallbackFileAccess(WslDevicePrivate *dev, const FilePath &rootPath)
        : m_dev(dev)
        , m_rootPath(rootPath)
    {}

    // Not the "env" UnixDeviceFileAccess would run, so that leaving the
    // Windows PATH out means the same with the bridge and without it.
    Result<Environment> deviceEnvironment() const final;

    Result<RunResult> runInShellImpl(
        const CommandLine &cmdLine, const QByteArray &stdInData) const final
    {
        // Split for the distribution, not for the host: UnixDeviceFileAccess
        // quoted these arguments the way a shell in the distribution parses
        // them.
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

private:
    WslDevicePrivate *const m_dev;
    const FilePath m_rootPath;
};

class WslDevicePrivate final : public QObject
{
public:
    explicit WslDevicePrivate(WslDevice *parent)
        : q(parent)
    {
        QObject::connect(q, &WslDevice::applied, this, [this] { dropFileAccess(); });
    }

    ~WslDevicePrivate() final { m_fileAccess.writeLocked()->reset(); }

    Result<CommandLine> withWslCmd(
        const QString &markerTemplate,
        const CommandLine &cmd,
        const std::optional<Environment> &env = std::nullopt,
        const std::optional<FilePath> &workDir = std::nullopt,
        bool withMarker = true) const;

    CommandLine launcherCommandLine(const QString &distribution, const QString &userName) const;

    Result<OsArch> osArch() const;
    Result<FilePath> cmdBridgePathInDistribution() const;
    Result<Environment> fetchEnvironment() const;

    DeviceFileAccessPtr cachedFileAccess() const
    {
        auto fileAccess = m_fileAccess.readLocked();
        if (*fileAccess && !shouldRetryBridge())
            return *fileAccess;
        return {};
    }

    DeviceFileAccessPtr createFileAccess();
    void dropFileAccess();
    void dropStaleFileAccess() const;

    QString distribution() const;
    QString mountRoot() const;

    WslDevice *const q;

private:
    Result<DeviceFileAccessPtr> createBridgeFileAccess(
        SynchronizedValue<DeviceFileAccessPtr>::unique_lock &fileAccess,
        const Utils::Result<Utils::FilePath> &inPlace,
        const Utils::Environment &bridgeEnvironment);

    DeviceFileAccessPtr bringUpFileAccess(int generation);

    DeviceFileAccessPtr fallBackToDirectAccess(
        SynchronizedValue<DeviceFileAccessPtr>::unique_lock &fileAccess, const QString &reason);

    bool shouldRetryBridge() const { return m_retryBridge && m_bridgeRetryTimer.hasExpired(); }

    std::function<void()> bridgeExitHandler();

    void detectMountRootAsync() const;

    void dropStaleTranslations() const;

    struct MountRootDetection
    {
        QString root;
        int attemptsLeft = 3;
        bool detecting = false;

        void reset() { *this = {}; }
    };

    bool takeMountRootAnswer(MountRootDetection *detection, const Result<QString> &root) const;

    mutable SynchronizedValue<DeviceFileAccessPtr> m_fileAccess;

    // Guarded by m_fileAccess.
    static constexpr int MaxBridgeAttempts = 3;
    static constexpr std::chrono::seconds BridgeRetryDelay{5};
    bool m_retryBridge = false;
    int m_bridgeAttempts = 0;
    QDeadlineTimer m_bridgeRetryTimer;

    bool m_creatingFileAccess = false;

    mutable int m_fileAccessGeneration = 0;

    mutable SynchronizedValue<std::optional<OsArch>> m_osArch;
    mutable SynchronizedValue<MountRootDetection> m_detectedMountRoot;
};

QString WslDeviceFileAccess::mapToDevicePath(const QString &hostPath) const
{
    return mappedHostPathToWslPath(m_dev->mountRoot(), m_dev->distribution(), hostPath);
}

Result<Environment> WslFallbackFileAccess::deviceEnvironment() const
{
    return m_dev->fetchEnvironment();
}

QString WslDevicePrivate::distribution() const
{
    return q->distribution();
}

bool WslDevicePrivate::takeMountRootAnswer(
    MountRootDetection *detection, const Result<QString> &root) const
{
    const QString had = detection->root;
    detection->detecting = false;
    if (root) {
        detection->root = *root;
    } else {
        --detection->attemptsLeft;
        qCWarning(wslDeviceLog).noquote()
            << "Could not read the automount root of" << q->distribution() << ":" << root.error();
    }

    return detection->root != had && !detection->root.isEmpty()
           && detection->root != QString(defaultMountRoot);
}

QString WslDevicePrivate::mountRoot() const
{
    if (const QString configured = q->mountRoot(); !configured.isEmpty())
        return normalizeMountRoot(configured);

    auto detected = m_detectedMountRoot.writeLocked();
    if (!detected->root.isEmpty())
        return detected->root;

    if (detected->attemptsLeft <= 0)
        return defaultMountRoot;

    // Starting a distribution is a virtual machine boot, and on the main
    // thread waiting for one is that many seconds of frozen window.
    if (QThread::isMainThread()) {
        if (!detected->detecting) {
            detected->detecting = true;
            detected.unlock();
            detectMountRootAsync();
        }
        return defaultMountRoot;
    }

    // A timeout costs an attempt each, so a handful of file operations at
    // once would spend them all on the first cold start.
    if (detected->detecting)
        return defaultMountRoot;

    // Not with the lock held: the main thread takes the same one on the way
    // to every path translation, and would sit out the whole boot on it.
    detected->detecting = true;
    detected.unlock();
    const Result<QString> root = detectMountRoot(q->distribution());

    auto answered = m_detectedMountRoot.writeLocked();
    const bool changed = takeMountRootAnswer(&*answered, root);
    const QString answer = answered->root.isEmpty() ? QString(defaultMountRoot) : answered->root;
    answered.unlock();

    if (changed) {
        QMetaObject::invokeMethod(q, [this] { dropStaleTranslations(); }, Qt::QueuedConnection);
    }

    return answer;
}

void WslDevicePrivate::detectMountRootAsync() const
{
    QFuture<Result<QString>> future = Utils::asyncRun(&detectMountRoot, q->distribution());
    future.then(q, [this](const Result<QString> &root) {
        auto detected = m_detectedMountRoot.writeLocked();
        const bool changed = takeMountRootAnswer(&*detected, root);
        detected.unlock();

        if (changed)
            dropStaleTranslations();
    });
    Utils::futureSynchronizer()->addFuture(future);
}

void WslDevicePrivate::dropStaleTranslations() const
{
    dropStaleFileAccess();
    FSEngine::invalidateFileInfoCache();
}

std::function<void()> WslDevicePrivate::bridgeExitHandler()
{
    return [self = QPointer<WslDevicePrivate>(this)] {
        if (!self)
            return;
        // Queued: this runs on the thread the bridge answers on, and dropping
        // the access destroys the very client that is reporting.
        QMetaObject::invokeMethod(
            self.get(),
            [self] {
                if (self)
                    self->dropFileAccess();
            },
            Qt::QueuedConnection);
    };
}

CommandLine WslDevicePrivate::launcherCommandLine(
    const QString &distribution, const QString &userName) const
{
    CommandLine cmd{wslExecutable(), {"--distribution", distribution}};
    if (!userName.isEmpty())
        cmd.addArgs({"--user", userName});
    return cmd;
}

Result<CommandLine> WslDevicePrivate::withWslCmd(
    const QString &markerTemplate,
    const CommandLine &cmd,
    const std::optional<Environment> &env,
    const std::optional<FilePath> &workDir,
    bool withMarker) const
{
    const FilePath wslExe = wslExecutable();
    if (wslExe.isEmpty())
        return ResultError(Tr::tr("No wsl.exe was found on this host."));

    if (q->distribution().isEmpty())
        return ResultError(Tr::tr("The device names no WSL distribution."));

    const QString workingDirectory
        = workDir && !workDir->isEmpty() ? q->rootPath().withNewMappedPath(*workDir).path()
                                         : QString();

    return wslCommandLine(
        wslExe,
        q->distribution(),
        q->userName(),
        workingDirectory,
        cmd,
        env,
        withMarker ? markerTemplate : QString());
}

Result<OsArch> WslDevicePrivate::osArch() const
{
    if (const std::optional<OsArch> cached = *m_osArch.readLocked())
        return *cached;

    const Result<OsArch> arch = detectArchitecture(q->distribution());
    if (arch)
        *m_osArch.writeLocked() = *arch;
    return arch;
}

Result<Environment> WslDevicePrivate::fetchEnvironment() const
{
    const Result<Environment> env = detectEnvironment(q->distribution(), q->userName());
    if (!env) {
        return ResultError(Tr::tr("Could not read the environment of \"%1\": %2")
                               .arg(q->distribution(), env.error()));
    }
    if (q->appendWindowsPath())
        return *env;
    return withoutWindowsPath(*env, mountRoot());
}

// WSL automounts the host's fixed drives and nothing else. Asking the
// distribution would start it, so the host is asked instead.
static bool isAutomountedDrive(QChar driveLetter)
{
#ifdef Q_OS_WIN
    const QString root = QString(driveLetter) + ":\\";
    return GetDriveTypeW(root.toStdWString().c_str()) == DRIVE_FIXED;
#else
    Q_UNUSED(driveLetter)
    return true;
#endif
}

static QString reachableWslPath(
    const QString &mountRoot, const QString &distribution, const FilePath &hostPath)
{
    const QString path = hostPath.path();
    if (path.size() >= 2 && path.at(0).isLetter() && path.at(1) == ':'
        && !isAutomountedDrive(path.at(0))) {
        return {};
    }

    return hostPathToWslPath(mountRoot, distribution, hostPath);
}

Result<FilePath> WslDevicePrivate::cmdBridgePathInDistribution() const
{
    const Result<OsArch> arch = osArch();
    if (!arch)
        return ResultError(arch.error());

    const Result<FilePath> hostBridge
        = CmdBridge::Client::getCmdBridgePath(OsTypeLinux, *arch, ICore::libexecPath());
    if (!hostBridge)
        return ResultError(hostBridge.error());

    const QString inDistribution = reachableWslPath(mountRoot(), q->distribution(), *hostBridge);
    if (inDistribution.isEmpty()) {
        return ResultError(Tr::tr("The distribution \"%1\" cannot reach \"%2\".")
                               .arg(q->distribution(), hostBridge->toUserOutput()));
    }

    return q->rootPath().withNewPath(inDistribution);
}

Result<DeviceFileAccessPtr> WslDevicePrivate::createBridgeFileAccess(
    SynchronizedValue<DeviceFileAccessPtr>::unique_lock &fileAccess,
    const Result<FilePath> &inPlace,
    const Environment &bridgeEnvironment)
{
    if (inPlace) {
        auto fAccess = std::make_unique<WslDeviceFileAccess>(this, bridgeExitHandler());
        fAccess->setPidMarker(Utils::pidMarkerTemplate());
        const Result<> initResult = fAccess->init(*inPlace, bridgeEnvironment, false);
        if (initResult)
            return DeviceFileAccessPtr(std::move(fAccess));

        qCWarning(wslDeviceLog).noquote()
            << "Could not run the command bridge from" << inPlace->toUserOutput() << ":"
            << initResult.error();
    } else {
        qCWarning(wslDeviceLog).noquote() << inPlace.error();
    }

    auto fAccess = std::make_unique<WslDeviceFileAccess>(this, bridgeExitHandler());
    fAccess->setPidMarker(Utils::pidMarkerTemplate());

    // Deploying runs commands on the device, which asks for the very file
    // access being set up here. Leave a fallback in place for the duration, or
    // those calls start another attempt of their own.
    *fileAccess = std::make_shared<WslFallbackFileAccess>(this, q->rootPath());
    fileAccess.unlock();
    const CmdBridge::FileAccess::DeployResult deployResult
        = fAccess->deployAndInit(ICore::libexecPath(), q->rootPath(), bridgeEnvironment);
    fileAccess.lock();

    if (!deployResult)
        return ResultError(deployResult.error().message);

    return DeviceFileAccessPtr(std::move(fAccess));
}

DeviceFileAccessPtr WslDevicePrivate::fallBackToDirectAccess(
    SynchronizedValue<DeviceFileAccessPtr>::unique_lock &fileAccess, const QString &reason)
{
    m_retryBridge = ++m_bridgeAttempts < MaxBridgeAttempts;
    m_bridgeRetryTimer = QDeadlineTimer(BridgeRetryDelay);

    qCWarning(wslDeviceLog).noquote() << "Failed to start the command bridge:" << reason
                                      << ", falling back to slow direct access";

    *fileAccess = std::make_shared<WslFallbackFileAccess>(this, q->rootPath());
    return *fileAccess;
}

DeviceFileAccessPtr WslDevicePrivate::bringUpFileAccess(int generation)
{
    // Asked before the lock is taken. Both start the distribution, and this
    // runs from the file access factory, so anything that reaches back for
    // the device's file access while they wait would deadlock on it.
    const Result<FilePath> inPlace = cmdBridgePathInDistribution();
    const Result<Environment> bridgeEnvironment = fetchEnvironment();

    SynchronizedValue<DeviceFileAccessPtr>::unique_lock fileAccess = m_fileAccess.writeLocked();
    m_creatingFileAccess = false;

    // The bridge answers deviceEnvironment() with the one it was started
    // with, so an empty one would leave the device with no PATH.
    if (!bridgeEnvironment)
        return fallBackToDirectAccess(fileAccess, bridgeEnvironment.error());

    const Result<DeviceFileAccessPtr> fAccess
        = createBridgeFileAccess(fileAccess, inPlace, *bridgeEnvironment);
    if (!fAccess)
        return fallBackToDirectAccess(fileAccess, fAccess.error());

    // The bridge was set up for an automount root or settings the device
    // has moved on from since. Not a failed attempt, so it is not counted.
    if (generation != m_fileAccessGeneration) {
        m_retryBridge = true;
        m_bridgeRetryTimer = QDeadlineTimer(std::chrono::seconds::zero());
        if (!*fileAccess)
            *fileAccess = std::make_shared<WslFallbackFileAccess>(this, q->rootPath());
        return *fileAccess;
    }

    *fileAccess = *fAccess;
    m_bridgeAttempts = 0;
    return *fileAccess;
}

DeviceFileAccessPtr WslDevicePrivate::createFileAccess()
{
    if (DeviceFileAccessPtr fileAccess = cachedFileAccess())
        return fileAccess;

    SynchronizedValue<DeviceFileAccessPtr>::unique_lock fileAccess = m_fileAccess.writeLocked();
    if (*fileAccess && !shouldRetryBridge())
        return *fileAccess;

    if (!*fileAccess)
        *fileAccess = std::make_shared<WslFallbackFileAccess>(this, q->rootPath());

    if (m_creatingFileAccess)
        return *fileAccess;

    m_creatingFileAccess = true;
    m_retryBridge = false;
    const int generation = m_fileAccessGeneration;

    // Starting a distribution is a virtual machine boot, see mountRoot().
    if (QThread::isMainThread()) {
        QFuture<void> future = Utils::asyncRun(
            [this, generation] { bringUpFileAccess(generation); });
        Utils::futureSynchronizer()->addFuture(future);
        return *fileAccess;
    }

    fileAccess.unlock();
    return bringUpFileAccess(generation);
}

void WslDevicePrivate::dropStaleFileAccess() const
{
    {
        auto fileAccess = m_fileAccess.writeLocked();
        fileAccess->reset();
        ++m_fileAccessGeneration;
    }

    q->invalidateSystemEnvironment();
    q->setDeviceState(IDevice::DeviceDisconnected);
}

void WslDevicePrivate::dropFileAccess()
{
    m_osArch.writeLocked()->reset();
    m_detectedMountRoot.writeLocked()->reset();

    {
        auto fileAccess = m_fileAccess.writeLocked();
        m_retryBridge = false;
        m_bridgeAttempts = 0;
    }

    dropStaleFileAccess();
}

static WrappedProcessInterface *makeProcessInterface(
    const IDevice::ConstPtr &device, WslDevicePrivate *devicePrivate)
{
    std::weak_ptr<const IDevice> weakDevice = device;

    const auto wrapCommandLine =
        [devicePrivate](
            const ProcessSetupData &setupData,
            const QString &markerTemplate,
            const QString & /*exitCodeTemplate*/) -> Result<CommandLine> {
        QTC_ASSERT(devicePrivate, return ResultError(Tr::tr("The WSL device is not initialized.")));

        return devicePrivate->withWslCmd(
            markerTemplate,
            setupData.m_commandLine,
            setupData.m_environment,
            setupData.rawWorkingDirectory(),
            !setupData.m_ptyData
                && !setupData.m_extraData.value(Utils::TARGET_REPORTS_PID).toBool());
    };

    const auto controlSignalFunction = [weakDevice](ControlSignal controlSignal, qint64 remotePid) {
        const IDevice::ConstPtr device = weakDevice.lock();
        if (!device)
            return;

        const auto access = std::dynamic_pointer_cast<WslDeviceFileAccess>(device->fileAccess());
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

} // namespace Internal

using namespace Internal;

WslDevice::WslDevice()
    : d(new WslDevicePrivate(this))
{
    distribution.setSettingsKey(WslDeviceDistributionKey);
    distribution.setLabelText(Tr::tr("Distribution:"));
    distribution.setReadOnly(true);

    userName.setSettingsKey(WslDeviceUserNameKey);
    userName.setLabelText(Tr::tr("User name:"));
    userName.setDisplayStyle(StringAspect::LineEditDisplay);
    userName.setToolTip(
        Tr::tr("The user to run commands as. Leave empty to use the default user of the "
               "distribution."));

    mountRoot.setSettingsKey(WslDeviceMountRootKey);
    mountRoot.setLabelText(Tr::tr("Automount root:"));
    mountRoot.setDisplayStyle(StringAspect::LineEditDisplay);
    mountRoot.setPlaceHolderText("/mnt");
    mountRoot.setToolTip(
        Tr::tr("Where the distribution mounts the Windows drives, as the \"root\" entry in the "
               "\"automount\" section of /etc/wsl.conf sets it. Leave empty to read it from "
               "the distribution."));

    appendWindowsPath.setSettingsKey(WslDeviceAppendWindowsPathKey);
    appendWindowsPath.setLabelText(Tr::tr("Keep the Windows PATH:"));
    appendWindowsPath.setDefaultValue(false);
    appendWindowsPath.setLabelPlacement(BoolAspect::LabelPlacement::InExtraLabel);
    appendWindowsPath.setToolTip(
        Tr::tr("WSL appends the host's PATH to the one inside the distribution, so that it can "
               "run Windows programs. Keeping it also makes Qt Creator find the host's own "
               "compilers and Qt installations through the automount root and offer them as "
               "the distribution's."));

    // Without one the debugger gets no QML channel and gives up before it
    // starts.
    freePortsAspect.setDefaultValue("10000-10100");

    allowEmptyCommand.setValue(true);

    setDisplayType(Tr::tr("WSL"));
    setOsType(OsTypeLinux);
    setupId(IDevice::ManuallyAdded);
    setType(Constants::WSL_DEVICE_TYPE);
    setMachineType(IDevice::Hardware);
    offerKitCreation();

    setFileAccessFactory([this]() -> DeviceFileAccessPtr {
        if (DeviceFileAccessPtr fileAccess = d->cachedFileAccess())
            return fileAccess;

        if (DeviceFileAccessPtr fileAccess = d->createFileAccess()) {
            setDeviceState(IDevice::DeviceReadyToUse);
            FSEngine::invalidateFileInfoCache();
            return fileAccess;
        }
        return nullptr;
    });

    setOpenTerminal(
        [this](const Environment &env, const FilePath &workingDir, const Continuation<> &cont) {
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
        });
}

WslDevice::~WslDevice()
{
    delete d;
}

void WslDevice::shutdown()
{
    d->dropFileAccess();
}

CommandLine WslDevice::createCommandLineForDisplay() const
{
    return d->launcherCommandLine(distribution.volatileValue(), userName.volatileValue());
}

IDeviceWidget *WslDevice::createWidget()
{
    return new WslDeviceWidget(shared_from_this());
}

ProcessInterface *WslDevice::createProcessInterface() const
{
    return makeProcessInterface(shared_from_this(), d);
}

FilePath WslDevice::rootPath() const
{
    return FilePath::fromParts(Constants::WSL_DEVICE_SCHEME, distribution(), u"/");
}

bool WslDevice::supportsQtTargetDeviceType(const QSet<Id> &targetDeviceTypes) const
{
    return targetDeviceTypes.contains(ProjectExplorer::Constants::DESKTOP_DEVICE_TYPE)
           || IDevice::supportsQtTargetDeviceType(targetDeviceTypes);
}

Result<> WslDevice::supportsBuildingProject(const FilePath &projectDir) const
{
    return handlesFile(projectDir).or_else([this, projectDir](const QString &) {
        return ensureReachable(projectDir);
    });
}

Result<> WslDevice::handlesFile(const FilePath &filePath) const
{
    // Without regard for case, as the share that carries the same name is
    // matched: see pathInDistributionShare().
    if (filePath.scheme() == Constants::WSL_DEVICE_SCHEME
        && filePath.host().compare(distribution(), Qt::CaseInsensitive) == 0) {
        return ResultOk;
    }
    return IDevice::handlesFile(filePath);
}

Result<> WslDevice::ensureReachable(const FilePath &other) const
{
    if (other.isEmpty())
        return ResultError(Tr::tr("Path is empty."));

    if (other.isSameDevice(rootPath()))
        return ResultOk;

    if (!reachableWslPath(d->mountRoot(), distribution(), other).isEmpty())
        return ResultOk;

    return ResultError(Tr::tr("The path \"%1\" is not reachable from the WSL distribution \"%2\".")
                           .arg(other.toUserOutput(), distribution()));
}

Result<FilePath> WslDevice::localSource(const FilePath &other) const
{
    if (!other.isSameDevice(rootPath())) {
        return ResultError(
            Tr::tr("\"%1\" is not a path in \"%2\".").arg(other.toUserOutput(), displayName()));
    }

    return FilePath::fromUserInput(
        wslPathToHostPath(d->mountRoot(), distribution(), other.path()));
}

FilePath WslDevice::configuredDevicePath(const FilePath &localPath) const
{
    const QString inDistribution = reachableWslPath(d->mountRoot(), distribution(), localPath);
    if (inDistribution.isEmpty())
        return {};
    return rootPath().withNewPath(inDistribution);
}

QUrl WslDevice::toolControlChannel(const ControlChannelHint &) const
{
    QUrl url;
    url.setScheme(Utils::urlTcpScheme());
    // WSL forwards a port a Linux process listens on to the Windows loopback.
    url.setHost("localhost");
    return url;
}

QString WslDevice::deviceStateToString() const
{
    if (deviceState() == IDevice::DeviceDisconnected)
        return Tr::tr("Ready (waiting for access to the distribution...)");
    return IDevice::deviceStateToString();
}

void WslDevice::fromMap(const Store &map)
{
    IDevice::fromMap(map);

    if (displayName() == defaultDisplayName())
        setDefaultDisplayName(Tr::tr("WSL (%1)").arg(distribution()));
}

ExecutableItem WslDevice::signalOperationRecipeImpl(
    const SignalOperationData &data, const Storage<Result<>> &resultStorage) const
{
    Storage<qint64> pid;

    const auto onFindProcessSetup = [this, data](Async<Result<qint64>> &task) {
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
            rootPath());
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

    const auto onProcessSetup = [rootPath = rootPath(), pid, data](Process &process) {
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

namespace Internal {

static bool namedAfterARootDirectory(const QString &distribution)
{
    static const QStringList rootDirectories{
        "bin",
        "boot",
        "dev",
        "etc",
        "home",
        "lib",
        "media",
        "mnt",
        "opt",
        "proc",
        "root",
        "run",
        "sbin",
        "srv",
        "sys",
        "tmp",
        "usr",
        "var"};
    return rootDirectories.contains(distribution, Qt::CaseInsensitive);
}

void configureWslDevice(const WslDevice::Ptr &device, const QString &distribution)
{
    device->distribution.setValue(distribution);
    device->setDefaultDisplayName(Tr::tr("WSL (%1)").arg(distribution));

    if (namedAfterARootDirectory(distribution)) {
        qCWarning(wslDeviceLog).noquote()
            << "The WSL distribution" << distribution
            << "is named after a directory at the root of its own file system. Paths under"
            << '/' + distribution << "are taken for its network share instead.";
    }
}

// Docker Desktop runs its engine in distributions of its own.
static bool isInternalDistribution(const QString &distribution)
{
    static const QStringList internal{"docker-desktop", "docker-desktop-data"};
    return internal.contains(distribution, Qt::CaseInsensitive);
}

static bool hasDeviceFor(const QString &distribution)
{
    bool found = false;
    DeviceManager::forEachDevice([&found, &distribution](const IDeviceConstPtr &dev) {
        if (found || dev->type() != Id(Constants::WSL_DEVICE_TYPE))
            return;
        const auto device = std::dynamic_pointer_cast<const WslDevice>(dev);
        if (device && device->distribution() == distribution)
            found = true;
    });
    return found;
}

static QStringList addableDistributions(const QStringList &installed)
{
    return Utils::filtered(installed, [](const QString &distribution) {
        return !isInternalDistribution(distribution) && !hasDeviceFor(distribution);
    });
}

static QString chooseDistribution()
{
    QDialog dialog(ICore::dialogParent());
    dialog.setWindowTitle(Tr::tr("WSL Distribution Selection"));

    auto combo = new QComboBox;
    combo->setEnabled(false);

    auto info = new InfoLabel(Tr::tr("Asking WSL about the installed distributions..."));
    info->setElideMode(Qt::ElideNone);

    auto buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    buttons->button(QDialogButtonBox::Ok)->setEnabled(false);
    QObject::connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    QObject::connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    // A wedged WSL service would otherwise freeze the settings page for as
    // long as it takes to time out.
    QFuture<Result<QStringList>> future = Utils::asyncRun(&installedDistributions);
    future.then(&dialog, [combo, info, buttons](const Result<QStringList> &installed) {
        if (!installed) {
            // "wsl.exe --list" fails outright on a host that has no
            // distribution installed.
            info->setType(InfoLabelType::Error);
            info->setText(
                Tr::tr("Could not list the WSL distributions of this host. If none is "
                       "installed, install one first.")
                + "\n\n" + installed.error());
            return;
        }

        const QStringList addable = addableDistributions(*installed);
        if (addable.isEmpty()) {
            info->setType(InfoLabelType::Warning);
            info->setText(
                installed->isEmpty()
                    ? Tr::tr("No WSL distribution is installed on this host.")
                    : Tr::tr("Every WSL distribution of this host has a device already."));
            return;
        }

        combo->addItems(addable);
        combo->setEnabled(true);
        info->hide();
        buttons->button(QDialogButtonBox::Ok)->setEnabled(true);
    });
    Utils::futureSynchronizer()->addFuture(future);

    using namespace Layouting;
    // clang-format off
    Column {
        Form {
            Tr::tr("Distribution:"), combo, br,
        },
        info,
        st,
        buttons,
    }.attachTo(&dialog);
    // clang-format on

    if (dialog.exec() != QDialog::Accepted)
        return {};

    return combo->currentText();
}

class WslDeviceFactory final : public IDeviceFactory
{
public:
    WslDeviceFactory()
        : IDeviceFactory(Constants::WSL_DEVICE_TYPE)
    {
        setDisplayName(Tr::tr("WSL Device"));
        setCreator([this]() -> IDevice::Ptr {
            const QString distribution = chooseDistribution();
            if (distribution.isEmpty())
                return {};
            const WslDevice::Ptr device = WslDevice::create();
            configureWslDevice(device, distribution);
            m_existingDevices.writeLocked()->push_back(device);
            return device;
        });
        setConstructionFunction([this] {
            const WslDevice::Ptr device = WslDevice::create();
            m_existingDevices.writeLocked()->push_back(device);
            return device;
        });
        setExecutionTypeId(ProjectExplorer::Constants::STDPROCESS_EXECUTION_TYPE_ID);
    }

    ~WslDeviceFactory() final
    {
        m_existingDevices.read([](const std::vector<std::weak_ptr<WslDevice>> &devices) {
            for (const std::weak_ptr<WslDevice> &weakDevice : devices) {
                if (const WslDevice::Ptr device = weakDevice.lock())
                    device->shutdown();
            }
        });
    }

private:
    SynchronizedValue<std::vector<std::weak_ptr<WslDevice>>> m_existingDevices;
};

void setupWslDevice()
{
    static GuardedObject<WslDeviceFactory> theWslDeviceFactory;
}

QStringList detectWslDistributions()
{
    if (wslExecutable().isEmpty())
        return {};

    const Result<QStringList> distributions = installedDistributions();
    if (!distributions) {
        qCDebug(wslDeviceLog).noquote() << "Not detecting WSL devices:" << distributions.error();
        return {};
    }
    return *distributions;
}

void addMissingWslDevices(const QStringList &distributions)
{
    IDeviceFactory *factory = IDeviceFactory::find(Constants::WSL_DEVICE_TYPE);
    QTC_ASSERT(factory, return);

    QStringList known = ICore::settings()->value(WslKnownDistributionsKey).toStringList();
    const int knownBefore = known.size();

    for (const QString &distribution : distributions) {
        if (isInternalDistribution(distribution))
            continue;

        // A distribution that was offered a device once is not offered one
        // again: the user is free to remove it, and a device that comes back
        // at every start is not removable at all.
        const bool offeredBefore = known.contains(distribution);
        if (!offeredBefore)
            known.append(distribution);
        if (offeredBefore || hasDeviceFor(distribution))
            continue;

        // Through the factory: IDeviceFactory::construct() is what gives a
        // device the tool aspects that the tool detection fills in.
        const auto device = std::dynamic_pointer_cast<WslDevice>(factory->construct());
        QTC_ASSERT(device, continue);
        configureWslDevice(device, distribution);
        // Manually added: the Remove button is enabled for an auto-detected
        // device only while it is disconnected. The id is still the
        // distribution's, so that a device and its kits survive a restart.
        device->setupId(
            IDevice::ManuallyAdded, Id::fromString(QString("wsl.%1").arg(distribution)));
        DeviceManager::addDevice(device);
    }

    if (known.size() != knownBefore)
        ICore::settings()->setValue(WslKnownDistributionsKey, known);
}

} // namespace Internal
} // namespace Wsl
