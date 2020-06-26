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

#include <coreplugin/icore.h>

#include <projectexplorer/applicationlauncher.h>
#include <projectexplorer/devicesupport/idevice.h>
#include <projectexplorer/runcontrol.h>

#include <remotelinux/linuxdeviceprocess.h>

#include <ssh/sshconnection.h>

#include <utils/portlist.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>

#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QWizard>

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
        r.executable = FilePath::fromString(Constants::AppcontrollerFilepath);
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
        showMessage(QdbDevice::tr("Starting command \"%1\" on device \"%2\".")
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
                errorString = QdbDevice::tr("Command failed on device \"%1\": %2")
                        .arg(m_deviceName, m_error);
            } else {
                errorString = QdbDevice::tr("Command failed on device \"%1\".").arg(m_deviceName);
            }
            showMessage(errorString, true);
            if (!m_stdout.isEmpty())
                showMessage(QdbDevice::tr("stdout was: \"%1\"").arg(m_stdout));
            if (!m_stderr.isEmpty())
                showMessage(QdbDevice::tr("stderr was: \"%1\"").arg(m_stderr));
        } else {
            showMessage(QdbDevice::tr("Commands on device \"%1\" finished successfully.")
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
    setDisplayType(tr("Boot2Qt Device"));

    addDeviceAction({tr("Reboot Device"), [](const IDevice::Ptr &device, QWidget *) {
        (void) new DeviceApplicationObserver(device, CommandLine{"reboot"});
    }});

    addDeviceAction({tr("Restore Default App"), [](const IDevice::Ptr &device, QWidget *) {
        (void) new DeviceApplicationObserver(device, CommandLine{"appcontroller", {"--remove-default"}});
    }});
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

// QdbDeviceWizard

class QdbSettingsPage : public QWizardPage
{
public:
    QdbSettingsPage()
    {
        setWindowTitle(QdbDevice::tr("WizardPage"));
        setTitle(QdbDevice::tr("Device Settings"));

        nameLineEdit = new QLineEdit(this);
        nameLineEdit->setPlaceholderText(QdbDevice::tr("A short, free-text description"));

        addressLineEdit = new QLineEdit(this);
        addressLineEdit->setPlaceholderText(QdbDevice::tr("Host name or IP address"));

        auto usbWarningLabel = new QLabel(this);
        usbWarningLabel->setText(QString("<html><head/><body><it><b>%1</it><p>%2</p></body></html>")
                                 .arg("Note:")
                                 .arg("Do not use this wizard for devices connected via USB.<br/>"
                                      "Those will be auto-detected.</p>"
                                      "<p>The connectivity to the device is tested after finishing."));

        auto formLayout = new QFormLayout(this);
        formLayout->addRow(QdbDevice::tr("Device name:"), nameLineEdit);
        formLayout->addRow(QdbDevice::tr("Device address:"), addressLineEdit);
        formLayout->addRow(usbWarningLabel);

        connect(nameLineEdit, &QLineEdit::textChanged, this, &QWizardPage::completeChanged);
        connect(addressLineEdit, &QLineEdit::textChanged, this, &QWizardPage::completeChanged);
    }

    QString deviceName() const { return nameLineEdit->text().trimmed(); }
    QString deviceAddress() const { return addressLineEdit->text().trimmed(); }

private:
    bool isComplete() const final {
        return !deviceName().isEmpty() && !deviceAddress().isEmpty();
    }

    QLineEdit *nameLineEdit;
    QLineEdit *addressLineEdit;

};

class QdbDeviceWizard : public QWizard
{
public:
    QdbDeviceWizard(QWidget *parent)
        : QWizard(parent)
    {
        setWindowTitle(QdbDevice::tr("Boot2Qt Network Device Setup"));
        settingsPage.setCommitPage(true);

        enum { SettingsPageId };

        setPage(SettingsPageId, &settingsPage);
    }

    ProjectExplorer::IDevice::Ptr device()
    {
        QdbDevice::Ptr device = QdbDevice::create();

        device->setDisplayName(settingsPage.deviceName());
        device->setupId(ProjectExplorer::IDevice::ManuallyAdded, Utils::Id());
        device->setType(Constants::QdbLinuxOsType);
        device->setMachineType(ProjectExplorer::IDevice::Hardware);

        device->setupDefaultNetworkSettings(settingsPage.deviceAddress());

        return device;
    }

private:
    QdbSettingsPage settingsPage;
};


// Device factory

QdbLinuxDeviceFactory::QdbLinuxDeviceFactory()
    : IDeviceFactory(Constants::QdbLinuxOsType)
{
    setDisplayName(QdbDevice::tr("Boot2Qt Device"));
    setCombinedIcon(":/qdb/images/qdbdevicesmall.png", ":/qdb/images/qdbdevice.png");
    setCanCreate(true);
    setConstructionFunction(&QdbDevice::create);
}

IDevice::Ptr QdbLinuxDeviceFactory::create() const
{
    QdbDeviceWizard wizard(Core::ICore::dialogParent());

    if (wizard.exec() != QDialog::Accepted)
        return IDevice::Ptr();
    return wizard.device();
}

} // namespace Internal
} // namespace Qdb
