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

#include "baremetaldevice.h"
#include "baremetaldeviceconfigurationwidget.h"
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

BareMetalDevice::Ptr BareMetalDevice::create()
{
    return Ptr(new BareMetalDevice);
}

BareMetalDevice::Ptr BareMetalDevice::create(const QString &name, Core::Id type, MachineType machineType, Origin origin, Core::Id id)
{
    return Ptr(new BareMetalDevice(name, type, machineType, origin, id));
}

BareMetalDevice::Ptr BareMetalDevice::create(const BareMetalDevice &other)
{
    return Ptr(new BareMetalDevice(other));
}

BareMetalDevice::~BareMetalDevice()
{
    if (GdbServerProvider *provider = GdbServerProviderManager::findProvider(m_gdbServerProviderId))
        provider->unregisterDevice(this);
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
    const QString channel = provider->channel();
    const int colon = channel.indexOf(QLatin1Char(':'));
    if (colon < 0)
        return;
    QSsh::SshConnectionParameters sshParams = sshParameters();
    sshParams.host = channel.left(colon);
    sshParams.port = channel.mid(colon + 1).toUShort();
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
            DefaultGdbServerProvider *newProvider = new DefaultGdbServerProvider;
            newProvider->setDisplayName(name);
            newProvider->m_host = sshParams.host;
            newProvider->m_port = sshParams.port;
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

BareMetalDevice::IDevice::Ptr BareMetalDevice::clone() const
{
    return Ptr(new BareMetalDevice(*this));
}

DeviceProcessSignalOperation::Ptr BareMetalDevice::signalOperation() const
{
    return DeviceProcessSignalOperation::Ptr();
}

QString BareMetalDevice::displayType() const
{
    return QCoreApplication::translate("BareMetal::Internal::BareMetalDevice", "Bare Metal");
}

IDeviceWidget *BareMetalDevice::createWidget()
{
    return new BareMetalDeviceConfigurationWidget(sharedFromThis());
}

QList<Core::Id> BareMetalDevice::actionIds() const
{
    return QList<Core::Id>(); // no actions
}

QString BareMetalDevice::displayNameForActionId(Core::Id actionId) const
{
    QTC_ASSERT(actionIds().contains(actionId), return QString());
    return QString();
}

void BareMetalDevice::executeAction(Core::Id actionId, QWidget *parent)
{
    QTC_ASSERT(actionIds().contains(actionId), return);
    Q_UNUSED(parent);
}

DeviceProcess *BareMetalDevice::createProcess(QObject *parent) const
{
    return new GdbServerProviderProcess(sharedFromThis(), parent);
}

BareMetalDevice::BareMetalDevice(const QString &name, Core::Id type, MachineType machineType, Origin origin, Core::Id id)
    : IDevice(type, origin, machineType, id)
{
    setDisplayName(name);
}

BareMetalDevice::BareMetalDevice(const BareMetalDevice &other)
    : IDevice(other)
{
    setGdbServerProviderId(other.gdbServerProviderId());
}

} //namespace Internal
} //namespace BareMetal
