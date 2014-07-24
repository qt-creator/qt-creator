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
#include "blackberryconfigurationmanager.h"
#include "blackberrycreatecertificatedialog.h"
#include "blackberrydebugtokenreader.h"

#include <coreplugin/icore.h>

#include <QDialog>
#include <QFileInfo>
#include <QString>
#include <QFile>
#include <QTextStream>
#include <QInputDialog>

using namespace Qnx::Internal;

namespace {
const QLatin1String DebugTokensGroup("DebugTokens");
const QLatin1String DebugTokensPath("DebugTokenPath");
}

BlackBerrySigningUtils & BlackBerrySigningUtils::instance()
{
    static BlackBerrySigningUtils utils;

    return utils;
}

BlackBerrySigningUtils::BlackBerrySigningUtils(QObject *parent) :
    QObject(parent),
    m_defaultCertificate(0),
    m_defaultCertificateStatus(NotOpened)
{
    loadDebugTokens();
}

bool BlackBerrySigningUtils::hasRegisteredKeys()
{
    QFileInfo cskFile(BlackBerryConfigurationManager::instance()->idTokenPath());

    return cskFile.exists();
}

bool BlackBerrySigningUtils::hasLegacyKeys()
{
    QFileInfo cskFile(BlackBerryConfigurationManager::instance()->barsignerCskPath());

    return cskFile.exists();
}

bool BlackBerrySigningUtils::hasDefaultCertificate()
{
    QFileInfo keystore(BlackBerryConfigurationManager::instance()->defaultKeystorePath());

    return keystore.exists();
}

QString BlackBerrySigningUtils::cskPassword(QWidget *passwordPromptParent, bool *ok)
{
    if (m_cskPassword.isEmpty())
        m_cskPassword = promptPassword(tr("Please provide your BlackBerry ID Token PIN."), passwordPromptParent, ok);
    else if (ok)
        *ok = true;

    return m_cskPassword;
}

QString BlackBerrySigningUtils::certificatePassword(QWidget *passwordPromptParent, bool *ok)
{
    if (m_certificatePassword.isEmpty()) {
        m_certificatePassword =
            promptPassword(tr("Please enter your certificate password."), passwordPromptParent, ok);
    } else if (ok) {
        *ok = true;
    }

    return m_certificatePassword;
}

const BlackBerryCertificate * BlackBerrySigningUtils::defaultCertificate() const
{
    return m_defaultCertificate;
}

BlackBerrySigningUtils::Status BlackBerrySigningUtils::defaultCertificateOpeningStatus() const
{
    return m_defaultCertificateStatus;
}

void BlackBerrySigningUtils::openDefaultCertificate(QWidget *passwordPromptParent)
{
    switch (m_defaultCertificateStatus) {
    case Opening:
        return;
    case Opened:
        emit defaultCertificateLoaded(BlackBerryCertificate::Success);
        return;
    default:
        m_defaultCertificateStatus = Opening;
    }

    bool ok;
    const QString password = certificatePassword(passwordPromptParent, &ok);

    // action has been canceled
    if (!ok) {
        m_defaultCertificateStatus = NotOpened;
        return;
    }

    if (m_defaultCertificate)
        m_defaultCertificate->deleteLater();

    m_defaultCertificate = new BlackBerryCertificate(BlackBerryConfigurationManager::instance()->defaultKeystorePath(),
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
    m_defaultCertificateStatus = Opened;
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
    m_defaultCertificateStatus = NotOpened;

    QFile::remove(BlackBerryConfigurationManager::instance()->defaultKeystorePath());
}

QStringList BlackBerrySigningUtils::debugTokens() const
{
    return m_debugTokens;
}

void BlackBerrySigningUtils::addDebugToken(const QString &dt)
{
    if (m_debugTokens.contains(dt) || !QFileInfo(dt).exists())
        return;

    m_debugTokens << dt;
    emit debugTokenListChanged();
}

void BlackBerrySigningUtils::removeDebugToken(const QString &dt)
{
    m_debugTokens.removeOne(dt);
    emit debugTokenListChanged();
}

bool BlackBerrySigningUtils::createCertificate()
{
    BlackBerryCreateCertificateDialog dialog;

    const int result = dialog.exec();

    if (result == QDialog::Rejected)
        return false;

    BlackBerryCertificate *certificate = dialog.certificate();

    if (certificate)
        setDefaultCertificate(certificate);

    return certificate;
}

void BlackBerrySigningUtils::certificateLoaded(int status)
{
    if (status != BlackBerryCertificate::Success) {
        m_defaultCertificateStatus = NotOpened;
        m_defaultCertificate->deleteLater();
        m_defaultCertificate = 0;

        // we have clear the password under any error since we are not able to distinquish
        // if password is correct or not in case BlackBerryCertificate::Error status happens
        clearCertificatePassword();
    } else
        m_defaultCertificateStatus = Opened;


    emit defaultCertificateLoaded(status);
}

void BlackBerrySigningUtils::saveDebugTokens()
{
    if (m_debugTokens.isEmpty())
        return;

    QSettings *settings = Core::ICore::settings();
    settings->beginGroup(DebugTokensGroup);

    int count = 0;
    foreach (const QString &dt, m_debugTokens) {
        settings->beginGroup(QString::fromLatin1("debugToken_%1").arg(++count));
        settings->setValue(DebugTokensPath, dt);
        settings->endGroup();
    }

    settings->endGroup();
}

void BlackBerrySigningUtils::loadDebugTokens()
{
    QSettings *settings = Core::ICore::settings();
    settings->beginGroup(DebugTokensGroup);

    foreach (const QString &dt, settings->childGroups()) {
        settings->beginGroup(dt);
        m_debugTokens << settings->value(DebugTokensPath).toString();
        settings->endGroup();
    }

    settings->endGroup();
}

QString BlackBerrySigningUtils::promptPassword(const QString &message,
                                               QWidget *dialogParent, bool *ok) const
{
    QInputDialog dialog(dialogParent);
    dialog.setWindowTitle(tr("Qt Creator"));
    dialog.setInputMode(QInputDialog::TextInput);
    dialog.setLabelText(message);
    dialog.setTextEchoMode(QLineEdit::Password);

    if (dialog.exec() == QDialog::Rejected) {
        if (ok)
            *ok = false;

        return QString();
    }

    if (ok)
        *ok = true;

    return dialog.textValue();
}
