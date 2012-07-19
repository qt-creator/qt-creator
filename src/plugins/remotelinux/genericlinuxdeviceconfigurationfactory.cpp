/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
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
**
**************************************************************************/
#include "genericlinuxdeviceconfigurationfactory.h"

#include "genericlinuxdeviceconfigurationwizard.h"
#include "linuxdeviceconfiguration.h"
#include "remotelinux_constants.h"

#include <utils/qtcassert.h>

using namespace ProjectExplorer;

namespace RemoteLinux {

GenericLinuxDeviceConfigurationFactory::GenericLinuxDeviceConfigurationFactory(QObject *parent)
    : IDeviceFactory(parent)
{
}

QString GenericLinuxDeviceConfigurationFactory::displayNameForId(Core::Id type) const
{
    QTC_ASSERT(type == Core::Id(Constants::GenericLinuxOsType), return QString());
    return tr("Generic Linux Device");
}

QList<Core::Id> GenericLinuxDeviceConfigurationFactory::availableCreationIds() const
{
    return QList<Core::Id>() << Core::Id(Constants::GenericLinuxOsType);
}

IDevice::Ptr GenericLinuxDeviceConfigurationFactory::create(Core::Id id) const
{
    QTC_ASSERT(id == Core::Id(Constants::GenericLinuxOsType), return IDevice::Ptr());
    GenericLinuxDeviceConfigurationWizard wizard;
    if (wizard.exec() != QDialog::Accepted)
        return IDevice::Ptr();
    return wizard.device();
}

bool GenericLinuxDeviceConfigurationFactory::canRestore(const QVariantMap &map) const
{
    return IDevice::typeFromMap(map) == Core::Id(Constants::GenericLinuxOsType);
}

IDevice::Ptr GenericLinuxDeviceConfigurationFactory::restore(const QVariantMap &map) const
{
    QTC_ASSERT(canRestore(map), return LinuxDeviceConfiguration::Ptr());
    const LinuxDeviceConfiguration::Ptr device = LinuxDeviceConfiguration::create();
    device->fromMap(map);
    return device;
}

} // namespace RemoteLinux
