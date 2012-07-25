/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (C) 2011 - 2012 Research In Motion
**
** Contact: Research In Motion (blackberry-qt@qnx.com)
** Contact: KDAB (info@kdab.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#include "qnxdeviceconfigurationwizard.h"

#include "qnxconstants.h"
#include "qnxdeviceconfigurationwizardpages.h"
#include "qnxdeviceconfiguration.h"

#include <remotelinux/genericlinuxdeviceconfigurationwizardpages.h>
#include <remotelinux/linuxdevicetestdialog.h>
#include <remotelinux/linuxdevicetester.h>
#include <utils/portlist.h>
#include <remotelinux/remotelinuxusedportsgatherer.h>
#include <ssh/sshconnection.h>

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
    sshParams.proxyType = QSsh::SshConnectionParameters::NoProxy;
    sshParams.host = m_setupPage->hostName();
    sshParams.userName = m_setupPage->userName();
    sshParams.port = 22;
    sshParams.timeout = 10;
    sshParams.authenticationType = m_setupPage->authenticationType();
    if (sshParams.authenticationType == QSsh::SshConnectionParameters::AuthenticationByPassword) {
        sshParams.password = m_setupPage->password();
    } else {
        sshParams.privateKeyFile = m_setupPage->privateKeyFilePath();
    }

    QnxDeviceConfiguration::Ptr devConf = QnxDeviceConfiguration::create(m_setupPage->configurationName(),
        Core::Id(Constants::QNX_QNX_OS_TYPE), IDevice::Hardware);
    devConf->setSshParameters(sshParams);
    devConf->setFreePorts(Utils::PortList::fromString(QLatin1String("10000-10100")));

    RemoteLinux::GenericLinuxDeviceTester *devTester = new RemoteLinux::GenericLinuxDeviceTester(this);
    devTester->usedPortsGatherer()->setCommand(QLatin1String(Constants::QNX_PORT_GATHERER_COMMAND));

    RemoteLinux::LinuxDeviceTestDialog dlg(devConf, devTester, this);
    dlg.exec();

    return devConf;
}
