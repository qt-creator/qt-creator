/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
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
#include "sshkeycreationdialog.h"
#include "ui_sshkeycreationdialog.h"

#include "linuxdeviceconfiguration.h"

#include <utils/ssh/sshkeygenerator.h>
#include <utils/fileutils.h>

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtGui/QApplication>
#include <QtGui/QDesktopServices>
#include <QtGui/QMessageBox>

using namespace Utils;
using namespace RemoteLinux::Internal;

SshKeyCreationDialog::SshKeyCreationDialog(QWidget *parent)
    : QDialog(parent), m_keyGenerator(0), m_ui(new Ui::SshKeyCreationDialog)
{
    m_ui->setupUi(this);
    m_ui->privateKeyFilePathChooser->setExpectedKind(PathChooser::File);
    const QString defaultPath = QDesktopServices::storageLocation(QDesktopServices::HomeLocation)
        + QLatin1String("/.ssh/qtc_id");
    m_ui->privateKeyFilePathChooser->setPath(defaultPath);
    filePathChanged();

    connect(m_ui->rsa, SIGNAL(toggled(bool)), this, SLOT(keyTypeChanged()));
    connect(m_ui->dsa, SIGNAL(toggled(bool)), this, SLOT(keyTypeChanged()));
    connect(m_ui->privateKeyFilePathChooser, SIGNAL(changed(QString)), SLOT(filePathChanged()));
    connect(m_ui->generateButton, SIGNAL(clicked()), this, SLOT(generateKeys()));
}

SshKeyCreationDialog::~SshKeyCreationDialog()
{
    delete m_keyGenerator;
}

void SshKeyCreationDialog::keyTypeChanged()
{
    m_ui->comboBox->setCurrentIndex(0);
    m_ui->comboBox->setEnabled(m_ui->rsa->isChecked());
}

void SshKeyCreationDialog::generateKeys()
{
    const SshKeyGenerator::KeyType keyType = m_ui->rsa->isChecked()
        ? SshKeyGenerator::Rsa
        : SshKeyGenerator::Dsa;

    if (!m_keyGenerator)
        m_keyGenerator = new SshKeyGenerator;

    QApplication::setOverrideCursor(Qt::BusyCursor);
    const bool success = m_keyGenerator->generateKeys(keyType, SshKeyGenerator::Mixed,
        m_ui->comboBox->currentText().toUShort());
    QApplication::restoreOverrideCursor();

    if (success) {
        saveKeys();
    } else {
        QMessageBox::critical(this, tr("Key Generation Failed"), m_keyGenerator->error());
    }
}

void SshKeyCreationDialog::filePathChanged()
{
    m_ui->generateButton->setEnabled(!privateKeyFilePath().isEmpty());
    m_ui->publicKeyFileLabel->setText(privateKeyFilePath() + QLatin1String(".pub"));
}

void SshKeyCreationDialog::saveKeys()
{
    const QString parentDir = QFileInfo(privateKeyFilePath()).dir().path();
    if (!QDir::root().mkpath(parentDir)) {
        QMessageBox::critical(this, tr("Failure To Save Key File"),
            tr("Failed to create directory: '%1'.").arg(parentDir));
        return;
    }

    FileSaver privSaver(privateKeyFilePath());
    privSaver.write(m_keyGenerator->privateKey());
    if (!privSaver.finalize(this))
        return;
    QFile::setPermissions(privateKeyFilePath(), QFile::ReadOwner | QFile::WriteOwner);

    FileSaver pubSaver(m_ui->publicKeyFileLabel->text());
    pubSaver.write(m_keyGenerator->publicKey());
    if (pubSaver.finalize(this))
        accept();
}

QString SshKeyCreationDialog::privateKeyFilePath() const
{
    return m_ui->privateKeyFilePathChooser->path();
}
