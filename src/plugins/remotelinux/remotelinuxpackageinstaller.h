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

#ifndef REMOTELINUXPACKAGEINSTALLER_H
#define REMOTELINUXPACKAGEINSTALLER_H

#include "remotelinux_export.h"

#include <projectexplorer/devicesupport/idevice.h>

#include <QSharedPointer>

namespace RemoteLinux {

namespace Internal {
class AbstractRemoteLinuxPackageInstallerPrivate;
} // namespace Internal

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
