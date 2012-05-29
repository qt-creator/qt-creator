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
#include "maddedeviceconfigurationfactory.h"

#include "maddedevice.h"
#include "maemoconstants.h"
#include "maemodeviceconfigwizard.h"

#include <utils/qtcassert.h>

using namespace ProjectExplorer;
using namespace RemoteLinux;

namespace Madde {
namespace Internal {

MaddeDeviceConfigurationFactory::MaddeDeviceConfigurationFactory(QObject *parent)
    : IDeviceFactory(parent)
{
}

QString MaddeDeviceConfigurationFactory::displayNameForId(Core::Id type) const
{
    return MaddeDevice::maddeDisplayType(type);
}

QList<Core::Id> MaddeDeviceConfigurationFactory::availableCreationIds() const
{
    return QList<Core::Id>() << Core::Id(Maemo5OsType) << Core::Id(HarmattanOsType) << Core::Id(MeeGoOsType);
}

IDevice::Ptr MaddeDeviceConfigurationFactory::create(Core::Id id) const
{
    MaemoDeviceConfigWizard wizard(id);
    if (wizard.exec() != QDialog::Accepted)
        return IDevice::Ptr();
    return wizard.device();
}

bool MaddeDeviceConfigurationFactory::canRestore(const QVariantMap &map) const
{
    const Core::Id type = IDevice::typeFromMap(map);
    return type == Core::Id(Maemo5OsType) || type == Core::Id(HarmattanOsType)
        || type == Core::Id(MeeGoOsType);
}

IDevice::Ptr MaddeDeviceConfigurationFactory::restore(const QVariantMap &map) const
{
    QTC_ASSERT(canRestore(map), return MaddeDevice::Ptr());
    const MaddeDevice::Ptr device = MaddeDevice::create();
    device->fromMap(map);
    return device;
}

} // namespace Internal
} // namespace Madde
