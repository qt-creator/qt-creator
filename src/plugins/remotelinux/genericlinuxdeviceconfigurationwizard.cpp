/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
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

#include "genericlinuxdeviceconfigurationwizard.h"

#include "genericlinuxdeviceconfigurationwizardpages.h"
#include "linuxdevice.h"
#include "linuxdevicetestdialog.h"
#include "linuxdevicetester.h"
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
    : QWizard(parent),
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
    QSsh::SshConnectionParameters sshParams;
    sshParams.options &= ~SshConnectionOptions(SshEnableStrictConformanceChecks); // For older SSH servers.
    sshParams.host = d->setupPage.hostName();
    sshParams.userName = d->setupPage.userName();
    sshParams.port = 22;
    sshParams.timeout = 10;
    sshParams.authenticationType = d->setupPage.authenticationType();
    if (sshParams.authenticationType == SshConnectionParameters::AuthenticationByPassword)
        sshParams.password = d->setupPage.password();
    else
        sshParams.privateKeyFile = d->setupPage.privateKeyFilePath();
    IDevice::Ptr device = LinuxDevice::create(d->setupPage.configurationName(),
        Core::Id(Constants::GenericLinuxOsType), IDevice::Hardware);
    device->setFreePorts(Utils::PortList::fromString(QLatin1String("10000-10100")));
    device->setSshParameters(sshParams);
    // Might be called after accept.
    QWidget *parent = isVisible() ? this : static_cast<QWidget *>(0);
    LinuxDeviceTestDialog dlg(device, new GenericLinuxDeviceTester(this), parent);
    dlg.exec();
    return device;
}

} // namespace RemoteLinux
