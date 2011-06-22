/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
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
#include "genericlinuxdeviceconfigurationwizard.h"

#include "genericlinuxdeviceconfigurationwizardpages.h"
#include "maemoconfigtestdialog.h"

using namespace Utils;

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
    : ILinuxDeviceConfigurationWizard(parent),
      m_d(new Internal::GenericLinuxDeviceConfigurationWizardPrivate(this))
{
    setWindowTitle(tr("New Generic Linux Device Configuration Setup"));
    setPage(Internal::SetupPageId, &m_d->setupPage);
    setPage(Internal::FinalPageId, &m_d->finalPage);
    m_d->finalPage.setCommitPage(true);
}

GenericLinuxDeviceConfigurationWizard::~GenericLinuxDeviceConfigurationWizard()
{
    delete m_d;
}

LinuxDeviceConfiguration::Ptr GenericLinuxDeviceConfigurationWizard::deviceConfiguration()
{
    Utils::SshConnectionParameters sshParams(SshConnectionParameters::NoProxy);
    sshParams.host = m_d->setupPage.hostName();
    sshParams.userName = m_d->setupPage.userName();
    sshParams.port = 22;
    sshParams.timeout = 10;
    sshParams.authenticationType = m_d->setupPage.authenticationType();
    if (sshParams.authenticationType == SshConnectionParameters::AuthenticationByPassword)
        sshParams.password = m_d->setupPage.password();
    else
        sshParams.privateKeyFile = m_d->setupPage.privateKeyFilePath();
    LinuxDeviceConfiguration::Ptr devConf = LinuxDeviceConfiguration::create(m_d->setupPage.configurationName(),
        LinuxDeviceConfiguration::GenericLinuxOsType, LinuxDeviceConfiguration::Physical,
        PortList::fromString(QLatin1String("10000-10100")), sshParams);
    Internal::MaemoConfigTestDialog dlg(devConf, this);
    dlg.exec();
    return devConf;
}

} // namespace RemoteLinux
