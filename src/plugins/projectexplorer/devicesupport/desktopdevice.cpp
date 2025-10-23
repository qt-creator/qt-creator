// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "desktopdevice.h"

#include "../projectexplorerconstants.h"
#include "../projectexplorertr.h"
#include "desktopprocesssignaloperation.h"
#include "devicemanager.h"
#include "idevice.h"
#include "idevicewidget.h"

#include <coreplugin/fileutils.h>

#include <solutions/tasking/tasktreerunner.h>

#include <utils/async.h>
#include <utils/devicefileaccess.h>
#include <utils/environment.h>
#include <utils/hostosinfo.h>
#include <utils/infolabel.h>
#include <utils/layoutbuilder.h>
#include <utils/portlist.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>
#include <utils/terminalcommand.h>
#include <utils/terminalhooks.h>
#include <utils/url.h>
#include <utils/utilsicons.h>

#include <QCoreApplication>
#include <QLineEdit>
#include <QPushButton>
#include <QRegularExpressionValidator>

#ifdef Q_OS_WIN
#include <cstring>
#include <stdlib.h>
#include <windows.h>
#endif

using namespace ProjectExplorer::Constants;
using namespace Tasking;
using namespace Utils;

namespace ProjectExplorer {

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

        connect(&m_detectionRunner, &Tasking::SingleTaskTreeRunner::aboutToStart, [=] {
            autoDetectButton->setEnabled(false);
        });
        connect(&m_detectionRunner, &Tasking::SingleTaskTreeRunner::done, [=] {
            autoDetectButton->setEnabled(true);
        });

        connect(autoDetectButton, &QPushButton::clicked, this, [device, autoDetectButton] {
            autoDetectButton->setEnabled(false);
            device->autoDetectDeviceTools();
            autoDetectButton->setEnabled(true);
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
    Tasking::SingleTaskTreeRunner m_detectionRunner;
};

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

    DeviceManager::setDeviceState(id(), IDevice::DeviceReadyToUse, true);
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
