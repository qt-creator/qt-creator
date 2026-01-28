// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "desktopdevice.h"

#include "../projectexplorerconstants.h"
#include "../projectexplorertr.h"
#include "devicemanager.h"
#include "idevice.h"
#include "idevicewidget.h"

#include <coreplugin/fileutils.h>

#include <QtTaskTree/QSingleTaskTreeRunner>

#include <utils/async.h>
#include <utils/devicefileaccess.h>
#include <utils/environment.h>
#include <utils/fileutils.h>
#include <utils/globaltasktree.h>
#include <utils/hostosinfo.h>
#include <utils/infolabel.h>
#include <utils/layoutbuilder.h>
#include <utils/portlist.h>
#include <utils/processinfo.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>
#include <utils/terminalcommand.h>
#include <utils/terminalhooks.h>
#include <utils/url.h>
#include <utils/utilsicons.h>
#include <utils/winutils.h>

#include <QCoreApplication>
#include <QDir>
#include <QGuiApplication>
#include <QLineEdit>
#include <QPushButton>
#include <QRegularExpressionValidator>

#ifdef Q_OS_WIN
#include <cstring>
#include <stdlib.h>
#include <windows.h>
#ifndef PROCESS_SUSPEND_RESUME
#define PROCESS_SUSPEND_RESUME 0x0800
#endif // PROCESS_SUSPEND_RESUME
#else // Q_OS_WIN
#include <errno.h>
#include <signal.h>
#endif // else Q_OS_WIN

using namespace ProjectExplorer::Constants;
using namespace QtTaskTree;
using namespace Utils;

namespace ProjectExplorer {

static Result<> cannotKillError(qint64 pid, const QString &why)
{
    return ResultError(Tr::tr("Cannot kill process with pid %1: %2").arg(pid).arg(why));
}

static Result<> appendCannotInterruptError(qint64 pid, const QString &why,
                                           const Result<> &previousResult = ResultOk)
{
    QString error = Tr::tr("Cannot interrupt process with pid %1: %2").arg(pid).arg(why);
    if (previousResult.has_value())
        error.append('\n' + previousResult.error());
    return ResultError(error);
}

static Result<> killProcessSilently(qint64 pid)
{
#ifdef Q_OS_WIN
    const DWORD rights = PROCESS_QUERY_INFORMATION|PROCESS_SET_INFORMATION
                         |PROCESS_VM_OPERATION|PROCESS_VM_WRITE|PROCESS_VM_READ
                         |PROCESS_DUP_HANDLE|PROCESS_TERMINATE|PROCESS_CREATE_THREAD|PROCESS_SUSPEND_RESUME;
    if (const HANDLE handle = OpenProcess(rights, FALSE, DWORD(pid))) {
        const Result<> result = TerminateProcess(handle, UINT(-1))
        ? ResultOk : cannotKillError(pid, winErrorMessage(GetLastError()));
        CloseHandle(handle);
        return result;
    } else {
        return cannotKillError(pid, Tr::tr("Cannot open process."));
    }
#else
    if (pid <= 0)
        return cannotKillError(pid, Tr::tr("Invalid process id."));
    else if (kill(pid, SIGKILL))
        return cannotKillError(pid, QString::fromLocal8Bit(strerror(errno)));
    return ResultOk;
#endif // Q_OS_WIN
}

static Result<> interruptProcessSilently(qint64 pid, const Utils::FilePath &debuggerCommand)
{
#ifdef Q_OS_WIN
    Result<> result = ResultOk;
    enum SpecialInterrupt { NoSpecialInterrupt, Win32Interrupt, Win64Interrupt };

    bool is64BitSystem = is64BitWindowsSystem();
    SpecialInterrupt si = NoSpecialInterrupt;
    if (is64BitSystem)
        si = is64BitWindowsBinary(debuggerCommand) ? Win64Interrupt : Win32Interrupt;
    /*
    Windows 64 bit has a 32 bit subsystem (WOW64) which makes it possible to run a
    32 bit application inside a 64 bit environment.
    When GDB is used DebugBreakProcess must be called from the same system (32/64 bit) running
    the inferior. If CDB is used we could in theory break wow64 processes,
    but the break is actually a wow64 breakpoint. CDB is configured to ignore these
    breakpoints, because they also appear on module loading.
    Therefore we need helper executables (win(32/64)interrupt.exe) on Windows 64 bit calling
    DebugBreakProcess from the correct system.

    DebugBreak matrix for windows

    Api = UseDebugBreakApi
    Win64 = UseWin64InterruptHelper
    Win32 = UseWin32InterruptHelper
    N/A = This configuration is not possible

          | Windows 32bit   | Windows 64bit
          | QtCreator 32bit | QtCreator 32bit                   | QtCreator 64bit
          | Inferior 32bit  | Inferior 32bit  | Inferior 64bit  | Inferior 32bit  | Inferior 64bit
----------|-----------------|-----------------|-----------------|-----------------|----------------
CDB 32bit | Api             | Api             | N/A             | Win32           | N/A
    64bit | N/A             | Win64           | Win64           | Api             | Api
----------|-----------------|-----------------|-----------------|-----------------|----------------
GDB 32bit | Api             | Api             | N/A             | Win32           | N/A
    64bit | N/A             | N/A             | Win64           | N/A             | Api
----------|-----------------|-----------------|-----------------|-----------------|----------------

    */
    HANDLE inferior = NULL;
    do {
        const DWORD rights = PROCESS_QUERY_INFORMATION|PROCESS_SET_INFORMATION
                             |PROCESS_VM_OPERATION|PROCESS_VM_WRITE|PROCESS_VM_READ
                             |PROCESS_DUP_HANDLE|PROCESS_TERMINATE|PROCESS_CREATE_THREAD|PROCESS_SUSPEND_RESUME;
        inferior = OpenProcess(rights, FALSE, pid);
        if (inferior == NULL) {
            return appendCannotInterruptError(pid, Tr::tr("Cannot open process: %1")
                                              + winErrorMessage(GetLastError()), result);
        }
        bool creatorIs64Bit = is64BitWindowsBinary(
            FilePath::fromUserInput(QCoreApplication::applicationFilePath()));
        if (!is64BitSystem
            || si == NoSpecialInterrupt
            || (si == Win64Interrupt && creatorIs64Bit)
            || (si == Win32Interrupt && !creatorIs64Bit)) {
            if (!DebugBreakProcess(inferior)) {
                result = appendCannotInterruptError(pid, Tr::tr("DebugBreakProcess failed:")
                                                    + QLatin1Char(' ') + winErrorMessage(GetLastError()), result);
            }
        } else if (si == Win32Interrupt || si == Win64Interrupt) {
            QString executable = QCoreApplication::applicationDirPath();
            executable += si == Win32Interrupt
                              ? QLatin1String("/win32interrupt.exe")
                              : QLatin1String("/win64interrupt.exe");
            if (!QFileInfo::exists(executable)) {
                result = appendCannotInterruptError(
                    pid,
                    Tr::tr("%1 does not exist. Your %2 installation seems to be corrupt.")
                        .arg(
                            QDir::toNativeSeparators(executable),
                            QGuiApplication::applicationDisplayName()),
                    result);
            }
            switch (QProcess::execute(executable, QStringList(QString::number(pid)))) {
            case -2:
                return appendCannotInterruptError(pid, Tr::tr(
                                                           "Cannot start %1. Check src\\tools\\win64interrupt\\win64interrupt.c "
                                                           "for more information.").arg(QDir::toNativeSeparators(executable)), result);
            case 0:
                break;
            default:
                return appendCannotInterruptError(pid, QDir::toNativeSeparators(executable)
                                                           + QLatin1Char(' ') + Tr::tr("could not break the process."), result);
                break;
            }
        }
    } while (false);
    if (inferior != NULL)
        CloseHandle(inferior);
    return result;
#else
    Q_UNUSED(debuggerCommand)
    if (pid <= 0)
        return appendCannotInterruptError(pid, Tr::tr("Invalid process id."));
    else if (kill(pid, SIGINT))
        return appendCannotInterruptError(pid, QString::fromLocal8Bit(strerror(errno)));
    return ResultOk;
#endif // Q_OS_WIN
}

static Result<> doSignalOperation(const SignalOperationData &data)
{
    Result<> result = ResultOk;
    switch (data.mode) {
    case SignalOperationMode::KillByPath: {
        const QList<ProcessInfo> processInfoList
            = ProcessInfo::processInfoList().value_or(QList<ProcessInfo>());
        for (const ProcessInfo &processInfo : processInfoList) {
            if (processInfo.commandLine == data.filePath.path())
                result = killProcessSilently(processInfo.processId);
        }
        break;
    }
    case SignalOperationMode::KillByPid:
        result = killProcessSilently(data.pid);
        break;
    case SignalOperationMode::InterruptByPid:
        result = interruptProcessSilently(data.pid, data.debuggerPath);
        break;
    };
    return result;
}

class DesktopDeviceConfigurationWidget final : public IDeviceWidget
{
public:
    explicit DesktopDeviceConfigurationWidget(const IDevicePtr &device)
        : IDeviceWidget(device)
    {
        m_freePortsLineEdit = new QLineEdit;
        m_portsWarningLabel = new InfoLabel(
            Tr::tr("You will need at least one port for QML debugging."),
            InfoLabel::Warning);

        auto autoDetectButton = new QPushButton(Tr::tr("Run Auto-Detection Now"));

        connect(autoDetectButton, &QPushButton::clicked, autoDetectButton, [device, autoDetectButton] {
            autoDetectButton->setEnabled(false);

            // clang-format off
            GlobalTaskTree::start(Group {
                device->autoDetectDeviceToolsRecipe(),
                QSyncTask([btn = QPointer<QWidget>(autoDetectButton)] {
                    if (btn)
                        btn->setEnabled(true);
                })
            });
            // clang-format on
        });

        using namespace Layouting;
        Form {
            Tr::tr("Machine type:"), Tr::tr("Physical Device"), br,
            Tr::tr("Free ports:"), m_freePortsLineEdit, br,
            empty, m_portsWarningLabel, br,
            noMargin,
            device->deviceToolsGui(),
            Row { autoDetectButton, st, },
        }.attachTo(this);

        connect(m_freePortsLineEdit, &QLineEdit::textChanged,
                this, &DesktopDeviceConfigurationWidget::updateFreePorts);

        QTC_CHECK(device->machineType() == IDevice::Hardware);
        m_freePortsLineEdit->setPlaceholderText(
                    QString::fromLatin1("eg: %1-%2").arg(DESKTOP_PORT_START).arg(DESKTOP_PORT_END));
        const auto portsValidator = new QRegularExpressionValidator(
            QRegularExpression(PortList::regularExpression()), this);
        m_freePortsLineEdit->setValidator(portsValidator);

        m_freePortsLineEdit->setText(device->freePorts().toString());
        updateFreePorts();
    }

    void updateDeviceFromUi() final
    {
        updateFreePorts();
    }

private:
    void updateFreePorts()
    {
        device()->setFreePorts(PortList::fromString(m_freePortsLineEdit->text()));
        m_portsWarningLabel->setVisible(!device()->freePorts().hasMore());
    }

    QLineEdit *m_freePortsLineEdit;
    QLabel *m_portsWarningLabel;
};

class DesktopDevicePrivate
{
public:
    QSingleTaskTreeRunner taskTreeRunner;
};

DesktopDevice::DesktopDevice()
    : d(new DesktopDevicePrivate)
{
    setFileAccess(DesktopDeviceFileAccess::instance());

    setupId(IDevice::AutoDetected, DESKTOP_DEVICE_ID);
    setType(DESKTOP_DEVICE_TYPE);
    setDefaultDisplayName(Tr::tr("Local PC"));
    setDisplayType(Tr::tr("Desktop"));

    DeviceManager::setDeviceState(id(), IDevice::DeviceReadyToUse, false);
    setMachineType(IDevice::Hardware);
    setOsType(HostOsInfo::hostOs());

    const QString portRange
        = QString::fromLatin1("%1-%2").arg(DESKTOP_PORT_START).arg(DESKTOP_PORT_END);
    setFreePorts(Utils::PortList::fromString(portRange));

    setOpenTerminal([](const Environment &env, const FilePath &path, const Continuation<> &cont) {
        const Environment realEnv = env.hasChanges() ? env : Environment::systemEnvironment();

        const Result<FilePath> shell = Terminal::defaultShellForDevice(path);
        if (!shell)
            return cont(ResultError(shell.error()));

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

        cont(ResultOk);
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

ExecutableItem DesktopDevice::signalOperationRecipe(const SignalOperationData &data,
                                                    const Storage<Result<>> &resultStorage) const
{
    const auto onSetup = [data, resultStorage] {
        const auto validResult = data.isValid();
        if (!validResult) {
            *resultStorage = validResult;
            return DoneResult::Error;
        }
        *resultStorage = doSignalOperation(data);
        return toDoneResult(*resultStorage == ResultOk);
    };

    return Group { QSyncTask(onSetup) };
}

QUrl DesktopDevice::toolControlChannel(const ControlChannelHint &) const
{
    QUrl url;
    url.setScheme(Utils::urlTcpScheme());
    url.setHost("localhost");
    return url;
}

Result<> DesktopDevice::handlesFile(const FilePath &filePath) const
{
    if (!filePath.isLocal())
        return ResultError(Tr::tr("\"%1\" can only handle local files.").arg(displayName()));

    if (deviceState() != DeviceReadyToUse)
        return ResultError(Tr::tr("Device \"%1\" is not ready to use.").arg(displayName()));

    return ResultOk;
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

void DesktopDevice::initDeviceToolAspects()
{
    IDevice::initDeviceToolAspects();
    GlobalTaskTree::start(autoDetectDeviceToolsRecipe());
}

} // namespace ProjectExplorer
