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

#include "sshkeycreationdialog.h"
#include "ui_sshkeycreationdialog.h"

#include "sshsettings.h"

#include <utils/fileutils.h>

#include <QApplication>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QProcess>
#include <QStandardPaths>

namespace QSsh {

SshKeyCreationDialog::SshKeyCreationDialog(QWidget *parent)
    : QDialog(parent), m_ui(new Ui::SshKeyCreationDialog)
{
    m_ui->setupUi(this);
    // Not using Utils::PathChooser::browseButtonLabel to avoid dependency
#ifdef Q_OS_MAC
    m_ui->privateKeyFileButton->setText(tr("Choose..."));
#else
    m_ui->privateKeyFileButton->setText(tr("Browse..."));
#endif
    const QString defaultPath = QStandardPaths::writableLocation(QStandardPaths::HomeLocation)
        + QLatin1String("/.ssh/qtc_id");
    setPrivateKeyFile(defaultPath);

    connect(m_ui->rsa, &QRadioButton::toggled,
            this, &SshKeyCreationDialog::keyTypeChanged);
    connect(m_ui->privateKeyFileButton, &QPushButton::clicked,
            this, &SshKeyCreationDialog::handleBrowseButtonClicked);
    connect(m_ui->generateButton, &QPushButton::clicked,
            this, &SshKeyCreationDialog::generateKeys);
    keyTypeChanged();
}

SshKeyCreationDialog::~SshKeyCreationDialog()
{
    delete m_ui;
}

void SshKeyCreationDialog::keyTypeChanged()
{
    m_ui->comboBox->clear();
    QStringList keySizes;
    if (m_ui->rsa->isChecked())
        keySizes << QLatin1String("1024") << QLatin1String("2048") << QLatin1String("4096");
    else if (m_ui->ecdsa->isChecked())
        keySizes << QLatin1String("256") << QLatin1String("384") << QLatin1String("521");
    m_ui->comboBox->addItems(keySizes);
    if (!keySizes.isEmpty())
        m_ui->comboBox->setCurrentIndex(0);
    m_ui->comboBox->setEnabled(!keySizes.isEmpty());
}

void SshKeyCreationDialog::generateKeys()
{
    if (SshSettings::keygenFilePath().isEmpty()) {
        showError(tr("The ssh-keygen tool was not found."));
        return;
    }
    if (QFileInfo::exists(privateKeyFilePath())) {
        showError(tr("Refusing to overwrite existing private key file \"%1\".")
                  .arg(QDir::toNativeSeparators(privateKeyFilePath())));
        return;
    }
    const QString keyTypeString = QLatin1String(m_ui->rsa->isChecked() ? "rsa": "ecdsa");
    QApplication::setOverrideCursor(Qt::BusyCursor);
    QProcess keygen;
    const QStringList args{"-t", keyTypeString, "-b", m_ui->comboBox->currentText(),
                "-N", QString(), "-f", privateKeyFilePath()};
    QString errorMsg;
    keygen.start(SshSettings::keygenFilePath().toString(), args);
    keygen.closeWriteChannel();
    if (!keygen.waitForStarted() || !keygen.waitForFinished())
        errorMsg = keygen.errorString();
    else if (keygen.exitCode() != 0)
        errorMsg = QString::fromLocal8Bit(keygen.readAllStandardError());
    if (!errorMsg.isEmpty()) {
        showError(tr("The ssh-keygen tool at \"%1\" failed: %2")
                  .arg(SshSettings::keygenFilePath().toUserOutput(), errorMsg));
    }
    QApplication::restoreOverrideCursor();
    accept();
}

void SshKeyCreationDialog::handleBrowseButtonClicked()
{
    const QString filePath = QFileDialog::getSaveFileName(this, tr("Choose Private Key File Name"));
    if (!filePath.isEmpty())
        setPrivateKeyFile(filePath);
}

void SshKeyCreationDialog::setPrivateKeyFile(const QString &filePath)
{
    m_ui->privateKeyFileValueLabel->setText(filePath);
    m_ui->generateButton->setEnabled(!privateKeyFilePath().isEmpty());
    m_ui->publicKeyFileLabel->setText(filePath + QLatin1String(".pub"));
}

void SshKeyCreationDialog::showError(const QString &details)
{
    QMessageBox::critical(this, tr("Key Generation Failed"), details);
}

QString SshKeyCreationDialog::privateKeyFilePath() const
{
    return m_ui->privateKeyFileValueLabel->text();
}

QString SshKeyCreationDialog::publicKeyFilePath() const
{
    return m_ui->publicKeyFileLabel->text();
}

} // namespace QSsh
