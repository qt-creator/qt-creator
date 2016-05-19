/****************************************************************************
**
** Copyright (C) 2016 BlackBerry Limited. All rights reserved.
** Contact: KDAB (info@kdab.com)
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

#include "qnxdevicewizard.h"

#include "qnxconstants.h"
#include "qnxdevice.h"

#include <projectexplorer/devicesupport/deviceusedportsgatherer.h>
#include <remotelinux/genericlinuxdeviceconfigurationwizardpages.h>
#include <utils/portlist.h>

using namespace ProjectExplorer;

namespace Qnx {
namespace Internal {

class QnxDeviceWizardSetupPage : public RemoteLinux::GenericLinuxDeviceConfigurationWizardSetupPage
{
public:
    QnxDeviceWizardSetupPage(QWidget *parent) :
        RemoteLinux::GenericLinuxDeviceConfigurationWizardSetupPage(parent)
    {}

    QString defaultConfigurationName() const { return QnxDeviceWizard::tr("QNX Device"); }
};

QnxDeviceWizard::QnxDeviceWizard(QWidget *parent) :
    Utils::Wizard(parent)
{
    setWindowTitle(tr("New QNX Device Configuration Setup"));

    m_setupPage = new QnxDeviceWizardSetupPage(this);
    m_finalPage = new RemoteLinux::GenericLinuxDeviceConfigurationWizardFinalPage(this);

    setPage(SetupPageId, m_setupPage);
    setPage(FinalPageId, m_finalPage);
    m_finalPage->setCommitPage(true);
}

IDevice::Ptr QnxDeviceWizard::device()
{
    QSsh::SshConnectionParameters sshParams;
    sshParams.options = QSsh::SshIgnoreDefaultProxy;
    sshParams.host = m_setupPage->hostName();
    sshParams.userName = m_setupPage->userName();
    sshParams.port = 22;
    sshParams.timeout = 10;
    sshParams.authenticationType = m_setupPage->authenticationType();
    if (sshParams.authenticationType == QSsh::SshConnectionParameters::AuthenticationTypeTryAllPasswordBasedMethods
        || sshParams.authenticationType == QSsh::SshConnectionParameters::AuthenticationTypePassword)
        sshParams.password = m_setupPage->password();
    else
        sshParams.privateKeyFile = m_setupPage->privateKeyFilePath();

    QnxDevice::Ptr device = QnxDevice::create(m_setupPage->configurationName(),
        Core::Id(Constants::QNX_QNX_OS_TYPE), IDevice::Hardware);
    device->setSshParameters(sshParams);
    device->setFreePorts(Utils::PortList::fromString(QLatin1String("10000-10100")));

    return device;
}

} // namespace Internal
} // namespace Qnx
