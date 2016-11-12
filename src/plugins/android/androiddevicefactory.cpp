/****************************************************************************
**
** Copyright (C) 2016 BogDan Vatra <bog_dan_ro@yahoo.com>
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

#include "androiddevicefactory.h"
#include "androiddevice.h"

#include "androidconstants.h"

#include <utils/icon.h>

#include <QIcon>

namespace Android {
namespace Internal {

AndroidDeviceFactory::AndroidDeviceFactory()
{
    setObjectName(QLatin1String("AndroidDeviceFactory"));
}

QString AndroidDeviceFactory::displayNameForId(Core::Id type) const
{
    if (type == Constants::ANDROID_DEVICE_TYPE)
        return tr("Android Device");
    return QString();
}

QList<Core::Id> AndroidDeviceFactory::availableCreationIds() const
{
    return QList<Core::Id>() << Core::Id(Constants::ANDROID_DEVICE_TYPE);
}

QIcon AndroidDeviceFactory::iconForId(Core::Id type) const
{
    Q_UNUSED(type)
    using namespace Utils;
    static const QIcon icon =
            Icon::combinedIcon({Icon({{":/android/images/androiddevicesmall.png",
                                       Theme::PanelTextColorDark}}, Icon::Tint),
                                Icon({{":/android/images/androiddevice.png",
                                       Theme::IconsBaseColor}})});
    return icon;
}

bool AndroidDeviceFactory::canCreate() const
{
    return false;
}

ProjectExplorer::IDevice::Ptr AndroidDeviceFactory::create(Core::Id id) const
{
    Q_UNUSED(id)
    return ProjectExplorer::IDevice::Ptr();
}

bool AndroidDeviceFactory::canRestore(const QVariantMap &map) const
{
    return ProjectExplorer::IDevice::typeFromMap(map) == Constants::ANDROID_DEVICE_TYPE;
}

ProjectExplorer::IDevice::Ptr AndroidDeviceFactory::restore(const QVariantMap &map) const
{
    Q_UNUSED(map)
    return ProjectExplorer::IDevice::Ptr(new AndroidDevice);
}

} // namespace Internal
} // namespace Android
