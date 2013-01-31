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
#include "remotelinuxrunconfigurationfactory.h"

#include "remotelinux_constants.h"
#include "remotelinuxrunconfiguration.h"

#include <projectexplorer/buildtargetinfo.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/project.h>
#include <projectexplorer/target.h>
#include <utils/qtcassert.h>

#include <QFileInfo>
#include <QString>
#include <QStringList>

using namespace ProjectExplorer;

namespace RemoteLinux {
namespace Internal {

namespace {
QString pathFromId(Core::Id id)
{
    QString idStr = QString::fromUtf8(id.name());
    if (!idStr.startsWith(RemoteLinuxRunConfiguration::IdPrefix))
        return QString();
    return idStr.mid(RemoteLinuxRunConfiguration::IdPrefix.size());
}

} // namespace

RemoteLinuxRunConfigurationFactory::RemoteLinuxRunConfigurationFactory(QObject *parent)
    : IRunConfigurationFactory(parent)
{
    setObjectName(QLatin1String("RemoteLinuxRunConfigurationFactory"));
}

RemoteLinuxRunConfigurationFactory::~RemoteLinuxRunConfigurationFactory()
{
}

bool RemoteLinuxRunConfigurationFactory::canCreate(Target *parent, const Core::Id id) const
{
    if (!canHandle(parent))
        return false;
    return !parent->applicationTargets().targetForProject(pathFromId(id)).isEmpty();
}

bool RemoteLinuxRunConfigurationFactory::canRestore(Target *parent, const QVariantMap &map) const
{
    if (!canHandle(parent))
        return false;
    return idFromMap(map).toString().startsWith(RemoteLinuxRunConfiguration::IdPrefix);
}

bool RemoteLinuxRunConfigurationFactory::canClone(Target *parent, RunConfiguration *source) const
{
    const RemoteLinuxRunConfiguration * const rlrc
            = qobject_cast<RemoteLinuxRunConfiguration *>(source);
    return rlrc && canCreate(parent, source->id());
}

QList<Core::Id> RemoteLinuxRunConfigurationFactory::availableCreationIds(Target *parent) const
{
    QList<Core::Id> result;
    if (!canHandle(parent))
        return result;

    foreach (const BuildTargetInfo &bti, parent->applicationTargets().list)
        result << (Core::Id(RemoteLinuxRunConfiguration::IdPrefix + bti.projectFilePath.toString()));
    return result;
}

QString RemoteLinuxRunConfigurationFactory::displayNameForId(const Core::Id id) const
{
    return QFileInfo(pathFromId(id)).completeBaseName()
        + tr(" (on Remote Generic Linux Host)");
}

RunConfiguration *RemoteLinuxRunConfigurationFactory::create(Target *parent, const Core::Id id)
{
    QTC_ASSERT(canCreate(parent, id), return 0);
    return new RemoteLinuxRunConfiguration(parent, id, pathFromId(id));
}

RunConfiguration *RemoteLinuxRunConfigurationFactory::restore(Target *parent,
    const QVariantMap &map)
{
    QTC_ASSERT(canRestore(parent, map), return 0);
    RemoteLinuxRunConfiguration *rc = new RemoteLinuxRunConfiguration(parent,
            Core::Id(RemoteLinuxRunConfiguration::IdPrefix), QString());
    if (rc->fromMap(map))
        return rc;

    delete rc;
    return 0;
}

RunConfiguration *RemoteLinuxRunConfigurationFactory::clone(Target *parent,
    RunConfiguration *source)
{
    QTC_ASSERT(canClone(parent, source), return 0);
    RemoteLinuxRunConfiguration *old = static_cast<RemoteLinuxRunConfiguration *>(source);
    return new RemoteLinuxRunConfiguration(parent, old);
}

bool RemoteLinuxRunConfigurationFactory::canHandle(const Target *target) const
{
    if (!target->project()->supportsKit(target->kit()))
        return false;
    const Core::Id deviceType = DeviceTypeKitInformation::deviceTypeId(target->kit());
    return deviceType == RemoteLinux::Constants::GenericLinuxOsType;
}

} // namespace Internal
} // namespace RemoteLinux
