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
#include <coreplugin/coreicons.h>
#include <utils/portlist.h>
#include <utils/fancylineedit.h>
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
    connect(m_ui->hostLineEdit, SIGNAL(editingFinished()), this, SLOT(hostNameEditingFinished()));
    connect(m_ui->userLineEdit, SIGNAL(editingFinished()), this, SLOT(userNameEditingFinished()));
    connect(m_ui->pwdLineEdit, SIGNAL(editingFinished()), this, SLOT(passwordEditingFinished()));
    connect(m_ui->passwordButton, SIGNAL(toggled(bool)), this, SLOT(authenticationTypeChanged()));
    connect(m_ui->keyFileLineEdit, SIGNAL(editingFinished()), this, SLOT(keyFileEditingFinished()));
    connect(m_ui->keyFileLineEdit, SIGNAL(browsingFinished()), this, SLOT(keyFileEditingFinished()));
    connect(m_ui->keyButton, SIGNAL(toggled(bool)), this, SLOT(authenticationTypeChanged()));
    connect(m_ui->timeoutSpinBox, SIGNAL(editingFinished()), this, SLOT(timeoutEditingFinished()));
    connect(m_ui->timeoutSpinBox, SIGNAL(valueChanged(int)), this, SLOT(timeoutEditingFinished()));
    connect(m_ui->sshPortSpinBox, SIGNAL(editingFinished()), this, SLOT(sshPortEditingFinished()));
    connect(m_ui->sshPortSpinBox, SIGNAL(valueChanged(int)), this, SLOT(sshPortEditingFinished()));
    connect(m_ui->showPasswordCheckBox, SIGNAL(toggled(bool)), this, SLOT(showPassword(bool)));
    connect(m_ui->portsLineEdit, SIGNAL(editingFinished()), this, SLOT(handleFreePortsChanged()));
    connect(m_ui->createKeyButton, SIGNAL(clicked()), SLOT(createNewKey()));
    connect(m_ui->gdbServerLineEdit, SIGNAL(editingFinished()), SLOT(gdbServerEditingFinished()));
    connect(m_ui->hostKeyCheckBox, &QCheckBox::toggled, this,
            &GenericLinuxDeviceConfigurationWidget::hostKeyCheckingChanged);

    initGui();
}

GenericLinuxDeviceConfigurationWidget::~GenericLinuxDeviceConfigurationWidget()
{
    delete m_ui;
}

void GenericLinuxDeviceConfigurationWidget::authenticationTypeChanged()
{
    SshConnectionParameters sshParams = device()->sshParameters();
    const bool usePassword = m_ui->passwordButton->isChecked();
    sshParams.authenticationType = usePassword
        ? SshConnectionParameters::AuthenticationTypeTryAllPasswordBasedMethods
        : SshConnectionParameters::AuthenticationTypePublicKey;
    device()->setSshParameters(sshParams);
    m_ui->pwdLineEdit->setEnabled(usePassword);
    m_ui->passwordLabel->setEnabled(usePassword);
    m_ui->keyFileLineEdit->setEnabled(!usePassword);
    m_ui->keyLabel->setEnabled(!usePassword);
}

void GenericLinuxDeviceConfigurationWidget::hostNameEditingFinished()
{
    SshConnectionParameters sshParams = device()->sshParameters();
    sshParams.host = m_ui->hostLineEdit->text().trimmed();
    device()->setSshParameters(sshParams);
}

void GenericLinuxDeviceConfigurationWidget::sshPortEditingFinished()
{
    SshConnectionParameters sshParams = device()->sshParameters();
    sshParams.port = m_ui->sshPortSpinBox->value();
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
    sshParams.userName = m_ui->userLineEdit->text();
    device()->setSshParameters(sshParams);
}

void GenericLinuxDeviceConfigurationWidget::passwordEditingFinished()
{
    SshConnectionParameters sshParams = device()->sshParameters();
    sshParams.password = m_ui->pwdLineEdit->text();
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

void GenericLinuxDeviceConfigurationWidget::showPassword(bool showClearText)
{
    m_ui->pwdLineEdit->setEchoMode(showClearText
        ? QLineEdit::Normal : QLineEdit::Password);
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
    passwordEditingFinished();
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
    m_ui->portsWarningLabel->setPixmap(Core::Icons::ERROR.pixmap());
    m_ui->portsWarningLabel->setToolTip(QLatin1String("<font color=\"red\">")
        + tr("You will need at least one port.") + QLatin1String("</font>"));
    m_ui->keyFileLineEdit->setExpectedKind(PathChooser::File);
    m_ui->keyFileLineEdit->setHistoryCompleter(QLatin1String("Ssh.KeyFile.History"));
    m_ui->keyFileLineEdit->lineEdit()->setMinimumWidth(0);
    QRegExpValidator * const portsValidator
        = new QRegExpValidator(QRegExp(PortList::regularExpression()), this);
    m_ui->portsLineEdit->setValidator(portsValidator);

    const SshConnectionParameters &sshParams = device()->sshParameters();

    if (sshParams.authenticationType != SshConnectionParameters::AuthenticationTypePublicKey)
        m_ui->passwordButton->setChecked(true);
    else
        m_ui->keyButton->setChecked(true);
    m_ui->timeoutSpinBox->setValue(sshParams.timeout);
    m_ui->hostLineEdit->setEnabled(!device()->isAutoDetected());
    m_ui->sshPortSpinBox->setEnabled(!device()->isAutoDetected());
    m_ui->hostKeyCheckBox->setChecked(sshParams.hostKeyCheckingMode != SshHostKeyCheckingNone);

    m_ui->hostLineEdit->setText(sshParams.host);
    m_ui->sshPortSpinBox->setValue(sshParams.port);
    m_ui->portsLineEdit->setText(device()->freePorts().toString());
    m_ui->timeoutSpinBox->setValue(sshParams.timeout);
    m_ui->userLineEdit->setText(sshParams.userName);
    m_ui->pwdLineEdit->setText(sshParams.password);
    m_ui->keyFileLineEdit->setPath(sshParams.privateKeyFile);
    m_ui->showPasswordCheckBox->setChecked(false);
    m_ui->gdbServerLineEdit->setText(device()->debugServerPath());
    updatePortsWarningLabel();
}
