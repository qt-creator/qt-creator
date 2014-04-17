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

#include "blackberrycreatecertificatedialog.h"
#include "blackberrycertificate.h"
#include "blackberryconfigurationmanager.h"
#include "ui_blackberrycreatecertificatedialog.h"

#include <QPushButton>
#include <QDir>
#include <QDebug>
#include <QMessageBox>

namespace Qnx {
namespace Internal {

BlackBerryCreateCertificateDialog::BlackBerryCreateCertificateDialog(
        QWidget *parent, Qt::WindowFlags f) :
    QDialog(parent, f),
    m_ui(new Ui_BlackBerryCreateCertificateDialog),
    m_certificate(0)
{
    m_ui->setupUi(this);
    m_ui->progressBar->hide();
    m_ui->status->clear();

    m_cancelButton = m_ui->buttonBox->button(QDialogButtonBox::Cancel);
    Q_ASSERT(m_cancelButton);

    m_okButton = m_ui->buttonBox->button(QDialogButtonBox::Ok);
    m_okButton->setEnabled(false);

    connect(m_cancelButton, SIGNAL(clicked()),
            this, SLOT(reject()));
    connect(m_okButton, SIGNAL(clicked()),
            this, SLOT(createCertificate()));
    connect(m_ui->author, SIGNAL(textChanged(QString)),
            this, SLOT(validate()));
    connect(m_ui->password, SIGNAL(textChanged(QString)),
            this, SLOT(validate()));
    connect(m_ui->password2, SIGNAL(textChanged(QString)),
            this, SLOT(validate()));
    connect(m_ui->showPassword, SIGNAL(stateChanged(int)),
            this, SLOT(checkBoxChanged(int)));
}

QString BlackBerryCreateCertificateDialog::author() const
{
    return m_ui->author->text();
}

QString BlackBerryCreateCertificateDialog::certPath() const
{
    return BlackBerryConfigurationManager::instance()->defaultKeystorePath();
}

QString BlackBerryCreateCertificateDialog::keystorePassword() const
{
    return m_ui->password->text();
}

BlackBerryCertificate * BlackBerryCreateCertificateDialog::certificate() const
{
    return m_certificate;
}

void BlackBerryCreateCertificateDialog::validate()
{
    if (m_ui->author->text().isEmpty()
            || m_ui->password->text().isEmpty()
            || m_ui->password2->text().isEmpty()) {
        m_ui->status->clear();
        m_okButton->setEnabled(false);
        return;
    }

    QFileInfo fileInfo(certPath());

    if (!fileInfo.dir().exists()) {
        m_ui->status->setText(tr("Base directory does not exist."));
        m_okButton->setEnabled(false);
        return;
    }

    if (m_ui->password->text() != m_ui->password2->text()) {
        m_ui->status->setText(tr("The entered passwords do not match."));
        m_okButton->setEnabled(false);
        return;
    }

    if (m_ui->password->text().size() < 6) {
        m_ui->status->setText(tr("Password must be at least 6 characters long."));
        m_okButton->setEnabled(false);
        return;
    }

    m_ui->status->clear();
    m_okButton->setEnabled(true);
}

void BlackBerryCreateCertificateDialog::createCertificate()
{
    setBusy(true);

    QFile file(certPath());

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

    m_certificate = new BlackBerryCertificate(certPath(),
            author(), keystorePassword());

    connect(m_certificate, SIGNAL(finished(int)), this, SLOT(certificateCreated(int)));
    m_certificate->store();
}

void BlackBerryCreateCertificateDialog::checkBoxChanged(int state)
{
    if (state == Qt::Checked) {
        m_ui->password->setEchoMode(QLineEdit::Normal);
        m_ui->password2->setEchoMode(QLineEdit::Normal);
    } else {
        m_ui->password->setEchoMode(QLineEdit::Password);
        m_ui->password2->setEchoMode(QLineEdit::Password);
    }
}

void BlackBerryCreateCertificateDialog::certificateCreated(int status)
{
    QString errorMessage;

    switch (status) {
    case BlackBerryCertificate::Success:
        accept();
        return;
    case BlackBerryCertificate::Busy:
        errorMessage = tr("The blackberry-keytool process is already running.");
        break;
    case BlackBerryCertificate::WrongPassword:
        errorMessage = tr("The password entered is invalid.");
        break;
    case BlackBerryCertificate::PasswordTooSmall:
        errorMessage = tr("The password entered is too short.");
        break;
    case BlackBerryCertificate::InvalidOutputFormat:
        errorMessage = tr("Invalid output format.");
        break;
    case BlackBerryCertificate::Error:
    default:
        errorMessage = tr("An unknown error occurred.");
        break;
    }

    m_certificate->deleteLater();
    m_certificate = 0;
    QMessageBox::critical(this, tr("Error"), errorMessage);

    reject();
}

void BlackBerryCreateCertificateDialog::setBusy(bool busy)
{
    m_okButton->setEnabled(!busy);
    m_cancelButton->setEnabled(!busy);
    m_ui->author->setEnabled(!busy);
    m_ui->password->setEnabled(!busy);
    m_ui->password2->setEnabled(!busy);
    m_ui->showPassword->setEnabled(!busy);
    m_ui->progressBar->setVisible(busy);

    if (busy)
        m_ui->status->setText(tr("Please be patient..."));
    else
        m_ui->status->clear();
}

}
} // namespace Qnx
