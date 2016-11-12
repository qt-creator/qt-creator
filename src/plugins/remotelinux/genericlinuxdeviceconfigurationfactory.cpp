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

#include "genericlinuxdeviceconfigurationfactory.h"

#include "genericlinuxdeviceconfigurationwizard.h"
#include "linuxdevice.h"
#include "remotelinux_constants.h"

#include <coreplugin/icore.h>

#include <utils/qtcassert.h>

#include <QIcon>

using namespace ProjectExplorer;

namespace RemoteLinux {

GenericLinuxDeviceConfigurationFactory::GenericLinuxDeviceConfigurationFactory(QObject *parent)
    : IDeviceFactory(parent)
{
}

QString GenericLinuxDeviceConfigurationFactory::displayNameForId(Core::Id type) const
{
    QTC_ASSERT(type == Constants::GenericLinuxOsType, return QString());
    return tr("Generic Linux Device");
}

QList<Core::Id> GenericLinuxDeviceConfigurationFactory::availableCreationIds() const
{
    return QList<Core::Id>() << Core::Id(Constants::GenericLinuxOsType);
}

QIcon GenericLinuxDeviceConfigurationFactory::iconForId(Core::Id type) const
{
    Q_UNUSED(type)
    return QIcon();
}

IDevice::Ptr GenericLinuxDeviceConfigurationFactory::create(Core::Id id) const
{
    QTC_ASSERT(id == Constants::GenericLinuxOsType, return IDevice::Ptr());
    GenericLinuxDeviceConfigurationWizard wizard(Core::ICore::mainWindow());
    if (wizard.exec() != QDialog::Accepted)
        return IDevice::Ptr();
    return wizard.device();
}

bool GenericLinuxDeviceConfigurationFactory::canRestore(const QVariantMap &map) const
{
    return IDevice::typeFromMap(map) == Constants::GenericLinuxOsType;
}

IDevice::Ptr GenericLinuxDeviceConfigurationFactory::restore(const QVariantMap &map) const
{
    QTC_ASSERT(canRestore(map), return IDevice::Ptr());
    const IDevice::Ptr device = LinuxDevice::create();
    device->fromMap(map);
    return device;
}

} // namespace RemoteLinux
