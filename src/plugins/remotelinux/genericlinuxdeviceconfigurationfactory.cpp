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
#include "genericlinuxdeviceconfigurationfactory.h"

#include "genericlinuxdeviceconfigurationwizard.h"
#include "linuxdevice.h"
#include "remotelinux_constants.h"

#include <coreplugin/icore.h>

#include <utils/qtcassert.h>

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
