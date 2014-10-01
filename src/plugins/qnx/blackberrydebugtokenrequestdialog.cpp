/**************************************************************************
**
** Copyright (C) 2014 BlackBerry Limited. All rights reserved.
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
** conditions see http://www.qt.io/licensing.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "blackberrydebugtokenrequestdialog.h"
#include "blackberrydebugtokenrequester.h"
#include "blackberrydeviceinformation.h"
#include "blackberryconfigurationmanager.h"
#include "blackberrysigningutils.h"
#include "ui_blackberrydebugtokenrequestdialog.h"

#include <QPushButton>
#include <QDir>
#include <QMessageBox>

namespace Qnx {
namespace Internal {

BlackBerryDebugTokenRequestDialog::BlackBerryDebugTokenRequestDialog(
        QWidget *parent, Qt::WindowFlags f) :
    QDialog(parent, f),
    m_ui(new Ui_BlackBerryDebugTokenRequestDialog),
    m_requester(new BlackBerryDebugTokenRequester(this)),
    m_deviceInfo(new BlackBerryDeviceInformation(this)),
    m_utils(BlackBerrySigningUtils::instance())
{
    m_ui->setupUi(this);
    m_ui->progressBar->hide();
    m_ui->status->clear();
    m_ui->debugTokenPath->setExpectedKind(Utils::PathChooser::SaveFile);
    m_ui->debugTokenPath->setHistoryCompleter(QLatin1String("BB.DebugToken.History"));
    m_ui->debugTokenPath->setPromptDialogTitle(tr("Request Debug Token"));
    m_ui->debugTokenPath->setPromptDialogFilter(tr("BAR Files (*.bar)"));

    m_cancelButton = m_ui->buttonBox->button(QDialogButtonBox::Cancel);
    m_okButton = m_ui->buttonBox->button(QDialogButtonBox::Ok);
    m_okButton->setEnabled(false);

    connect(m_cancelButton, SIGNAL(clicked()),
            this, SLOT(reject()));
    connect(m_okButton, SIGNAL(clicked()),
            this, SLOT(requestDebugToken()));
    connect(m_ui->debugTokenPath, SIGNAL(changed(QString)),
            this, SLOT(validate()));
    connect(m_ui->debugTokenPath, SIGNAL(beforeBrowsing()),
            this, SLOT(setDefaultPath()));
    connect(m_ui->debugTokenPath, SIGNAL(editingFinished()),
            this, SLOT(appendExtension()));
    connect(m_ui->debugTokenPath, SIGNAL(editingFinished()),
            this, SLOT(expandPath()));
    connect(m_ui->devicePin, SIGNAL(textChanged(QString)),
            this, SLOT(validate()));
    connect(m_requester, SIGNAL(finished(int)),
            this, SLOT(debugTokenArrived(int)));
    connect(m_deviceInfo, SIGNAL(finished(int)),
            this, SLOT(setDevicePin(int)));
}

BlackBerryDebugTokenRequestDialog::~BlackBerryDebugTokenRequestDialog()
{
    delete m_ui;
}

QString BlackBerryDebugTokenRequestDialog::debugToken() const
{
    return m_ui->debugTokenPath->path();
}

void BlackBerryDebugTokenRequestDialog::setDevicePin(const QString &devicePin)
{
    m_ui->devicePin->setText(devicePin);
}

void BlackBerryDebugTokenRequestDialog::setTargetDetails(const QString &deviceIp, const QString &password)
{
    m_ui->devicePin->setPlaceholderText(tr("Requesting Device PIN..."));
    m_deviceInfo->setDeviceTarget(deviceIp, password);
}

void BlackBerryDebugTokenRequestDialog::validate()
{
    if (!m_ui->debugTokenPath->isValid() || m_ui->devicePin->text().isEmpty()) {
        m_okButton->setEnabled(false);
        return;
    }

    QFileInfo fileInfo(m_ui->debugTokenPath->path());

    if (!fileInfo.dir().exists()) {
        m_ui->status->setText(tr("Base directory does not exist."));
        m_okButton->setEnabled(false);
        return;
    }

    m_ui->status->clear();
    m_okButton->setEnabled(true);
}

void BlackBerryDebugTokenRequestDialog::requestDebugToken()
{
    setBusy(true);

    QFile file(m_ui->debugTokenPath->path());

    if (file.exists()) {
        const int result = QMessageBox::question(this, tr("Are you sure?"),
                tr("The file \"%1\" will be overwritten. Do you want to proceed?")
                .arg(file.fileName()), QMessageBox::Yes | QMessageBox::No);

        if (result & QMessageBox::Yes) {
            file.remove();
        } else {
            setBusy(false);
            return;
        }
    }

    bool ok;
    const QString cskPassword = m_utils.cskPassword(this, &ok);

    if (!ok) {
        setBusy(false);
        return;
    }

    const QString certificatePassword = m_utils.certificatePassword(this, &ok);

    if (!ok) {
        setBusy(false);
        return;
    }

    m_requester->requestDebugToken(m_ui->debugTokenPath->path(),
            cskPassword, BlackBerryConfigurationManager::instance()->defaultKeystorePath(),
            certificatePassword, m_ui->devicePin->text());
}

void BlackBerryDebugTokenRequestDialog::setDefaultPath()
{
    const QString path = m_ui->debugTokenPath->path();
    const QString defaultFileName = QLatin1String("/debugToken.bar");

    if (path.isEmpty()) {
        m_ui->debugTokenPath->setPath(QDir::homePath() + defaultFileName);
        return;
    }

    const QFileInfo fileInfo(path);

    if (fileInfo.isDir())
        m_ui->debugTokenPath->setPath(path + defaultFileName);
}

void BlackBerryDebugTokenRequestDialog::appendExtension()
{
    QString path = m_ui->debugTokenPath->path();

    if (path.isEmpty())
        return;

    if (!path.endsWith(QLatin1String(".bar"))) {
        path += QLatin1String(".bar");
        m_ui->debugTokenPath->setPath(path);
    }
}

void BlackBerryDebugTokenRequestDialog::expandPath()
{
    const QString path = m_ui->debugTokenPath->path();

    if (path.isEmpty() || path.startsWith(QLatin1Char('/')))
            return;

    const QFileInfo fileInfo(path);

    m_ui->debugTokenPath->setPath(fileInfo.absoluteFilePath());
}

void BlackBerryDebugTokenRequestDialog::debugTokenArrived(int status)
{
    QString errorString = tr("Failed to request debug token:") + QLatin1Char(' ');

    switch (status) {
    case BlackBerryDebugTokenRequester::Success:
        accept();
        return;
    case BlackBerryDebugTokenRequester::WrongCskPassword:
        m_utils.clearCskPassword();
        errorString += tr("Wrong CSK password.");
        break;
    case BlackBerryDebugTokenRequester::WrongKeystorePassword:
        m_utils.clearCertificatePassword();
        errorString += tr("Wrong keystore password.");
        break;
    case BlackBerryDebugTokenRequester::NetworkUnreachable:
        errorString += tr("Network unreachable.");
        break;
    case BlackBerryDebugTokenRequester::IllegalPin:
        errorString += tr("Illegal device PIN.");
        break;
    case BlackBerryDebugTokenRequester::FailedToStartInferiorProcess:
        errorString += tr("Failed to start inferior process.");
        break;
    case BlackBerryDebugTokenRequester::InferiorProcessTimedOut:
        errorString += tr("Inferior processes timed out.");
        break;
    case BlackBerryDebugTokenRequester::InferiorProcessCrashed:
        errorString += tr("Inferior process has crashed.");
        break;
    case BlackBerryDebugTokenRequester::InferiorProcessReadError:
    case BlackBerryDebugTokenRequester::InferiorProcessWriteError:
        errorString += tr("Failed to communicate with the inferior process.");
        break;
    case BlackBerryDebugTokenRequester::NotYetRegistered:
        errorString += tr("Not yet registered to request debug tokens.");
        break;
    case BlackBerryDebugTokenRequester::UnknownError:
    default:
        m_utils.clearCertificatePassword();
        m_utils.clearCskPassword();
        errorString += tr("An unknwon error has occurred.");
        break;
    }

    QFile file(m_ui->debugTokenPath->path());

    if (file.exists())
        file.remove();

    QMessageBox::critical(this, tr("Error"), errorString);

    setBusy(false);
}

void BlackBerryDebugTokenRequestDialog::setDevicePin(int status)
{
    m_ui->devicePin->setPlaceholderText(QString());
    if (status != BlackBerryDeviceInformation::Success)
        return;

    const QString devicePin = m_deviceInfo->devicePin();
    if (devicePin.isEmpty())
        return;

    m_ui->devicePin->setText(devicePin);
}

void BlackBerryDebugTokenRequestDialog::setBusy(bool busy)
{
    m_okButton->setEnabled(!busy);
    m_cancelButton->setEnabled(!busy);
    m_ui->debugTokenPath->setEnabled(!busy);
    m_ui->devicePin->setEnabled(!busy);
    m_ui->progressBar->setVisible(busy);

    if (busy)
        m_ui->status->setText(tr("Requesting debug token..."));
    else
        m_ui->status->clear();
}

}
} // namespace Qnx
