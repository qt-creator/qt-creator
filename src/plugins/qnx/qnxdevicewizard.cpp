// Copyright (C) 2016 BlackBerry Limited. All rights reserved.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qnxdevicewizard.h"

#include "qnxconstants.h"
#include "qnxtr.h"

#include <projectexplorer/devicesupport/sshparameters.h>
#include <remotelinux/genericlinuxdeviceconfigurationwizardpages.h>
#include <utils/portlist.h>

using namespace ProjectExplorer;

namespace Qnx::Internal {

QnxDeviceWizard::QnxDeviceWizard(QWidget *parent) :
    Utils::Wizard(parent)
{
    setWindowTitle(Tr::tr("New QNX Device Configuration Setup"));

    m_setupPage = new RemoteLinux::GenericLinuxDeviceConfigurationWizardSetupPage(this);
    m_keyDeploymentPage
            = new RemoteLinux::GenericLinuxDeviceConfigurationWizardKeyDeploymentPage(this);
    m_finalPage = new RemoteLinux::GenericLinuxDeviceConfigurationWizardFinalPage(this);

    setPage(SetupPageId, m_setupPage);
    setPage(KeyDeploymenPageId, m_keyDeploymentPage);
    setPage(FinalPageId, m_finalPage);
    m_finalPage->setCommitPage(true);
    SshParameters sshParams;
    sshParams.timeout = 10;
    m_device = QnxDevice::create();
    m_device->setupId(IDevice::ManuallyAdded);
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

} // Qnx::Internal
