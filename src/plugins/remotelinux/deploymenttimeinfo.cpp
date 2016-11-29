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
#include "deploymenttimeinfo.h"

#include <projectexplorer/deployablefile.h>
#include <projectexplorer/target.h>
#include <qtsupport/qtkitinformation.h>

#include <ssh/sshconnection.h>
#include <ssh/sshconnectionmanager.h>

#include <QPointer>
#include <QDateTime>
#include <QFileInfo>

using namespace ProjectExplorer;
using namespace QSsh;

namespace RemoteLinux {

namespace {
const char LastDeployedHostsKey[] = "ProjectExplorer.RunConfiguration.LastDeployedHosts";
const char LastDeployedSysrootsKey[] = "ProjectExplorer.RunConfiguration.LastDeployedSysroots";
const char LastDeployedFilesKey[] = "ProjectExplorer.RunConfiguration.LastDeployedFiles";
const char LastDeployedRemotePathsKey[] = "ProjectExplorer.RunConfiguration.LastDeployedRemotePaths";
const char LastDeployedTimesKey[] = "ProjectExplorer.RunConfiguration.LastDeployedTimes";

class DeployParameters
{

public:
    DeployParameters(const DeployableFile &d, const QString &h, const QString &s)
        : file(d), host(h), sysroot(s) {}

    bool operator==(const DeployParameters &other) const {
        return file == other.file &&  host == other.host &&  sysroot == other.sysroot;
    }

    DeployableFile file;
    QString host;
    QString sysroot;
};

uint qHash(const DeployParameters &p) {
    return qHash(qMakePair(qMakePair(p.file, p.host), p.sysroot));
}

} // anonymous namespace

class DeploymentTimeInfoPrivate
{
public:
    QHash<DeployParameters, QDateTime> lastDeployed;
};


DeploymentTimeInfo::DeploymentTimeInfo() : d(new DeploymentTimeInfoPrivate())
{

}

DeploymentTimeInfo::~DeploymentTimeInfo()
{
    delete d;
}

void DeploymentTimeInfo::saveDeploymentTimeStamp(const DeployableFile &deployableFile,
                                                 const Kit *kit)
{
    if (!kit)
        return;

    QString systemRoot;
    if (SysRootKitInformation::hasSysRoot(kit))
        systemRoot = SysRootKitInformation::sysRoot(kit).toString();

    const IDevice::ConstPtr deviceConfiguration = DeviceKitInformation::device(kit);
    const QString host = deviceConfiguration->sshParameters().host;

    d->lastDeployed.insert(
                DeployParameters(deployableFile, host, systemRoot),
                QDateTime::currentDateTime());
}

bool DeploymentTimeInfo::hasChangedSinceLastDeployment(const DeployableFile &deployableFile,
                                                       const ProjectExplorer::Kit *kit) const
{
    if (!kit)
        return false;

    QString systemRoot;
    if (SysRootKitInformation::hasSysRoot(kit))
        systemRoot = SysRootKitInformation::sysRoot(kit).toString();

    const IDevice::ConstPtr deviceConfiguration = DeviceKitInformation::device(kit);
    const QString host = deviceConfiguration->sshParameters().host;

    const DeployParameters dp(deployableFile, host, systemRoot);

    const QDateTime &lastDeployed = d->lastDeployed.value(dp);
    const QDateTime lastModified = deployableFile.localFilePath().toFileInfo().lastModified();

    return !lastDeployed.isValid() || (lastModified > lastDeployed);
}

QVariantMap DeploymentTimeInfo::exportDeployTimes() const
{
    QVariantMap map;
    QVariantList hostList;
    QVariantList fileList;
    QVariantList sysrootList;
    QVariantList remotePathList;
    QVariantList timeList;
    typedef QHash<DeployParameters, QDateTime>::ConstIterator DepIt;

    for (DepIt it = d->lastDeployed.constBegin(); it != d->lastDeployed.constEnd(); ++it) {
        fileList << it.key().file.localFilePath().toString();
        remotePathList << it.key().file.remoteDirectory();
        hostList << it.key().host;
        sysrootList << it.key().sysroot;
        timeList << it.value();
    }
    map.insert(QLatin1String(LastDeployedHostsKey), hostList);
    map.insert(QLatin1String(LastDeployedSysrootsKey), sysrootList);
    map.insert(QLatin1String(LastDeployedFilesKey), fileList);
    map.insert(QLatin1String(LastDeployedRemotePathsKey), remotePathList);
    map.insert(QLatin1String(LastDeployedTimesKey), timeList);
    return map;
}

void DeploymentTimeInfo::importDeployTimes(const QVariantMap &map)
{
    const QVariantList &hostList = map.value(QLatin1String(LastDeployedHostsKey)).toList();
    const QVariantList &sysrootList = map.value(QLatin1String(LastDeployedSysrootsKey)).toList();
    const QVariantList &fileList = map.value(QLatin1String(LastDeployedFilesKey)).toList();
    const QVariantList &remotePathList
        = map.value(QLatin1String(LastDeployedRemotePathsKey)).toList();
    const QVariantList &timeList = map.value(QLatin1String(LastDeployedTimesKey)).toList();

    const int elemCount = qMin(qMin(qMin(hostList.size(), fileList.size()),
                                    qMin(remotePathList.size(), timeList.size())),
                               sysrootList.size());

    for (int i = 0; i < elemCount; ++i) {
        const DeployableFile df(fileList.at(i).toString(), remotePathList.at(i).toString());
        const DeployParameters dp(df, hostList.at(i).toString(), sysrootList.at(i).toString());
        d->lastDeployed.insert(dp, timeList.at(i).toDateTime());
    }
}

} // namespace RemoteLinux
