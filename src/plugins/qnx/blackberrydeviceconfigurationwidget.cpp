/**************************************************************************
**
** Copyright (C) 2012 - 2014 BlackBerry Limited. All rights reserved.
**
** Contact: BlackBerry (qt@blackberry.com)
** Contact: KDAB (info@kdab.com)
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

#include "blackberrydeviceconfigurationwidget.h"
#include "blackberrydebugtokenuploader.h"
#include "blackberrydebugtokenrequestdialog.h"
#include "ui_blackberrydeviceconfigurationwidget.h"
#include "blackberrydeviceconnectionmanager.h"
#include "blackberrysigningutils.h"
#include "blackberrydebugtokenreader.h"
#include "qnxconstants.h"
#include "qnxutils.h"

#include <ssh/sshconnection.h>
#include <texteditor/texteditorsettings.h>
#include <texteditor/fontsettings.h>
#include <utils/pathchooser.h>
#include <utils/fancylineedit.h>

#include <QProgressDialog>
#include <QMessageBox>
#include <QFileInfo>
#include <QFileDialog>
#include <QDir>
#include <QAbstractButton>

using namespace ProjectExplorer;
using namespace Qnx::Internal;

BlackBerryDeviceConfigurationWidget::BlackBerryDeviceConfigurationWidget(const IDevice::Ptr &device, QWidget *parent) :
    IDeviceWidget(device, parent),
    ui(new Ui::BlackBerryDeviceConfigurationWidget),
    progressDialog(new QProgressDialog(this)),
    uploader(new BlackBerryDebugTokenUploader(this)),
    m_utils(BlackBerrySigningUtils::instance())
{
    ui->setupUi(this);

    ui->connectionLog->setFont(TextEditor::TextEditorSettings::fontSettings().font());

    populateDebugTokenCombo(deviceConfiguration()->debugToken());

    connect(ui->hostLineEdit, SIGNAL(editingFinished()), this, SLOT(hostNameEditingFinished()));
    connect(ui->pwdLineEdit, SIGNAL(editingFinished()), this, SLOT(passwordEditingFinished()));
    connect(ui->keyFileLineEdit, SIGNAL(editingFinished()), this, SLOT(keyFileEditingFinished()));
    connect(ui->keyFileLineEdit, SIGNAL(browsingFinished()), this, SLOT(keyFileEditingFinished()));
    connect(ui->showPasswordCheckBox, SIGNAL(toggled(bool)), this, SLOT(showPassword(bool)));
    connect(ui->debugToken, SIGNAL(currentTextChanged(QString)), this, SLOT(updateUploadButton()));
    connect(ui->debugToken, SIGNAL(currentTextChanged(QString)), this, SLOT(debugTokenEditingFinished()));
    connect(uploader, SIGNAL(finished(int)), this, SLOT(uploadFinished(int)));

    connect(BlackBerryDeviceConnectionManager::instance(), SIGNAL(connectionOutput(Core::Id,QString)),
            this, SLOT(appendConnectionLog(Core::Id,QString)));
    connect(BlackBerryDeviceConnectionManager::instance(), SIGNAL(deviceAboutToConnect(Core::Id)),
            this, SLOT(clearConnectionLog(Core::Id)));

    connect(ui->importButton, SIGNAL(clicked()), this, SLOT(importDebugToken()));
    connect(ui->requestButton, SIGNAL(clicked()), this, SLOT(requestDebugToken()));
    connect(ui->uploadButton, SIGNAL(clicked()), this, SLOT(uploadDebugToken()));

    connect(&m_utils, SIGNAL(debugTokenListChanged()), this, SLOT(updateDebugTokenCombo()));

    initGui();
}

BlackBerryDeviceConfigurationWidget::~BlackBerryDeviceConfigurationWidget()
{
    m_utils.saveDebugTokens();
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
    deviceConfiguration()->setDebugToken(ui->debugToken->currentText());
}

void BlackBerryDeviceConfigurationWidget::importDebugToken()
{
    const QString debugToken = QFileDialog::getOpenFileName(this, tr("Select Debug Token"),
                                                            QString(), tr("Bar file (*.bar)"));

    if (debugToken.isEmpty())
        return;

    BlackBerryDebugTokenReader debugTokenReader(debugToken);
    if (!debugTokenReader.isValid()) {
        QMessageBox::warning(this, tr("Invalid Debug Token"),
                             tr("Debug token file %1 cannot be read.").arg(debugToken));
        return;
    }

    m_utils.addDebugToken(debugToken);
    populateDebugTokenCombo(debugToken);
    debugTokenEditingFinished();
}

void BlackBerryDeviceConfigurationWidget::requestDebugToken()
{
    BlackBerryDebugTokenRequestDialog dialog;

    if (!ui->hostLineEdit->text().isEmpty() && !ui->pwdLineEdit->text().isEmpty())
        dialog.setTargetDetails(ui->hostLineEdit->text(), ui->pwdLineEdit->text());

    const int result = dialog.exec();

    if (result != QDialog::Accepted)
        return;

    m_utils.addDebugToken(dialog.debugToken());
    populateDebugTokenCombo(dialog.debugToken());
    debugTokenEditingFinished();
}

void BlackBerryDeviceConfigurationWidget::uploadDebugToken()
{
    // check the debug token path before even laucnhing the uploader process
    if (!QFileInfo(ui->debugToken->currentText()).exists()) {
        QMessageBox::critical(this, tr("Error"), tr("Invalid debug token path."));
        return;
    }

    progressDialog->show();

    uploader->uploadDebugToken(ui->debugToken->currentText(),
            ui->hostLineEdit->text(), ui->pwdLineEdit->text());
}

void BlackBerryDeviceConfigurationWidget::updateUploadButton()
{
    const QString path = ui->debugToken->currentText();
    ui->uploadButton->setEnabled(QFileInfo(path).exists());
}

void BlackBerryDeviceConfigurationWidget::uploadFinished(int status)
{
    progressDialog->hide();

    QString errorString = tr("Failed to upload debug token:") + QLatin1Char(' ');

    switch (status) {
    case BlackBerryDebugTokenUploader::Success:
        QMessageBox::information(this, tr("Qt Creator"), tr("Debug token successfully uploaded."));
        return;
    case BlackBerryDebugTokenUploader::NoRouteToHost:
        errorString += tr("No route to host.");
        break;
    case BlackBerryDebugTokenUploader::AuthenticationFailed:
        errorString += tr("Authentication failed.");
        break;
    case BlackBerryDebugTokenUploader::DevelopmentModeDisabled:
        errorString += tr("Development mode is disabled on the device.");
        break;
    case BlackBerryDebugTokenUploader::FailedToStartInferiorProcess:
        errorString += tr("Failed to start inferior process.");
        break;
    case BlackBerryDebugTokenUploader::InferiorProcessTimedOut:
        errorString += tr("Inferior processes timed out.");
        break;
    case BlackBerryDebugTokenUploader::InferiorProcessCrashed:
        errorString += tr("Inferior process has crashed.");
        break;
    case BlackBerryDebugTokenUploader::InferiorProcessReadError:
    case BlackBerryDebugTokenUploader::InferiorProcessWriteError:
        errorString += tr("Failed to communicate with the inferior process.");
        break;
    case BlackBerryDebugTokenUploader::UnknownError:
        errorString += tr("An unknwon error has happened.");
        break;
    }

    QMessageBox::critical(this, tr("Error"), errorString);
}

void BlackBerryDeviceConfigurationWidget::appendConnectionLog(Core::Id deviceId, const QString &line)
{
    if (deviceId == device()->id())
        ui->connectionLog->appendPlainText(line.trimmed());
}

void BlackBerryDeviceConfigurationWidget::clearConnectionLog(Core::Id deviceId)
{
    if (deviceId == device()->id())
        ui->connectionLog->clear();
}

void BlackBerryDeviceConfigurationWidget::populateDebugTokenCombo(const QString& current)
{
    ui->debugToken->clear();
    ui->debugToken->addItems(m_utils.debugTokens());
    ui->debugToken->setCurrentText(current);
}

void BlackBerryDeviceConfigurationWidget::updateDebugTokenCombo()
{
    populateDebugTokenCombo(ui->debugToken->currentText());
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
    ui->keyFileLineEdit->setExpectedKind(Utils::PathChooser::File);
    ui->keyFileLineEdit->setHistoryCompleter(QLatin1String("BB.Key.History"));
    ui->keyFileLineEdit->lineEdit()->setMinimumWidth(0);

    const QSsh::SshConnectionParameters &sshParams = deviceConfiguration()->sshParameters();

    ui->hostLineEdit->setEnabled(!deviceConfiguration()->isAutoDetected());

    ui->hostLineEdit->setText(sshParams.host);
    ui->pwdLineEdit->setText(sshParams.password);
    ui->keyFileLineEdit->setPath(sshParams.privateKeyFile);
    ui->showPasswordCheckBox->setChecked(false);
    if (deviceConfiguration()->machineType() == IDevice::Emulator) {
        ui->debugToken->setEnabled(false);
        ui->requestButton->setEnabled(false);
        ui->uploadButton->setEnabled(false);
        ui->debugTokenLabel->setEnabled(false);
    }

    progressDialog->setWindowModality(Qt::WindowModal);
    progressDialog->setWindowTitle(tr("Operation in Progress"));
    progressDialog->setCancelButton(0);
    progressDialog->setLabelText(tr("Uploading debug token"));
    progressDialog->setMinimum(0);
    progressDialog->setMaximum(0);

    ui->connectionLog->setPlainText(BlackBerryDeviceConnectionManager::instance()->connectionLog(device()->id()).trimmed());
}

BlackBerryDeviceConfiguration::Ptr BlackBerryDeviceConfigurationWidget::deviceConfiguration() const
{
    return device().dynamicCast<BlackBerryDeviceConfiguration>();
}
