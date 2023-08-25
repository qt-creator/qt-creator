// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "genericlinuxdeviceconfigurationwidget.h"

#include "remotelinux_constants.h"
#include "remotelinuxtr.h"
#include "sshkeycreationdialog.h"

#include <projectexplorer/devicesupport/devicemanager.h>
#include <projectexplorer/devicesupport/idevice.h>
#include <projectexplorer/devicesupport/sshparameters.h>

#include <utils/fancylineedit.h>
#include <utils/layoutbuilder.h>
#include <utils/pathchooser.h>
#include <utils/portlist.h>
#include <utils/utilsicons.h>

#include <QCheckBox>
#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QRadioButton>
#include <QSpacerItem>
#include <QSpinBox>

using namespace ProjectExplorer;
using namespace Utils;

namespace RemoteLinux::Internal {

GenericLinuxDeviceConfigurationWidget::GenericLinuxDeviceConfigurationWidget(
        const IDevice::Ptr &device) :
    IDeviceWidget(device)
{
    m_defaultAuthButton = new QRadioButton(Tr::tr("Default"), this);

    m_keyButton = new QRadioButton(Tr::tr("Specific &key"));

    m_hostLineEdit = new FancyLineEdit(this);
    m_hostLineEdit->setPlaceholderText(Tr::tr("IP or host name of the device"));
    m_hostLineEdit->setHistoryCompleter("HostName");

    m_sshPortSpinBox = new QSpinBox(this);
    m_sshPortSpinBox->setMinimum(0);
    m_sshPortSpinBox->setMaximum(65535);
    m_sshPortSpinBox->setValue(22);

    m_hostKeyCheckBox = new QCheckBox(Tr::tr("&Check host key"));

    m_portsLineEdit = new FancyLineEdit(this);
    m_portsLineEdit->setToolTip(Tr::tr("You can enter lists and ranges like this: '1024,1026-1028,1030'."));
    m_portsLineEdit->setHistoryCompleter("PortRange");

    m_portsWarningLabel = new QLabel(this);

    m_timeoutSpinBox = new QSpinBox(this);
    m_timeoutSpinBox->setMaximum(10000);
    m_timeoutSpinBox->setSingleStep(10);
    m_timeoutSpinBox->setValue(1000);
    m_timeoutSpinBox->setSuffix(Tr::tr("s"));

    m_userLineEdit = new FancyLineEdit(this);
    m_userLineEdit->setHistoryCompleter("UserName");

    m_keyLabel = new QLabel(Tr::tr("Private key file:"));

    m_keyFileLineEdit = new PathChooser(this);

    auto createKeyButton = new QPushButton(Tr::tr("Create New..."));

    m_machineTypeValueLabel = new QLabel(this);

    const QString hint = Tr::tr("Leave empty to look up executable in $PATH");
    m_gdbServerLineEdit = new PathChooser(this);
    m_gdbServerLineEdit->setExpectedKind(PathChooser::ExistingCommand);
    m_gdbServerLineEdit->setPlaceholderText(hint);
    m_gdbServerLineEdit->setToolTip(hint);
    m_gdbServerLineEdit->setHistoryCompleter("GdbServer");
    m_gdbServerLineEdit->setAllowPathFromDevice(true);

    m_qmlRuntimeLineEdit = new PathChooser(this);
    m_qmlRuntimeLineEdit->setExpectedKind(PathChooser::ExistingCommand);
    m_qmlRuntimeLineEdit->setPlaceholderText(hint);
    m_qmlRuntimeLineEdit->setToolTip(hint);
    m_qmlRuntimeLineEdit->setHistoryCompleter("QmlRuntime");
    m_qmlRuntimeLineEdit->setAllowPathFromDevice(true);

    m_sourceProfileCheckBox =
        new QCheckBox(Tr::tr("Source %1 and %2").arg("/etc/profile").arg("$HOME/.profile"));

    m_linkDeviceComboBox = new QComboBox;
    m_linkDeviceComboBox->addItem(Tr::tr("Direct"), QVariant());

    auto dm = DeviceManager::instance();
    const int dmCount = dm->deviceCount();
    for (int i = 0; i < dmCount; ++i) {
        IDevice::ConstPtr dev =  dm->deviceAt(i);
        m_linkDeviceComboBox->addItem(dev->displayName(), dev->id().toSetting());
    }

    auto sshPortLabel = new QLabel(Tr::tr("&SSH port:"));
    sshPortLabel->setBuddy(m_sshPortSpinBox);

    using namespace Layouting;

    Form {
        Tr::tr("Machine type:"), m_machineTypeValueLabel, st, br,
        Tr::tr("Authentication type:"), m_defaultAuthButton, m_keyButton, st, br,
        Tr::tr("&Host name:"), m_hostLineEdit, sshPortLabel, m_sshPortSpinBox, m_hostKeyCheckBox, st, br,
        Tr::tr("Free ports:"), m_portsLineEdit, m_portsWarningLabel, Tr::tr("Timeout:"), m_timeoutSpinBox, st, br,
        Tr::tr("&Username:"), m_userLineEdit, st, br,
        m_keyLabel, m_keyFileLineEdit, createKeyButton, br,
        Tr::tr("GDB server executable:"), m_gdbServerLineEdit, br,
        Tr::tr("QML runtime executable:"), m_qmlRuntimeLineEdit, br,
        QString(), m_sourceProfileCheckBox, br,
        Tr::tr("Access via:"), m_linkDeviceComboBox
    }.attachTo(this);

    connect(m_hostLineEdit, &QLineEdit::editingFinished,
            this, &GenericLinuxDeviceConfigurationWidget::hostNameEditingFinished);
    connect(m_userLineEdit, &QLineEdit::editingFinished,
            this, &GenericLinuxDeviceConfigurationWidget::userNameEditingFinished);
    connect(m_keyFileLineEdit, &PathChooser::editingFinished,
            this, &GenericLinuxDeviceConfigurationWidget::keyFileEditingFinished);
    connect(m_keyFileLineEdit, &PathChooser::browsingFinished,
            this, &GenericLinuxDeviceConfigurationWidget::keyFileEditingFinished);
    connect(m_keyButton, &QAbstractButton::toggled,
            this, &GenericLinuxDeviceConfigurationWidget::authenticationTypeChanged);
    connect(m_timeoutSpinBox, &QAbstractSpinBox::editingFinished,
            this, &GenericLinuxDeviceConfigurationWidget::timeoutEditingFinished);
    connect(m_timeoutSpinBox, &QSpinBox::valueChanged,
            this, &GenericLinuxDeviceConfigurationWidget::timeoutEditingFinished);
    connect(m_sshPortSpinBox, &QAbstractSpinBox::editingFinished,
            this, &GenericLinuxDeviceConfigurationWidget::sshPortEditingFinished);
    connect(m_sshPortSpinBox, &QSpinBox::valueChanged,
            this, &GenericLinuxDeviceConfigurationWidget::sshPortEditingFinished);
    connect(m_portsLineEdit, &QLineEdit::editingFinished,
            this, &GenericLinuxDeviceConfigurationWidget::handleFreePortsChanged);
    connect(createKeyButton, &QAbstractButton::clicked,
            this, &GenericLinuxDeviceConfigurationWidget::createNewKey);
    connect(m_gdbServerLineEdit, &PathChooser::editingFinished,
            this, &GenericLinuxDeviceConfigurationWidget::gdbServerEditingFinished);
    connect(m_qmlRuntimeLineEdit, &PathChooser::editingFinished,
            this, &GenericLinuxDeviceConfigurationWidget::qmlRuntimeEditingFinished);
    connect(m_hostKeyCheckBox, &QCheckBox::toggled,
            this, &GenericLinuxDeviceConfigurationWidget::hostKeyCheckingChanged);
    connect(m_sourceProfileCheckBox, &QCheckBox::toggled,
            this, &GenericLinuxDeviceConfigurationWidget::sourceProfileCheckingChanged);
    connect(m_linkDeviceComboBox, &QComboBox::currentIndexChanged,
            this, &GenericLinuxDeviceConfigurationWidget::linkDeviceChanged);

    initGui();
}

GenericLinuxDeviceConfigurationWidget::~GenericLinuxDeviceConfigurationWidget() = default;

void GenericLinuxDeviceConfigurationWidget::authenticationTypeChanged()
{
    SshParameters sshParams = device()->sshParameters();
    const bool useKeyFile = m_keyButton->isChecked();
    sshParams.authenticationType = useKeyFile
            ? SshParameters::AuthenticationTypeSpecificKey
            : SshParameters::AuthenticationTypeAll;
    device()->setSshParameters(sshParams);
    m_keyFileLineEdit->setEnabled(useKeyFile);
    m_keyLabel->setEnabled(useKeyFile);
}

void GenericLinuxDeviceConfigurationWidget::hostNameEditingFinished()
{
    SshParameters sshParams = device()->sshParameters();
    sshParams.setHost(m_hostLineEdit->text().trimmed());
    device()->setSshParameters(sshParams);
}

void GenericLinuxDeviceConfigurationWidget::sshPortEditingFinished()
{
    SshParameters sshParams = device()->sshParameters();
    sshParams.setPort(m_sshPortSpinBox->value());
    device()->setSshParameters(sshParams);
}

void GenericLinuxDeviceConfigurationWidget::timeoutEditingFinished()
{
    SshParameters sshParams = device()->sshParameters();
    sshParams.timeout = m_timeoutSpinBox->value();
    device()->setSshParameters(sshParams);
}

void GenericLinuxDeviceConfigurationWidget::userNameEditingFinished()
{
    SshParameters sshParams = device()->sshParameters();
    sshParams.setUserName(m_userLineEdit->text());
    device()->setSshParameters(sshParams);
}

void GenericLinuxDeviceConfigurationWidget::keyFileEditingFinished()
{
    SshParameters sshParams = device()->sshParameters();
    sshParams.privateKeyFile = m_keyFileLineEdit->filePath();
    device()->setSshParameters(sshParams);
}

void GenericLinuxDeviceConfigurationWidget::gdbServerEditingFinished()
{
    device()->setDebugServerPath(m_gdbServerLineEdit->filePath());
}

void GenericLinuxDeviceConfigurationWidget::qmlRuntimeEditingFinished()
{
    device()->setQmlRunCommand(m_qmlRuntimeLineEdit->filePath());
}

void GenericLinuxDeviceConfigurationWidget::handleFreePortsChanged()
{
    device()->setFreePorts(PortList::fromString(m_portsLineEdit->text()));
    updatePortsWarningLabel();
}

void GenericLinuxDeviceConfigurationWidget::setPrivateKey(const FilePath &path)
{
    m_keyFileLineEdit->setFilePath(path);
    keyFileEditingFinished();
}

void GenericLinuxDeviceConfigurationWidget::createNewKey()
{
    SshKeyCreationDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted)
        setPrivateKey(dialog.privateKeyFilePath());
}

void GenericLinuxDeviceConfigurationWidget::hostKeyCheckingChanged(bool doCheck)
{
    SshParameters sshParams = device()->sshParameters();
    sshParams.hostKeyCheckingMode
            = doCheck ? SshHostKeyCheckingAllowNoMatch : SshHostKeyCheckingNone;
    device()->setSshParameters(sshParams);
}

void GenericLinuxDeviceConfigurationWidget::sourceProfileCheckingChanged(bool doCheck)
{
    device()->setExtraData(Constants::SourceProfile, doCheck);
}

void GenericLinuxDeviceConfigurationWidget::linkDeviceChanged(int index)
{
    const QVariant deviceId = m_linkDeviceComboBox->itemData(index);
    device()->setExtraData(Constants::LinkDevice, deviceId);
}

void GenericLinuxDeviceConfigurationWidget::updateDeviceFromUi()
{
    hostNameEditingFinished();
    sshPortEditingFinished();
    timeoutEditingFinished();
    userNameEditingFinished();
    keyFileEditingFinished();
    handleFreePortsChanged();
    gdbServerEditingFinished();
    sshPortEditingFinished();
    timeoutEditingFinished();
    sourceProfileCheckingChanged(m_sourceProfileCheckBox->isChecked());
    linkDeviceChanged(m_linkDeviceComboBox->currentIndex());
    qmlRuntimeEditingFinished();
}

void GenericLinuxDeviceConfigurationWidget::updatePortsWarningLabel()
{
    m_portsWarningLabel->setVisible(!device()->freePorts().hasMore());
}

void GenericLinuxDeviceConfigurationWidget::initGui()
{
    if (device()->machineType() == IDevice::Hardware)
        m_machineTypeValueLabel->setText(Tr::tr("Physical Device"));
    else
        m_machineTypeValueLabel->setText(Tr::tr("Emulator"));
    m_portsWarningLabel->setPixmap(Utils::Icons::CRITICAL.pixmap());
    m_portsWarningLabel->setToolTip(QLatin1String("<font color=\"red\">")
        + Tr::tr("You will need at least one port.") + QLatin1String("</font>"));
    m_keyFileLineEdit->setExpectedKind(PathChooser::File);
    m_keyFileLineEdit->setHistoryCompleter("Ssh.KeyFile.History");
    m_keyFileLineEdit->lineEdit()->setMinimumWidth(0);
    QRegularExpressionValidator * const portsValidator
        = new QRegularExpressionValidator(QRegularExpression(PortList::regularExpression()), this);
    m_portsLineEdit->setValidator(portsValidator);

    const SshParameters &sshParams = device()->sshParameters();

    switch (sshParams.authenticationType) {
    case SshParameters::AuthenticationTypeSpecificKey:
        m_keyButton->setChecked(true);
        break;
    case SshParameters::AuthenticationTypeAll:
        m_defaultAuthButton->setChecked(true);
        break;
    }
    m_timeoutSpinBox->setValue(sshParams.timeout);
    m_hostLineEdit->setEnabled(!device()->isAutoDetected());
    m_sshPortSpinBox->setEnabled(!device()->isAutoDetected());
    m_hostKeyCheckBox->setChecked(sshParams.hostKeyCheckingMode != SshHostKeyCheckingNone);
    m_sourceProfileCheckBox->setChecked(device()->extraData(Constants::SourceProfile).toBool());
    Id linkDeviceId = Id::fromSetting(device()->extraData(Constants::LinkDevice));
    auto dm = DeviceManager::instance();
    int found = -1;
    for (int i = 0, n = dm->deviceCount(); i < n; ++i) {
        if (dm->deviceAt(i)->id() == linkDeviceId) {
            found = i;
            break;
        }
    }
    m_linkDeviceComboBox->setCurrentIndex(found + 1); // There's the "Direct" entry first.

    m_hostLineEdit->setText(sshParams.host());
    m_sshPortSpinBox->setValue(sshParams.port());
    m_portsLineEdit->setText(device()->freePorts().toString());
    m_timeoutSpinBox->setValue(sshParams.timeout);
    m_userLineEdit->setText(sshParams.userName());
    m_keyFileLineEdit->setFilePath(sshParams.privateKeyFile);
    m_keyFileLineEdit->setEnabled(
        sshParams.authenticationType == SshParameters::AuthenticationTypeSpecificKey);
    m_gdbServerLineEdit->setFilePath(device()->debugServerPath());
    m_qmlRuntimeLineEdit->setFilePath(device()->qmlRunCommand());

    updatePortsWarningLabel();
}

} // RemoteLinux::Internal
