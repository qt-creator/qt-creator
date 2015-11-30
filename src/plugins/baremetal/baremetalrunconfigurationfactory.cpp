/****************************************************************************
**
** Copyright (C) 2015 Tim Sander <tim@krieglstein.org>
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

#include "baremetalrunconfigurationfactory.h"
#include "baremetalconstants.h"
#include "baremetalcustomrunconfiguration.h"
#include "baremetalrunconfiguration.h"

#include <projectexplorer/buildtargetinfo.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/project.h>
#include <projectexplorer/target.h>
#include <utils/qtcassert.h>

#include <QFileInfo>
#include <QString>

using namespace ProjectExplorer;

namespace BareMetal {
namespace Internal {

static QString pathFromId(Core::Id id)
{
    QByteArray idStr = id.name();
    if (!idStr.startsWith(BareMetalRunConfiguration::IdPrefix))
        return QString();
    return QString::fromUtf8(idStr.mid(int(strlen(BareMetalRunConfiguration::IdPrefix))));
}

BareMetalRunConfigurationFactory::BareMetalRunConfigurationFactory(QObject *parent) :
    IRunConfigurationFactory(parent)
{
    setObjectName(QLatin1String("BareMetalRunConfigurationFactory"));
}

BareMetalRunConfigurationFactory::~BareMetalRunConfigurationFactory()
{
}

bool BareMetalRunConfigurationFactory::canCreate(Target *parent, Core::Id id) const
{
    if (!canHandle(parent))
        return false;
    return id == BareMetalCustomRunConfiguration::runConfigId()
            || !parent->applicationTargets().targetForProject(pathFromId(id)).isEmpty();
}

bool BareMetalRunConfigurationFactory::canRestore(Target *parent, const QVariantMap &map) const
{
    if (!canHandle(parent))
        return false;
    const Core::Id id = idFromMap(map);
    return id == BareMetalCustomRunConfiguration::runConfigId()
            || idFromMap(map).name().startsWith(BareMetalRunConfiguration::IdPrefix);
}

bool BareMetalRunConfigurationFactory::canClone(Target *parent, RunConfiguration *source) const
{
    const BareMetalRunConfiguration * const bmrc
            = qobject_cast<BareMetalRunConfiguration *>(source);
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
        result << base.withSuffix(bti.projectFilePath.toString());
    result << BareMetalCustomRunConfiguration::runConfigId();
    return result;
}

QString BareMetalRunConfigurationFactory::displayNameForId(Core::Id id) const
{
    if (id == BareMetalCustomRunConfiguration::runConfigId())
        return BareMetalCustomRunConfiguration::runConfigDefaultDisplayName();
    return tr("%1 (on GDB server or hardware debugger)")
        .arg(QFileInfo(pathFromId(id)).completeBaseName());
}

RunConfiguration *BareMetalRunConfigurationFactory::doCreate(Target *parent, Core::Id id)
{
    if (id == BareMetalCustomRunConfiguration::runConfigId())
        return new BareMetalCustomRunConfiguration(parent);
    return new BareMetalRunConfiguration(parent, id, pathFromId(id));
}

RunConfiguration *BareMetalRunConfigurationFactory::doRestore(Target *parent, const QVariantMap &map)
{
    if (idFromMap(map) == BareMetalCustomRunConfiguration::runConfigId())
        return new BareMetalCustomRunConfiguration(parent);
    return doCreate(parent,Core::Id(BareMetalRunConfiguration::IdPrefix));
}

RunConfiguration *BareMetalRunConfigurationFactory::clone(Target *parent, RunConfiguration *source)
{
    QTC_ASSERT(canClone(parent, source), return 0);
    if (BareMetalCustomRunConfiguration *old = qobject_cast<BareMetalCustomRunConfiguration *>(source))
        return new BareMetalCustomRunConfiguration(parent, old);
    BareMetalRunConfiguration *old = static_cast<BareMetalRunConfiguration*>(source);
    return new BareMetalRunConfiguration(parent,old);
}

bool BareMetalRunConfigurationFactory::canHandle(const Target *target) const
{
    if (!target->project()->supportsKit(target->kit()))
        return false;
    const Core::Id deviceType = DeviceTypeKitInformation::deviceTypeId(target->kit());
    return deviceType == BareMetal::Constants::BareMetalOsType;
}

} // namespace Internal
} // namespace BareMetal
