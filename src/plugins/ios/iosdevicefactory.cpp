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

#include "iosdevicefactory.h"
#include "iosdevice.h"

#include "iosconstants.h"

#include <utils/icon.h>

#include <QIcon>

namespace Ios {
namespace Internal {

IosDeviceFactory::IosDeviceFactory()
{
    setObjectName(QLatin1String("IosDeviceFactory"));
}

QString IosDeviceFactory::displayNameForId(Core::Id type) const
{
    return type == Constants::IOS_DEVICE_TYPE ? IosDevice::name() : QString();
}

QList<Core::Id> IosDeviceFactory::availableCreationIds() const
{
    return QList<Core::Id>() << Core::Id(Constants::IOS_DEVICE_TYPE);
}

QIcon IosDeviceFactory::iconForId(Core::Id type) const
{
    Q_UNUSED(type)
    using namespace Utils;
    static const QIcon icon =
            Icon::combinedIcon({Icon({{":/ios/images/iosdevicesmall.png",
                                       Theme::PanelTextColorDark}}, Icon::Tint),
                                Icon({{":/ios/images/iosdevice.png",
                                       Theme::IconsBaseColor}})});
    return icon;
}

bool IosDeviceFactory::canCreate() const
{
    return false;
}

ProjectExplorer::IDevice::Ptr IosDeviceFactory::create(Core::Id id) const
{
    Q_UNUSED(id)
    return ProjectExplorer::IDevice::Ptr();
}

bool IosDeviceFactory::canRestore(const QVariantMap &map) const
{
    if (ProjectExplorer::IDevice::typeFromMap(map) != Constants::IOS_DEVICE_TYPE)
        return false;
    QVariantMap vMap = map.value(QLatin1String(Constants::EXTRA_INFO_KEY)).toMap();
    if (vMap.isEmpty()
            || vMap.value(QLatin1String("deviceName")).toString() == QLatin1String("*unknown*"))
        return false; // transient device (probably generated during an activation)
    return true;
}

ProjectExplorer::IDevice::Ptr IosDeviceFactory::restore(const QVariantMap &map) const
{
    IosDevice *newDev = new IosDevice;
    newDev->fromMap(map);
    // updating the active ones should be enough...
    //IosDeviceManager::instance()->updateInfo(newDev->uniqueDeviceID());
    return ProjectExplorer::IDevice::Ptr(newDev);
}

} // namespace Internal
} // namespace Ios
