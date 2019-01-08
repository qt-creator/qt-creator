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
const char LastDeployedLocalTimesKey[] = "RemoteLinux.LastDeployedLocalTimes";
const char LastDeployedRemoteTimesKey[] = "RemoteLinux.LastDeployedRemoteTimes";

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
    struct Timestamps
    {
        QDateTime local;
        QDateTime remote;
    };
    QHash<DeployParameters, Timestamps> lastDeployed;

    DeployParameters parameters(const ProjectExplorer::DeployableFile &deployableFile,
                                const ProjectExplorer::Kit *kit) const
    {
        QString systemRoot;
        QString host;

        if (kit) {
            systemRoot = SysRootKitInformation::sysRoot(kit).toString();
            const IDevice::ConstPtr deviceConfiguration = DeviceKitInformation::device(kit);
            host = deviceConfiguration->sshParameters().host();
        }

        return DeployParameters(deployableFile, host, systemRoot);
    }
};


DeploymentTimeInfo::DeploymentTimeInfo() : d(new DeploymentTimeInfoPrivate())
{

}

DeploymentTimeInfo::~DeploymentTimeInfo()
{
    delete d;
}

void DeploymentTimeInfo::saveDeploymentTimeStamp(const DeployableFile &deployableFile,
                                                 const Kit *kit, const QDateTime &remoteTimestamp)
{
    d->lastDeployed.insert(
                d->parameters(deployableFile, kit),
                { deployableFile.localFilePath().toFileInfo().lastModified(), remoteTimestamp });
}

bool DeploymentTimeInfo::hasLocalFileChanged(const DeployableFile &deployableFile,
                                             const ProjectExplorer::Kit *kit) const
{
    const auto &lastDeployed = d->lastDeployed.value(d->parameters(deployableFile, kit));
    const QDateTime lastModified = deployableFile.localFilePath().toFileInfo().lastModified();
    return !lastDeployed.local.isValid() || lastModified != lastDeployed.local;
}

bool DeploymentTimeInfo::hasRemoteFileChanged(const DeployableFile &deployableFile,
                                              const ProjectExplorer::Kit *kit,
                                              const QDateTime &remoteTimestamp) const
{
    const auto &lastDeployed = d->lastDeployed.value(d->parameters(deployableFile, kit));
    return !lastDeployed.remote.isValid() || remoteTimestamp != lastDeployed.remote;
}

QVariantMap DeploymentTimeInfo::exportDeployTimes() const
{
    QVariantMap map;
    QVariantList hostList;
    QVariantList fileList;
    QVariantList sysrootList;
    QVariantList remotePathList;
    QVariantList localTimeList;
    QVariantList remoteTimeList;
    using DepIt = QHash<DeployParameters, DeploymentTimeInfoPrivate::Timestamps>::ConstIterator;

    for (DepIt it = d->lastDeployed.constBegin(); it != d->lastDeployed.constEnd(); ++it) {
        fileList << it.key().file.localFilePath().toString();
        remotePathList << it.key().file.remoteDirectory();
        hostList << it.key().host;
        sysrootList << it.key().sysroot;
        localTimeList << it.value().local;
        remoteTimeList << it.value().remote;
    }
    map.insert(QLatin1String(LastDeployedHostsKey), hostList);
    map.insert(QLatin1String(LastDeployedSysrootsKey), sysrootList);
    map.insert(QLatin1String(LastDeployedFilesKey), fileList);
    map.insert(QLatin1String(LastDeployedRemotePathsKey), remotePathList);
    map.insert(QLatin1String(LastDeployedLocalTimesKey), localTimeList);
    map.insert(QLatin1String(LastDeployedRemoteTimesKey), remoteTimeList);
    return map;
}

void DeploymentTimeInfo::importDeployTimes(const QVariantMap &map)
{
    const QVariantList &hostList = map.value(QLatin1String(LastDeployedHostsKey)).toList();
    const QVariantList &sysrootList = map.value(QLatin1String(LastDeployedSysrootsKey)).toList();
    const QVariantList &fileList = map.value(QLatin1String(LastDeployedFilesKey)).toList();
    const QVariantList &remotePathList
        = map.value(QLatin1String(LastDeployedRemotePathsKey)).toList();

    QVariantList localTimesList;
    const auto localTimes = map.find(QLatin1String(LastDeployedLocalTimesKey));
    if (localTimes != map.end()) {
        localTimesList = localTimes.value().toList();
    } else {
        localTimesList = map.value(QLatin1String(LastDeployedTimesKey)).toList();
    }

    const QVariantList remoteTimesList
            = map.value(QLatin1String(LastDeployedRemoteTimesKey)).toList();

    const int elemCount = qMin(qMin(qMin(hostList.size(), fileList.size()),
                                    qMin(remotePathList.size(), localTimesList.size())),
                               sysrootList.size());

    for (int i = 0; i < elemCount; ++i) {
        const DeployableFile df(fileList.at(i).toString(), remotePathList.at(i).toString());
        const DeployParameters dp(df, hostList.at(i).toString(), sysrootList.at(i).toString());
        d->lastDeployed.insert(dp, { localTimesList.at(i).toDateTime(),
                                     remoteTimesList.length() > i
                                            ? remoteTimesList.at(i).toDateTime()
                                            : QDateTime() });
    }
}

} // namespace RemoteLinux
