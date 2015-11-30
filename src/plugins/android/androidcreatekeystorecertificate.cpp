/**************************************************************************
**
** Copyright (C) 2015 BogDan Vatra <bog_dan_ro@yahoo.com>
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
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
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
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
    connect(ui->certificateAliasLineEdit, SIGNAL(textChanged(QString)), this, SLOT(checkCertificateAlias()));
    connect(ui->countryLineEdit, SIGNAL(textChanged(QString)), this, SLOT(checkCountryCode()));
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
    return ui->certificateAliasLineEdit->text();
}

QString AndroidCreateKeystoreCertificate::certificatePassword()
{
    return (ui->samePasswordCheckBox->checkState() == Qt::Checked)
            ? keystorePassword()
            : ui->certificatePassLineEdit->text();
}

AndroidCreateKeystoreCertificate::PasswordStatus AndroidCreateKeystoreCertificate::checkKeystorePassword()
{
    if (ui->keystorePassLineEdit->text().length() < 6) {
        ui->infoLabel->setText(tr("<span style=\" color:#ff0000;\">Keystore password is too short</span>"));
        return Invalid;
    }
    if (ui->keystorePassLineEdit->text() != ui->keystoreRetypePassLineEdit->text()) {
            ui->infoLabel->setText(tr("<span style=\" color:#ff0000;\">Keystore passwords do not match</span>"));
            return NoMatch;
    }

    ui->infoLabel->clear();
    return Match;
}

AndroidCreateKeystoreCertificate::PasswordStatus AndroidCreateKeystoreCertificate::checkCertificatePassword()
{
    if (ui->samePasswordCheckBox->checkState() == Qt::Checked)
        return Match;

    if (ui->certificatePassLineEdit->text().length() < 6) {
        ui->infoLabel->setText(tr("<span style=\" color:#ff0000;\">Certificate password is too short</span>"));
        return Invalid;
    }
    if (ui->certificatePassLineEdit->text() != ui->certificateRetypePassLineEdit->text()) {
        ui->infoLabel->setText(tr("<span style=\" color:#ff0000;\">Certificate passwords do not match</span>"));
        return NoMatch;
    }

    ui->infoLabel->clear();
    return Match;
}

bool AndroidCreateKeystoreCertificate::checkCertificateAlias()
{
    if (ui->certificateAliasLineEdit->text().length() == 0) {
        ui->infoLabel->setText(tr("<span style=\" color:#ff0000;\">Certificate alias is missing</span>"));
        return false;
    }

    ui->infoLabel->clear();
    return true;
}

bool AndroidCreateKeystoreCertificate::checkCountryCode()
{
    if (!ui->countryLineEdit->text().contains(QRegExp(QLatin1String("[A-Z]{2}")))) {
        ui->infoLabel->setText(tr("<span style=\" color:#ff0000;\">Invalid country code</span>"));
        return false;
    }

    ui->infoLabel->clear();
    return true;
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
    if (!validateUserInput())
        return;

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
           << QLatin1String("-storepass") << keystorePassword()
           << QLatin1String("-alias") << certificateAlias()
           << QLatin1String("-keysize") << ui->keySizeSpinBox->text()
           << QLatin1String("-validity") << ui->validitySpinBox->text()
           << QLatin1String("-keypass") << certificatePassword()
           << QLatin1String("-dname") << distinguishedNames;

    QProcess genKeyCertProc;
    genKeyCertProc.start(AndroidConfigurations::currentConfig().keytoolPath().toString(), params );

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

void AndroidCreateKeystoreCertificate::on_samePasswordCheckBox_stateChanged(int state)
{
    if (state == Qt::Checked) {
        ui->certificatePassLineEdit->setDisabled(true);
        ui->certificateRetypePassLineEdit->setDisabled(true);
        ui->certificateShowPassCheckBox->setDisabled(true);
    }

    if (state == Qt::Unchecked) {
        ui->certificatePassLineEdit->setEnabled(true);
        ui->certificateRetypePassLineEdit->setEnabled(true);
        ui->certificateShowPassCheckBox->setEnabled(true);
    }

    validateUserInput();
}

bool AndroidCreateKeystoreCertificate::validateUserInput()
{
    switch (checkKeystorePassword()) {
    case Invalid:
        ui->keystorePassLineEdit->setFocus();
        return false;
    case NoMatch:
        ui->keystoreRetypePassLineEdit->setFocus();
        return false;
    default:
        break;
    }

    if (!checkCertificateAlias()) {
        ui->certificateAliasLineEdit->setFocus();
        return false;
    }

    switch (checkCertificatePassword()) {
    case Invalid:
        ui->certificatePassLineEdit->setFocus();
        return false;
    case NoMatch:
        ui->certificateRetypePassLineEdit->setFocus();
        return false;
    default:
        break;
    }

    if (!checkCountryCode()) {
        ui->countryLineEdit->setFocus();
        return false;
    }

    return true;
}
