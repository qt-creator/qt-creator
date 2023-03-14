// Copyright (C) 2016 BlackBerry Limited. All rights reserved.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qnxdevicewizard.h"

#include "qnxconstants.h"
#include "qnxdevice.h"
#include "qnxtr.h"

#include <coreplugin/icore.h>

#include <projectexplorer/devicesupport/sshparameters.h>

#include <remotelinux/genericlinuxdeviceconfigurationwizardpages.h>

#include <utils/portlist.h>
#include <utils/wizard.h>

using namespace ProjectExplorer;
using namespace RemoteLinux;
using namespace Utils;

namespace Qnx::Internal {

class QnxDeviceWizard : public Wizard
{
public:
    QnxDeviceWizard() : Wizard(Core::ICore::dialogParent())
    {
        setWindowTitle(Tr::tr("New QNX Device Configuration Setup"));

        addPage(&m_setupPage);
        addPage(&m_keyDeploymentPage);
        addPage(&m_finalPage);
        m_finalPage.setCommitPage(true);

        SshParameters sshParams;
        sshParams.timeout = 10;
        m_device = QnxDevice::create();
        m_device->setupId(IDevice::ManuallyAdded);
        m_device->setType(Constants::QNX_QNX_OS_TYPE);
        m_device->setMachineType(IDevice::Hardware);
        m_device->setSshParameters(sshParams);
        m_device->setFreePorts(PortList::fromString("10000-10100"));

        m_setupPage.setDevice(m_device);
        m_keyDeploymentPage.setDevice(m_device);
    }

    IDevice::Ptr device() const { return m_device; }

private:
    GenericLinuxDeviceConfigurationWizardSetupPage m_setupPage;
    GenericLinuxDeviceConfigurationWizardKeyDeploymentPage m_keyDeploymentPage;
    GenericLinuxDeviceConfigurationWizardFinalPage m_finalPage;

    LinuxDevice::Ptr m_device;
};


IDevice::Ptr runDeviceWizard()
{
    QnxDeviceWizard wizard;
    if (wizard.exec() != QDialog::Accepted)
        return IDevice::Ptr();
    return wizard.device();
}

} // Qnx::Internal
