// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "desktopdevice.h"

#include "../projectexplorerconstants.h"
#include "../projectexplorertr.h"
#include "desktopdeviceconfigurationwidget.h"
#include "desktopprocesssignaloperation.h"

#include <coreplugin/fileutils.h>

#include <utils/async.h>
#include <utils/devicefileaccess.h>
#include <utils/environment.h>
#include <utils/hostosinfo.h>
#include <utils/portlist.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>
#include <utils/terminalcommand.h>
#include <utils/terminalhooks.h>
#include <utils/url.h>

#include <QCoreApplication>

#ifdef Q_OS_WIN
#include <cstring>
#include <stdlib.h>
#include <windows.h>
#endif

using namespace ProjectExplorer::Constants;
using namespace Utils;

namespace ProjectExplorer {

class DesktopDevicePrivate : public QObject
{};

DesktopDevice::DesktopDevice()
    : d(new DesktopDevicePrivate())
{
    setFileAccess(DesktopDeviceFileAccess::instance());

    setupId(IDevice::AutoDetected, DESKTOP_DEVICE_ID);
    setType(DESKTOP_DEVICE_TYPE);
    setDefaultDisplayName(Tr::tr("Local PC"));
    setDisplayType(Tr::tr("Desktop"));

    setDeviceState(IDevice::DeviceStateUnknown);
    setMachineType(IDevice::Hardware);
    setOsType(HostOsInfo::hostOs());

    const QString portRange
        = QString::fromLatin1("%1-%2").arg(DESKTOP_PORT_START).arg(DESKTOP_PORT_END);
    setFreePorts(Utils::PortList::fromString(portRange));

    setOpenTerminal([](const Environment &env, const FilePath &path) -> expected_str<void> {
        const Environment realEnv = env.hasChanges() ? env : Environment::systemEnvironment();

        const expected_str<FilePath> shell = Terminal::defaultShellForDevice(path);
        if (!shell)
            return make_unexpected(shell.error());

        Process process;
        process.setTerminalMode(TerminalMode::Detached);
        process.setEnvironment(realEnv);
        process.setCommand(CommandLine{*shell});
        process.setWorkingDirectory(path);
        process.start();

        return {};
    });
}

DesktopDevice::~DesktopDevice() = default;

IDevice::DeviceInfo DesktopDevice::deviceInformation() const
{
    return {};
}

IDeviceWidget *DesktopDevice::createWidget()
{
    return new DesktopDeviceConfigurationWidget(shared_from_this());
}

bool DesktopDevice::canCreateProcessModel() const
{
    return true;
}

DeviceProcessSignalOperation::Ptr DesktopDevice::signalOperation() const
{
    return DeviceProcessSignalOperation::Ptr(new DesktopProcessSignalOperation());
}

QUrl DesktopDevice::toolControlChannel(const ControlChannelHint &) const
{
    QUrl url;
    url.setScheme(Utils::urlTcpScheme());
    url.setHost("localhost");
    return url;
}

bool DesktopDevice::usableAsBuildDevice() const
{
    return true;
}

bool DesktopDevice::handlesFile(const FilePath &filePath) const
{
    return !filePath.needsDevice();
}

FilePath DesktopDevice::filePath(const QString &pathOnDevice) const
{
    return FilePath::fromParts({}, {}, pathOnDevice);
}

expected_str<Environment> DesktopDevice::systemEnvironmentWithError() const
{
    return Environment::systemEnvironment();
}

FilePath DesktopDevice::rootPath() const
{
    if (id() == DESKTOP_DEVICE_ID)
        return HostOsInfo::root();
    return IDevice::rootPath();
}

struct DeployToolsAvailability
{
    bool rsync;
    bool sftp;
};

static DeployToolsAvailability hostDeployTools()
{
    auto check = [](const QString &tool) {
        return FilePath::fromPathPart(tool).searchInPath().isExecutableFile();
    };
    return {check("rsync"), check("sftp")};
}

void DesktopDevice::fromMap(const Store &map)
{
    IDevice::fromMap(map);

    auto updateExtraData = [this](const DeployToolsAvailability &tools) {
        setExtraData(Constants::SUPPORTS_RSYNC, tools.rsync);
        setExtraData(Constants::SUPPORTS_SFTP, tools.sftp);
    };

    if (HostOsInfo::isWindowsHost()) {
        QFutureWatcher<DeployToolsAvailability> *w = new QFutureWatcher<DeployToolsAvailability>(this);
        connect(w, &QFutureWatcher<DeployToolsAvailability>::finished, this, [w, updateExtraData]() {
            updateExtraData(w->result());
            w->deleteLater();
        });
        w->setFuture(Utils::asyncRun([]() { return hostDeployTools(); }));
    } else {
        updateExtraData(hostDeployTools());
    }
}

} // namespace ProjectExplorer
