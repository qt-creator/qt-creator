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

#pragma once

#include "abstractremotelinuxdeployservice.h"
#include "remotelinux_export.h"

#include <ssh/sftpdefs.h>

#include <QList>

QT_FORWARD_DECLARE_CLASS(QDateTime)
QT_FORWARD_DECLARE_CLASS(QString)

namespace ProjectExplorer { class DeployableFile; }
namespace QSsh { class SshRemoteProcess; }
namespace RemoteLinux {
namespace Internal { class GenericDirectUploadServicePrivate; }

class REMOTELINUX_EXPORT GenericDirectUploadService : public AbstractRemoteLinuxDeployService
{
    Q_OBJECT
public:
    GenericDirectUploadService(QObject *parent = nullptr);
    ~GenericDirectUploadService() override;

    void setDeployableFiles(const QList<ProjectExplorer::DeployableFile> &deployableFiles);
    void setIncrementalDeployment(bool incremental);
    void setIgnoreMissingFiles(bool ignoreMissingFiles);

  protected:
    bool isDeploymentNecessary() const override;

    void doDeviceSetup() override;
    void stopDeviceSetup() override;

    void doDeploy() override;
    void stopDeployment() override;

private:
    QDateTime timestampFromStat(const ProjectExplorer::DeployableFile &file,
                                QSsh::SshRemoteProcess *statProc);
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
