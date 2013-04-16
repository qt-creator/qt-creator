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
