// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qdbdevice.h"

#include "qdbconstants.h"
#include "qdbtr.h"
#include "qdbutils.h"

#include <coreplugin/icore.h>

#include <projectexplorer/devicesupport/idevice.h>
#include <projectexplorer/devicesupport/sshparameters.h>

#include <remotelinux/linuxprocessinterface.h>

#include <utils/portlist.h>
#include <utils/process.h>
#include <utils/qtcassert.h>
#include <utils/theme/theme.h>

#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QWizard>

using namespace ProjectExplorer;
using namespace RemoteLinux;
using namespace Utils;

namespace Qdb::Internal {

class QdbProcessImpl : public SshProcessInterface
{
public:
    QdbProcessImpl(const IDevice::ConstPtr &device) : SshProcessInterface(device) {}
    ~QdbProcessImpl() { killIfRunning(); }

private:
    void handleSendControlSignal(ControlSignal controlSignal) final
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
        connect(&m_appRunner, &Process::done, this, &DeviceApplicationObserver::handleDone);

        QTC_ASSERT(device, return);
        m_deviceName = device->displayName();

        m_appRunner.setCommand(command);
        m_appRunner.start();
        showMessage(Tr::tr("Starting command \"%1\" on device \"%2\".")
                    .arg(command.toUserOutput(), m_deviceName));
    }

private:
    void handleDone()
    {
        const QString stdOut = m_appRunner.cleanedStdOut();
        const QString stdErr = m_appRunner.cleanedStdErr();

        // FIXME: Needed in a post-adb world?
        // adb does not forward exit codes and all stderr goes to stdout.
        const bool failure = m_appRunner.result() != ProcessResult::FinishedWithSuccess
                || stdOut.contains("fail")
                || stdOut.contains("error")
                || stdOut.contains("not found");

        if (failure) {
            QString errorString;
            if (!m_appRunner.errorString().isEmpty()) {
                errorString = Tr::tr("Command failed on device \"%1\": %2")
                        .arg(m_deviceName, m_appRunner.errorString());
            } else {
                errorString = Tr::tr("Command failed on device \"%1\".").arg(m_deviceName);
            }
            showMessage(errorString, true);
            if (!stdOut.isEmpty())
                showMessage(Tr::tr("stdout was: \"%1\"").arg(stdOut));
            if (!stdErr.isEmpty())
                showMessage(Tr::tr("stderr was: \"%1\"").arg(stdErr));
        } else {
            showMessage(Tr::tr("Commands on device \"%1\" finished successfully.")
                        .arg(m_deviceName));
        }
        deleteLater();
    }

    Process m_appRunner;
    QString m_deviceName;
};


// QdbDevice

QdbDevice::QdbDevice()
{
    setDisplayType(Tr::tr("Boot2Qt Device"));
    setType(Constants::QdbLinuxOsType);

    addDeviceAction({Tr::tr("Reboot Device"), [](const IDevice::Ptr &device, QWidget *) {
        (void) new DeviceApplicationObserver(device, {device->filePath("reboot"), {}});
    }});

    addDeviceAction({Tr::tr("Restore Default App"), [](const IDevice::Ptr &device, QWidget *) {
        (void) new DeviceApplicationObserver(device, {device->filePath("appcontroller"), {"--remove-default"}});
    }});
}

ProjectExplorer::IDeviceWidget *QdbDevice::createWidget()
{
    ProjectExplorer::IDeviceWidget *w = RemoteLinux::LinuxDevice::createWidget();

    return w;
}

ProcessInterface *QdbDevice::createProcessInterface() const
{
    return new QdbProcessImpl(sharedFromThis());
}

void QdbDevice::setSerialNumber(const QString &serial)
{
    m_serialNumber = serial;
}

QString QdbDevice::serialNumber() const
{
    return m_serialNumber;
}

void QdbDevice::fromMap(const Store &map)
{
    ProjectExplorer::IDevice::fromMap(map);
    setSerialNumber(map.value("Qdb.SerialNumber").toString());
}

Store QdbDevice::toMap() const
{
    Store map = ProjectExplorer::IDevice::toMap();
    map.insert("Qdb.SerialNumber", serialNumber());
    return map;
}

void QdbDevice::setupDefaultNetworkSettings(const QString &host)
{
    setFreePorts(PortList::fromString("10000-10100"));

    SshParameters parameters = sshParameters();
    parameters.setHost(host);
    parameters.setUserName("root");
    parameters.setPort(22);
    parameters.timeout = 10;
    parameters.authenticationType = SshParameters::AuthenticationTypeAll;
    setSshParameters(parameters);
}

// QdbDeviceWizard

class QdbSettingsPage : public QWizardPage
{
public:
    QdbSettingsPage()
    {
        setWindowTitle(Tr::tr("WizardPage"));
        setTitle(Tr::tr("Device Settings"));

        nameLineEdit = new QLineEdit(this);
        nameLineEdit->setPlaceholderText(Tr::tr("A short, free-text description"));

        addressLineEdit = new QLineEdit(this);
        addressLineEdit->setPlaceholderText(Tr::tr("Host name or IP address"));

        auto usbWarningLabel = new QLabel(this);
        usbWarningLabel->setText(QString("<html><head/><body><it><b>%1</it><p>%2</p></body></html>")
                                 .arg("Note:")
                                 .arg("Do not use this wizard for devices connected via USB.<br/>"
                                      "Those will be auto-detected.</p>"
                                      "<p>The connectivity to the device is tested after finishing."));

        auto formLayout = new QFormLayout(this);
        formLayout->addRow(Tr::tr("Device name:"), nameLineEdit);
        formLayout->addRow(Tr::tr("Device address:"), addressLineEdit);
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
        setWindowTitle(Tr::tr("Boot2Qt Network Device Setup"));
        settingsPage.setCommitPage(true);

        enum { SettingsPageId };

        setPage(SettingsPageId, &settingsPage);
    }

    ProjectExplorer::IDevice::Ptr device()
    {
        QdbDevice::Ptr device = QdbDevice::create();

        device->settings()->displayName.setValue(settingsPage.deviceName());
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
    setDisplayName(Tr::tr("Boot2Qt Device"));
    setCombinedIcon(":/qdb/images/qdbdevicesmall.png", ":/qdb/images/qdbdevice.png");
    setQuickCreationAllowed(true);
    setConstructionFunction(&QdbDevice::create);
    setCreator([] {
        QdbDeviceWizard wizard(Core::ICore::dialogParent());
        if (!creatorTheme()->preferredStyles().isEmpty())
            wizard.setWizardStyle(QWizard::ModernStyle);
        if (wizard.exec() != QDialog::Accepted)
            return IDevice::Ptr();
        return wizard.device();
    });
}

} // Qdb::Internal
