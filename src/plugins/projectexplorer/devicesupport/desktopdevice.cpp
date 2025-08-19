// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "desktopdevice.h"

#include "../projectexplorerconstants.h"
#include "../projectexplorertr.h"
#include "desktopdeviceconfigurationwidget.h"
#include "desktopprocesssignaloperation.h"

#include <coreplugin/fileutils.h>

#include <solutions/tasking/tasktreerunner.h>

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
using namespace Tasking;
using namespace Utils;

namespace ProjectExplorer {

class DesktopDevicePrivate
{
public:
    SingleTaskTreeRunner taskTreeRunner;
};

DesktopDevice::DesktopDevice()
    : d(new DesktopDevicePrivate)
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

    setOpenTerminal([](const Environment &env, const FilePath &path) -> Result<> {
        const Environment realEnv = env.hasChanges() ? env : Environment::systemEnvironment();

        const Result<FilePath> shell = Terminal::defaultShellForDevice(path);
        if (!shell)
            return make_unexpected(shell.error());

        Process process;
        process.setTerminalMode(TerminalMode::Detached);
        process.setEnvironment(realEnv);
        process.setCommand(CommandLine{*shell});
        FilePath workingDir = path;
        if (!workingDir.isDir())
            workingDir = workingDir.parentDir();
        if (QTC_GUARD(workingDir.exists()))
            process.setWorkingDirectory(workingDir);
        process.start();

        return {};
    });

    struct DeployToolsAvailability
    {
        bool rsync;
        bool sftp;
    };

    static auto hostDeployTools = []() -> DeployToolsAvailability
    {
        static auto check = [](const QString &tool) {
            return FilePath::fromPathPart(tool).searchInPath().isExecutableFile();
        };
        return {check("rsync"), check("sftp")};
    };

    auto updateExtraData = [this](const DeployToolsAvailability &tools) {
        setExtraData(Constants::SUPPORTS_RSYNC, tools.rsync);
        setExtraData(Constants::SUPPORTS_SFTP, tools.sftp);
    };

    if (HostOsInfo::isWindowsHost()) {
        const auto onSetup = [](Async<DeployToolsAvailability> &task) {
            task.setConcurrentCallData(hostDeployTools);
        };
        const auto onDone = [updateExtraData](const Async<DeployToolsAvailability> &task) {
            updateExtraData(task.result());
        };
        d->taskTreeRunner.start({AsyncTask<DeployToolsAvailability>(onSetup, onDone)});
    } else {
        updateExtraData(hostDeployTools());
    }
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

bool DesktopDevice::handlesFile(const FilePath &filePath) const
{
    return filePath.isLocal();
}

FilePath DesktopDevice::filePath(const QString &pathOnDevice) const
{
    return FilePath::fromParts({}, {}, pathOnDevice);
}

Result<Environment> DesktopDevice::systemEnvironmentWithError() const
{
    return Environment::systemEnvironment();
}

FilePath DesktopDevice::rootPath() const
{
    // FIXME: This is ugly as  .filePath(xxx) and .rootPath().withNewPath(xxx) diverge here.
    if (id() == DESKTOP_DEVICE_ID)
        return HostOsInfo::root();
    return IDevice::rootPath();
}

void DesktopDevice::fromMap(const Store &map)
{
    IDevice::fromMap(map);
}

} // namespace ProjectExplorer
