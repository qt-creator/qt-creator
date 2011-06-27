/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
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
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#ifndef MAEMODIRECTDEVICEUPLOADSTEP_H
#define MAEMODIRECTDEVICEUPLOADSTEP_H

#include "abstractmaemodeploystep.h"

#include <utils/ssh/sftpdefs.h>

#include <QtCore/QList>
#include <QtCore/QSharedPointer>

namespace Utils {
class SshRemoteProcess;
class SftpChannel;
}

namespace RemoteLinux {
class DeployableFile;

namespace Internal {

class MaemoDirectDeviceUploadStep : public AbstractMaemoDeployStep
{
    Q_OBJECT
public:
    MaemoDirectDeviceUploadStep(ProjectExplorer::BuildStepList *bc);
    MaemoDirectDeviceUploadStep(ProjectExplorer::BuildStepList *bc,
        MaemoDirectDeviceUploadStep *other);
    ~MaemoDirectDeviceUploadStep();

    static const QString Id;
    static QString displayName();

private slots:
    void handleSftpInitialized();
    void handleSftpInitializationFailed(const QString &errorMessage);
    void handleUploadFinished(Utils::SftpJobId jobId, const QString &errorMsg);
    void handleMkdirFinished(int exitStatus);

private:
    enum ExtendedState { Inactive, InitializingSftp, Uploading };

    virtual bool isDeploymentPossibleInternal(QString &whynot) const;
    virtual bool isDeploymentNeeded(const QString &hostName) const;
    virtual void startInternal();
    virtual void stopInternal();
    virtual const AbstractMaemoPackageCreationStep *packagingStep() const { return 0; }

    void ctor();
    void setFinished();
    void checkDeploymentNeeded(const QString &hostName,
        const DeployableFile &deployable) const;
    void uploadNextFile();

    QSharedPointer<Utils::SftpChannel> m_uploader;
    QSharedPointer<Utils::SshRemoteProcess> m_mkdirProc;
    mutable QList<DeployableFile> m_filesToUpload;
    ExtendedState m_extendedState;
};

} // namespace Internal
} // namespace RemoteLinux

#endif // MAEMODIRECTDEVICEUPLOADSTEP_H
