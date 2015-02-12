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

#include "iossimulatorfactory.h"
#include <QLatin1String>
#include "iosconstants.h"
#include "iossimulator.h"
#include "utils/qtcassert.h"

namespace Ios {
namespace Internal {

IosSimulatorFactory::IosSimulatorFactory()
{
    setObjectName(QLatin1String("IosSimulatorFactory"));
}

QString IosSimulatorFactory::displayNameForId(Core::Id type) const
{
    if (type == Constants::IOS_SIMULATOR_TYPE)
        return tr("iOS Simulator");
    return QString();
}

QList<Core::Id> IosSimulatorFactory::availableCreationIds() const
{
    return QList<Core::Id>() << Core::Id(Constants::IOS_SIMULATOR_TYPE);
}

bool IosSimulatorFactory::canCreate() const
{
    return false;
}

ProjectExplorer::IDevice::Ptr IosSimulatorFactory::create(Core::Id id) const
{
    Q_UNUSED(id)
    return ProjectExplorer::IDevice::Ptr();
}

bool IosSimulatorFactory::canRestore(const QVariantMap &map) const
{
    return ProjectExplorer::IDevice::typeFromMap(map) == Constants::IOS_SIMULATOR_TYPE;
}

ProjectExplorer::IDevice::Ptr IosSimulatorFactory::restore(const QVariantMap &map) const
{
    QTC_ASSERT(canRestore(map), return ProjectExplorer::IDevice::Ptr());
    const ProjectExplorer::IDevice::Ptr device = ProjectExplorer::IDevice::Ptr(new IosSimulator());
    device->fromMap(map);
    return device;
}

} // namespace Internal
} // namespace Ios
