/**************************************************************************
**
** Copyright (C) 2011 - 2013 Research In Motion
**
** Contact: Research In Motion (blackberry-qt@qnx.com)
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

#include "blackberrydeviceconfigurationwizard.h"
#include "blackberrydeviceconfigurationwizardpages.h"
#include "qnxconstants.h"
#include "blackberrydeviceconfiguration.h"

#include <ssh/sshconnection.h>
#include <ssh/sshkeygenerator.h>
#include <utils/portlist.h>
#include <utils/fileutils.h>
#include <utils/qtcassert.h>

#include <QDir>
#include <QFileInfo>
#include <QMessageBox>
#include <QHostInfo>

using namespace Qnx;
using namespace Qnx::Internal;

BlackBerryDeviceConfigurationWizard::BlackBerryDeviceConfigurationWizard(QWidget *parent) :
    QWizard(parent)
{
    setWindowTitle(tr("New BlackBerry Device Configuration Setup"));

    m_setupPage = new BlackBerryDeviceConfigurationWizardSetupPage(this);
    m_sshKeyPage = new BlackBerryDeviceConfigurationWizardSshKeyPage(this);
    m_finalPage = new BlackBerryDeviceConfigurationWizardFinalPage(this);

    setPage(SetupPageId, m_setupPage);
    setPage(SshKeyPageId, m_sshKeyPage);
    setPage(FinalPageId, m_finalPage);
    m_finalPage->setCommitPage(true);
}

ProjectExplorer::IDevice::Ptr BlackBerryDeviceConfigurationWizard::device()
{
    QSsh::SshConnectionParameters sshParams;
    sshParams.options = QSsh::SshIgnoreDefaultProxy;
    sshParams.host = m_setupPage->hostName();
    sshParams.password = m_setupPage->password();
    sshParams.authenticationType = QSsh::SshConnectionParameters::AuthenticationByKey;
    sshParams.privateKeyFile = m_sshKeyPage->privateKey();
    sshParams.userName = QLatin1String("devuser");
    sshParams.timeout = 10;
    sshParams.port = 22;

    BlackBerryDeviceConfiguration::Ptr configuration = BlackBerryDeviceConfiguration::create(m_setupPage->deviceName(),
                                                                                             Core::Id(Constants::QNX_BB_OS_TYPE),
                                                                                             m_setupPage->machineType());
    configuration->setSshParameters(sshParams);
    configuration->setDebugToken(m_setupPage->debugToken());
    return configuration;
}

void BlackBerryDeviceConfigurationWizard::accept()
{
    if (m_sshKeyPage->isGenerated()) {
        if (saveKeys())
            QWizard::accept();
    } else {
        QWizard::accept();
    }
}

bool BlackBerryDeviceConfigurationWizard::saveKeys()
{
    const QString privKeyPath = m_sshKeyPage->privateKey();
    const QString pubKeyPath = m_sshKeyPage->publicKey();

    const QString storeLocation = QFileInfo(privKeyPath).absolutePath();
    if (!QDir::root().mkpath(storeLocation)) {
        QMessageBox::critical(this, tr("Failure to Save Key File"),
                              tr("Failed to create directory: '%1'.").arg(storeLocation));
        return false;
    }

    if (QFileInfo(privKeyPath).exists()) {
        QMessageBox::critical(this, tr("Failure to Save Key File"),
                              tr("Private key file already exists: '%1'").arg(privKeyPath));
        return false;
    }
    if (QFileInfo(pubKeyPath).exists()) {
        QMessageBox::critical(this, tr("Failure to Save Key File"),
                              tr("Public key file already exists: '%1'").arg(pubKeyPath));
        return false;
    }

    Utils::FileSaver privSaver(privKeyPath);
    privSaver.write(m_sshKeyPage->keyGenerator()->privateKey());
    if (!privSaver.finalize(this))
        return false; // finalize shows an error message if necessary
    QFile::setPermissions(privKeyPath, QFile::ReadOwner | QFile::WriteOwner);

    Utils::FileSaver pubSaver(pubKeyPath);

    // blackberry-connect requires an @ character to be included in the RSA comment
    const QString atHost = QLatin1String("@") + QHostInfo::localHostName();
    QByteArray pubKeyContent = m_sshKeyPage->keyGenerator()->publicKey();
    pubKeyContent.append(atHost.toLocal8Bit());

    pubSaver.write(pubKeyContent);
    if (!pubSaver.finalize(this))
        return false;

    return true;
}
