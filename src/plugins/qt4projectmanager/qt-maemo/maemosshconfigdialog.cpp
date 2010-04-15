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
#include "ne7sshobject.h"

#include <ne7ssh.h>

#include <QtCore/QDir>

#include <QtNetwork/QHostInfo>

#include <QtGui/QApplication>
#include <QtGui/QDesktopServices>
#include <QtGui/QFileDialog>

using namespace Qt4ProjectManager::Internal;

MaemoSshConfigDialog::MaemoSshConfigDialog(QWidget *parent)
    : QDialog(parent)
    , home(QDesktopServices::storageLocation(QDesktopServices::HomeLocation))
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
    algorithm = m_ui.rsa->isChecked() ? "rsa" : "dsa";
    tmpKey = QDir::tempPath().append(QLatin1Char('/') + algorithm).toUtf8();
    QByteArray userId = QString(home.mid(home.lastIndexOf(QLatin1Char('/')) + 1)
        + QLatin1Char('@') + QHostInfo::localHostName()).toUtf8();

    QFile::remove(tmpKey);
    QFile::remove(tmpKey + ".pub");

    QApplication::setOverrideCursor(Qt::BusyCursor);

    QSharedPointer<ne7ssh> ssh = Ne7SshObject::instance()->get();
    if (ssh->generateKeyPair(algorithm, userId, tmpKey, tmpKey + ".pub",
        m_ui.comboBox->currentText().toUShort())) {
        QFile file(tmpKey + ".pub");
        if (file.open(QIODevice::ReadOnly))
            m_ui.plainTextEdit->setPlainText(file.readAll());
        m_ui.savePublicKey->setEnabled(true);
        m_ui.savePrivateKey->setEnabled(true);
    } else {
        m_ui.plainTextEdit->setPlainText(tr("Could not create SSH key pair."));
    }

    QApplication::restoreOverrideCursor();
}

void MaemoSshConfigDialog::savePublicKey()
{
    checkSshDir();
    copyFile(QFileDialog::getSaveFileName(this, tr("Choose folder to save "
        "public key file"), home + QString::fromLatin1("/.ssh/id_%1.pub")
        .arg(algorithm.constData())), true);
}

void MaemoSshConfigDialog::savePrivateKey()
{
    checkSshDir();
    copyFile(QFileDialog::getSaveFileName(this, tr("Choose folder to save "
        "private key file"), home + QString::fromLatin1("/.ssh/id_%1")
        .arg(algorithm.constData())), false);
}

void MaemoSshConfigDialog::checkSshDir()
{
    QDir dir(home + QString::fromLatin1("/.ssh"));
    if (!dir.exists())
        dir.mkpath(home + QString::fromLatin1("/.ssh"));
}

void MaemoSshConfigDialog::copyFile(const QString &file, bool pubKey)
{
    if (!file.isEmpty()) {
        if (!QFile::exists(file) || QFile::remove(file)) {
            QFile(tmpKey + (pubKey ? ".pub" : "")).copy(file);
            if (pubKey)
                emit publicKeyGenerated(file);
            else
                emit privateKeyGenerated(file);
        }
    }
}
