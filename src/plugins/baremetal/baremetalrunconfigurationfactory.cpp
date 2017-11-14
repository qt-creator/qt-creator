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

#include "baremetalrunconfigurationfactory.h"
#include "baremetalconstants.h"
#include "baremetalcustomrunconfiguration.h"
#include "baremetalrunconfiguration.h"

#include <projectexplorer/buildtargetinfo.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/project.h>
#include <projectexplorer/target.h>
#include <utils/qtcassert.h>

using namespace ProjectExplorer;

namespace BareMetal {
namespace Internal {

static bool canHandle(const Target *target)
{
    if (!target->project()->supportsKit(target->kit()))
        return false;
    const Core::Id deviceType = DeviceTypeKitInformation::deviceTypeId(target->kit());
    return deviceType == BareMetal::Constants::BareMetalOsType;
}

// BareMetalRunConfigurationFactory

BareMetalRunConfigurationFactory::BareMetalRunConfigurationFactory(QObject *parent) :
    IRunConfigurationFactory(parent)
{
    setObjectName("BareMetalRunConfigurationFactory");
    registerRunConfiguration<BareMetalRunConfiguration>();
}

bool BareMetalRunConfigurationFactory::canCreate(Target *parent, Core::Id id) const
{
    if (!canHandle(parent))
        return false;
    const QString targetName = BareMetalRunConfiguration::targetNameFromId(id);
    return !parent->applicationTargets().targetFilePath(targetName).isEmpty();
}

bool BareMetalRunConfigurationFactory::canRestore(Target *parent, const QVariantMap &map) const
{
    if (!canHandle(parent))
        return false;
    return idFromMap(map).name().startsWith(BareMetalRunConfiguration::IdPrefix);
}

bool BareMetalRunConfigurationFactory::canClone(Target *parent, RunConfiguration *source) const
{
    auto bmrc = qobject_cast<BareMetalRunConfiguration *>(source);
    return bmrc && canCreate(parent, source->id());
}

QList<Core::Id> BareMetalRunConfigurationFactory::availableCreationIds(Target *parent, CreationMode mode) const
{
    Q_UNUSED(mode)
    QList<Core::Id> result;
    if (!canHandle(parent))
        return result;

    const Core::Id base = Core::Id(BareMetalRunConfiguration::IdPrefix);
    foreach (const BuildTargetInfo &bti, parent->applicationTargets().list)
        result << base.withSuffix(bti.projectFilePath.toString() + QLatin1Char('/') + bti.targetName);
    return result;
}

QString BareMetalRunConfigurationFactory::displayNameForId(Core::Id id) const
{
    return tr("%1 (on GDB server or hardware debugger)")
        .arg(BareMetalRunConfiguration::targetNameFromId(id));
}


// BareMetalCustomRunConfigurationFactory

BareMetalCustomRunConfigurationFactory::BareMetalCustomRunConfigurationFactory(QObject *parent) :
    IRunConfigurationFactory(parent)
{
    setObjectName("BareMetalCustomRunConfigurationFactory");
    registerRunConfiguration<BareMetalCustomRunConfiguration>();
}

bool BareMetalCustomRunConfigurationFactory::canCreate(Target *parent, Core::Id id) const
{
    if (!canHandle(parent))
        return false;
    return id == BareMetalCustomRunConfiguration::runConfigId();
}

bool BareMetalCustomRunConfigurationFactory::canRestore(Target *parent, const QVariantMap &map) const
{
    if (!canHandle(parent))
        return false;
    const Core::Id id = idFromMap(map);
    return id == BareMetalCustomRunConfiguration::runConfigId();
}

bool BareMetalCustomRunConfigurationFactory::canClone(Target *parent, RunConfiguration *source) const
{
    auto bmrc = qobject_cast<BareMetalCustomRunConfiguration *>(source);
    return bmrc && canCreate(parent, source->id());
}

QList<Core::Id> BareMetalCustomRunConfigurationFactory::availableCreationIds(Target *parent, CreationMode mode) const
{
    Q_UNUSED(mode)
    QList<Core::Id> result;
    if (!canHandle(parent))
        return result;

    result << BareMetalCustomRunConfiguration::runConfigId();
    return result;
}

QString BareMetalCustomRunConfigurationFactory::displayNameForId(Core::Id) const
{
    return BareMetalCustomRunConfiguration::runConfigDefaultDisplayName();
}

} // namespace Internal
} // namespace BareMetal
