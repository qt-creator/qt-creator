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

#include "remotelinuxrunconfigurationfactory.h"

#include "remotelinux_constants.h"
#include "remotelinuxcustomrunconfiguration.h"
#include "remotelinuxrunconfiguration.h"

#include <projectexplorer/buildtargetinfo.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/project.h>
#include <projectexplorer/target.h>
#include <utils/qtcassert.h>

#include <QString>

using namespace ProjectExplorer;

namespace RemoteLinux {
namespace Internal {

static QString stringFromId(Core::Id id)
{
    QByteArray idStr = id.name();
    if (!idStr.startsWith(RemoteLinuxRunConfiguration::IdPrefix))
        return QString();
    return QString::fromUtf8(idStr.mid(int(strlen(RemoteLinuxRunConfiguration::IdPrefix))));
}

static bool canHandle(const Target *target)
{
    if (!target->project()->supportsKit(target->kit()))
        return false;
    const Core::Id deviceType = DeviceTypeKitInformation::deviceTypeId(target->kit());
    return deviceType == RemoteLinux::Constants::GenericLinuxOsType;
}

// RemoteLinuxRunConfigurationFactory

RemoteLinuxRunConfigurationFactory::RemoteLinuxRunConfigurationFactory(QObject *parent)
    : IRunConfigurationFactory(parent)
{
    setObjectName("RemoteLinuxRunConfigurationFactory");
    registerRunConfiguration<RemoteLinuxRunConfiguration>();
}

bool RemoteLinuxRunConfigurationFactory::canCreate(Target *parent, Core::Id id) const
{
    if (!canHandle(parent))
        return false;
    return parent->applicationTargets().hasTarget(stringFromId(id));
}

bool RemoteLinuxRunConfigurationFactory::canRestore(Target *parent, const QVariantMap &map) const
{
    if (!canHandle(parent))
        return false;
    const Core::Id id = idFromMap(map);
    return id.name().startsWith(RemoteLinuxRunConfiguration::IdPrefix);
}

bool RemoteLinuxRunConfigurationFactory::canClone(Target *parent, RunConfiguration *source) const
{
    auto rlrc = qobject_cast<RemoteLinuxRunConfiguration *>(source);
    return rlrc && canCreate(parent, source->id());
}

QList<Core::Id> RemoteLinuxRunConfigurationFactory::availableCreationIds(Target *parent, CreationMode mode) const
{
    Q_UNUSED(mode)
    QList<Core::Id> result;
    if (!canHandle(parent))
        return result;

    const Core::Id base = Core::Id(RemoteLinuxRunConfiguration::IdPrefix);
    foreach (const BuildTargetInfo &bti, parent->applicationTargets().list)
        result << base.withSuffix(bti.targetName);
    return result;
}

QString RemoteLinuxRunConfigurationFactory::displayNameForId(Core::Id id) const
{
    return stringFromId(id) + QLatin1Char(' ') + tr("(on Remote Generic Linux Host)");
}

// RemoteLinuxCustomRunConfigurationFactory

RemoteLinuxCustomRunConfigurationFactory::RemoteLinuxCustomRunConfigurationFactory(QObject *parent)
    : IRunConfigurationFactory(parent)
{
    setObjectName("RemoteLinuxCustomRunConfiguration");
    registerRunConfiguration<RemoteLinuxCustomRunConfiguration>();
}

bool RemoteLinuxCustomRunConfigurationFactory::canCreate(Target *parent, Core::Id id) const
{
    if (!canHandle(parent))
        return false;
    return id == RemoteLinuxCustomRunConfiguration::runConfigId();
}

bool RemoteLinuxCustomRunConfigurationFactory::canRestore(Target *parent, const QVariantMap &map) const
{
    if (!canHandle(parent))
        return false;
    const Core::Id id = idFromMap(map);
    return id == RemoteLinuxCustomRunConfiguration::runConfigId();
}

bool RemoteLinuxCustomRunConfigurationFactory::canClone(Target *parent, RunConfiguration *source) const
{
    auto rlrc = qobject_cast<RemoteLinuxCustomRunConfiguration *>(source);
    return rlrc && canCreate(parent, source->id());
}

QList<Core::Id> RemoteLinuxCustomRunConfigurationFactory::availableCreationIds(Target *parent, CreationMode mode) const
{
    Q_UNUSED(mode)
    QList<Core::Id> result;
    if (!canHandle(parent))
        return result;

    result << RemoteLinuxCustomRunConfiguration::runConfigId();
    return result;
}

QString RemoteLinuxCustomRunConfigurationFactory::displayNameForId(Core::Id) const
{
    return RemoteLinuxCustomRunConfiguration::runConfigDefaultDisplayName();
}

} // namespace Internal
} // namespace RemoteLinux
