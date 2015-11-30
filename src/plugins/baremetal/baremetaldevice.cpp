/****************************************************************************
**
** Copyright (C) 2015 Tim Sander <tim@krieglstein.org>
** Copyright (C) 2015 Denis Shienkov <denis.shienkov@gmail.com>
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

QString BareMetalDevice::gdbServerProviderId() const
{
    return m_gdbServerProviderId;
}

void BareMetalDevice::setGdbServerProviderId(const QString &id)
{
    m_gdbServerProviderId = id;
}

void BareMetalDevice::fromMap(const QVariantMap &map)
{
    IDevice::fromMap(map);
    QString gdbServerProvider = map.value(QLatin1String(gdbServerProviderIdKeyC)).toString();
    if (gdbServerProvider.isEmpty()) {
        const QString name = displayName();
        if (GdbServerProvider *provider =
                GdbServerProviderManager::instance()->findByDisplayName(name)) {
            gdbServerProvider = provider->id();
        } else {
            const QSsh::SshConnectionParameters sshParams = sshParameters();
            DefaultGdbServerProvider *newProvider = new DefaultGdbServerProvider;
            newProvider->setDisplayName(name);
            newProvider->m_host = sshParams.host;
            newProvider->m_port = sshParams.port;
            if (GdbServerProviderManager::instance()->registerProvider(newProvider))
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
