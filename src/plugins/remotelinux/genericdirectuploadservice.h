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
    void handleChmodFinished(int exitStatus);
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
