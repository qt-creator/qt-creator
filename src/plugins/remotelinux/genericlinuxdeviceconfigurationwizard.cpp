/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
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
namespace {
enum PageId { SetupPageId, FinalPageId };
} // anonymous namespace

class GenericLinuxDeviceConfigurationWizardPrivate
{
public:
    GenericLinuxDeviceConfigurationWizardPrivate(QWidget *parent)
        : setupPage(parent), finalPage(parent)
    {
    }

    GenericLinuxDeviceConfigurationWizardSetupPage setupPage;
    GenericLinuxDeviceConfigurationWizardFinalPage finalPage;
};
} // namespace Internal

GenericLinuxDeviceConfigurationWizard::GenericLinuxDeviceConfigurationWizard(QWidget *parent)
    : Utils::Wizard(parent),
      d(new Internal::GenericLinuxDeviceConfigurationWizardPrivate(this))
{
    setWindowTitle(tr("New Generic Linux Device Configuration Setup"));
    setPage(Internal::SetupPageId, &d->setupPage);
    setPage(Internal::FinalPageId, &d->finalPage);
    d->finalPage.setCommitPage(true);
}

GenericLinuxDeviceConfigurationWizard::~GenericLinuxDeviceConfigurationWizard()
{
    delete d;
}

IDevice::Ptr GenericLinuxDeviceConfigurationWizard::device()
{
    SshConnectionParameters sshParams;
    sshParams.options &= ~SshConnectionOptions(SshEnableStrictConformanceChecks); // For older SSH servers.
    sshParams.host = d->setupPage.hostName();
    sshParams.userName = d->setupPage.userName();
    sshParams.port = 22;
    sshParams.timeout = 10;
    sshParams.authenticationType = d->setupPage.authenticationType();
    if (sshParams.authenticationType != SshConnectionParameters::AuthenticationTypePublicKey)
        sshParams.password = d->setupPage.password();
    else
        sshParams.privateKeyFile = d->setupPage.privateKeyFilePath();
    IDevice::Ptr device = LinuxDevice::create(d->setupPage.configurationName(),
        Core::Id(Constants::GenericLinuxOsType), IDevice::Hardware);
    device->setFreePorts(Utils::PortList::fromString(QLatin1String("10000-10100")));
    device->setSshParameters(sshParams);
    return device;
}

} // namespace RemoteLinux
