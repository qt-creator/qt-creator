// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "deploymenttimeinfo.h"

#include <projectexplorer/deployablefile.h>
#include <projectexplorer/devicesupport/idevice.h>
#include <projectexplorer/devicesupport/sshparameters.h>
#include <projectexplorer/kitaspects.h>
#include <projectexplorer/target.h>

#include <QDateTime>

using namespace ProjectExplorer;
using namespace Utils;

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
    bool operator==(const DeployParameters &other) const {
        return file == other.file &&  host == other.host &&  sysroot == other.sysroot;
    }
    friend auto qHash(const DeployParameters &p) {
        return qHash(qMakePair(qMakePair(p.file, p.host), p.sysroot));
    }

    DeployableFile file;
    QString host;
    QString sysroot;
};

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

    DeployParameters parameters(const DeployableFile &deployableFile,
                                const Kit *kit) const
    {
        QString systemRoot;
        QString host;

        if (kit) {
            systemRoot = SysRootKitAspect::sysRoot(kit).toString();
            const IDevice::ConstPtr deviceConfiguration = DeviceKitAspect::device(kit);
            host = deviceConfiguration->sshParameters().host();
        }

        return DeployParameters{deployableFile, host, systemRoot};
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
                { deployableFile.localFilePath().lastModified(), remoteTimestamp });
}

bool DeploymentTimeInfo::hasLocalFileChanged(const DeployableFile &deployableFile,
                                             const Kit *kit) const
{
    const auto &lastDeployed = d->lastDeployed.value(d->parameters(deployableFile, kit));
    const QDateTime lastModified = deployableFile.localFilePath().lastModified();
    return !lastDeployed.local.isValid() || lastModified != lastDeployed.local;
}

bool DeploymentTimeInfo::hasRemoteFileChanged(const DeployableFile &deployableFile,
                                              const Kit *kit,
                                              const QDateTime &remoteTimestamp) const
{
    const auto &lastDeployed = d->lastDeployed.value(d->parameters(deployableFile, kit));
    return !lastDeployed.remote.isValid() || remoteTimestamp != lastDeployed.remote;
}

Store DeploymentTimeInfo::exportDeployTimes() const
{
    Store map;
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
    map.insert(LastDeployedHostsKey, hostList);
    map.insert(LastDeployedSysrootsKey, sysrootList);
    map.insert(LastDeployedFilesKey, fileList);
    map.insert(LastDeployedRemotePathsKey, remotePathList);
    map.insert(LastDeployedLocalTimesKey, localTimeList);
    map.insert(LastDeployedRemoteTimesKey, remoteTimeList);
    return map;
}

void DeploymentTimeInfo::importDeployTimes(const Store &map)
{
    const QVariantList &hostList = map.value(LastDeployedHostsKey).toList();
    const QVariantList &sysrootList = map.value(LastDeployedSysrootsKey).toList();
    const QVariantList &fileList = map.value(LastDeployedFilesKey).toList();
    const QVariantList &remotePathList = map.value(LastDeployedRemotePathsKey).toList();

    QVariantList localTimesList;
    const auto localTimes = map.find(LastDeployedLocalTimesKey);
    if (localTimes != map.end()) {
        localTimesList = localTimes.value().toList();
    } else {
        localTimesList = map.value(LastDeployedTimesKey).toList();
    }

    const QVariantList remoteTimesList = map.value(LastDeployedRemoteTimesKey).toList();

    const int elemCount = qMin(qMin(qMin(hostList.size(), fileList.size()),
                                    qMin(remotePathList.size(), localTimesList.size())),
                               sysrootList.size());

    for (int i = 0; i < elemCount; ++i) {
        const DeployableFile df(FilePath::fromSettings(fileList.at(i)),
                                remotePathList.at(i).toString());
        const DeployParameters dp{df, hostList.at(i).toString(), sysrootList.at(i).toString()};
        d->lastDeployed.insert(dp, { localTimesList.at(i).toDateTime(),
                                     remoteTimesList.length() > i
                                            ? remoteTimesList.at(i).toDateTime()
                                            : QDateTime() });
    }
}

} // namespace RemoteLinux
