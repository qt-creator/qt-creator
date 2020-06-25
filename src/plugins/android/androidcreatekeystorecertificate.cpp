/****************************************************************************
**
** Copyright (C) 2016 BogDan Vatra <bog_dan_ro@yahoo.com>
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

#include "androidcreatekeystorecertificate.h"
#include "androidconfigurations.h"
#include "ui_androidcreatekeystorecertificate.h"

#include <utils/synchronousprocess.h>

#include <QFileDialog>
#include <QRegularExpression>
#include <QMessageBox>

using namespace Utils;

using namespace Android::Internal;

AndroidCreateKeystoreCertificate::AndroidCreateKeystoreCertificate(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AndroidCreateKeystoreCertificate)
{
    ui->setupUi(this);
    connect(ui->keystorePassLineEdit, &QLineEdit::textChanged,
            this, &AndroidCreateKeystoreCertificate::checkKeystorePassword);
    connect(ui->keystoreRetypePassLineEdit, &QLineEdit::textChanged,
            this, &AndroidCreateKeystoreCertificate::checkKeystorePassword);
    connect(ui->certificatePassLineEdit, &QLineEdit::textChanged,
            this, &AndroidCreateKeystoreCertificate::checkCertificatePassword);
    connect(ui->certificateRetypePassLineEdit, &QLineEdit::textChanged,
            this, &AndroidCreateKeystoreCertificate::checkCertificatePassword);
    connect(ui->certificateAliasLineEdit, &QLineEdit::textChanged,
            this, &AndroidCreateKeystoreCertificate::checkCertificateAlias);
    connect(ui->countryLineEdit, &QLineEdit::textChanged,
            this, &AndroidCreateKeystoreCertificate::checkCountryCode);
    connect(ui->buttonBox, &QDialogButtonBox::rejected,
            this, &QDialog::reject);
    connect(ui->keystorePassLineEdit,
            &QLineEdit::editingFinished,
            ui->keystoreRetypePassLineEdit,
            QOverload<>::of(&QLineEdit::setFocus));
}

AndroidCreateKeystoreCertificate::~AndroidCreateKeystoreCertificate()
{
    delete ui;
}

Utils::FilePath AndroidCreateKeystoreCertificate::keystoreFilePath()
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
    if (!ui->countryLineEdit->text().contains(QRegularExpression("[A-Z]{2}"))) {
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

    m_keystoreFilePath = Utils::FilePath::fromString(QFileDialog::getSaveFileName(this, tr("Keystore Filename"),
                                                                                  QDir::homePath() + QLatin1String("/android_release.keystore"),
                                                                                  tr("Keystore files (*.keystore *.jks)")));
    if (m_keystoreFilePath.isEmpty())
        return;
    QString distinguishedNames(QString::fromLatin1("CN=%1, O=%2, L=%3, C=%4")
                               .arg(ui->commonNameLineEdit->text().replace(QLatin1Char(','), QLatin1String("\\,")))
                               .arg(ui->organizationNameLineEdit->text().replace(QLatin1Char(','), QLatin1String("\\,")))
                               .arg(ui->localityNameLineEdit->text().replace(QLatin1Char(','), QLatin1String("\\,")))
                               .arg(ui->countryLineEdit->text().replace(QLatin1Char(','), QLatin1String("\\,"))));

    if (!ui->organizationUnitLineEdit->text().isEmpty())
        distinguishedNames += QLatin1String(", OU=") + ui->organizationUnitLineEdit->text().replace(QLatin1Char(','), QLatin1String("\\,"));

    if (!ui->stateNameLineEdit->text().isEmpty())
        distinguishedNames += QLatin1String(", S=") + ui->stateNameLineEdit->text().replace(QLatin1Char(','), QLatin1String("\\,"));

    const CommandLine command(AndroidConfigurations::currentConfig().keytoolPath(),
                            { "-genkey", "-keyalg", "RSA",
                              "-keystore",  m_keystoreFilePath.toString(),
                              "-storepass", keystorePassword(),
                              "-alias", certificateAlias(),
                              "-keysize", ui->keySizeSpinBox->text(),
                              "-validity", ui->validitySpinBox->text(),
                              "-keypass", certificatePassword(),
                              "-dname", distinguishedNames});

    SynchronousProcess genKeyCertProc;
    genKeyCertProc.setTimeoutS(15);
    SynchronousProcessResponse response = genKeyCertProc.run(command);

    if (response.result != Utils::SynchronousProcessResponse::Finished || response.exitCode != 0) {
        QMessageBox::critical(this, tr("Error"),
                              response.exitMessage(command.executable().toString(), 15) + '\n' + response.allOutput());
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
