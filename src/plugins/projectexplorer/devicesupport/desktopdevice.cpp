// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "desktopdevice.h"

#include "../projectexplorerconstants.h"
#include "../projectexplorertr.h"
#include "desktopprocesssignaloperation.h"
#include "deviceprocesslist.h"
#include "processlist.h"

#include <coreplugin/fileutils.h>

#include <utils/devicefileaccess.h>
#include <utils/environment.h>
#include <utils/hostosinfo.h>
#include <utils/portlist.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>
#include <utils/terminalcommand.h>
#include <utils/url.h>

#include <QCoreApplication>
#include <QDateTime>

#ifdef Q_OS_WIN
#include <windows.h>
#include <stdlib.h>
#include <cstring>
#endif

using namespace ProjectExplorer::Constants;
using namespace Utils;

namespace ProjectExplorer {

static void startTerminalEmulator(const QString &workingDir, const Environment &env)
{
#ifdef Q_OS_WIN
    STARTUPINFO si;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);

    PROCESS_INFORMATION pinfo;
    ZeroMemory(&pinfo, sizeof(pinfo));

    static const auto quoteWinCommand = [](const QString &program) {
        const QChar doubleQuote = QLatin1Char('"');

        // add the program as the first arg ... it works better
        QString programName = program;
        programName.replace(QLatin1Char('/'), QLatin1Char('\\'));
        if (!programName.startsWith(doubleQuote) && !programName.endsWith(doubleQuote)
                && programName.contains(QLatin1Char(' '))) {
            programName.prepend(doubleQuote);
            programName.append(doubleQuote);
        }
        return programName;
    };
    const QString cmdLine = quoteWinCommand(qtcEnvironmentVariable("COMSPEC"));
    // cmdLine is assumed to be detached -
    // https://blogs.msdn.microsoft.com/oldnewthing/20090601-00/?p=18083

    const QString totalEnvironment = env.toStringList().join(QChar(QChar::Null)) + QChar(QChar::Null);
    LPVOID envPtr = (env != Environment::systemEnvironment())
            ? (WCHAR *)(totalEnvironment.utf16()) : nullptr;

    const bool success = CreateProcessW(0, (WCHAR *)cmdLine.utf16(),
                                  0, 0, FALSE, CREATE_NEW_CONSOLE | CREATE_UNICODE_ENVIRONMENT,
                                  envPtr, workingDir.isEmpty() ? 0 : (WCHAR *)workingDir.utf16(),
                                  &si, &pinfo);

    if (success) {
        CloseHandle(pinfo.hThread);
        CloseHandle(pinfo.hProcess);
    }
#else
    const TerminalCommand term = TerminalCommand::terminalEmulator();
    QProcess process;
    process.setProgram(term.command.nativePath());
    process.setArguments(ProcessArgs::splitArgs(term.openArgs, HostOsInfo::hostOs()));
    process.setProcessEnvironment(env.toProcessEnvironment());
    process.setWorkingDirectory(workingDir);
    process.startDetached();
#endif
}

DesktopDevice::DesktopDevice()
{
    setFileAccess(DesktopDeviceFileAccess::instance());

    setupId(IDevice::AutoDetected, DESKTOP_DEVICE_ID);
    setType(DESKTOP_DEVICE_TYPE);
    setDefaultDisplayName(Tr::tr("Local PC"));
    setDisplayType(Tr::tr("Desktop"));

    setDeviceState(IDevice::DeviceStateUnknown);
    setMachineType(IDevice::Hardware);
    setOsType(HostOsInfo::hostOs());

    const QString portRange =
            QString::fromLatin1("%1-%2").arg(DESKTOP_PORT_START).arg(DESKTOP_PORT_END);
    setFreePorts(Utils::PortList::fromString(portRange));

    setOpenTerminal([](const Environment &env, const FilePath &path) {
        const QFileInfo fileInfo = path.toFileInfo();
        const QString workingDir = QDir::toNativeSeparators(fileInfo.isDir() ?
                                                            fileInfo.absoluteFilePath() :
                                                            fileInfo.absolutePath());
        const Environment realEnv = env.hasChanges() ? env : Environment::systemEnvironment();
        startTerminalEmulator(workingDir, realEnv);
    });
}

IDevice::DeviceInfo DesktopDevice::deviceInformation() const
{
    return DeviceInfo();
}

IDeviceWidget *DesktopDevice::createWidget()
{
    return nullptr;
    // DesktopDeviceConfigurationWidget currently has just one editable field viz. free ports.
    // Querying for an available port is quite straightforward. Having a field for the port
    // range can be confusing to the user. Hence, disabling the widget for now.
}

bool DesktopDevice::canAutoDetectPorts() const
{
    return true;
}

bool DesktopDevice::canCreateProcessModel() const
{
    return true;
}

DeviceProcessList *DesktopDevice::createProcessListModel(QObject *parent) const
{
    return new ProcessList(sharedFromThis(), parent);
}

DeviceProcessSignalOperation::Ptr DesktopDevice::signalOperation() const
{
    return DeviceProcessSignalOperation::Ptr(new DesktopProcessSignalOperation());
}

PortsGatheringMethod DesktopDevice::portsGatheringMethod() const
{
    return {
        [this](QAbstractSocket::NetworkLayerProtocol protocol) -> CommandLine {
            // We might encounter the situation that protocol is given IPv6
            // but the consumer of the free port information decides to open
            // an IPv4(only) port. As a result the next IPv6 scan will
            // report the port again as open (in IPv6 namespace), while the
            // same port in IPv4 namespace might still be blocked, and
            // re-use of this port fails.
            // GDBserver behaves exactly like this.

            Q_UNUSED(protocol)

            if (HostOsInfo::isWindowsHost() || HostOsInfo::isMacHost())
                return {filePath("netstat"), {"-a", "-n"}};
            if (HostOsInfo::isLinuxHost())
                return {filePath("/bin/sh"), {"-c", "cat /proc/net/tcp*"}};
            return {};
        },

        &Port::parseFromNetstatOutput
    };
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

Environment DesktopDevice::systemEnvironment() const
{
    return Environment::systemEnvironment();
}

FilePath DesktopDevice::rootPath() const
{
    if (id() == DESKTOP_DEVICE_ID)
        return FilePath::fromParts({}, {}, QDir::rootPath());
    return IDevice::rootPath();
}

} // namespace ProjectExplorer
