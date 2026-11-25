// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "wsldevice.h"

#include "bridgeddeviceaccess.h"
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
#include <utils/qtcassert.h>
#include <utils/shutdownguard.h>
#include <utils/synchronizedvalue.h>
#include <utils/url.h>

#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QLoggingCategory>
#include <QPushButton>
#include <QThread>

#include <optional>

#ifdef Q_OS_WIN
#include <qt_windows.h>
#endif

using namespace Core;
using namespace ProjectExplorer;
using namespace QtTaskTree;
using namespace Remote::Internal;
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

class WslFallbackFileAccess final : public BridgeFallbackFileAccess
{
public:
    WslFallbackFileAccess(WslDevicePrivate *dev, const FilePath &rootPath)
        : BridgeFallbackFileAccess(rootPath)
        , m_dev(dev)
    {}

    // Not the "env" UnixDeviceFileAccess would run, so that leaving the
    // Windows PATH out means the same with the bridge and without it.
    Result<Environment> deviceEnvironment() const final;

private:
    WslDevicePrivate *const m_dev;
};

class WslDevicePrivate final : public QObject
{
public:
    explicit WslDevicePrivate(WslDevice *parent)
        : q(parent)
    {
        QObject::connect(q, &WslDevice::applied, this, [this] { m_access.drop(); });
    }

    Result<CommandLine> withWslCmd(
        const QString &markerTemplate,
        const CommandLine &cmd,
        const std::optional<Environment> &env,
        const std::optional<FilePath> &workDir,
        bool withMarker) const;

    CommandLine launcherCommandLine(const QString &distribution, const QString &userName) const;

    Result<OsArch> osArch() const;
    Result<FilePath> cmdBridgePathInDistribution() const;
    Result<Environment> fetchEnvironment() const;

    QString distribution() const;
    QString mountRoot() const;

    WslDevice *const q;

private:
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

    BridgedDeviceAccess::Hooks accessHooks();

    mutable SynchronizedValue<std::optional<OsArch>> m_osArch;
    mutable SynchronizedValue<MountRootDetection> m_detectedMountRoot;

public:
    // Last: it waits for the bring-ups, which use the members above.
    BridgedDeviceAccess m_access{&wslDeviceLog, accessHooks()};
};

QString WslDeviceFileAccess::mapToDevicePath(const QString &hostPath) const
{
    return mappedHostPathToWslPath(m_dev->mountRoot(), m_dev->distribution(), hostPath);
}

Result<Environment> WslFallbackFileAccess::deviceEnvironment() const
{
    return m_dev->fetchEnvironment();
}

BridgedDeviceAccess::Hooks WslDevicePrivate::accessHooks()
{
    BridgedDeviceAccess::Hooks hooks;
    hooks.rootPath = [this] { return q->rootPath(); };
    hooks.bridgeEnvironment = [this] { return fetchEnvironment(); };
    hooks.bridgeInPlace = [this] { return cmdBridgePathInDistribution(); };
    hooks.createBridge = [this](const std::function<void()> &errorExitHandler) {
        return std::make_unique<WslDeviceFileAccess>(this, errorExitHandler);
    };
    hooks.createFallback = [this] {
        return std::make_shared<WslFallbackFileAccess>(this, q->rootPath());
    };
    hooks.resetCaches = [this] {
        m_osArch.writeLocked()->reset();
        m_detectedMountRoot.writeLocked()->reset();
    };
    hooks.dropped = [this] {
        q->invalidateSystemEnvironment();
        q->setDeviceState(IDevice::DeviceDisconnected);
    };
    return hooks;
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
    const Result<QString> root = detectMountRoot(q->launcher(), q->distribution());

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
    QFuture<Result<QString>> future = Utils::asyncRun(&detectMountRoot, q->launcher(),
                                                           q->distribution());
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
    m_access.dropStale();
    FSEngine::invalidateFileInfoCache();
}

CommandLine WslDevicePrivate::launcherCommandLine(
    const QString &distribution, const QString &userName) const
{
    CommandLine cmd{q->launcher(), {"--distribution", distribution}};
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
    const FilePath wslExe = q->launcher();
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

    const Result<OsArch> arch = detectArchitecture(q->launcher(), q->distribution());
    if (arch)
        *m_osArch.writeLocked() = *arch;
    return arch;
}

Result<Environment> WslDevicePrivate::fetchEnvironment() const
{
    const Result<Environment> env = detectEnvironment(q->launcher(), q->distribution(),
                                                       q->userName());
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
        if (DeviceFileAccessPtr fileAccess = d->m_access.cachedFileAccess())
            return fileAccess;

        if (DeviceFileAccessPtr fileAccess = d->m_access.fileAccess()) {
            setDeviceState(IDevice::DeviceReadyToUse);
            FSEngine::invalidateFileInfoCache();
            return fileAccess;
        }
        return nullptr;
    });

    setOpenTerminal(openTerminalWithDefaultShell([this] { return rootPath(); }));
}

WslDevice::~WslDevice()
{
    delete d;
}

void WslDevice::shutdown()
{
    d->m_access.drop();
}

FilePath WslDevice::launcher() const
{
    return wslExecutable();
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
    return createBridgedProcessInterface(
        shared_from_this(),
        [this](
            const QString &markerTemplate,
            const CommandLine &cmd,
            const std::optional<Environment> &env,
            const std::optional<FilePath> &workDir,
            bool withMarker) {
            return d->withWslCmd(markerTemplate, cmd, env, workDir, withMarker);
        });
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
    return bridgedSignalOperationRecipe(rootPath(), data, resultStorage);
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
    QFuture<Result<QStringList>> future = Utils::asyncRun(&installedDistributions, wslExecutable());
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
    const FilePath wsl = wslExecutable();
    if (wsl.isEmpty())
        return {};

    const Result<QStringList> distributions = installedDistributions(wsl);
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
