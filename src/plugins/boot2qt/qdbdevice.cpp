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

#include <projectexplorer/devicesupport/idevice.h>
#include <projectexplorer/runcontrol.h>

#include <remotelinux/linuxprocessinterface.h>

#include <ssh/sshconnection.h>

#include <utils/portlist.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>

#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QWizard>

using namespace ProjectExplorer;
using namespace RemoteLinux;
using namespace Utils;

namespace Qdb {
namespace Internal {

class QdbProcessImpl : public LinuxProcessInterface
{
public:
    QdbProcessImpl(const LinuxDevice *linuxDevice)
        : LinuxProcessInterface(linuxDevice) {}
    ~QdbProcessImpl() { killIfRunning(); }

private:
    void sendControlSignal(ControlSignal controlSignal) final
    {
        QTC_ASSERT(controlSignal != ControlSignal::Interrupt, return);
        QTC_ASSERT(controlSignal != ControlSignal::KickOff, return);
        runInShell({Constants::AppcontrollerFilepath, {"--stop"}});
    }
};

class DeviceApplicationObserver : public QObject
{
public:
    DeviceApplicationObserver(const IDevice::ConstPtr &device, const CommandLine &command)
    {
        connect(&m_appRunner, &QtcProcess::done, this, &DeviceApplicationObserver::handleDone);

        QTC_ASSERT(device, return);
        m_deviceName = device->displayName();

        m_appRunner.setCommand(command);
        m_appRunner.start();
        showMessage(QdbDevice::tr("Starting command \"%1\" on device \"%2\".")
                    .arg(command.toUserOutput(), m_deviceName));
    }

private:
    void handleDone()
    {
        const QString stdOut = m_appRunner.stdOut();
        const QString stdErr = m_appRunner.stdErr();

        // FIXME: Needed in a post-adb world?
        // adb does not forward exit codes and all stderr goes to stdout.
        const bool failure = m_appRunner.result() != ProcessResult::FinishedWithSuccess
                || stdOut.contains("fail")
                || stdOut.contains("error")
                || stdOut.contains("not found");

        if (failure) {
            QString errorString;
            if (!m_appRunner.errorString().isEmpty()) {
                errorString = QdbDevice::tr("Command failed on device \"%1\": %2")
                        .arg(m_deviceName, m_appRunner.errorString());
            } else {
                errorString = QdbDevice::tr("Command failed on device \"%1\".").arg(m_deviceName);
            }
            showMessage(errorString, true);
            if (!stdOut.isEmpty())
                showMessage(QdbDevice::tr("stdout was: \"%1\"").arg(stdOut));
            if (!stdErr.isEmpty())
                showMessage(QdbDevice::tr("stderr was: \"%1\"").arg(stdErr));
        } else {
            showMessage(QdbDevice::tr("Commands on device \"%1\" finished successfully.")
                        .arg(m_deviceName));
        }
        deleteLater();
    }

    QtcProcess m_appRunner;
    QString m_deviceName;
};


// QdbDevice

QdbDevice::QdbDevice()
{
    setDisplayType(tr("Boot2Qt Device"));

    addDeviceAction({tr("Reboot Device"), [this](const IDevice::Ptr &device, QWidget *) {
                         (void) new DeviceApplicationObserver(device, {filePath("reboot"), {}});
    }});

    addDeviceAction({tr("Restore Default App"), [this](const IDevice::Ptr &device, QWidget *) {
        (void) new DeviceApplicationObserver(device, {filePath("appcontroller"), {"--remove-default"}});
    }});
}

ProjectExplorer::IDeviceWidget *QdbDevice::createWidget()
{
    ProjectExplorer::IDeviceWidget *w = RemoteLinux::LinuxDevice::createWidget();

    return w;
}

ProcessInterface *QdbDevice::createProcessInterface() const
{
    return new QdbProcessImpl(this);
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
    setConstructionFunction(&QdbDevice::create);
    setCreator([] {
        QdbDeviceWizard wizard(Core::ICore::dialogParent());
        if (wizard.exec() != QDialog::Accepted)
            return IDevice::Ptr();
        return wizard.device();
    });
}

} // namespace Internal
} // namespace Qdb
