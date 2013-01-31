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
#include "maemorunconfiguration.h"

#include "maddedevice.h"
#include "maemoconstants.h"
#include "maemoglobal.h"
#include "maemoremotemountsmodel.h"
#include "maemorunconfigurationwidget.h"

#include <debugger/debuggerconstants.h>
#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>
#include <utils/portlist.h>
#include <ssh/sshconnection.h>

#include <QDir>
#include <QFileInfo>

using namespace ProjectExplorer;
using namespace RemoteLinux;

namespace Madde {
namespace Internal {

MaemoRunConfiguration::MaemoRunConfiguration(Target *parent, Core::Id id,
        const QString &proFilePath)
    : RemoteLinuxRunConfiguration(parent, id, proFilePath)
{
    init();
}

MaemoRunConfiguration::MaemoRunConfiguration(Target *parent,
        MaemoRunConfiguration *source)
    : RemoteLinuxRunConfiguration(parent, source)
{
    init();
}

void MaemoRunConfiguration::init()
{
    m_remoteMounts = new MaemoRemoteMountsModel(this);
    connect(m_remoteMounts, SIGNAL(rowsInserted(QModelIndex,int,int)), this,
        SLOT(handleRemoteMountsChanged()));
    connect(m_remoteMounts, SIGNAL(rowsRemoved(QModelIndex,int,int)), this,
        SLOT(handleRemoteMountsChanged()));
    connect(m_remoteMounts, SIGNAL(dataChanged(QModelIndex,QModelIndex)), this,
        SLOT(handleRemoteMountsChanged()));
    connect(m_remoteMounts, SIGNAL(modelReset()), SLOT(handleRemoteMountsChanged()));

    if (DeviceTypeKitInformation::deviceTypeId(target()->kit()) != HarmattanOsType)
        debuggerAspect()->suppressQmlDebuggingOptions();
}

bool MaemoRunConfiguration::isEnabled() const
{
    if (!RemoteLinuxRunConfiguration::isEnabled())
        return false;
    if (!hasEnoughFreePorts(NormalRunMode)) {
        setDisabledReason(tr("Not enough free ports on the device."));
        return false;
    }
    return true;
}

QWidget *MaemoRunConfiguration::createConfigurationWidget()
{
    return new MaemoRunConfigurationWidget(this);
}

QVariantMap MaemoRunConfiguration::toMap() const
{
    QVariantMap map = RemoteLinuxRunConfiguration::toMap();
    map.unite(m_remoteMounts->toMap());
    return map;
}

bool MaemoRunConfiguration::fromMap(const QVariantMap &map)
{
    if (!RemoteLinuxRunConfiguration::fromMap(map))
        return false;
    m_remoteMounts->fromMap(map);
    return true;
}

QString MaemoRunConfiguration::environmentPreparationCommand() const
{
    return MaemoGlobal::remoteSourceProfilesCommand();
}

QString MaemoRunConfiguration::commandPrefix() const
{
    IDevice::ConstPtr dev = DeviceKitInformation::device(target()->kit());
    if (!dev)
        return QString();

    const QString prefix = environmentPreparationCommand() + QLatin1Char(';');

    return QString::fromLatin1("%1 %2").arg(prefix, userEnvironmentChangesAsString());
}

Utils::PortList MaemoRunConfiguration::freePorts() const
{
    return MaemoGlobal::freePorts(target()->kit());
}

bool MaemoRunConfiguration::hasEnoughFreePorts(RunMode mode) const
{
    const int freePortCount = freePorts().count();
    Core::Id typeId = DeviceTypeKitInformation::deviceTypeId(target()->kit());
    const bool remoteMountsAllowed = MaddeDevice::allowsRemoteMounts(typeId);
    const int mountDirCount = remoteMountsAllowed
        ? remoteMounts()->validMountSpecificationCount() : 0;
    if (mode == DebugRunMode || mode == DebugRunModeWithBreakOnMain)
        return freePortCount >= mountDirCount + portsUsedByDebuggers();
    if (mode == NormalRunMode)
        return freePortCount >= mountDirCount;
    return false;
}

void MaemoRunConfiguration::handleRemoteMountsChanged()
{
    emit remoteMountsChanged();
    updateEnabledState();
}

} // namespace Internal
} // namespace Madde
