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
#ifndef ABSTRACTREMOTELINUXDEPLOYACTION_H
#define ABSTRACTREMOTELINUXDEPLOYACTION_H

#include "remotelinux_export.h"

#include <QObject>
#include <QSharedPointer>
#include <QVariantMap>
QT_FORWARD_DECLARE_CLASS(QString)

namespace QSsh { class SshConnection; }

namespace Qt4ProjectManager {
class Qt4BuildConfiguration;
}

namespace RemoteLinux {
class DeployableFile;
class DeploymentInfo;
class LinuxDeviceConfiguration;

namespace Internal {
class AbstractRemoteLinuxDeployServicePrivate;
}

class REMOTELINUX_EXPORT AbstractRemoteLinuxDeployService : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(AbstractRemoteLinuxDeployService)
public:
    explicit AbstractRemoteLinuxDeployService(QObject *parent = 0);
    ~AbstractRemoteLinuxDeployService();

    void setDeviceConfiguration(const QSharedPointer<const LinuxDeviceConfiguration> &deviceConfiguration);
    void setBuildConfiguration(Qt4ProjectManager::Qt4BuildConfiguration *bc);
    void start();
    void stop();

    QVariantMap exportDeployTimes() const;
    void importDeployTimes(const QVariantMap &map);

    virtual bool isDeploymentPossible(QString *whyNot = 0) const;

signals:
    void errorMessage(const QString &message);
    void progressMessage(const QString &message);
    void warningMessage(const QString &message);
    void stdOutData(const QString &data);
    void stdErrData(const QString &data);

protected:
    const Qt4ProjectManager::Qt4BuildConfiguration *qt4BuildConfiguration() const;
    QSharedPointer<const LinuxDeviceConfiguration> deviceConfiguration() const;
    QSsh::SshConnection *connection() const;

    void saveDeploymentTimeStamp(const DeployableFile &deployableFile);
    bool hasChangedSinceLastDeployment(const DeployableFile &deployableFile) const;

    void handleDeviceSetupDone(bool success);
    void handleDeploymentDone();

private slots:
    void handleConnected();
    void handleConnectionFailure();

private:
    Q_SIGNAL void finished();

    virtual bool isDeploymentNecessary() const = 0;

    // Should do things needed *before* connecting. Call handleDeviceSetupDone() afterwards.
    virtual void doDeviceSetup() = 0;
    virtual void stopDeviceSetup() = 0;

    virtual void doDeploy() = 0;
    virtual void stopDeployment() = 0;

    void setFinished();

    Internal::AbstractRemoteLinuxDeployServicePrivate * const d;
};

} // namespace RemoteLinux

#endif // ABSTRACTREMOTELINUXDEPLOYACTION_H
