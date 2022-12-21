// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QVariantMap>

QT_BEGIN_NAMESPACE
class QDateTime;
QT_END_NAMESPACE

namespace ProjectExplorer {
class DeployableFile;
class Kit;
}

namespace RemoteLinux {

class DeploymentTimeInfoPrivate;

class DeploymentTimeInfo
{
public:
    DeploymentTimeInfo();
    ~DeploymentTimeInfo();

    void importDeployTimes(const QVariantMap &map);
    QVariantMap exportDeployTimes() const;

    void saveDeploymentTimeStamp(const ProjectExplorer::DeployableFile &deployableFile,
                                 const ProjectExplorer::Kit *kit,
                                 const QDateTime &remoteTimestamp);

    bool hasLocalFileChanged(const ProjectExplorer::DeployableFile &deployableFile,
                             const ProjectExplorer::Kit *kit) const;

    bool hasRemoteFileChanged(const ProjectExplorer::DeployableFile &deployableFile,
                              const ProjectExplorer::Kit *kit,
                              const QDateTime &remoteTimestamp) const;

private:
    DeploymentTimeInfoPrivate *d;
};

} // namespace RemoteLinux
