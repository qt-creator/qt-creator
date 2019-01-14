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

#include <projectexplorer/devicesupport/deviceusedportsgatherer.h>
#include <remotelinux/genericlinuxdeviceconfigurationwizardpages.h>
#include <utils/portlist.h>

using namespace ProjectExplorer;

namespace Qnx {
namespace Internal {

QnxDeviceWizard::QnxDeviceWizard(QWidget *parent) :
    Utils::Wizard(parent)
{
    setWindowTitle(tr("New QNX Device Configuration Setup"));

    m_setupPage = new RemoteLinux::GenericLinuxDeviceConfigurationWizardSetupPage(this);
    m_keyDeploymentPage
            = new RemoteLinux::GenericLinuxDeviceConfigurationWizardKeyDeploymentPage(this);
    m_finalPage = new RemoteLinux::GenericLinuxDeviceConfigurationWizardFinalPage(this);

    setPage(SetupPageId, m_setupPage);
    setPage(KeyDeploymenPageId, m_keyDeploymentPage);
    setPage(FinalPageId, m_finalPage);
    m_finalPage->setCommitPage(true);
    QSsh::SshConnectionParameters sshParams;
    sshParams.timeout = 10;
    m_device = QnxDevice::create();
    m_device->setupId(IDevice::ManuallyAdded);
    m_device->setDisplayName(tr("QNX Device"));
    m_device->setType(Constants::QNX_QNX_OS_TYPE);
    m_device->setMachineType(IDevice::Hardware);
    m_device->setSshParameters(sshParams);
    m_device->setFreePorts(Utils::PortList::fromString(QLatin1String("10000-10100")));
    m_setupPage->setDevice(m_device);
    m_keyDeploymentPage->setDevice(m_device);
}

IDevice::Ptr QnxDeviceWizard::device()
{
    return m_device;
}

} // namespace Internal
} // namespace Qnx
