/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
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
#ifndef REMOTELINUXUSEDPORTSGATHERER_H
#define REMOTELINUXUSEDPORTSGATHERER_H

#include "remotelinux_export.h"

#include <QtCore/QList>
#include <QtCore/QObject>
#include <QtCore/QSharedPointer>

QT_FORWARD_DECLARE_CLASS(QString)

namespace Utils {
class SshConnection;
} // namespace Utils

namespace RemoteLinux {
class LinuxDeviceConfiguration;
class PortList;

namespace Internal {
class RemoteLinuxUsedPortsGathererPrivate;
} // namespace Internal

class REMOTELINUX_EXPORT RemoteLinuxUsedPortsGatherer : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(RemoteLinuxUsedPortsGatherer)
public:
    explicit RemoteLinuxUsedPortsGatherer(QObject *parent = 0);
    ~RemoteLinuxUsedPortsGatherer();
    void start(const QSharedPointer<Utils::SshConnection> &connection,
        const QSharedPointer<const LinuxDeviceConfiguration> &devConf);
    void stop();
    int getNextFreePort(PortList *freePorts) const; // returns -1 if no more are left
    QList<int> usedPorts() const;

signals:
    void error(const QString &errMsg);
    void portListReady();

private slots:
    void handleConnectionError();
    void handleProcessClosed(int exitStatus);
    void handleRemoteStdOut(const QByteArray &output);
    void handleRemoteStdErr(const QByteArray &output);

private:
    void setupUsedPorts();

    Internal::RemoteLinuxUsedPortsGathererPrivate * const d;
};

} // namespace RemoteLinux

#endif // REMOTELINUXUSEDPORTSGATHERER_H
