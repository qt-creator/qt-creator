/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "desktopdevicefactory.h"
#include "desktopdevice.h"
#include "projectexplorerconstants.h"

namespace ProjectExplorer {
namespace Internal {

DesktopDeviceFactory::DesktopDeviceFactory(QObject *parent) : IDeviceFactory(parent)
{ }

QString DesktopDeviceFactory::displayNameForId(Core::Id type) const
{
    if (type == Core::Id(Constants::DESKTOP_DEVICE_TYPE))
        return tr("Desktop");
    return QString();
}

QList<Core::Id> DesktopDeviceFactory::availableCreationIds() const
{
    return QList<Core::Id>() << Core::Id(Constants::DESKTOP_DEVICE_TYPE);
}

bool DesktopDeviceFactory::canCreate() const
{
    return false;
}

IDevice::Ptr DesktopDeviceFactory::create(Core::Id id) const
{
    Q_UNUSED(id);
    return IDevice::Ptr();
}

bool DesktopDeviceFactory::canRestore(const QVariantMap &map) const
{
    return IDevice::idFromMap(map) == Core::Id(Constants::DESKTOP_DEVICE_ID);
}

IDevice::Ptr DesktopDeviceFactory::restore(const QVariantMap &map) const
{
    Q_UNUSED(map);
    return IDevice::Ptr(new DesktopDevice);
}

} // namespace Internal
} // namespace ProjectExplorer
