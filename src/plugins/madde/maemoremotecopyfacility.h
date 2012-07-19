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

#ifndef MAEMOREMOTECOPYFACILITY_H
#define MAEMOREMOTECOPYFACILITY_H

#include <remotelinux/deployablefile.h>

#include <QList>
#include <QObject>
#include <QSharedPointer>
#include <QString>

namespace QSsh {
class SshConnection;
class SshRemoteProcessRunner;
}

namespace RemoteLinux {
class LinuxDeviceConfiguration;
}

namespace Madde {
namespace Internal {

class MaemoRemoteCopyFacility : public QObject
{
    Q_OBJECT
public:
    explicit MaemoRemoteCopyFacility(QObject *parent = 0);
    ~MaemoRemoteCopyFacility();

    void copyFiles(QSsh::SshConnection *connection,
        const QSharedPointer<const RemoteLinux::LinuxDeviceConfiguration> &devConf,
        const QList<RemoteLinux::DeployableFile> &deployables, const QString &mountPoint);
    void cancel();

signals:
    void stdoutData(const QString &output);
    void stderrData(const QString &output);
    void progress(const QString &message);
    void fileCopied(const RemoteLinux::DeployableFile &deployable);
    void finished(const QString &errorMsg = QString());

private slots:
    void handleConnectionError();
    void handleCopyFinished(int exitStatus);
    void handleRemoteStdout();
    void handleRemoteStderr();

private:
    void copyNextFile();
    void setFinished();

    QSsh::SshRemoteProcessRunner *m_copyRunner;
    QSsh::SshRemoteProcessRunner *m_killProcess;
    QSharedPointer<const RemoteLinux::LinuxDeviceConfiguration> m_devConf;
    QList<RemoteLinux::DeployableFile> m_deployables;
    QString m_mountPoint;
    bool m_isCopying; // TODO: Redundant due to being in sync with m_copyRunner?
};

} // namespace Internal
} // namespace Madde

#endif // MAEMOREMOTECOPYFACILITY_H
