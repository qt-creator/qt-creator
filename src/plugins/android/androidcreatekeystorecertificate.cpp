/**************************************************************************
**
** Copyright (c) 2013 BogDan Vatra <bog_dan_ro@yahoo.com>
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

#include "androidcreatekeystorecertificate.h"
#include "androidconfigurations.h"
#include "ui_androidcreatekeystorecertificate.h"

#include <QFileDialog>
#include <QProcess>
#include <QMessageBox>

using namespace Android::Internal;

AndroidCreateKeystoreCertificate::AndroidCreateKeystoreCertificate(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AndroidCreateKeystoreCertificate)
{
    ui->setupUi(this);
    connect(ui->keystorePassLineEdit, SIGNAL(textChanged(QString)), this, SLOT(checkKeystorePassword()));
    connect(ui->keystoreRetypePassLineEdit, SIGNAL(textChanged(QString)), this, SLOT(checkKeystorePassword()));
    connect(ui->certificatePassLineEdit, SIGNAL(textChanged(QString)), this, SLOT(checkCertificatePassword()));
    connect(ui->certificateRetypePassLineEdit, SIGNAL(textChanged(QString)), this, SLOT(checkCertificatePassword()));
}

AndroidCreateKeystoreCertificate::~AndroidCreateKeystoreCertificate()
{
    delete ui;
}

Utils::FileName AndroidCreateKeystoreCertificate::keystoreFilePath()
{
    return m_keystoreFilePath;
}

QString AndroidCreateKeystoreCertificate::keystorePassword()
{
    return ui->keystorePassLineEdit->text();
}

QString AndroidCreateKeystoreCertificate::certificateAlias()
{
    return ui->aliasNameLineEdit->text();
}

QString AndroidCreateKeystoreCertificate::certificatePassword()
{
    return ui->certificatePassLineEdit->text();
}

AndroidCreateKeystoreCertificate::PasswordStatus AndroidCreateKeystoreCertificate::checkKeystorePassword()
{
    if (ui->keystorePassLineEdit->text().length() < 6) {
        ui->keystorePassInfoLabel->setText(tr("<span style=\" color:#ff0000;\">Password is too short</span>"));
        return Invalid;
    }
    if (ui->keystorePassLineEdit->text() != ui->keystoreRetypePassLineEdit->text()) {
            ui->keystorePassInfoLabel->setText(tr("<span style=\" color:#ff0000;\">Passwords don't match</span>"));
            return NoMatch;
    }
    ui->keystorePassInfoLabel->setText(tr("<span style=\" color:#00ff00;\">Password is ok</span>"));
    return Match;
}

AndroidCreateKeystoreCertificate::PasswordStatus AndroidCreateKeystoreCertificate::checkCertificatePassword()
{
    if (ui->certificatePassLineEdit->text().length() < 6) {
        ui->certificatePassInfoLabel->setText(tr("<span style=\" color:#ff0000;\">Password is too short</span>"));
        return Invalid;
    }
    if (ui->certificatePassLineEdit->text() != ui->certificateRetypePassLineEdit->text()) {
        ui->certificatePassInfoLabel->setText(tr("<span style=\" color:#ff0000;\">Passwords don't match</span>"));
        return NoMatch;
    }
    ui->certificatePassInfoLabel->setText(tr("<span style=\" color:#00ff00;\">Password is ok</span>"));
    return Match;
}

void AndroidCreateKeystoreCertificate::on_keystoreShowPassCheckBox_stateChanged(int state)
{
    ui->keystorePassLineEdit->setEchoMode(state == Qt::Checked ? QLineEdit::Normal : QLineEdit::Password);
    ui->keystoreRetypePassLineEdit->setEchoMode(ui->keystorePassLineEdit->echoMode());
}

void AndroidCreateKeystoreCertificate::on_certificateShowPassCheckBox_stateChanged(int state)
{
    ui->certificatePassLineEdit->setEchoMode(state == Qt::Checked ? QLineEdit::Normal : QLineEdit::Password);
    ui->certificateRetypePassLineEdit->setEchoMode(ui->certificatePassLineEdit->echoMode());
}

void AndroidCreateKeystoreCertificate::on_buttonBox_accepted()
{
    switch (checkKeystorePassword()) {
    case Invalid:
        ui->keystorePassLineEdit->setFocus();
        return;
    case NoMatch:
        ui->keystoreRetypePassLineEdit->setFocus();
        return;
    default:
        break;
    }

    switch (checkCertificatePassword()) {
    case Invalid:
        ui->certificatePassLineEdit->setFocus();
        return;
    case NoMatch:
        ui->certificateRetypePassLineEdit->setFocus();
        return;
    default:
        break;
    }

    if (!ui->aliasNameLineEdit->text().length())
        ui->aliasNameLineEdit->setFocus();

    if (!ui->commonNameLineEdit->text().length())
        ui->commonNameLineEdit->setFocus();

    if (!ui->organizationNameLineEdit->text().length())
        ui->organizationNameLineEdit->setFocus();

    if (!ui->localityNameLineEdit->text().length())
        ui->localityNameLineEdit->setFocus();

    if (!ui->countryLineEdit->text().length())
        ui->countryLineEdit->setFocus();

    m_keystoreFilePath = Utils::FileName::fromString(QFileDialog::getSaveFileName(this, tr("Keystore file name"),
                                                                                  QDir::homePath() + QLatin1String("/android_release.keystore"),
                                                                                  tr("Keystore files (*.keystore *.jks)")));
    if (m_keystoreFilePath.isEmpty())
        return;
    QString distinguishedNames(QString::fromLatin1("CN=%1, O=%2, L=%3, C=%4")
                               .arg(ui->commonNameLineEdit->text().replace(QLatin1Char(','), QLatin1String("\\,")))
                               .arg(ui->organizationNameLineEdit->text().replace(QLatin1Char(','), QLatin1String("\\,")))
                               .arg(ui->localityNameLineEdit->text().replace(QLatin1Char(','), QLatin1String("\\,")))
                               .arg(ui->countryLineEdit->text().replace(QLatin1Char(','), QLatin1String("\\,"))));

    if (ui->organizationUnitLineEdit->text().length())
        distinguishedNames += QLatin1String(", OU=") + ui->organizationUnitLineEdit->text().replace(QLatin1Char(','), QLatin1String("\\,"));

    if (ui->stateNameLineEdit->text().length())
        distinguishedNames += QLatin1String(", S=") + ui->stateNameLineEdit->text().replace(QLatin1Char(','), QLatin1String("\\,"));

    QStringList params;
    params << QLatin1String("-genkey") << QLatin1String("-keyalg") << QLatin1String("RSA")
           << QLatin1String("-keystore") << m_keystoreFilePath.toString()
           << QLatin1String("-storepass") << ui->keystorePassLineEdit->text()
           << QLatin1String("-alias") << ui->aliasNameLineEdit->text()
           << QLatin1String("-keysize") << ui->keySizeSpinBox->text()
           << QLatin1String("-validity") << ui->validitySpinBox->text()
           << QLatin1String("-keypass") << ui->certificatePassLineEdit->text()
           << QLatin1String("-dname") << distinguishedNames;

    QProcess genKeyCertProc;
    genKeyCertProc.start(AndroidConfigurations::instance().keytoolPath().toString(), params );

    if (!genKeyCertProc.waitForStarted() || !genKeyCertProc.waitForFinished())
        return;

    if (genKeyCertProc.exitCode()) {
        QMessageBox::critical(this, tr("Error")
                              , QString::fromLatin1(genKeyCertProc.readAllStandardOutput())
                              + QString::fromLatin1(genKeyCertProc.readAllStandardError()));
        return;
    }
    accept();
}
