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
#ifndef REMOTELINUXPACKAGEINSTALLER_H
#define REMOTELINUXPACKAGEINSTALLER_H

#include "remotelinux_export.h"

#include <QObject>
#include <QSharedPointer>
#include <QString>

namespace RemoteLinux {
class LinuxDeviceConfiguration;

namespace Internal {
class AbstractRemoteLinuxPackageInstallerPrivate;
} // namespace Internal

class REMOTELINUX_EXPORT AbstractRemoteLinuxPackageInstaller : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(AbstractRemoteLinuxPackageInstaller)
public:
    ~AbstractRemoteLinuxPackageInstaller();

    void installPackage(const QSharedPointer<const LinuxDeviceConfiguration> &deviceConfig,
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
