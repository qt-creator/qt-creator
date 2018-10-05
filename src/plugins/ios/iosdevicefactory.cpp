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

namespace Ios {
namespace Internal {

IosDeviceFactory::IosDeviceFactory()
    : ProjectExplorer::IDeviceFactory(Constants::IOS_DEVICE_ID)
{
    setObjectName(QLatin1String("IosDeviceFactory"));
    setDisplayName(IosDevice::name());
    setCombinedIcon(":/ios/images/iosdevicesmall.png",
                     ":/ios/images/iosdevice.png");
    setConstructionFunction([] { return ProjectExplorer::IDevice::Ptr(new IosDevice); });
}

bool IosDeviceFactory::canRestore(const QVariantMap &map) const
{
    QVariantMap vMap = map.value(QLatin1String(Constants::EXTRA_INFO_KEY)).toMap();
    if (vMap.isEmpty()
            || vMap.value(QLatin1String("deviceName")).toString() == QLatin1String("*unknown*"))
        return false; // transient device (probably generated during an activation)
    return true;
}

} // namespace Internal
} // namespace Ios
