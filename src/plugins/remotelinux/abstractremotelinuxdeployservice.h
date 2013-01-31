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
#ifndef ABSTRACTREMOTELINUXDEPLOYACTION_H
#define ABSTRACTREMOTELINUXDEPLOYACTION_H

#include "remotelinux_export.h"

#include <projectexplorer/devicesupport/idevice.h>

#include <QObject>
#include <QSharedPointer>
#include <QVariantMap>

namespace QSsh { class SshConnection; }

namespace ProjectExplorer {
class DeployableFile;
class Kit;
class Target;
}

namespace RemoteLinux {
namespace Internal { class AbstractRemoteLinuxDeployServicePrivate; }

class REMOTELINUX_EXPORT AbstractRemoteLinuxDeployService : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(AbstractRemoteLinuxDeployService)
public:
    explicit AbstractRemoteLinuxDeployService(QObject *parent = 0);
    ~AbstractRemoteLinuxDeployService();

    void setTarget(ProjectExplorer::Target *bc);
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
    const ProjectExplorer::Target *target() const;
    const ProjectExplorer::Kit *profile() const;
    ProjectExplorer::IDevice::ConstPtr deviceConfiguration() const;
    QSsh::SshConnection *connection() const;

    void saveDeploymentTimeStamp(const ProjectExplorer::DeployableFile &deployableFile);
    bool hasChangedSinceLastDeployment(const ProjectExplorer::DeployableFile &deployableFile) const;

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
