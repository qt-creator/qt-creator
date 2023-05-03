// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "sshkeycreationdialog.h"

#include "remotelinuxtr.h"

#include <projectexplorer/devicesupport/sshsettings.h>

#include <utils/fileutils.h>
#include <utils/layoutbuilder.h>
#include <utils/pathchooser.h>
#include <utils/process.h>

#include <QApplication>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QGroupBox>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QRadioButton>
#include <QStandardPaths>

using namespace ProjectExplorer;
using namespace Utils;

namespace RemoteLinux {

SshKeyCreationDialog::SshKeyCreationDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(Tr::tr("SSH Key Configuration"));
    resize(385, 231);

    m_rsa = new QRadioButton(Tr::tr("&RSA"));
    m_rsa->setChecked(true);

    m_ecdsa = new QRadioButton(Tr::tr("ECDSA"));

    m_comboBox = new QComboBox;

    m_privateKeyFileValueLabel = new QLabel;

    m_publicKeyFileLabel = new QLabel;

    auto privateKeyFileButton = new QPushButton(PathChooser::browseButtonLabel());
    auto buttonBox = new QDialogButtonBox;
    m_generateButton = buttonBox->addButton(Tr::tr("&Generate And Save Key Pair"),
                                            QDialogButtonBox::AcceptRole);
    buttonBox->addButton(QDialogButtonBox::Cancel);

    // clang-format off
    using namespace Layouting;
    Column {
        Group {
            title(Tr::tr("Options")),
            Form {
                Tr::tr("Key algorithm:"), m_rsa, m_ecdsa, st, br,
                Tr::tr("Key &size:"), m_comboBox, st, br,
                Tr::tr("Private key file:"), m_privateKeyFileValueLabel, privateKeyFileButton, st, br,
                Tr::tr("Public key file:"), m_publicKeyFileLabel
            }
        },
        st,
        buttonBox
    }.attachTo(this);
    // clang-format on

    const QString defaultPath = QStandardPaths::writableLocation(QStandardPaths::HomeLocation)
        + QLatin1String("/.ssh/qtc_id");
    setPrivateKeyFile(FilePath::fromString(defaultPath));

    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::close);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &SshKeyCreationDialog::generateKeys);
    connect(m_rsa, &QRadioButton::toggled, this, &SshKeyCreationDialog::keyTypeChanged);
    connect(privateKeyFileButton, &QPushButton::clicked,
            this, &SshKeyCreationDialog::handleBrowseButtonClicked);
    keyTypeChanged();
}

SshKeyCreationDialog::~SshKeyCreationDialog() = default;

void SshKeyCreationDialog::keyTypeChanged()
{
    m_comboBox->clear();
    QStringList keySizes;
    if (m_rsa->isChecked())
        keySizes << QLatin1String("1024") << QLatin1String("2048") << QLatin1String("4096");
    else if (m_ecdsa->isChecked())
        keySizes << QLatin1String("256") << QLatin1String("384") << QLatin1String("521");
    m_comboBox->addItems(keySizes);
    if (!keySizes.isEmpty())
        m_comboBox->setCurrentIndex(0);
    m_comboBox->setEnabled(!keySizes.isEmpty());
}

void SshKeyCreationDialog::generateKeys()
{
    if (SshSettings::keygenFilePath().isEmpty()) {
        showError(Tr::tr("The ssh-keygen tool was not found."));
        return;
    }
    if (privateKeyFilePath().exists()) {
        showError(Tr::tr("Refusing to overwrite existing private key file \"%1\".")
                  .arg(privateKeyFilePath().toUserOutput()));
        return;
    }
    const QString keyTypeString = QLatin1String(m_rsa->isChecked() ? "rsa": "ecdsa");
    QApplication::setOverrideCursor(Qt::BusyCursor);
    Process keygen;
    const QStringList args{"-t", keyTypeString, "-b", m_comboBox->currentText(),
                "-N", QString(), "-f", privateKeyFilePath().path()};
    QString errorMsg;
    keygen.setCommand({SshSettings::keygenFilePath(), args});
    keygen.start();
    if (!keygen.waitForFinished())
        errorMsg = keygen.errorString();
    else if (keygen.exitCode() != 0)
        errorMsg = QString::fromLocal8Bit(keygen.readAllRawStandardError());
    if (!errorMsg.isEmpty()) {
        showError(Tr::tr("The ssh-keygen tool at \"%1\" failed: %2")
                  .arg(SshSettings::keygenFilePath().toUserOutput(), errorMsg));
    }
    QApplication::restoreOverrideCursor();
    accept();
}

void SshKeyCreationDialog::handleBrowseButtonClicked()
{
    const FilePath filePath = FileUtils::getSaveFilePath(this, Tr::tr("Choose Private Key File Name"));
    if (!filePath.isEmpty())
        setPrivateKeyFile(filePath);
}

void SshKeyCreationDialog::setPrivateKeyFile(const FilePath &filePath)
{
    m_privateKeyFileValueLabel->setText(filePath.toUserOutput());
    m_generateButton->setEnabled(!privateKeyFilePath().isEmpty());
    m_publicKeyFileLabel->setText(filePath.toUserOutput() + ".pub");
}

void SshKeyCreationDialog::showError(const QString &details)
{
    QMessageBox::critical(this, Tr::tr("Key Generation Failed"), details);
}

FilePath SshKeyCreationDialog::privateKeyFilePath() const
{
    return FilePath::fromUserInput(m_privateKeyFileValueLabel->text());
}

FilePath SshKeyCreationDialog::publicKeyFilePath() const
{
    return FilePath::fromUserInput(m_publicKeyFileLabel->text());
}

} // namespace RemoteLinux
