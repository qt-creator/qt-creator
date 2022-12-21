// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "genericlinuxdeviceconfigurationwizard.h"

#include "genericlinuxdeviceconfigurationwizardpages.h"
#include "linuxdevice.h"
#include "remotelinux_constants.h"
#include "remotelinuxtr.h"

#include <projectexplorer/devicesupport/idevice.h>
#include <projectexplorer/devicesupport/sshparameters.h>

#include <utils/portlist.h>
#include <utils/fileutils.h>

using namespace ProjectExplorer;

namespace RemoteLinux {
namespace Internal {
enum PageId { SetupPageId, KeyDeploymentPageId, FinalPageId };

class GenericLinuxDeviceConfigurationWizardPrivate
{
public:
    GenericLinuxDeviceConfigurationWizardPrivate(QWidget *parent)
        : setupPage(parent), keyDeploymentPage(parent), finalPage(parent)
    {
    }

    GenericLinuxDeviceConfigurationWizardSetupPage setupPage;
    GenericLinuxDeviceConfigurationWizardKeyDeploymentPage keyDeploymentPage;
    GenericLinuxDeviceConfigurationWizardFinalPage finalPage;
    LinuxDevice::Ptr device;
};
} // namespace Internal

GenericLinuxDeviceConfigurationWizard::GenericLinuxDeviceConfigurationWizard(QWidget *parent)
    : Utils::Wizard(parent),
      d(new Internal::GenericLinuxDeviceConfigurationWizardPrivate(this))
{
    setWindowTitle(Tr::tr("New Remote Linux Device Configuration Setup"));
    setPage(Internal::SetupPageId, &d->setupPage);
    setPage(Internal::KeyDeploymentPageId, &d->keyDeploymentPage);
    setPage(Internal::FinalPageId, &d->finalPage);
    d->finalPage.setCommitPage(true);
    d->device = LinuxDevice::create();
    d->device->setupId(IDevice::ManuallyAdded, Utils::Id());
    d->device->setType(Constants::GenericLinuxOsType);
    d->device->setMachineType(IDevice::Hardware);
    d->device->setFreePorts(Utils::PortList::fromString(QLatin1String("10000-10100")));
    SshParameters sshParams;
    sshParams.timeout = 10;
    d->device->setSshParameters(sshParams);
    d->setupPage.setDevice(d->device);
    d->keyDeploymentPage.setDevice(d->device);
}

GenericLinuxDeviceConfigurationWizard::~GenericLinuxDeviceConfigurationWizard()
{
    delete d;
}

IDevice::Ptr GenericLinuxDeviceConfigurationWizard::device()
{
    return d->device;
}

} // namespace RemoteLinux
