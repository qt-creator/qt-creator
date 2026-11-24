// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "flatpakdevice.h"

#include "bridgeddeviceaccess.h"
#include "flatpakapi.h"
#include "remotelinux_constants.h"
#include "remotelinuxtr.h"

#include <coreplugin/icore.h>

#include <projectexplorer/devicesupport/devicemanager.h>
#include <projectexplorer/devicesupport/idevicefactory.h>
#include <projectexplorer/devicesupport/idevicewidget.h>
#include <projectexplorer/projectexplorerconstants.h>

#include <client/bridgedfileaccess.h>

#include <utils/commandline.h>
#include <utils/devicefileaccess.h>
#include <utils/environment.h>
#include <utils/fsengine/fsengine.h>
#include <utils/guiutils.h>
#include <utils/hostosinfo.h>
#include <utils/infolabel.h>
#include <utils/layoutbuilder.h>
#include <utils/pathchooser.h>
#include <utils/qtcassert.h>
#include <utils/shutdownguard.h>
#include <utils/synchronizedvalue.h>
#include <utils/url.h>

#include <QLoggingCategory>
#include <QMessageBox>

using namespace Core;
using namespace ProjectExplorer;
using namespace QtTaskTree;
using namespace Utils;

namespace Remote {
namespace Internal {

static Q_LOGGING_CATEGORY(flatpakDeviceLog, "qtc.flatpak.device", QtWarningMsg);

const char FlatpakDeviceLauncherKey[] = "FlatpakDeviceLauncher";
const char FlatpakHostDeviceOfferedKey[] = "Flatpak/HostDeviceOffered";

// Nothing to report an unreachable path to here. It fails on the host
// instead.
static QString mappedToHost(const QString &sandboxPath)
{
    const QString onHost = sandboxPathToHostPath(sandboxPath);
    return onHost.isEmpty() ? sandboxPath : onHost;
}

class FlatpakDeviceFileAccess final : public CmdBridge::FileAccess
{
public:
    using CmdBridge::FileAccess::FileAccess;

    QString mapToDevicePath(const QString &sandboxPath) const final
    {
        return mappedToHost(sandboxPath);
    }
};

class FlatpakFallbackFileAccess final : public BridgeFallbackFileAccess
{
public:
    using BridgeFallbackFileAccess::BridgeFallbackFileAccess;

    QString mapToDevicePath(const QString &sandboxPath) const final
    {
        return mappedToHost(sandboxPath);
    }
};

class FlatpakDevicePrivate final : public QObject
{
public:
    explicit FlatpakDevicePrivate(FlatpakDevice *parent)
        : q(parent)
    {
        QObject::connect(q, &FlatpakDevice::applied, this, [this] { m_access.drop(); });
    }

    Result<CommandLine> withHostCmd(
        const QString &markerTemplate,
        const CommandLine &cmd,
        const std::optional<Environment> &env,
        const std::optional<FilePath> &workDir,
        bool withMarker) const;

    FlatpakDevice *const q;

private:
    BridgedDeviceAccess::Hooks accessHooks();

public:
    BridgedDeviceAccess m_access{&flatpakDeviceLog, accessHooks()};
};

BridgedDeviceAccess::Hooks FlatpakDevicePrivate::accessHooks()
{
    BridgedDeviceAccess::Hooks hooks;
    hooks.rootPath = [this] { return q->rootPath(); };
    hooks.bridgeEnvironment = [this] { return detectHostEnvironment(q->launcher()); };
    // The bridge that comes with Qt Creator is in /app, which the host does
    // not see, so it is always copied over.
    hooks.createBridge = [](const std::function<void()> &errorExitHandler) {
        return std::make_unique<FlatpakDeviceFileAccess>(errorExitHandler);
    };
    hooks.createFallback = [this] {
        return std::make_shared<FlatpakFallbackFileAccess>(q->rootPath());
    };
    hooks.dropped = [this] {
        q->invalidateSystemEnvironment();
        q->setDeviceState(IDevice::DeviceDisconnected);
    };
    return hooks;
}

Result<CommandLine> FlatpakDevicePrivate::withHostCmd(
    const QString &markerTemplate,
    const CommandLine &cmd,
    const std::optional<Environment> &env,
    const std::optional<FilePath> &workDir,
    bool withMarker) const
{
    const FilePath launcher = q->launcher();
    if (launcher.isEmpty())
        return ResultError(Tr::tr("The device names no program to run commands on the host."));

    const QString workingDirectory
        = workDir && !workDir->isEmpty() ? q->rootPath().withNewMappedPath(*workDir).path()
                                         : QString();

    QStringList unsetNames;
    if (env) {
        for (const EnvironmentItem &item : q->environmentChanges(*env)) {
            if (item.operation == EnvironmentItem::Unset)
                unsetNames.append(item.name);
        }
    }

    return hostCommandLine(
        launcher, workingDirectory, cmd, env, withMarker ? markerTemplate : QString(), unsetNames);
}

class FlatpakDeviceWidget final : public IDeviceWidget
{
public:
    explicit FlatpakDeviceWidget(const FlatpakDevice::Ptr &device)
        : IDeviceWidget(device)
    {
        auto stateLabel = new InfoLabel;
        stateLabel->setElideMode(Qt::ElideNone);
        if (isInsideFlatpakSandbox()) {
            stateLabel->setType(InfoLabelType::Ok);
            stateLabel->setText(Tr::tr("Qt Creator runs in a Flatpak sandbox."));
        } else {
            stateLabel->setType(InfoLabelType::Warning);
            stateLabel->setText(
                Tr::tr("Qt Creator does not run in a Flatpak sandbox. The device runs its "
                       "commands through the program set above all the same."));
        }

        using namespace Layouting;

        // clang-format off
        Column {
            noMargin,
            Form {
                noMargin,
                Tr::tr("State:"), stateLabel, br,
                device->launcher, br,
                device->freePortsAspect, br,
                device->deviceToolsGui(),
            }, br,
        }.attachTo(this);
        // clang-format on

        installMarkSettingsDirtyTriggerRecursively(this);
    }

    void updateDeviceFromUi() final {}
};

} // namespace Internal

using namespace Internal;

FlatpakDevice::FlatpakDevice()
    : d(new FlatpakDevicePrivate(this))
{
    launcher.setSettingsKey(FlatpakDeviceLauncherKey);
    launcher.setLabelText(Tr::tr("Host launcher:"));
    launcher.setDefaultPathValue(FilePath::fromString(defaultLauncher));
    launcher.setExpectedKind(PathChooserKind::ExistingCommand);
    launcher.setToolTip(
        Tr::tr("The program that runs a command on the host, outside the sandbox. It is called "
               "with \"--host\" and \"--directory=\" like flatpak-spawn. The Flatpak needs "
               "\"--talk-name=org.freedesktop.Flatpak\" for flatpak-spawn to work."));

    // Without one the debugger gets no QML channel and gives up before it
    // starts.
    freePortsAspect.setDefaultValue("10000-10100");

    allowEmptyCommand.setValue(true);

    setDisplayType(Tr::tr("Flatpak Host"));
    setDefaultDisplayName(Tr::tr("Flatpak Host"));
    setOsType(OsTypeLinux);
    setupId(IDevice::ManuallyAdded);
    setType(Constants::FlatpakDeviceType);
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

FlatpakDevice::~FlatpakDevice()
{
    delete d;
}

void FlatpakDevice::shutdown()
{
    d->m_access.drop();
}

IDeviceWidget *FlatpakDevice::createWidget()
{
    return new FlatpakDeviceWidget(std::static_pointer_cast<FlatpakDevice>(shared_from_this()));
}

ProcessInterface *FlatpakDevice::createProcessInterface() const
{
    return createBridgedProcessInterface(
        shared_from_this(),
        [this](
            const QString &markerTemplate,
            const CommandLine &cmd,
            const std::optional<Environment> &env,
            const std::optional<FilePath> &workDir,
            bool withMarker) {
            return d->withHostCmd(markerTemplate, cmd, env, workDir, withMarker);
        });
}

FilePath FlatpakDevice::rootPath() const
{
    return FilePath::fromParts(
        Constants::FlatpakDeviceScheme, Constants::FlatpakDeviceHost, u"/");
}

bool FlatpakDevice::supportsQtTargetDeviceType(const QSet<Id> &targetDeviceTypes) const
{
    return targetDeviceTypes.contains(ProjectExplorer::Constants::DESKTOP_DEVICE_TYPE)
           || IDevice::supportsQtTargetDeviceType(targetDeviceTypes);
}

Result<> FlatpakDevice::supportsBuildingProject(const FilePath &projectDir) const
{
    return handlesFile(projectDir).or_else([this, projectDir](const QString &) {
        return ensureReachable(projectDir);
    });
}

Result<> FlatpakDevice::handlesFile(const FilePath &filePath) const
{
    if (filePath.scheme() == Constants::FlatpakDeviceScheme
        && filePath.host() == Constants::FlatpakDeviceHost) {
        return ResultOk;
    }
    return IDevice::handlesFile(filePath);
}

Result<> FlatpakDevice::ensureReachable(const FilePath &other) const
{
    if (other.isEmpty())
        return ResultError(Tr::tr("Path is empty."));

    if (other.isSameDevice(rootPath()))
        return ResultOk;

    if (!configuredDevicePath(other).isEmpty())
        return ResultOk;

    return ResultError(
        Tr::tr("The path \"%1\" is not reachable from the host.").arg(other.toUserOutput()));
}

Result<FilePath> FlatpakDevice::localSource(const FilePath &other) const
{
    if (!other.isSameDevice(rootPath())) {
        return ResultError(
            Tr::tr("\"%1\" is not a path in \"%2\".").arg(other.toUserOutput(), displayName()));
    }

    const QString inSandbox = hostPathToSandboxPath(other.path());
    if (inSandbox.isEmpty()) {
        return ResultError(
            Tr::tr("The path \"%1\" is not visible in the sandbox.").arg(other.toUserOutput()));
    }

    const FilePath local = FilePath::fromString(inSandbox);
    if (!local.parentDir().exists()) {
        return ResultError(
            Tr::tr("The path \"%1\" is not shared with the sandbox.").arg(other.toUserOutput()));
    }

    return local;
}

FilePath FlatpakDevice::configuredDevicePath(const FilePath &localPath) const
{
    if (!localPath.isLocal())
        return {};

    const QString onHost = sandboxPathToHostPath(localPath.path());
    if (onHost.isEmpty())
        return {};
    return rootPath().withNewPath(onHost);
}

QUrl FlatpakDevice::toolControlChannel(const ControlChannelHint &) const
{
    QUrl url;
    url.setScheme(Utils::urlTcpScheme());
    // A Flatpak with network access shares the host's network namespace.
    url.setHost("localhost");
    return url;
}

QString FlatpakDevice::deviceStateToString() const
{
    if (deviceState() == IDevice::DeviceDisconnected)
        return Tr::tr("Ready (waiting for access to the host...)");
    return IDevice::deviceStateToString();
}

ExecutableItem FlatpakDevice::signalOperationRecipeImpl(
    const SignalOperationData &data, const Storage<Result<>> &resultStorage) const
{
    return bridgedSignalOperationRecipe(rootPath(), data, resultStorage);
}

namespace Internal {

static bool hasFlatpakDevice()
{
    bool found = false;
    DeviceManager::forEachDevice([&found](const IDeviceConstPtr &dev) {
        if (dev->type() == Id(Constants::FlatpakDeviceType))
            found = true;
    });
    return found;
}

class FlatpakDeviceFactory final : public IDeviceFactory
{
public:
    FlatpakDeviceFactory()
        : IDeviceFactory(Constants::FlatpakDeviceType)
    {
        setDisplayName(Tr::tr("Flatpak Host Device"));
        if (HostOsInfo::isLinuxHost())
            setCreator([this]() -> IDevice::Ptr {
                // All Flatpak devices share one root path, so only the first one is ever reached.
                if (hasFlatpakDevice()) {
                    QMessageBox::information(
                        Core::ICore::dialogParent(),
                        Tr::tr("Flatpak Host Device"),
                        Tr::tr("There is a Flatpak host device already."));
                    return {};
                }
                return createDevice();
            });
        setConstructionFunction([this] { return createDevice(); });
        setExecutionTypeId(ProjectExplorer::Constants::STDPROCESS_EXECUTION_TYPE_ID);
    }

    ~FlatpakDeviceFactory() final
    {
        m_existingDevices.read([](const std::vector<std::weak_ptr<FlatpakDevice>> &devices) {
            for (const std::weak_ptr<FlatpakDevice> &weakDevice : devices) {
                if (const FlatpakDevice::Ptr device = weakDevice.lock())
                    device->shutdown();
            }
        });
    }

private:
    FlatpakDevice::Ptr createDevice()
    {
        const FlatpakDevice::Ptr device = FlatpakDevice::create();
        m_existingDevices.writeLocked()->push_back(device);
        return device;
    }

    SynchronizedValue<std::vector<std::weak_ptr<FlatpakDevice>>> m_existingDevices;
};

void setupFlatpakDevice()
{
    static GuardedObject<FlatpakDeviceFactory> theFlatpakDeviceFactory;
}

static void offerFlatpakHostDevice()
{
    // Offered once only: the user is free to remove it, and a device that
    // comes back at every start is not removable at all.
    if (ICore::settings()->value(FlatpakHostDeviceOfferedKey).toBool())
        return;
    ICore::settings()->setValue(FlatpakHostDeviceOfferedKey, true);

    if (hasFlatpakDevice())
        return;

    IDeviceFactory *factory = IDeviceFactory::find(Constants::FlatpakDeviceType);
    QTC_ASSERT(factory, return);

    // Through the factory: IDeviceFactory::construct() is what gives a
    // device the tool aspects that the tool detection fills in.
    const auto device = std::dynamic_pointer_cast<FlatpakDevice>(factory->construct());
    QTC_ASSERT(device, return);
    // Manually added: the Remove button is enabled for an auto-detected
    // device only while it is disconnected.
    device->setupId(IDevice::ManuallyAdded, Id(Constants::FlatpakDeviceId));
    DeviceManager::addDevice(device);
}

void addMissingFlatpakDevice()
{
    if (!isInsideFlatpakSandbox())
        return;

    // The saved devices are not loaded yet when the plugins are
    // initialized, and neither is the choice of the default device.
    if (DeviceManager::isLoaded()) {
        offerFlatpakHostDevice();
        return;
    }
    QObject::connect(
        DeviceManager::instance(),
        &DeviceManager::devicesLoaded,
        DeviceManager::instance(),
        &offerFlatpakHostDevice,
        Qt::SingleShotConnection);
}

} // namespace Internal
} // namespace Remote
