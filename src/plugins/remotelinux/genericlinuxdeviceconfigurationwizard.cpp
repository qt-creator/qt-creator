// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "genericlinuxdeviceconfigurationwizard.h"

#include "genericlinuxdeviceconfigurationwizardpages.h"
#include "linuxdevice.h"
#include "remotelinux_constants.h"
#include "remotelinuxtr.h"

#include <coreplugin/icore.h>

#include <projectexplorer/devicesupport/idevice.h>
#include <projectexplorer/devicesupport/sshparameters.h>

#include <utils/fileutils.h>

using namespace ProjectExplorer;

namespace RemoteLinux {
namespace Internal {

class GenericLinuxDeviceConfigurationWizardPrivate
{
public:
    GenericLinuxDeviceConfigurationWizardSetupPage setupPage;
    GenericLinuxDeviceConfigurationWizardKeyDeploymentPage keyDeploymentPage;
    GenericLinuxDeviceConfigurationWizardFinalPage finalPage;
    LinuxDevice::Ptr device;
};

} // namespace Internal

GenericLinuxDeviceConfigurationWizard::GenericLinuxDeviceConfigurationWizard()
    : Utils::Wizard(Core::ICore::dialogParent())
    , d(new Internal::GenericLinuxDeviceConfigurationWizardPrivate)
{
    setWindowTitle(Tr::tr("New Remote Linux Device Configuration Setup"));
    addPage(&d->setupPage);
    addPage(&d->keyDeploymentPage);
    addPage(&d->finalPage);
    d->finalPage.setCommitPage(true);

    d->device = LinuxDevice::create();

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
