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

#include "qnxdeviceconfigurationwizard.h"

#include "qnxconstants.h"
#include "qnxdeviceconfigurationwizardpages.h"
#include "qnxdeviceconfiguration.h"

#include <projectexplorer/devicesupport/deviceusedportsgatherer.h>
#include <remotelinux/genericlinuxdeviceconfigurationwizardpages.h>
#include <remotelinux/linuxdevicetestdialog.h>
#include <remotelinux/linuxdevicetester.h>
#include <utils/portlist.h>

using namespace ProjectExplorer;
using namespace Qnx;
using namespace Qnx::Internal;

QnxDeviceConfigurationWizard::QnxDeviceConfigurationWizard(QWidget *parent) :
    QWizard(parent)
{
    setWindowTitle(tr("New QNX Device Configuration Setup"));

    m_setupPage = new QnxDeviceConfigurationWizardSetupPage(this);
    m_finalPage = new RemoteLinux::GenericLinuxDeviceConfigurationWizardFinalPage(this);

    setPage(SetupPageId, m_setupPage);
    setPage(FinalPageId, m_finalPage);
    m_finalPage->setCommitPage(true);
}

IDevice::Ptr QnxDeviceConfigurationWizard::device()
{
    QSsh::SshConnectionParameters sshParams;
    sshParams.options = QSsh::SshIgnoreDefaultProxy;
    sshParams.host = m_setupPage->hostName();
    sshParams.userName = m_setupPage->userName();
    sshParams.port = 22;
    sshParams.timeout = 10;
    sshParams.authenticationType = m_setupPage->authenticationType();
    if (sshParams.authenticationType == QSsh::SshConnectionParameters::AuthenticationByPassword)
        sshParams.password = m_setupPage->password();
    else
        sshParams.privateKeyFile = m_setupPage->privateKeyFilePath();

    QnxDeviceConfiguration::Ptr device = QnxDeviceConfiguration::create(m_setupPage->configurationName(),
        Core::Id(Constants::QNX_QNX_OS_TYPE), IDevice::Hardware);
    device->setSshParameters(sshParams);
    device->setFreePorts(Utils::PortList::fromString(QLatin1String("10000-10100")));

    RemoteLinux::GenericLinuxDeviceTester *devTester = new RemoteLinux::GenericLinuxDeviceTester(this);
    RemoteLinux::LinuxDeviceTestDialog dlg(device, devTester, this);
    dlg.exec();

    return device;
}
