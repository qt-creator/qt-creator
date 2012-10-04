/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (C) 2011 - 2012 Research In Motion
**
** Contact: Research In Motion (blackberry-qt@qnx.com)
** Contact: KDAB (info@kdab.com)
**
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
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#include "blackberrydeviceconfigurationwidget.h"
#include "ui_blackberrydeviceconfigurationwidget.h"
#include "qnxconstants.h"

#include <ssh/sshconnection.h>
#include <utils/pathchooser.h>

using namespace ProjectExplorer;
using namespace Qnx::Internal;

BlackBerryDeviceConfigurationWidget::BlackBerryDeviceConfigurationWidget(const IDevice::Ptr &device, QWidget *parent) :
    IDeviceWidget(device, parent),
    ui(new Ui::BlackBerryDeviceConfigurationWidget)
{
    ui->setupUi(this);
    connect(ui->hostLineEdit, SIGNAL(editingFinished()), this, SLOT(hostNameEditingFinished()));
    connect(ui->pwdLineEdit, SIGNAL(editingFinished()), this, SLOT(passwordEditingFinished()));
    connect(ui->keyFileLineEdit, SIGNAL(editingFinished()), this, SLOT(keyFileEditingFinished()));
    connect(ui->keyFileLineEdit, SIGNAL(browsingFinished()), this, SLOT(keyFileEditingFinished()));
    connect(ui->showPasswordCheckBox, SIGNAL(toggled(bool)), this, SLOT(showPassword(bool)));
    connect(ui->debugToken, SIGNAL(editingFinished()), this, SLOT(debugTokenEditingFinished()));

    initGui();
}

BlackBerryDeviceConfigurationWidget::~BlackBerryDeviceConfigurationWidget()
{
    delete ui;
}

void BlackBerryDeviceConfigurationWidget::hostNameEditingFinished()
{
    QSsh::SshConnectionParameters sshParams = deviceConfiguration()->sshParameters();
    sshParams.host = ui->hostLineEdit->text();
    deviceConfiguration()->setSshParameters(sshParams);
}

void BlackBerryDeviceConfigurationWidget::passwordEditingFinished()
{
    QSsh::SshConnectionParameters sshParams = deviceConfiguration()->sshParameters();
    sshParams.password = ui->pwdLineEdit->text();
    deviceConfiguration()->setSshParameters(sshParams);
}

void BlackBerryDeviceConfigurationWidget::keyFileEditingFinished()
{
    QSsh::SshConnectionParameters sshParams = deviceConfiguration()->sshParameters();
    sshParams.privateKeyFile = ui->keyFileLineEdit->path();
    deviceConfiguration()->setSshParameters(sshParams);
}

void BlackBerryDeviceConfigurationWidget::showPassword(bool showClearText)
{
    ui->pwdLineEdit->setEchoMode(showClearText
        ? QLineEdit::Normal : QLineEdit::Password);
}

void BlackBerryDeviceConfigurationWidget::debugTokenEditingFinished()
{
    deviceConfiguration()->setDebugToken(ui->debugToken->path());
}

void BlackBerryDeviceConfigurationWidget::updateDeviceFromUi()
{
    hostNameEditingFinished();
    passwordEditingFinished();
    keyFileEditingFinished();
    debugTokenEditingFinished();
}

void BlackBerryDeviceConfigurationWidget::initGui()
{
    ui->debugToken->setExpectedKind(Utils::PathChooser::File);
    ui->debugToken->setPromptDialogFilter(QLatin1String("*.bar"));

    ui->keyFileLineEdit->setExpectedKind(Utils::PathChooser::File);
    ui->keyFileLineEdit->lineEdit()->setMinimumWidth(0);

    const QSsh::SshConnectionParameters &sshParams = deviceConfiguration()->sshParameters();

    ui->hostLineEdit->setEnabled(!deviceConfiguration()->isAutoDetected());

    ui->hostLineEdit->setText(sshParams.host);
    ui->pwdLineEdit->setText(sshParams.password);
    ui->keyFileLineEdit->setPath(sshParams.privateKeyFile);
    ui->showPasswordCheckBox->setChecked(false);
    ui->debugToken->setPath(deviceConfiguration()->debugToken());

    if (deviceConfiguration()->machineType() == IDevice::Emulator) {
        ui->debugToken->setEnabled(false);
        ui->debugTokenLabel->setEnabled(false);
    }
}

BlackBerryDeviceConfiguration::Ptr BlackBerryDeviceConfigurationWidget::deviceConfiguration() const
{
    return device().dynamicCast<BlackBerryDeviceConfiguration>();
}
