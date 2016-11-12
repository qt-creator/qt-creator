/****************************************************************************
**
** Copyright (C) 2016 Tim Sander <tim@krieglstein.org>
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

#include "baremetaldeviceconfigurationfactory.h"

#include "baremetaldeviceconfigurationwizard.h"
#include "baremetalconstants.h"
#include "baremetaldevice.h"

#include <utils/icon.h>
#include <utils/qtcassert.h>

#include <QIcon>

using namespace ProjectExplorer;

namespace BareMetal {
namespace Internal {

BareMetalDeviceConfigurationFactory::BareMetalDeviceConfigurationFactory(QObject *parent)
    : IDeviceFactory(parent)
{
}

QString BareMetalDeviceConfigurationFactory::displayNameForId(Core::Id type) const
{
    return type == Constants::BareMetalOsType ? tr("Bare Metal Device") : QString();
}

QList<Core::Id> BareMetalDeviceConfigurationFactory::availableCreationIds() const
{
    return QList<Core::Id>() << Core::Id(Constants::BareMetalOsType);
}

QIcon BareMetalDeviceConfigurationFactory::iconForId(Core::Id type) const
{
    Q_UNUSED(type)
    using namespace Utils;
    static const QIcon icon =
            Icon::combinedIcon({Icon({{":/baremetal/images/baremetaldevicesmall.png",
                                       Theme::PanelTextColorDark}}, Icon::Tint),
                                Icon({{":/baremetal/images/baremetaldevice.png",
                                       Theme::IconsBaseColor}})});
    return icon;
}

IDevice::Ptr BareMetalDeviceConfigurationFactory::create(Core::Id id) const
{
    QTC_ASSERT(id == Constants::BareMetalOsType, return IDevice::Ptr());
    BareMetalDeviceConfigurationWizard wizard;
    if (wizard.exec() != QDialog::Accepted)
        return IDevice::Ptr();
    return wizard.device();
}

bool BareMetalDeviceConfigurationFactory::canRestore(const QVariantMap &map) const
{
   return IDevice::typeFromMap(map) == Constants::BareMetalOsType;
}

IDevice::Ptr BareMetalDeviceConfigurationFactory::restore(const QVariantMap &map) const
{
    QTC_ASSERT(canRestore(map), return IDevice::Ptr());
    const IDevice::Ptr device = BareMetalDevice::create();
    device->fromMap(map);
    return device;
}

} // namespace Internal
} // namespace BareMetal
