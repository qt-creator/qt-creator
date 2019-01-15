/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
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

#include "genericlinuxdeviceconfigurationwizard.h"

#include "genericlinuxdeviceconfigurationwizardpages.h"
#include "linuxdevice.h"
#include "remotelinux_constants.h"

#include <utils/portlist.h>

using namespace ProjectExplorer;
using namespace QSsh;

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
    setWindowTitle(tr("New Generic Linux Device Configuration Setup"));
    setPage(Internal::SetupPageId, &d->setupPage);
    setPage(Internal::KeyDeploymentPageId, &d->keyDeploymentPage);
    setPage(Internal::FinalPageId, &d->finalPage);
    d->finalPage.setCommitPage(true);
    d->device = LinuxDevice::create();
    d->device->setupId(IDevice::ManuallyAdded, Core::Id());
    d->device->setDisplayName(tr("Generic Linux Device"));
    d->device->setType(Constants::GenericLinuxOsType);
    d->device->setMachineType(IDevice::Hardware);
    d->device->setFreePorts(Utils::PortList::fromString(QLatin1String("10000-10100")));
    SshConnectionParameters sshParams;
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
