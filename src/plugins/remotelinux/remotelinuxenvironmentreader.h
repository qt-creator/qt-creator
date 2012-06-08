/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef REMOTELINUXENVIRONMENTREADER_H
#define REMOTELINUXENVIRONMENTREADER_H

#include <utils/environment.h>

#include <QObject>
#include <QSharedPointer>

namespace QSsh {
class SshRemoteProcessRunner;
}

namespace RemoteLinux {
class LinuxDeviceConfiguration;
class RemoteLinuxRunConfiguration;

namespace Internal {

class RemoteLinuxEnvironmentReader : public QObject
{
    Q_OBJECT
public:
    RemoteLinuxEnvironmentReader(RemoteLinuxRunConfiguration *config, QObject *parent = 0);
    ~RemoteLinuxEnvironmentReader();

    void start(const QString &environmentSetupCommand);
    void stop();

    Utils::Environment remoteEnvironment() const { return m_env; }

signals:
    void finished();
    void error(const QString &error);

private slots:
    void handleConnectionFailure();
    void handleCurrentDeviceConfigChanged();

    void remoteProcessFinished(int exitCode);

private:
    void setFinished();

    bool m_stop;
    Utils::Environment m_env;
    QSharedPointer<const LinuxDeviceConfiguration> m_devConfig;
    RemoteLinuxRunConfiguration *m_runConfig;
    QSsh::SshRemoteProcessRunner *m_remoteProcessRunner;
};

} // namespace Internal
} // namespace RemoteLinux

#endif  // REMOTELINUXENVIRONMENTREADER_H
