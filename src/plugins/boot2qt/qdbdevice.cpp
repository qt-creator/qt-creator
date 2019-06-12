/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "qdbdevice.h"

#include "qdbutils.h"
#include "qdbconstants.h"
#include "qdbdevicedebugsupport.h"
#include "qdbdevicewizard.h"

#include <coreplugin/icore.h>

#include <projectexplorer/applicationlauncher.h>
#include <projectexplorer/devicesupport/idevice.h>
#include <projectexplorer/runcontrol.h>

#include <remotelinux/linuxdeviceprocess.h>

#include <ssh/sshconnection.h>

#include <utils/portlist.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>

using namespace ProjectExplorer;
using namespace Utils;

namespace Qdb {
namespace Internal {

class QdbDeviceProcess : public RemoteLinux::LinuxDeviceProcess
{
public:
    QdbDeviceProcess(const QSharedPointer<const IDevice> &device, QObject *parent)
        : RemoteLinux::LinuxDeviceProcess(device, parent)
    {
    }

    void terminate() override
    {
        ProjectExplorer::Runnable r;
        r.executable = Constants::AppcontrollerFilepath;
        r.commandLineArguments = QStringLiteral("--stop");

        (new ApplicationLauncher(this))->start(r, device());
    }
};


class DeviceApplicationObserver : public ApplicationLauncher
{
public:
    DeviceApplicationObserver(const IDevice::ConstPtr &device, const CommandLine &command)
    {
        connect(&m_appRunner, &ApplicationLauncher::remoteStdout, this,
                &DeviceApplicationObserver::handleStdout);
        connect(&m_appRunner, &ApplicationLauncher::remoteStderr, this,
                &DeviceApplicationObserver::handleStderr);
        connect(&m_appRunner, &ApplicationLauncher::reportError, this,
                &DeviceApplicationObserver::handleError);
        connect(&m_appRunner, &ApplicationLauncher::finished, this,
                &DeviceApplicationObserver::handleFinished);

        QTC_ASSERT(device, return);
        m_deviceName = device->displayName();

        Runnable r;
        r.setCommandLine(command);
        m_appRunner.start(r, device);
        showMessage(QdbDevice::tr("Starting command '%1' on device '%2'.")
                    .arg(command.toUserOutput(), m_deviceName));
    }

private:
    void handleStdout(const QString &data) { m_stdout += data; }
    void handleStderr(const QString &data) { m_stderr += data; }
    void handleError(const QString &message) { m_error = message; }

    void handleFinished(bool success)
    {
        if (success && (m_stdout.contains("fail") || m_stdout.contains("error")
                        || m_stdout.contains("not found"))) {
            // FIXME: Needed in a post-adb world?
            success = false; // adb does not forward exit codes and all stderr goes to stdout.
        }
        if (!success) {
            QString errorString;
            if (!m_error.isEmpty()) {
                errorString = QdbDevice::tr("Command failed on device '%1': %2")
                        .arg(m_deviceName, m_error);
            } else {
                errorString = QdbDevice::tr("Command failed on device '%1'.").arg(m_deviceName);
            }
            showMessage(errorString, true);
            if (!m_stdout.isEmpty())
                showMessage(QdbDevice::tr("stdout was: '%1'").arg(m_stdout));
            if (!m_stderr.isEmpty())
                showMessage(QdbDevice::tr("stderr was: '%1'").arg(m_stderr));
        } else {
            showMessage(QdbDevice::tr("Commands on device '%1' finished successfully.")
                        .arg(m_deviceName));
        }
        deleteLater();
    }

    QString m_stdout;
    QString m_stderr;
    ProjectExplorer::ApplicationLauncher m_appRunner;
    QString m_deviceName;
    QString m_error;
};


// QdbDevice

QdbDevice::QdbDevice()
{
    addDeviceAction({tr("Reboot Device"), [](const IDevice::Ptr &device, QWidget *) {
        CommandLine cmd{FilePath::fromString("reboot")};
        (void) new DeviceApplicationObserver(device, cmd);
    }});

    addDeviceAction({tr("Restore Default App"), [](const IDevice::Ptr &device, QWidget *) {
        CommandLine cmd{FilePath::fromString("appcontroller"), {"--remove-default"}};
        (void) new DeviceApplicationObserver(device, cmd);
    }});
}

QString QdbDevice::displayType() const
{
    return tr("Boot2Qt Device");
}

ProjectExplorer::IDeviceWidget *QdbDevice::createWidget()
{
    ProjectExplorer::IDeviceWidget *w = RemoteLinux::LinuxDevice::createWidget();

    return w;
}

ProjectExplorer::DeviceProcess *QdbDevice::createProcess(QObject *parent) const
{
    return new QdbDeviceProcess(sharedFromThis(), parent);
}

void QdbDevice::setSerialNumber(const QString &serial)
{
    m_serialNumber = serial;
}

QString QdbDevice::serialNumber() const
{
    return m_serialNumber;
}

void QdbDevice::fromMap(const QVariantMap &map)
{
    ProjectExplorer::IDevice::fromMap(map);
    setSerialNumber(map.value("Qdb.SerialNumber").toString());
}

QVariantMap QdbDevice::toMap() const
{
    QVariantMap map = ProjectExplorer::IDevice::toMap();
    map.insert("Qdb.SerialNumber", serialNumber());
    return map;
}

void QdbDevice::setupDefaultNetworkSettings(const QString &host)
{
    setFreePorts(Utils::PortList::fromString("10000-10100"));

    QSsh::SshConnectionParameters parameters = sshParameters();
    parameters.setHost(host);
    parameters.setUserName("root");
    parameters.setPort(22);
    parameters.timeout = 10;
    parameters.authenticationType = QSsh::SshConnectionParameters::AuthenticationTypeAll;
    setSshParameters(parameters);
}

std::function<ProjectExplorer::RunWorker *(ProjectExplorer::RunControl *)>
    QdbDevice::workerCreator(Core::Id id) const
{
    if (id == "PerfRecorder") {
        return [](ProjectExplorer::RunControl *runControl) {
            return new QdbDevicePerfProfilerSupport(runControl);
        };
    }
    return {};
}


// Device factory

static IDevice::Ptr createDevice(QdbDeviceWizard::DeviceType deviceType)
{
    QdbDeviceWizard wizard(deviceType, Core::ICore::mainWindow());

    if (wizard.exec() != QDialog::Accepted)
        return IDevice::Ptr();
    return wizard.device();
}

QdbLinuxDeviceFactory::QdbLinuxDeviceFactory()
    : IDeviceFactory(Constants::QdbLinuxOsType)
{
    setDisplayName(tr("Boot2Qt Device"));
    setCombinedIcon(":/qdb/images/qdbdevicesmall.png", ":/qdb/images/qdbdevice.png");
    setCanCreate(true);
    setConstructionFunction(&QdbDevice::create);
}

IDevice::Ptr QdbLinuxDeviceFactory::create() const
{
    return createDevice(QdbDeviceWizard::HardwareDevice);
}

} // namespace Internal
} // namespace Qdb
