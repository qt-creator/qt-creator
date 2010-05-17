/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of Qt Creator.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "maemosshconfigdialog.h"

#include "maemodeviceconfigurations.h"

#include <coreplugin/ssh/sshkeygenerator.h>

#include <QtCore/QDir>
#include <QtGui/QApplication>
#include <QtGui/QDesktopServices>
#include <QtGui/QFileDialog>
#include <QtGui/QMessageBox>
#include <QtNetwork/QHostInfo>


using namespace Qt4ProjectManager::Internal;

MaemoSshConfigDialog::MaemoSshConfigDialog(QWidget *parent)
    : QDialog(parent)
    , home(QDesktopServices::storageLocation(QDesktopServices::HomeLocation))
    , m_keyGenerator(new Core::SshKeyGenerator)
{
    m_ui.setupUi(this);

    connect(m_ui.rsa, SIGNAL(toggled(bool)), this, SLOT(slotToggled()));
    connect(m_ui.dsa, SIGNAL(toggled(bool)), this, SLOT(slotToggled()));

    connect(m_ui.generateButton, SIGNAL(clicked()), this, SLOT(generateSshKey()));
    connect(m_ui.savePublicKey, SIGNAL(clicked()), this, SLOT(savePublicKey()));
    connect(m_ui.savePrivateKey, SIGNAL(clicked()), this, SLOT(savePrivateKey()));
}

MaemoSshConfigDialog::~MaemoSshConfigDialog()
{
}

void MaemoSshConfigDialog::slotToggled()
{
    m_ui.comboBox->setCurrentIndex(0);
    m_ui.comboBox->setEnabled(m_ui.rsa->isChecked());
}

void MaemoSshConfigDialog::generateSshKey()
{
    const Core::SshKeyGenerator::KeyType keyType = m_ui.rsa->isChecked()
        ? Core::SshKeyGenerator::Rsa
        : Core::SshKeyGenerator::Dsa;

    QByteArray userId = QString(home.mid(home.lastIndexOf(QLatin1Char('/')) + 1)
        + QLatin1Char('@') + QHostInfo::localHostName()).toUtf8();

    QApplication::setOverrideCursor(Qt::BusyCursor);

    if (m_keyGenerator->generateKeys(keyType, userId,
                                     m_ui.comboBox->currentText().toUShort())) {
        m_ui.plainTextEdit->setPlainText(m_keyGenerator->publicKey());
        m_ui.savePublicKey->setEnabled(true);
        m_ui.savePrivateKey->setEnabled(true);
    } else {
        m_ui.plainTextEdit->setPlainText(m_keyGenerator->error());
    }

    QApplication::restoreOverrideCursor();
}

void MaemoSshConfigDialog::savePublicKey()
{
    saveKey(true);
}

void MaemoSshConfigDialog::savePrivateKey()
{
    saveKey(false);
}

void MaemoSshConfigDialog::checkSshDir()
{
    QDir dir(home + QString::fromLatin1("/.ssh"));
    if (!dir.exists())
        dir.mkpath(home + QString::fromLatin1("/.ssh"));
}

void MaemoSshConfigDialog::saveKey(bool publicKey)
{
    checkSshDir();
    const QString suggestedTypeSuffix =
        m_keyGenerator->type() == Core::SshKeyGenerator::Rsa ? "rsa" : "dsa";
    const QString suggestedName = home + QString::fromLatin1("/.ssh/id_%1%2")
        .arg(suggestedTypeSuffix).arg(publicKey ? ".pub" : "");
    const QString dlgTitle
        = publicKey ? tr("Save Public Key File") : tr("Save Private Key File");
    const QString fileName
        = QFileDialog::getSaveFileName(this, dlgTitle, suggestedName);
    if (fileName.isEmpty())
        return;

    QFile file(fileName);
    const bool canOpen = file.open(QIODevice::WriteOnly);
    if (canOpen)
        file.write(publicKey
            ? m_keyGenerator->publicKey().toUtf8()
            : m_keyGenerator->privateKey().toUtf8());
    if (!canOpen || file.error() != QFile::NoError) {
        QMessageBox::critical(this, tr("Error writing file"),
                              tr("Could not write file '%1':\n %2")
                              .arg(fileName, file.errorString()));
    } else if (!publicKey) {
        emit privateKeyGenerated(fileName);
    }
}
