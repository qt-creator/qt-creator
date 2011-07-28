/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
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
#include "sshkeycreationdialog.h"

#include "linuxdeviceconfiguration.h"

#include <utils/ssh/sshkeygenerator.h>
#include <utils/fileutils.h>

#include <QtCore/QDir>
#include <QtGui/QApplication>
#include <QtGui/QDesktopServices>
#include <QtGui/QFileDialog>
#include <QtGui/QMessageBox>
#include <QtNetwork/QHostInfo>

using namespace Utils;
using namespace RemoteLinux::Internal;

SshKeyCreationDialog::SshKeyCreationDialog(QWidget *parent)
    : QDialog(parent)
    , home(QDesktopServices::storageLocation(QDesktopServices::HomeLocation))
    , m_keyGenerator(new SshKeyGenerator)
{
    m_ui.setupUi(this);

    connect(m_ui.rsa, SIGNAL(toggled(bool)), this, SLOT(slotToggled()));
    connect(m_ui.dsa, SIGNAL(toggled(bool)), this, SLOT(slotToggled()));

    connect(m_ui.generateButton, SIGNAL(clicked()), this, SLOT(generateSshKey()));
    connect(m_ui.savePublicKey, SIGNAL(clicked()), this, SLOT(savePublicKey()));
    connect(m_ui.savePrivateKey, SIGNAL(clicked()), this, SLOT(savePrivateKey()));
}

SshKeyCreationDialog::~SshKeyCreationDialog()
{
}

void SshKeyCreationDialog::slotToggled()
{
    m_ui.comboBox->setCurrentIndex(0);
    m_ui.comboBox->setEnabled(m_ui.rsa->isChecked());
}

void SshKeyCreationDialog::generateSshKey()
{
    const SshKeyGenerator::KeyType keyType = m_ui.rsa->isChecked()
        ? SshKeyGenerator::Rsa
        : SshKeyGenerator::Dsa;

    QApplication::setOverrideCursor(Qt::BusyCursor);

    if (m_keyGenerator->generateKeys(keyType, SshKeyGenerator::Mixed,
                                     m_ui.comboBox->currentText().toUShort())) {
        m_ui.plainTextEdit->setPlainText(m_keyGenerator->publicKey());
        m_ui.savePublicKey->setEnabled(true);
        m_ui.savePrivateKey->setEnabled(true);
    } else {
        m_ui.plainTextEdit->setPlainText(m_keyGenerator->error());
    }

    QApplication::restoreOverrideCursor();
}

void SshKeyCreationDialog::savePublicKey()
{
    saveKey(true);
}

void SshKeyCreationDialog::savePrivateKey()
{
    saveKey(false);
}

void SshKeyCreationDialog::checkSshDir()
{
    QDir dir(home + QString::fromLatin1("/.ssh"));
    if (!dir.exists())
        dir.mkpath(home + QString::fromLatin1("/.ssh"));
}

void SshKeyCreationDialog::saveKey(bool publicKey)
{
    checkSshDir();
    const QString suggestedTypeSuffix =
        m_keyGenerator->type() == SshKeyGenerator::Rsa ? "rsa" : "dsa";
    const QString suggestedName = home + QString::fromLatin1("/.ssh/id_%1%2")
        .arg(suggestedTypeSuffix).arg(publicKey ? ".pub" : "");
    const QString dlgTitle
        = publicKey ? tr("Save Public Key File") : tr("Save Private Key File");
    const QString fileName
        = QFileDialog::getSaveFileName(this, dlgTitle, suggestedName);
    if (fileName.isEmpty())
        return;

    Utils::FileSaver saver(fileName);
    saver.write(publicKey
            ? m_keyGenerator->publicKey()
            : m_keyGenerator->privateKey());
    if (saver.finalize(this) && !publicKey)
        emit privateKeyGenerated(fileName);
    if (!publicKey)
        QFile::setPermissions(fileName, QFile::ReadOwner | QFile::WriteOwner);
}
