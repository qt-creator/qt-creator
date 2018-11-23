/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "genericlinuxdeviceconfigurationwidget.h"
#include "ui_genericlinuxdeviceconfigurationwidget.h"

#include <coreplugin/coreconstants.h>
#include <utils/portlist.h>
#include <utils/fancylineedit.h>
#include <utils/utilsicons.h>
#include <ssh/sshconnection.h>
#include <ssh/sshkeycreationdialog.h>

using namespace ProjectExplorer;
using namespace RemoteLinux;
using namespace QSsh;
using namespace Utils;

GenericLinuxDeviceConfigurationWidget::GenericLinuxDeviceConfigurationWidget(
        const IDevice::Ptr &deviceConfig, QWidget *parent) :
    IDeviceWidget(deviceConfig, parent),
    m_ui(new Ui::GenericLinuxDeviceConfigurationWidget)
{
    m_ui->setupUi(this);
    connect(m_ui->hostLineEdit, &QLineEdit::editingFinished,
            this, &GenericLinuxDeviceConfigurationWidget::hostNameEditingFinished);
    connect(m_ui->userLineEdit, &QLineEdit::editingFinished,
            this, &GenericLinuxDeviceConfigurationWidget::userNameEditingFinished);
    connect(m_ui->keyFileLineEdit, &PathChooser::editingFinished,
            this, &GenericLinuxDeviceConfigurationWidget::keyFileEditingFinished);
    connect(m_ui->keyFileLineEdit, &PathChooser::browsingFinished,
            this, &GenericLinuxDeviceConfigurationWidget::keyFileEditingFinished);
    connect(m_ui->keyButton, &QAbstractButton::toggled,
            this, &GenericLinuxDeviceConfigurationWidget::authenticationTypeChanged);
    connect(m_ui->timeoutSpinBox, &QAbstractSpinBox::editingFinished,
            this, &GenericLinuxDeviceConfigurationWidget::timeoutEditingFinished);
    connect(m_ui->timeoutSpinBox, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this, &GenericLinuxDeviceConfigurationWidget::timeoutEditingFinished);
    connect(m_ui->sshPortSpinBox, &QAbstractSpinBox::editingFinished,
            this, &GenericLinuxDeviceConfigurationWidget::sshPortEditingFinished);
    connect(m_ui->sshPortSpinBox, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this, &GenericLinuxDeviceConfigurationWidget::sshPortEditingFinished);
    connect(m_ui->portsLineEdit, &QLineEdit::editingFinished,
            this, &GenericLinuxDeviceConfigurationWidget::handleFreePortsChanged);
    connect(m_ui->createKeyButton, &QAbstractButton::clicked,
            this, &GenericLinuxDeviceConfigurationWidget::createNewKey);
    connect(m_ui->gdbServerLineEdit, &QLineEdit::editingFinished,
            this, &GenericLinuxDeviceConfigurationWidget::gdbServerEditingFinished);
    connect(m_ui->hostKeyCheckBox, &QCheckBox::toggled,
            this, &GenericLinuxDeviceConfigurationWidget::hostKeyCheckingChanged);
    m_ui->gdbServerLineEdit->setToolTip(m_ui->gdbServerLineEdit->placeholderText());

    initGui();
}

GenericLinuxDeviceConfigurationWidget::~GenericLinuxDeviceConfigurationWidget()
{
    delete m_ui;
}

void GenericLinuxDeviceConfigurationWidget::authenticationTypeChanged()
{
    SshConnectionParameters sshParams = device()->sshParameters();
    const bool useKeyFile = m_ui->keyButton->isChecked();
    sshParams.authenticationType = useKeyFile
            ? SshConnectionParameters::AuthenticationTypeSpecificKey
            : SshConnectionParameters::AuthenticationTypeAll;
    device()->setSshParameters(sshParams);
    m_ui->keyFileLineEdit->setEnabled(useKeyFile);
    m_ui->keyLabel->setEnabled(useKeyFile);
}

void GenericLinuxDeviceConfigurationWidget::hostNameEditingFinished()
{
    SshConnectionParameters sshParams = device()->sshParameters();
    sshParams.setHost(m_ui->hostLineEdit->text().trimmed());
    device()->setSshParameters(sshParams);
}

void GenericLinuxDeviceConfigurationWidget::sshPortEditingFinished()
{
    SshConnectionParameters sshParams = device()->sshParameters();
    sshParams.setPort(m_ui->sshPortSpinBox->value());
    device()->setSshParameters(sshParams);
}

void GenericLinuxDeviceConfigurationWidget::timeoutEditingFinished()
{
    SshConnectionParameters sshParams = device()->sshParameters();
    sshParams.timeout = m_ui->timeoutSpinBox->value();
    device()->setSshParameters(sshParams);
}

void GenericLinuxDeviceConfigurationWidget::userNameEditingFinished()
{
    SshConnectionParameters sshParams = device()->sshParameters();
    sshParams.setUserName(m_ui->userLineEdit->text());
    device()->setSshParameters(sshParams);
}

void GenericLinuxDeviceConfigurationWidget::keyFileEditingFinished()
{
    SshConnectionParameters sshParams = device()->sshParameters();
    sshParams.privateKeyFile = m_ui->keyFileLineEdit->path();
    device()->setSshParameters(sshParams);
}

void GenericLinuxDeviceConfigurationWidget::gdbServerEditingFinished()
{
    device()->setDebugServerPath(m_ui->gdbServerLineEdit->text());
}

void GenericLinuxDeviceConfigurationWidget::handleFreePortsChanged()
{
    device()->setFreePorts(PortList::fromString(m_ui->portsLineEdit->text()));
    updatePortsWarningLabel();
}

void GenericLinuxDeviceConfigurationWidget::setPrivateKey(const QString &path)
{
    m_ui->keyFileLineEdit->setPath(path);
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
    SshConnectionParameters sshParams = device()->sshParameters();
    sshParams.hostKeyCheckingMode
            = doCheck ? SshHostKeyCheckingAllowNoMatch : SshHostKeyCheckingNone;
    device()->setSshParameters(sshParams);
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
}

void GenericLinuxDeviceConfigurationWidget::updatePortsWarningLabel()
{
    m_ui->portsWarningLabel->setVisible(!device()->freePorts().hasMore());
}

void GenericLinuxDeviceConfigurationWidget::initGui()
{
    if (device()->machineType() == IDevice::Hardware)
        m_ui->machineTypeValueLabel->setText(tr("Physical Device"));
    else
        m_ui->machineTypeValueLabel->setText(tr("Emulator"));
    m_ui->portsWarningLabel->setPixmap(Utils::Icons::CRITICAL.pixmap());
    m_ui->portsWarningLabel->setToolTip(QLatin1String("<font color=\"red\">")
        + tr("You will need at least one port.") + QLatin1String("</font>"));
    m_ui->keyFileLineEdit->setExpectedKind(PathChooser::File);
    m_ui->keyFileLineEdit->setHistoryCompleter(QLatin1String("Ssh.KeyFile.History"));
    m_ui->keyFileLineEdit->lineEdit()->setMinimumWidth(0);
    QRegExpValidator * const portsValidator
        = new QRegExpValidator(QRegExp(PortList::regularExpression()), this);
    m_ui->portsLineEdit->setValidator(portsValidator);

    const SshConnectionParameters &sshParams = device()->sshParameters();

    switch (sshParams.authenticationType) {
    case SshConnectionParameters::AuthenticationTypeSpecificKey:
        m_ui->keyButton->setChecked(true);
        break;
    case SshConnectionParameters::AuthenticationTypeAll:
        m_ui->defaultAuthButton->setChecked(true);
        break;
    }
    m_ui->timeoutSpinBox->setValue(sshParams.timeout);
    m_ui->hostLineEdit->setEnabled(!device()->isAutoDetected());
    m_ui->sshPortSpinBox->setEnabled(!device()->isAutoDetected());
    m_ui->hostKeyCheckBox->setChecked(sshParams.hostKeyCheckingMode != SshHostKeyCheckingNone);

    m_ui->hostLineEdit->setText(sshParams.host());
    m_ui->sshPortSpinBox->setValue(sshParams.port());
    m_ui->portsLineEdit->setText(device()->freePorts().toString());
    m_ui->timeoutSpinBox->setValue(sshParams.timeout);
    m_ui->userLineEdit->setText(sshParams.userName());
    m_ui->keyFileLineEdit->setPath(sshParams.privateKeyFile);
    m_ui->gdbServerLineEdit->setText(device()->debugServerPath());
    updatePortsWarningLabel();
}
