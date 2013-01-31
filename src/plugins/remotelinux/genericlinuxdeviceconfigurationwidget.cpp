/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "genericlinuxdeviceconfigurationwidget.h"
#include "ui_genericlinuxdeviceconfigurationwidget.h"

#include <utils/portlist.h>
#include <ssh/sshconnection.h>
#include <ssh/sshkeycreationdialog.h>

#include <QTextStream>

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
        ? SshConnectionParameters::AuthenticationByPassword
        : SshConnectionParameters::AuthenticationByKey;
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

void GenericLinuxDeviceConfigurationWidget::updateDeviceFromUi()
{
    hostNameEditingFinished();
    sshPortEditingFinished();
    timeoutEditingFinished();
    userNameEditingFinished();
    passwordEditingFinished();
    keyFileEditingFinished();
    handleFreePortsChanged();
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
    m_ui->portsWarningLabel->setPixmap(QPixmap(QLatin1String(":/projectexplorer/images/compile_error.png")));
    m_ui->portsWarningLabel->setToolTip(QLatin1String("<font color=\"red\">")
        + tr("You will need at least one port.") + QLatin1String("</font>"));
    m_ui->keyFileLineEdit->setExpectedKind(PathChooser::File);
    m_ui->keyFileLineEdit->lineEdit()->setMinimumWidth(0);
    QRegExpValidator * const portsValidator
        = new QRegExpValidator(QRegExp(PortList::regularExpression()), this);
    m_ui->portsLineEdit->setValidator(portsValidator);

    const SshConnectionParameters &sshParams = device()->sshParameters();

    if (sshParams.authenticationType == SshConnectionParameters::AuthenticationByPassword)
        m_ui->passwordButton->setChecked(true);
    else
        m_ui->keyButton->setChecked(true);
    m_ui->timeoutSpinBox->setValue(sshParams.timeout);
    m_ui->hostLineEdit->setEnabled(!device()->isAutoDetected());
    m_ui->sshPortSpinBox->setEnabled(!device()->isAutoDetected());

    m_ui->hostLineEdit->setText(sshParams.host);
    m_ui->sshPortSpinBox->setValue(sshParams.port);
    m_ui->portsLineEdit->setText(device()->freePorts().toString());
    m_ui->timeoutSpinBox->setValue(sshParams.timeout);
    m_ui->userLineEdit->setText(sshParams.userName);
    m_ui->pwdLineEdit->setText(sshParams.password);
    m_ui->keyFileLineEdit->setPath(sshParams.privateKeyFile);
    m_ui->showPasswordCheckBox->setChecked(false);
    updatePortsWarningLabel();
}
