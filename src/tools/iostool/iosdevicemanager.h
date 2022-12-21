// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "iostooltypes.h"

#include <QObject>
#include <QString>
#include <QStringList>
#include <QMap>

namespace Ios {
namespace Internal {
class DevInfoSession;
class IosDeviceManagerPrivate;
} // namespace Internal


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
                      const QString &deviceId, int timeout = 1000, QString deltaPath = QString());
    void requestDeviceInfo(const QString &deviceId, int timeout = 1000);
    int processGdbServer(ServiceConnRef conn);
    void stopGdbServer(ServiceConnRef conn, int phase);
    QStringList errors();

signals:
    void deviceAdded(const QString &deviceId);
    void deviceRemoved(const QString &deviceId);
    void isTransferringApp(const QString &bundlePath, const QString &deviceId, int progress,
                           const QString &info);
    void didTransferApp(const QString &bundlePath, const QString &deviceId,
                        Ios::IosDeviceManager::OpStatus status);
    void didStartApp(const QString &bundlePath, const QString &deviceId,
                     Ios::IosDeviceManager::OpStatus status, ServiceConnRef conn, int gdbFd,
                     Ios::DeviceSession *deviceSession);
    void deviceInfo(const QString &deviceId, const Ios::IosDeviceManager::Dict &info);
    void appOutput(const QString &output);
    void errorMsg(const QString &msg);

private:
    friend class Internal::IosDeviceManagerPrivate;
    friend class Internal::DevInfoSession;
    IosDeviceManager(QObject *parent = 0);
    void checkPendingLookups();
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
