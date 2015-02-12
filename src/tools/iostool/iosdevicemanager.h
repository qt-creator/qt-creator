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
#ifndef IOSMANAGER_H
#define IOSMANAGER_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QMap>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <string.h>

#include <qglobal.h>

namespace Ios {
namespace Internal {
class DevInfoSession;
class IosDeviceManagerPrivate;
} // namespace Internal

typedef unsigned int ServiceSocket;

class DeviceSession;

class IosDeviceManager : public QObject
{
    Q_OBJECT
public:
    typedef QMap<QString,QString> Dict;
    enum OpStatus {
        Success = 0,
        Warning = 1,
        Failure = 2
    };
    enum AppOp {
        None = 0,
        Install = 1,
        Run = 2,
        InstallAndRun = 3
    };

    static IosDeviceManager *instance();
    bool watchDevices();
    void requestAppOp(const QString &bundlePath, const QStringList &extraArgs, AppOp appOp,
                      const QString &deviceId, int timeout = 1000);
    void requestDeviceInfo(const QString &deviceId, int timeout = 1000);
    int processGdbServer(int fd);
    void stopGdbServer(int fd, int phase);
    QStringList errors();
signals:
    void deviceAdded(const QString &deviceId);
    void deviceRemoved(const QString &deviceId);
    void isTransferringApp(const QString &bundlePath, const QString &deviceId, int progress,
                           const QString &info);
    void didTransferApp(const QString &bundlePath, const QString &deviceId,
                        Ios::IosDeviceManager::OpStatus status);
    void didStartApp(const QString &bundlePath, const QString &deviceId,
                     Ios::IosDeviceManager::OpStatus status, int gdbFd,
                     Ios::DeviceSession *deviceSession);
    void deviceInfo(const QString &deviceId, const Ios::IosDeviceManager::Dict &info);
    void appOutput(const QString &output);
    void errorMsg(const QString &msg);
private slots:
    void checkPendingLookups();
private:
    friend class Internal::IosDeviceManagerPrivate;
    friend class Internal::DevInfoSession;
    IosDeviceManager(QObject *parent = 0);
    Internal::IosDeviceManagerPrivate *d;
};

class DeviceSession {
public:
    DeviceSession(const QString &deviceId);
    virtual ~DeviceSession();
    QString deviceId;
    virtual int qmljsDebugPort() const = 0;
    virtual bool connectToPort(quint16 port, ServiceSocket *fd) = 0;
};

} // namespace Ios

#endif // IOSMANAGER_H
