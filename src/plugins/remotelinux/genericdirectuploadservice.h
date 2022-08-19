// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include "remotelinux_export.h"

#include "abstractremotelinuxdeployservice.h"

#include <QList>

QT_BEGIN_NAMESPACE
class QDateTime;
class QString;
QT_END_NAMESPACE

namespace ProjectExplorer { class DeployableFile; }
namespace Utils { class QtcProcess; }

namespace RemoteLinux {
namespace Internal { class GenericDirectUploadServicePrivate; }

enum class IncrementalDeployment { Enabled, Disabled, NotSupported };

class REMOTELINUX_EXPORT GenericDirectUploadService : public AbstractRemoteLinuxDeployService
{
    Q_OBJECT
public:
    GenericDirectUploadService(QObject *parent = nullptr);
    ~GenericDirectUploadService() override;

    void setDeployableFiles(const QList<ProjectExplorer::DeployableFile> &deployableFiles);
    void setIncrementalDeployment(IncrementalDeployment incremental);
    void setIgnoreMissingFiles(bool ignoreMissingFiles);

protected:
    bool isDeploymentNecessary() const override;

    void doDeploy() override;
    void stopDeployment() override;

private:
    void runStat(const ProjectExplorer::DeployableFile &file);
    QDateTime timestampFromStat(const ProjectExplorer::DeployableFile &file,
                                Utils::QtcProcess *statProc);
    void checkForStateChangeOnRemoteProcFinished();

    QList<ProjectExplorer::DeployableFile> collectFilesToUpload(
            const ProjectExplorer::DeployableFile &file) const;
    void setFinished();
    void queryFiles();
    void uploadFiles();
    void chmod();

    Internal::GenericDirectUploadServicePrivate * const d;
};

} //namespace RemoteLinux
