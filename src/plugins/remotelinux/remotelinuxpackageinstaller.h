/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef REMOTELINUXPACKAGEINSTALLER_H
#define REMOTELINUXPACKAGEINSTALLER_H

#include "remotelinux_export.h"

#include <projectexplorer/devicesupport/idevice.h>

namespace RemoteLinux {

namespace Internal { class AbstractRemoteLinuxPackageInstallerPrivate; }

class REMOTELINUX_EXPORT AbstractRemoteLinuxPackageInstaller : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(AbstractRemoteLinuxPackageInstaller)
public:
    ~AbstractRemoteLinuxPackageInstaller();

    void installPackage(const ProjectExplorer::IDevice::ConstPtr &deviceConfig,
        const QString &packageFilePath, bool removePackageFile);
    void cancelInstallation();

signals:
    void stdoutData(const QString &output);
    void stderrData(const QString &output);
    void finished(const QString &errorMsg = QString());

protected:
    explicit AbstractRemoteLinuxPackageInstaller(QObject *parent = 0);

private slots:
    void handleConnectionError();
    void handleInstallationFinished(int exitStatus);
    void handleInstallerOutput();
    void handleInstallerErrorOutput();

private:
    virtual QString installCommandLine(const QString &packageFilePath) const = 0;
    virtual QString cancelInstallationCommandLine() const = 0;

    virtual void prepareInstallation() {}
    virtual QString errorString() const { return QString(); }

    void setFinished();

    Internal::AbstractRemoteLinuxPackageInstallerPrivate * const d;
};


class REMOTELINUX_EXPORT RemoteLinuxTarPackageInstaller : public AbstractRemoteLinuxPackageInstaller
{
    Q_OBJECT
public:
    RemoteLinuxTarPackageInstaller(QObject *parent = 0);

private:
    QString installCommandLine(const QString &packageFilePath) const;
    QString cancelInstallationCommandLine() const;
};


} // namespace RemoteLinux

#endif // REMOTELINUXPACKAGEINSTALLER_H
