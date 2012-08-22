/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
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
#ifndef DIRECTDEVICEUPLOADACTION_H
#define DIRECTDEVICEUPLOADACTION_H

#include "abstractremotelinuxdeployservice.h"
#include "remotelinux_export.h"

#include <ssh/sftpdefs.h>

#include <QList>

QT_FORWARD_DECLARE_CLASS(QString)

namespace ProjectExplorer { class DeployableFile; }

namespace RemoteLinux {
namespace Internal { class GenericDirectUploadServicePrivate; }

class REMOTELINUX_EXPORT GenericDirectUploadService : public AbstractRemoteLinuxDeployService
{
    Q_OBJECT
public:
    GenericDirectUploadService(QObject *parent = 0);
    ~GenericDirectUploadService();

    void setDeployableFiles(const QList<ProjectExplorer::DeployableFile> &deployableFiles);
    void setIncrementalDeployment(bool incremental);

  protected:
    bool isDeploymentNecessary() const;

    void doDeviceSetup();
    void stopDeviceSetup();

    void doDeploy();
    void stopDeployment();

private slots:
    void handleSftpInitialized();
    void handleSftpInitializationFailed(const QString &errorMessage);
    void handleUploadFinished(QSsh::SftpJobId jobId, const QString &errorMsg);
    void handleMkdirFinished(int exitStatus);
    void handleLnFinished(int exitStatus);
    void handleStdOutData();
    void handleStdErrData();

private:
    void checkDeploymentNeeded(const ProjectExplorer::DeployableFile &file) const;
    void setFinished();
    void uploadNextFile();

    Internal::GenericDirectUploadServicePrivate * const d;
};

} //namespace RemoteLinux

#endif // DIRECTDEVICEUPLOADACTION_H
