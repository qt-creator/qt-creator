/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
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
**
**************************************************************************/
#include "maemorunconfiguration.h"

#include "maddedevice.h"
#include "maemoconstants.h"
#include "maemoglobal.h"
#include "maemoremotemountsmodel.h"
#include "maemorunconfigurationwidget.h"
#include "qt4maemodeployconfiguration.h"

#include <debugger/debuggerconstants.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>
#include <qt4projectmanager/qt4buildconfiguration.h>
#include <qtsupport/qtprofileinformation.h>
#include <utils/portlist.h>
#include <ssh/sshconnection.h>

#include <QDir>
#include <QFileInfo>

using namespace ProjectExplorer;
using namespace Qt4ProjectManager;
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

    if (DeviceTypeProfileInformation::deviceTypeId(target()->profile()) != Core::Id(HarmattanOsType))
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
    LinuxDeviceConfiguration::ConstPtr dev =
            ProjectExplorer::DeviceProfileInformation::device(target()->profile())
            .dynamicCast<const LinuxDeviceConfiguration>();
    if (!dev)
        return QString();

    QString prefix = environmentPreparationCommand() + QLatin1Char(';');
    if (dev->type() == Core::Id(MeeGoOsType))
        prefix += QLatin1String("DISPLAY=:0.0 ");

    return QString::fromLatin1("%1 %2").arg(prefix, userEnvironmentChangesAsString());
}

Utils::PortList MaemoRunConfiguration::freePorts() const
{
    QtSupport::BaseQtVersion *version = QtSupport::QtProfileInformation::qtVersion(target()->profile());
    return MaemoGlobal::freePorts(ProjectExplorer::DeviceProfileInformation::device(target()->profile())
                                  .staticCast<const LinuxDeviceConfiguration>(), version);
}

QString MaemoRunConfiguration::localDirToMountForRemoteGdb() const
{
    const QString projectDir
        = QDir::fromNativeSeparators(QDir::cleanPath(activeBuildConfiguration()
            ->target()->project()->projectDirectory()));
    const QString execDir
        = QDir::fromNativeSeparators(QFileInfo(localExecutableFilePath()).path());
    const int length = qMin(projectDir.length(), execDir.length());
    int lastSeparatorPos = 0;
    for (int i = 0; i < length; ++i) {
        if (projectDir.at(i) != execDir.at(i))
            return projectDir.left(lastSeparatorPos);
        if (projectDir.at(i) == QLatin1Char('/'))
            lastSeparatorPos = i;
    }
    return projectDir.length() == execDir.length()
        ? projectDir : projectDir.left(lastSeparatorPos);
}

QString MaemoRunConfiguration::remoteProjectSourcesMountPoint() const
{
    return MaemoGlobal::homeDirOnDevice(ProjectExplorer::DeviceProfileInformation::device(target()->profile())->sshParameters().userName)
        + QLatin1String("/gdbSourcesDir_")
        + QFileInfo(localExecutableFilePath()).fileName();
}

bool MaemoRunConfiguration::hasEnoughFreePorts(RunMode mode) const
{
    const int freePortCount = freePorts().count();
    Core::Id typeId = ProjectExplorer::DeviceTypeProfileInformation::deviceTypeId(target()->profile());
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
