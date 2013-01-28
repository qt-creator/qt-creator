/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/
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
    return QList<Core::Id>() << Core::Id(Maemo5OsType) << Core::Id(HarmattanOsType);
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
    return type == Maemo5OsType || type == HarmattanOsType;
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
