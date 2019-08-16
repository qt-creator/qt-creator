/****************************************************************************
**
** Copyright (C) 2016 Tim Sander <tim@krieglstein.org>
** Copyright (C) 2016 Denis Shienkov <denis.shienkov@gmail.com>
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

#include "baremetalconstants.h"
#include "baremetaldevice.h"
#include "baremetaldeviceconfigurationwidget.h"
#include "baremetaldeviceconfigurationwizard.h"

#include "defaultgdbserverprovider.h"

#include "gdbserverprovidermanager.h"
#include "gdbserverproviderprocess.h"

#include <coreplugin/id.h>

#include <ssh/sshconnection.h>
#include <utils/qtcassert.h>

#include <QCoreApplication>

using namespace ProjectExplorer;

namespace BareMetal {
namespace Internal {

const char gdbServerProviderIdKeyC[] = "GdbServerProviderId";

// BareMetalDevice

BareMetalDevice::BareMetalDevice()
{
    setDisplayType(QCoreApplication::translate("BareMetal::Internal::BareMetalDevice", "Bare Metal"));
    setDefaultDisplayName(defaultDisplayName());
    setOsType(Utils::OsTypeOther);
}

BareMetalDevice::~BareMetalDevice()
{
    if (GdbServerProvider *provider = GdbServerProviderManager::findProvider(m_gdbServerProviderId))
        provider->unregisterDevice(this);
}

QString BareMetalDevice::defaultDisplayName()
{
    return QCoreApplication::translate("BareMetal::Internal::BareMetalDevice", "Bare Metal Device");
}

QString BareMetalDevice::gdbServerProviderId() const
{
    return m_gdbServerProviderId;
}

void BareMetalDevice::setGdbServerProviderId(const QString &id)
{
    if (id == m_gdbServerProviderId)
        return;
    if (GdbServerProvider *currentProvider = GdbServerProviderManager::findProvider(m_gdbServerProviderId))
        currentProvider->unregisterDevice(this);
    m_gdbServerProviderId = id;
    if (GdbServerProvider *provider = GdbServerProviderManager::findProvider(id)) {
        setChannelByServerProvider(provider);
        provider->registerDevice(this);
    }
}

void BareMetalDevice::unregisterProvider(GdbServerProvider *provider)
{
    if (provider->id() == m_gdbServerProviderId)
        m_gdbServerProviderId.clear();
}

void BareMetalDevice::providerUpdated(GdbServerProvider *provider)
{
    GdbServerProvider *myProvider = GdbServerProviderManager::findProvider(m_gdbServerProviderId);
    if (provider == myProvider)
        setChannelByServerProvider(provider);
}

void BareMetalDevice::setChannelByServerProvider(GdbServerProvider *provider)
{
    QTC_ASSERT(provider, return);
    const QString channel = provider->channelString();
    const int colon = channel.indexOf(QLatin1Char(':'));
    if (colon < 0)
        return;
    QSsh::SshConnectionParameters sshParams = sshParameters();
    sshParams.setHost(channel.left(colon));
    sshParams.setPort(channel.midRef(colon + 1).toUShort());
    setSshParameters(sshParams);
}

void BareMetalDevice::fromMap(const QVariantMap &map)
{
    IDevice::fromMap(map);
    QString gdbServerProvider = map.value(QLatin1String(gdbServerProviderIdKeyC)).toString();
    if (gdbServerProvider.isEmpty()) {
        const QString name = displayName();
        if (GdbServerProvider *provider = GdbServerProviderManager::findByDisplayName(name)) {
            gdbServerProvider = provider->id();
        } else {
            const QSsh::SshConnectionParameters sshParams = sshParameters();
            const auto newProvider = new DefaultGdbServerProvider;
            newProvider->setChannel(sshParams.url);
            newProvider->setDisplayName(name);
            if (GdbServerProviderManager::registerProvider(newProvider))
                gdbServerProvider = newProvider->id();
            else
                delete newProvider;
        }
    }
    setGdbServerProviderId(gdbServerProvider);
}

QVariantMap BareMetalDevice::toMap() const
{
    QVariantMap map = IDevice::toMap();
    map.insert(QLatin1String(gdbServerProviderIdKeyC), gdbServerProviderId());
    return map;
}

DeviceProcessSignalOperation::Ptr BareMetalDevice::signalOperation() const
{
    return DeviceProcessSignalOperation::Ptr();
}

IDeviceWidget *BareMetalDevice::createWidget()
{
    return new BareMetalDeviceConfigurationWidget(sharedFromThis());
}

DeviceProcess *BareMetalDevice::createProcess(QObject *parent) const
{
    return new GdbServerProviderProcess(sharedFromThis(), parent);
}

// Factory

BareMetalDeviceFactory::BareMetalDeviceFactory()
    : IDeviceFactory(Constants::BareMetalOsType)
{
    setDisplayName(tr("Bare Metal Device"));
    setCombinedIcon(":/baremetal/images/baremetaldevicesmall.png",
                    ":/baremetal/images/baremetaldevice.png");
    setCanCreate(true);
    setConstructionFunction(&BareMetalDevice::create);
}

IDevice::Ptr BareMetalDeviceFactory::create() const
{
    BareMetalDeviceConfigurationWizard wizard;
    if (wizard.exec() != QDialog::Accepted)
        return {};
    return wizard.device();
}

} //namespace Internal
} //namespace BareMetal
