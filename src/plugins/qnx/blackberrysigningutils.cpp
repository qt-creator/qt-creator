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

#include "blackberrysigningutils.h"
#include "blackberrycertificate.h"
#include "blackberryconfiguration.h"
#include "blackberryconfigurationmanager.h"

#include <QFileInfo>
#include <QString>
#include <QFile>
#include <QTextStream>
#include <QInputDialog>

using namespace Qnx::Internal;

BlackBerrySigningUtils & BlackBerrySigningUtils::instance()
{
    static BlackBerrySigningUtils utils;

    return utils;
}

BlackBerrySigningUtils::BlackBerrySigningUtils(QObject *parent) :
    QObject(parent),
    m_defaultCertificate(0)
{
}

bool BlackBerrySigningUtils::hasRegisteredKeys()
{
    BlackBerryConfigurationManager &configuration = BlackBerryConfigurationManager::instance();

    QFileInfo cskFile(configuration.idTokenPath());

    return cskFile.exists();
}

bool BlackBerrySigningUtils::hasLegacyKeys()
{
    BlackBerryConfigurationManager &configuration = BlackBerryConfigurationManager::instance();

    QFileInfo cskFile(configuration.barsignerCskPath());

    return cskFile.exists();
}

bool BlackBerrySigningUtils::hasDefaultCertificate()
{
    BlackBerryConfigurationManager &configuration = BlackBerryConfigurationManager::instance();

    QFileInfo keystore(configuration.defaultKeystorePath());

    return keystore.exists();
}

QString BlackBerrySigningUtils::cskPassword()
{
    if (m_cskPassword.isEmpty())
        m_cskPassword = promptPassword(tr("Please provide your bbidtoken.csk PIN."));

    return m_cskPassword;
}

QString BlackBerrySigningUtils::certificatePassword()
{
    if (m_certificatePassword.isEmpty())
        m_certificatePassword = promptPassword(tr("Please enter your certificate password."));

    return m_certificatePassword;
}

const BlackBerryCertificate * BlackBerrySigningUtils::defaultCertificate() const
{
    return m_defaultCertificate;
}

void BlackBerrySigningUtils::openDefaultCertificate()
{
    if (m_defaultCertificate) {
        emit defaultCertificateLoaded(BlackBerryCertificate::Success);
        return;
    }

    const QString password = certificatePassword();

    BlackBerryConfigurationManager &configManager = BlackBerryConfigurationManager::instance();

    m_defaultCertificate = new BlackBerryCertificate(configManager.defaultKeystorePath(),
            QString(), password, this);

    connect(m_defaultCertificate, SIGNAL(finished(int)), this, SLOT(certificateLoaded(int)));

    m_defaultCertificate->load();
}

void BlackBerrySigningUtils::setDefaultCertificate(BlackBerryCertificate *certificate)
{
    if (m_defaultCertificate)
        m_defaultCertificate->deleteLater();

    certificate->setParent(this);
    m_defaultCertificate = certificate;
}

void BlackBerrySigningUtils::clearCskPassword()
{
    m_cskPassword.clear();
}

void BlackBerrySigningUtils::clearCertificatePassword()
{
    m_certificatePassword.clear();
}

void BlackBerrySigningUtils::deleteDefaultCertificate()
{
    clearCertificatePassword();
    m_defaultCertificate->deleteLater();
    m_defaultCertificate = 0;

    BlackBerryConfigurationManager &configuration = BlackBerryConfigurationManager::instance();

    QFile::remove(configuration.defaultKeystorePath());
}

void BlackBerrySigningUtils::certificateLoaded(int status)
{
    if (status != BlackBerryCertificate::Success) {
        m_defaultCertificate->deleteLater();
        m_defaultCertificate = 0;

        if (status == BlackBerryCertificate::WrongPassword)
            clearCertificatePassword();
    }

    emit defaultCertificateLoaded(status);
}

QString BlackBerrySigningUtils::promptPassword(const QString &message) const
{
    QInputDialog dialog;
    dialog.setWindowTitle(tr("Qt Creator"));
    dialog.setInputMode(QInputDialog::TextInput);
    dialog.setLabelText(message);
    dialog.setTextEchoMode(QLineEdit::Password);

    if (dialog.exec() == QDialog::Rejected)
        return QString();

    return dialog.textValue();
}
