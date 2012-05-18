/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "genericlinuxdeviceconfigurationwidget.h"
#include "ui_genericlinuxdeviceconfigurationwidget.h"

#include <utils/portlist.h>
#include <ssh/sshconnection.h>
#include <ssh/sshkeycreationdialog.h>

#include <QTextStream>

using namespace RemoteLinux;
using namespace QSsh;
using namespace Utils;

GenericLinuxDeviceConfigurationWidget::GenericLinuxDeviceConfigurationWidget(
        const LinuxDeviceConfiguration::Ptr &deviceConfig,
        QWidget *parent) :
    ProjectExplorer::IDeviceWidget(deviceConfig, parent),
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
    SshConnectionParameters sshParams = deviceConfiguration()->sshParameters();
    const bool usePassword = m_ui->passwordButton->isChecked();
    sshParams.authenticationType = usePassword
        ? SshConnectionParameters::AuthenticationByPassword
        : SshConnectionParameters::AuthenticationByKey;
    deviceConfiguration()->setSshParameters(sshParams);
    m_ui->pwdLineEdit->setEnabled(usePassword);
    m_ui->passwordLabel->setEnabled(usePassword);
    m_ui->keyFileLineEdit->setEnabled(!usePassword);
    m_ui->keyLabel->setEnabled(!usePassword);
}

void GenericLinuxDeviceConfigurationWidget::hostNameEditingFinished()
{
    SshConnectionParameters sshParams = deviceConfiguration()->sshParameters();
    sshParams.host = m_ui->hostLineEdit->text().trimmed();
    deviceConfiguration()->setSshParameters(sshParams);
}

void GenericLinuxDeviceConfigurationWidget::sshPortEditingFinished()
{
    SshConnectionParameters sshParams = deviceConfiguration()->sshParameters();
    sshParams.port = m_ui->sshPortSpinBox->value();
    deviceConfiguration()->setSshParameters(sshParams);
}

void GenericLinuxDeviceConfigurationWidget::timeoutEditingFinished()
{
    SshConnectionParameters sshParams = deviceConfiguration()->sshParameters();
    sshParams.timeout = m_ui->timeoutSpinBox->value();
    deviceConfiguration()->setSshParameters(sshParams);
}

void GenericLinuxDeviceConfigurationWidget::userNameEditingFinished()
{
    SshConnectionParameters sshParams = deviceConfiguration()->sshParameters();
    sshParams.userName = m_ui->userLineEdit->text();
    deviceConfiguration()->setSshParameters(sshParams);
}

void GenericLinuxDeviceConfigurationWidget::passwordEditingFinished()
{
    SshConnectionParameters sshParams = deviceConfiguration()->sshParameters();
    sshParams.password = m_ui->pwdLineEdit->text();
    deviceConfiguration()->setSshParameters(sshParams);
}

void GenericLinuxDeviceConfigurationWidget::keyFileEditingFinished()
{
    SshConnectionParameters sshParams = deviceConfiguration()->sshParameters();
    sshParams.privateKeyFile = m_ui->keyFileLineEdit->path();
    deviceConfiguration()->setSshParameters(sshParams);
}

void GenericLinuxDeviceConfigurationWidget::handleFreePortsChanged()
{
    deviceConfiguration()->setFreePorts(PortList::fromString(m_ui->portsLineEdit->text()));
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

void GenericLinuxDeviceConfigurationWidget::updatePortsWarningLabel()
{
    m_ui->portsWarningLabel->setVisible(!deviceConfiguration()->freePorts().hasMore());
}

void GenericLinuxDeviceConfigurationWidget::initGui()
{
    if (deviceConfiguration()->machineType() == LinuxDeviceConfiguration::Hardware)
        m_ui->machineTypeValueLabel->setText(tr("Physical Device"));
    else
        m_ui->machineTypeValueLabel->setText(tr("Emulator"));
    m_ui->portsWarningLabel->setPixmap(QPixmap(":/projectexplorer/images/compile_error.png"));
    m_ui->portsWarningLabel->setToolTip(QLatin1String("<font color=\"red\">")
        + tr("You will need at least one port.") + QLatin1String("</font>"));
    m_ui->keyFileLineEdit->setExpectedKind(PathChooser::File);
    m_ui->keyFileLineEdit->lineEdit()->setMinimumWidth(0);
    QRegExpValidator * const portsValidator
        = new QRegExpValidator(QRegExp(PortList::regularExpression()), this);
    m_ui->portsLineEdit->setValidator(portsValidator);

    const SshConnectionParameters &sshParams = deviceConfiguration()->sshParameters();

    if (sshParams.authenticationType == SshConnectionParameters::AuthenticationByPassword)
        m_ui->passwordButton->setChecked(true);
    else
        m_ui->keyButton->setChecked(true);
    m_ui->timeoutSpinBox->setValue(sshParams.timeout);
    m_ui->hostLineEdit->setEnabled(!deviceConfiguration()->isAutoDetected());
    m_ui->sshPortSpinBox->setEnabled(!deviceConfiguration()->isAutoDetected());

    m_ui->hostLineEdit->setText(sshParams.host);
    m_ui->sshPortSpinBox->setValue(sshParams.port);
    m_ui->portsLineEdit->setText(deviceConfiguration()->freePorts().toString());
    m_ui->timeoutSpinBox->setValue(sshParams.timeout);
    m_ui->userLineEdit->setText(sshParams.userName);
    m_ui->pwdLineEdit->setText(sshParams.password);
    m_ui->keyFileLineEdit->setPath(sshParams.privateKeyFile);
    m_ui->showPasswordCheckBox->setChecked(false);
    updatePortsWarningLabel();
}

LinuxDeviceConfiguration::Ptr GenericLinuxDeviceConfigurationWidget::deviceConfiguration() const
{
    return device().staticCast<LinuxDeviceConfiguration>();
}
